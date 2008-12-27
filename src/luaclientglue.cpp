#include "luaclientglue.hpp"
#include "client.hpp"
#include "lua/lua.hpp"
#include "exception.hpp"
#include "regexp/regexpmanager.hpp"
#include "forkcommand.hpp"
#include "connection/connectionmanager.hpp"

#include <boost/bind.hpp>

#include <iostream>
#include <sstream>

#include <lauxlib.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/algorithm/string/trim.hpp>

const unsigned int MAX_SEND_LINES = 10;
const int MAX_COMMAND_RETURN_LINES = 10;
const int MAX_RECURSIONS = 10;
const unsigned int MAX_LINE_LENGTH = 420;

LuaClientGlue::LuaClientGlue(Lua& lua, Client& client)
    : lua_(lua)
    , client_(client)
    , regExpManager_(client.GetConfig().GetRegExpsFilename())
    , reminderManager_(client.GetConfig().GetRemindersFilename())
    , recursions_(0)
{
    AddFunction(&LuaClientGlue::JoinChannel, "Join");
    AddFunction(&LuaClientGlue::Send, "Send");
    AddFunction(&LuaClientGlue::RegisterForEvent, "RegisterForEvent");
    AddFunction(&LuaClientGlue::GetLogName, "GetLogName");
    AddFunction(&LuaClientGlue::GetLastLine, "GetLastLine");
    AddFunction(&LuaClientGlue::GetMyNick, "GetMyNick");
    AddFunction(&LuaClientGlue::GetChannelNicks, "GetChannelNicks");

    AddFunction(&LuaClientGlue::AddRegExp, "RegExpAdd");
    AddFunction(&LuaClientGlue::DeleteRegExp, "RegExpDelete");
    AddFunction(&LuaClientGlue::RegExpMatchAndReply, "RegExpMatchAndReply");
    AddFunction(&LuaClientGlue::RegExpFindMatch, "RegExpFindMatch");
    AddFunction(&LuaClientGlue::RegExpFindRegExp, "RegExpFindRegExp");
    AddFunction(&LuaClientGlue::RegExpChangeReply, "RegExpChangeReply");
    AddFunction(&LuaClientGlue::RegExpChangeRegExp, "RegExpChangeRegExp");
    AddFunction(&LuaClientGlue::RegExpMoveUp, "RegExpMoveUp");
    AddFunction(&LuaClientGlue::RegExpMoveDown, "RegExpMoveDown");

    AddFunction(&LuaClientGlue::RunCommand, "Execute");

    AddFunction(&LuaClientGlue::AddReminder, "ReminderAdd");
    AddFunction(&LuaClientGlue::FindReminder, "ReminderFind");

    AddFunction(&LuaClientGlue::RecurseMessage, "RecurseMessage");

    AddFunction(&LuaClientGlue::RegisterBlockingCall, "RegisterBlockingCall");

    messageHandle_ =
	client_.RegisterForMessages(boost::bind(&LuaClientGlue::OnMessageEvent,
						this, _1, _2, _3, _4));

    CheckReminders();
}

int LuaClientGlue::JoinChannel(lua_State* lua)
{
    int argumentCount = lua_gettop(lua);
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);
    if ( argumentCount >= 3 )
    {
	CheckArgument(lua, 3, LUA_TSTRING);
    }

    std::string server = lua_tostring(lua, 1);
    std::string channel = lua_tostring(lua, 2);
    std::string key;
    if ( argumentCount == 3 )
    {
	key = lua_tostring(lua, 3);
    }

    try
    {
	client_.JoinChannel(server, channel, key);
    }
    catch ( Exception& e)
    {
	return luaL_error(lua, e.GetMessage().c_str());
    }

    return 0;
}

int LuaClientGlue::Send(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);
    CheckArgument(lua, 3, LUA_TSTRING);

    std::string server = lua_tostring(lua, 1);
    std::string target = lua_tostring(lua, 2);
    std::string message= lua_tostring(lua, 3);

    try
    {
	client_.SendMessage(server, target, message);
    }
    catch ( Exception& e)
    {
	luaL_error(lua, e.GetMessage().c_str());
    }

    return 0;
}

int LuaClientGlue::RegisterForEvent(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TFUNCTION);

    std::string event = lua_tostring(lua, 1);

    if ( std::string("ON_MESSAGE") == event )
    {
	try
	{
	    lua_pushvalue(lua, 2);
	    FunctionStatePair function(LuaFunction(lua), lua);
	    lua_pop(lua, 1);
	    eventFunctions_["ON_MESSAGE"].push_back(function);
	}
	catch ( Exception& e )
	{
	    return luaL_error(lua, e.GetMessage().c_str());
	}
    }
    else
    {
	return luaL_error(lua, "Invalid event");
    }
    return 0;
}


int LuaClientGlue::GetLogName(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);

    const char* server = lua_tostring(lua, 1);
    const char* target = lua_tostring(lua, 2);

    std::string logName = client_.GetLogName(server, target);

    lua_pushstring(lua, logName.c_str());
    return 1;
}

int LuaClientGlue::GetLastLine(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);
    CheckArgument(lua, 3, LUA_TSTRING);

    const char* server = lua_tostring(lua, 1);
    const char* target = lua_tostring(lua, 2);
    const char* nick = lua_tostring(lua, 3);

    long timestamp = -1;
    std::string logLine;
    try
    {
	logLine = client_.GetLastLine(server, target, nick, timestamp);
    }
    catch ( Exception& )
    {
    }
    if ( timestamp < 0 )
    {
	return 0;
    }

    lua_pushstring(lua, logLine.c_str());
    lua_pushinteger(lua, time(0)-timestamp);
    return 2;
}

int LuaClientGlue::GetMyNick(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);

    const char* server = lua_tostring(lua, 1);

    std::string nick = client_.GetNick(server);

    lua_pushstring(lua, nick.c_str());
    return 1;
}

int LuaClientGlue::GetChannelNicks(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);

    std::string server = lua_tostring(lua, 1);
    std::string channel = lua_tostring(lua, 2);

    const Server::NickContainer& nicks = client_.GetChannelNicks(server,
								 channel);

    int count = std::distance(nicks.begin(), nicks.end());

    lua_createtable(lua, count, 0);
    int mainTableIndex = lua_gettop(lua);

    for(Server::NickContainer::const_iterator nick = nicks.begin();
	nick != nicks.end();
	++nick)
    {
	lua_pushstring(lua, nick->c_str());
	int currentMainIndex = std::distance(nicks.begin(), nick)+1;
	lua_rawseti(lua, mainTableIndex, currentMainIndex);
    }
    return 1;
}

int LuaClientGlue::AddRegExp(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);

    const char* regexp = lua_tostring(lua, 1);
    const char* reply = lua_tostring(lua, 2);

    RegExpManager::RegExpResult res = regExpManager_.AddRegExp(regexp, reply);
    if ( res.Success )
    {
	lua_pushboolean(lua, 1);
	return 1;
    }
    lua_pushboolean(lua, 0);
    lua_pushstring(lua, res.ErrorMessage.c_str());
    return 2;
}

int LuaClientGlue::DeleteRegExp(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);

    const char* regexp = lua_tostring(lua, 1);

    bool result = regExpManager_.RemoveRegExp(regexp);
    lua_pushboolean(lua, result);
    return 1;
}

int LuaClientGlue::RegExpMatchAndReply(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TFUNCTION);
    
    const char* message = lua_tostring(lua, 1);

    try
    {
	lua_pushvalue(lua, 2);
	regExpOperation_.reset(new FunctionStatePair(LuaFunction(lua), lua));
	lua_pop(lua, 1);
    }
    catch ( Exception& e )
    {
	return luaL_error(lua, e.GetMessage().c_str());
    }

    std::string reply = regExpManager_.FindMatchAndReply(message,
							 boost::bind(&LuaClientGlue::RegExpOperation, this, _1, _2, _3));
    lua_pushstring(lua, reply.c_str());
    return 1;
}

int LuaClientGlue::RegExpFindMatch(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);

    const char* message = lua_tostring(lua, 1);

    RegExpManager::RegExpIteratorRange matches =
	regExpManager_.FindMatches(message);

    return FillRegExpTable(lua, matches);
}

int LuaClientGlue::RegExpFindRegExp(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);

    std::string searchString = lua_tostring(lua, 1);

    RegExpManager::RegExpIteratorRange matches =
	regExpManager_.FindRegExps(searchString);

    return FillRegExpTable(lua, matches);
}

int LuaClientGlue::RegExpChangeReply(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);

    const char* regexp = lua_tostring(lua, 1);
    const char* reply = lua_tostring(lua, 2);

    bool success = regExpManager_.ChangeReply(regexp, reply);
    lua_pushboolean(lua, success);
    return 1;
}

int LuaClientGlue::RegExpChangeRegExp(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);

    const char* regexp = lua_tostring(lua, 1);
    const char* newRegexp = lua_tostring(lua, 2);

    RegExpManager::RegExpResult res = regExpManager_.ChangeRegExp(regexp,
								  newRegexp);
    lua_pushboolean(lua, res.Success);
    if ( res.Success )
    {
	return 1;
    }
    lua_pushstring(lua, res.ErrorMessage.c_str());
    return 2;
}

int LuaClientGlue::RegExpMoveUp(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);

    std::string regexp = lua_tostring(lua, 1);

    bool result = regExpManager_.MoveUp(regexp);

    lua_pushboolean(lua, result);
    return 1;
}

int LuaClientGlue::RegExpMoveDown(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);

    std::string regexp = lua_tostring(lua, 1);

    bool result = regExpManager_.MoveDown(regexp);

    lua_pushboolean(lua, result);
    return 1;
}

int LuaClientGlue::RunCommand(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);

    const char* command = lua_tostring(lua, 1);

    std::string result = ForkCommand(command);

    std::string::size_type pos = result.find('\n');
    int rows = 1;
    while ( pos != std::string::npos && rows < MAX_COMMAND_RETURN_LINES )
    {
	pos=result.find('\n',pos+1);
	++rows;
    }
    if ( pos != std::string::npos )
    {
	// Replace any remaining lines with a single error message
	result.replace(pos+1, std::string::npos, "Too many lines.");
    }

    lua_pushstring(lua, result.c_str());
    return 1;
}

int LuaClientGlue::AddReminder(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TNUMBER);
    CheckArgument(lua, 2, LUA_TSTRING);
    CheckArgument(lua, 3, LUA_TSTRING);
    CheckArgument(lua, 4, LUA_TSTRING);

    time_t seconds = lua_tointeger(lua, 1);
    const char* server = lua_tostring(lua, 2);
    const char* channel = lua_tostring(lua, 3);
    const char* message = lua_tostring(lua, 4);

    reminderManager_.CreateReminder(seconds, server, channel, message);

    CheckReminders();

    return 0;
}

int LuaClientGlue::FindReminder(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);
    CheckArgument(lua, 3, LUA_TSTRING);

    std::string server = lua_tostring(lua, 1);
    std::string channel = lua_tostring(lua, 2);
    std::string searchString = lua_tostring(lua, 3);

    ReminderManager::ReminderIteratorRange reminders =
	reminderManager_.FindReminders(server, channel, searchString);

    int reminderCount = std::distance(reminders.first, reminders.second);
    lua_createtable(lua, reminderCount, 0);
    int mainTableIndex = lua_gettop(lua);

    for(ReminderManager::ReminderIterator reminder = reminders.first;
	reminder != reminders.second;
	++reminder)
    {
	lua_createtable(lua, 2, 0);
	int subTableIndex = lua_gettop(lua);
	lua_pushinteger(lua, reminder->Timestamp-time(0));
	lua_rawseti(lua, subTableIndex, 1);
	lua_pushstring(lua, reminder->Message.c_str());
	lua_rawseti(lua, subTableIndex, 2);
	int currentMainIndex = std::distance(reminders.first, reminder) + 1;
	lua_rawseti(lua, mainTableIndex, currentMainIndex);
    }
    return 1;
}


int LuaClientGlue::RecurseMessage(lua_State* lua)
{
    ++recursions_;
    if ( recursions_ > MAX_RECURSIONS )
    {
	lua_pushstring(lua, "Too many recursions.");
	return 1;
    }

    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);
    CheckArgument(lua, 3, LUA_TSTRING);
    CheckArgument(lua, 4, LUA_TSTRING);
    CheckArgument(lua, 5, LUA_TSTRING);
    CheckArgument(lua, 6, LUA_TSTRING);

    std::string server = lua_tostring(lua, 1);
    std::string fromNick = lua_tostring(lua, 2);
    std::string fromUser = lua_tostring(lua, 3);
    std::string fromHost = lua_tostring(lua, 4);
    std::string to = lua_tostring(lua, 5);
    std::string message = lua_tostring(lua, 6);

    lua_pop(lua, lua_gettop(lua));

    std::cerr<<"Trying to recurse '"<<message<<"'..."<<std::endl;

    StringContainerPtr lines = ProcessMessageEvent(server, fromNick,
						   fromUser, fromHost,
						   to, message);

    std::cerr<<"done"<<std::endl;

    std::stringstream concat;

    std::copy(lines->begin(), lines->end(),
	      std::ostream_iterator<std::string>(concat, "\n"));

    lua_pushstring(lua, concat.str().c_str());
    return 1;
}

int LuaClientGlue::RegisterBlockingCall(lua_State* lua)
{
    bool directOnly = false;
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TFUNCTION);
    if ( lua_gettop(lua) >= 3 )
    {
	CheckArgument(lua, 3, LUA_TBOOLEAN);
	directOnly = lua_toboolean(lua, 3);
    }

    std::string matchString = lua_tostring(lua, 1);

    try
    {
	boost::regex regexp(matchString, boost::regex::extended);

	// Push function at position 2 on top of stack in case there are
	// other things between the function and the top of the stack
	lua_pushvalue(lua, 2);
	FunctionStatePair function(LuaFunction(lua), lua);
	lua_pop(lua, 1);

	blockingCalls_.push_back(BlockingCall(regexp, function, directOnly));
    }
    catch ( boost::regex_error& e )
    {
	return luaL_error(lua, e.what());
    }
    catch ( Exception& e )
    {
	return luaL_error(lua, e.GetMessage().c_str());
    }

    return 0;
}


void LuaClientGlue::CheckArgument(lua_State* lua, int argumentNumber,
				  int expectedType)
{
    std::string message = "";
    int argumentCount = lua_gettop(lua);
    const char* expectedTypeName = lua_typename(lua, expectedType);
    if ( expectedTypeName != 0 )
    {
	std::string actualTypeName = "no value";
	if ( argumentCount >= argumentNumber )
	{
	    const char* typeName = lua_typename(lua,
						lua_type(lua, argumentNumber));
	    if ( typeName != 0 )
	    {
		actualTypeName = typeName;
	    }
	}

	message = std::string("Expected ") + expectedTypeName
	    + std::string(", got ") + actualTypeName;
    }
    return luaL_argcheck(lua,
			 argumentCount >= argumentNumber &&
			 lua_type(lua, argumentNumber) == expectedType,
			 argumentNumber, message.c_str());
}


void LuaClientGlue::OnMessageEvent(const std::string& server,
				   const Irc::Prefix& from,
				   const std::string& to,
				   const std::string& message)
{
    // Extract nick, user and host from the from-user string (nick!user@host)
    const std::string& fromNick = from.GetNick();
    const std::string& fromUser = from.GetUser();
    const std::string& fromHost = from.GetHost();

    // If the message isn't directly sent to us we reply to the channel
    // it was sent to. If it was sent directly to us we reply to the sender
    std::string replyTo = fromNick;
    if ( to != client_.GetNick(server) )
    {
	replyTo = to;
    }

    recursions_ = 0;
    StringContainerPtr lines = ProcessMessageEvent(server, fromNick, fromUser,
						   fromHost, to, message);
    recursions_ = 0;

    try
    {
	unsigned int lineCount = 0;
	for(StringContainer::iterator line = lines->begin();
	    line != lines->end();
	    ++line)
	{
	    std::string msg = *line;
	    while (msg.size() > MAX_LINE_LENGTH && lineCount < MAX_SEND_LINES)
	    {
		// Find last space before the line exceeds length
		std::string::size_type pos = msg.rfind(" ", MAX_LINE_LENGTH);
		if ( pos == std::string::npos )
		{
		    // If no such space we just split at the limit
		    pos = MAX_LINE_LENGTH;
		}
		// Send the limited string, remove it from the line
		// and increase the line count
		client_.SendMessage(server, replyTo, msg.substr(0, pos));
		msg.erase(0, pos+1);
		++lineCount;
	    }

	    if ( lineCount >= MAX_SEND_LINES )
	    {
		client_.SendMessage(server, replyTo, "Too many lines.");
		break;
	    }
	    client_.SendMessage(server, replyTo, msg);
	    ++lineCount;
	}
    }
    catch ( Exception& )
    {
	// This is bad, can't really handle so log and rethrow
	std::cerr<<"Received event with server tag that matches no server"
		 <<std::endl;
	throw;
    }
}

void LuaClientGlue::OnTimerEvent()
{
    ReminderManager::ReminderIteratorRange iterators;
    iterators = reminderManager_.GetDueReminders();

    for(ReminderManager::ReminderIterator reminder = iterators.first;
	reminder != iterators.second;
	++reminder)
    {
	client_.SendMessage(reminder->Server, reminder->Channel,
			    reminder->Message);
    }

    CheckReminders();
}

std::string LuaClientGlue::RegExpOperation(const std::string& reply,
					   const std::string& message,
					   const RegExp& regexp)
{
    std::string result = reply;
    if ( regExpOperation_ )
    {
	lua_State* lua = regExpOperation_->second;
	if ( lua && lua_status(lua) == 0 )
	{
	    regExpOperation_->first.Push();
	    if ( lua_isfunction(lua, -1) != 0 )
	    {
		lua_pushstring(lua, reply.c_str());
		lua_pushstring(lua, message.c_str());
		lua_pushstring(lua, regexp.GetRegExp().c_str());
		lua_pushstring(lua, regexp.GetReply().c_str());
		if ( lua_pcall(lua, 4, 1, 0) == 0 )
		{
		    if ( lua_isstring(lua, -1) )
		    {
			const char* r = lua_tostring(lua, -1);
			if ( r ) { result = r; }
		    }
		    lua_pop(lua, 1);
		}
		else
		{
		    const char* message = lua_tostring(lua, -1);
		    if ( message != 0 )
		    {
			result = message;
		    }
		    lua_pop(lua, 1);
		}
	    }
	}
    }
    return result;
}

void LuaClientGlue::AddFunction(Function f, const std::string& name)
{
    functionHandles_.push_back(
	lua_.RegisterFunction(name, boost::bind(f, this, _1)));
}

void LuaClientGlue::CheckReminders()
{
    try
    {
	const ReminderManager::Reminder& r=reminderManager_.GetNextReminder();
	ConnectionManager& connectionManager=ConnectionManager::Instance();

	connectionManager.RegisterTimer(r.Timestamp-time(0),
			boost::bind(&LuaClientGlue::OnTimerEvent, this));

    }
    catch ( Exception& )
    {
    }
}

int LuaClientGlue::FillRegExpTable(lua_State* lua,
				   RegExpManager::RegExpIteratorRange regExps)
{
    int count = std::distance(regExps.first, regExps.second);

    lua_createtable(lua, count, 0);
    int mainTableIndex = lua_gettop(lua);

    for(RegExpManager::RegExpIterator regExp = regExps.first;
	regExp != regExps.second;
	++regExp)
    {
	lua_createtable(lua, 3, 0);
	int subTableIndex = lua_gettop(lua);
	lua_pushstring(lua, regExp->GetRegExp().c_str());
	lua_rawseti(lua, subTableIndex, 1);
	lua_pushstring(lua, regExp->GetReply().c_str());
	lua_rawseti(lua, subTableIndex, 2);
	lua_pushstring(lua,
		       regExpManager_.Encode(regExp->GetRegExp()).c_str());
	lua_rawseti(lua, subTableIndex, 3);
	int currentMainIndex = std::distance(regExps.first, regExp)+1;
	lua_rawseti(lua, mainTableIndex, currentMainIndex);
    }
    return 1;
}


LuaClientGlue::StringContainerPtr
LuaClientGlue::ProcessMessageEvent(const std::string& server,
				   const std::string& fromNick,
				   const std::string& fromUser,
				   const std::string& fromHost,
				   const std::string& to,
				   const std::string& message)
{
    StringContainerPtr result(new StringContainer());

    std::string directMessage = message;
    std::string nick = client_.GetNick(server);
    bool direct = nick == to; // Message is direct if privmsg to us
    if ( message.find(nick+":") == 0 || message.find(nick+",") == 0 )
    {
	directMessage = message.substr(nick.size()+1);
	boost::algorithm::trim_left(directMessage);
	direct = true;
	std::cout<<"Scrubbed blocking call string: '"<<directMessage
		 <<"'"<<std::endl;
    }

    for(BlockingCallContainer::iterator blockingCall = blockingCalls_.begin();
	blockingCall != blockingCalls_.end();
	++blockingCall)
    {

	if ( ( direct || !blockingCall->directOnly_ ) &&
	     boost::regex_search(directMessage, blockingCall->regexp_) )
	{
	    std::cout<<"blocking call matched"<<std::endl;
	    result = CallEventHandler(blockingCall->functionStatePair_,
				      server, fromNick, fromUser, fromHost,
				      to, directMessage);
	    if ( result && result->size() > 0 )
	    {
		return result;
	    }
	    std::cout<<"blocking call gave no results"<<std::endl;
	}
    }

    for(FunctionContainer::iterator handler
	    = eventFunctions_["ON_MESSAGE"].begin();
	handler != eventFunctions_["ON_MESSAGE"].end();
	++handler)
    {
	StringContainerPtr localResult = CallEventHandler(*handler, server,
							  fromNick, fromUser,
							  fromHost, to,
							  message);
	if ( localResult && localResult->size() > 0 )
	{
	    result->splice(result->end(), *localResult);
	}
    }
    return result;
}

LuaClientGlue::StringContainerPtr
LuaClientGlue::CallEventHandler(FunctionStatePair functionStatePair,
				     const std::string& server,
				     const std::string& fromNick,
				     const std::string& fromUser,
				     const std::string& fromHost,
				     const std::string& to,
				     const std::string& message)
{
    StringContainerPtr result(new StringContainer());

    lua_State* lua = functionStatePair.second;
    if ( lua && lua_status(lua) == 0 )
    {
	functionStatePair.first.Push();
	
	lua_pushstring(lua, server.c_str());
	lua_pushstring(lua, fromNick.c_str());
	lua_pushstring(lua, fromUser.c_str());
	lua_pushstring(lua, fromHost.c_str());
	lua_pushstring(lua, to.c_str());
	lua_pushstring(lua, message.c_str());
	if ( lua_pcall(lua, 6, LUA_MULTRET, 0) == 0 )
	{
	    int resultCount = lua_gettop(lua);
	    
	    for(int resultNumber = 1;
		resultNumber <= resultCount &&
		    result->size() <= MAX_SEND_LINES;
		++resultNumber)
	    {
		const char* message = lua_tostring(lua, resultNumber);
		if ( message )
		{
		    result->push_back(message);
		}
		else if ( lua_type(lua, resultNumber) == LUA_TTABLE )
		{
		    size_t tableSize = lua_objlen(lua, resultNumber);
		    for(unsigned int tableIndex=1;
			tableIndex<=tableSize &&
			    result->size() <= MAX_SEND_LINES;
			++tableIndex)
		    {
			lua_rawgeti(lua, resultNumber, tableIndex);
			message = lua_tostring(lua, -1);
			if ( message )
			{
			    result->push_back(message);
			}
			lua_pop(lua, 1);
		    }
		}
	    }
	    lua_pop(lua, resultCount);
	}
	else
	{
	    const char* message = lua_tostring(lua, -1);
	    if ( message )
	    {
		result->push_back(message);
	    }
	    lua_pop(lua, lua_gettop(lua));
	}
	assert(lua_gettop(lua) == 0);
    }
    return result;
}
