#include "glue.hpp"
#include "gluemanager.hpp"
#include "../client.hpp"
#include "../exception.hpp"
#include "../irc/prefix.hpp"
#include "../lua/luafunction.hpp"

#include <boost/bind.hpp>
#include <boost/algorithm/string/trim.hpp>

#ifdef LUA_EXTERN
extern "C" {
#include <lauxlib.h>
}
#else
#include <lauxlib.h>
#endif

const int MAX_RECURSIONS = 10;
const unsigned int MAX_SEND_LINES = 10;
const unsigned int MAX_LINE_LENGTH = 420;

class MessageGlue : public Glue
{
public:
    MessageGlue();

    void Reset(boost::shared_ptr<Lua> lua,
	       Client* client);

    int Send(lua_State* lua);
    int RecurseMessage(lua_State* lua);
    int RegisterForEvent(lua_State* lua);
    int RegisterBlockingCall(lua_State* lua);

private:
    void AddFunctions();

    void OnMessageEvent(const std::string& server, const Irc::Prefix& from,
			const std::string& to, const std::string& message);

    typedef std::list<std::string> StringContainer;
    typedef boost::shared_ptr<StringContainer> StringContainerPtr;
    StringContainerPtr ProcessMessageEvent(const std::string& server,
					   const std::string& fromNick,
					   const std::string& fromUser,
					   const std::string& fromHost,
					   const std::string& to,
					   const std::string& message);

    typedef std::pair<LuaFunction,lua_State*> FunctionStatePair;
    StringContainerPtr CallEventHandler(FunctionStatePair functionStatePair,
					const std::string& server,
					const std::string& fromNick,
					const std::string& fromUser,
					const std::string& fromHost,
					const std::string& to,
					const std::string& message);

    typedef std::list<FunctionStatePair> FunctionContainer;
    typedef std::map<std::string,FunctionContainer> EventFunctionMap;
    EventFunctionMap eventFunctions_;

    struct BlockingCall
    {
	BlockingCall(boost::regex regexp, FunctionStatePair pair, bool direct)
	    : regexp_(regexp), functionStatePair_(pair), directOnly_(direct)
	{
	}
	boost::regex regexp_;
	FunctionStatePair functionStatePair_;
	bool directOnly_;
    };
    typedef std::list<BlockingCall> BlockingCallContainer;
    BlockingCallContainer blockingCalls_;

    Client::MessageEventReceiverHandle messageHandle_;
    int recursions_;

    std::string lastServer_;
    std::string lastFromNick_;
    std::string lastFromUser_;
    std::string lastFromHost_;
    std::string lastTo_;
};

MessageGlue messageGlue;

MessageGlue::MessageGlue()
{
    GlueManager::Instance().RegisterGlue(this);
}

void MessageGlue::AddFunctions()
{
    GlueManager::Instance().AddFunction(boost::bind(&MessageGlue::Send,
						    this, _1), "Send");
    GlueManager::Instance().AddFunction(boost::bind(
					    &MessageGlue::RecurseMessage,
					    this, _1),
					"RecurseMessage");
    GlueManager::Instance().AddFunction(boost::bind(
					    &MessageGlue::RegisterForEvent,
					    this, _1),
					"RegisterForEvent");
    GlueManager::Instance().AddFunction(boost::bind(
					    &MessageGlue::RegisterBlockingCall,
					    this, _1),
					"RegisterBlockingCall");
}

void MessageGlue::Reset(boost::shared_ptr<Lua> lua,
			Client* client)
{
    Glue::Reset(lua, client);
    messageHandle_ =
	client_->RegisterForMessages(boost::bind(&MessageGlue::OnMessageEvent,
						 this, _1, _2, _3, _4));
}


int MessageGlue::Send(lua_State* lua)
{
    try
    {
	std::string server, target, message;
	int argumentCount = lua_gettop(lua);
	CheckArgument(lua, 1, LUA_TSTRING);
	message= lua_tostring(lua, 1);
	if ( argumentCount >= 2 )
	{
	    CheckArgument(lua, 2, LUA_TSTRING);
	    target = lua_tostring(lua, 2);
	    if ( argumentCount >= 3 )
	    {
		CheckArgument(lua, 3, LUA_TSTRING);
		server = lua_tostring(lua, 3);
	    }
	}
	client_->SendMessage(message, target, server);
    }
    catch ( Exception& e)
    {
	luaL_error(lua, e.GetMessage().c_str());
    }

    return 0;
}

int MessageGlue::RecurseMessage(lua_State* lua)
{
    ++recursions_;
    if ( recursions_ > MAX_RECURSIONS )
    {
	lua_pushstring(lua, "Too many recursions.");
	return 1;
    }

    std::string server = lastServer_;
    std::string fromNick = lastFromNick_;
    std::string fromUser = lastFromUser_;
    std::string fromHost = lastFromHost_;
    std::string to = lastTo_;
    std::string message;

    int argumentCount = lua_gettop(lua);
    CheckArgument(lua, 1, LUA_TSTRING);
    message = lua_tostring(lua, 1);
    if ( argumentCount >= 2 )
    {
	CheckArgument(lua, 2, LUA_TSTRING);
	to = lua_tostring(lua, 2);
	if ( argumentCount >= 3 )
	{
	    CheckArgument(lua, 3, LUA_TSTRING);
	    fromNick = lua_tostring(lua, 3);
	    if ( argumentCount >= 4 )
	    {
		CheckArgument(lua, 4, LUA_TSTRING);
		fromUser = lua_tostring(lua, 4);
		if ( argumentCount >= 5 )
		{
		    CheckArgument(lua, 5, LUA_TSTRING);
		    fromHost = lua_tostring(lua, 5);
		    if ( argumentCount >= 6 )
		    {
			CheckArgument(lua, 6, LUA_TSTRING);
			server = lua_tostring(lua, 6);
		    }
		}
	    }
	}
    }

    lua_pop(lua, lua_gettop(lua));

    StringContainerPtr lines = ProcessMessageEvent(server, fromNick,
						   fromUser, fromHost,
						   to, message);

    std::stringstream concat;

    std::copy(lines->begin(), lines->end(),
	      std::ostream_iterator<std::string>(concat, "\n"));

    lua_pushstring(lua, concat.str().c_str());
    return 1;
}

int MessageGlue::RegisterForEvent(lua_State* lua)
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

int MessageGlue::RegisterBlockingCall(lua_State* lua)
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

void MessageGlue::OnMessageEvent(const std::string& server,
				 const Irc::Prefix& from,
				 const std::string& to,
				 const std::string& message)
{
    // Extract nick, user and host from the from-user string (nick!user@host)
    const std::string& fromNick = from.GetNick();
    const std::string& fromUser = from.GetUser();
    const std::string& fromHost = from.GetHost();

    lastServer_ = server;
    lastFromNick_ = fromNick;
    lastFromUser_ = fromUser;
    lastFromHost_ = fromHost;
    lastTo_ = to;

    // If the message was sent to a channel we reply to that channel.
    // If it was sent directly to us we reply to the sender
    std::string replyTo = fromNick;
    if ( to.find_first_of("&#") == 0 )
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
		client_->SendMessage(msg.substr(0, pos), replyTo, server);
		msg.erase(0, pos+1);
		++lineCount;
	    }

	    if ( lineCount >= MAX_SEND_LINES )
	    {
		client_->SendMessage("Too many lines.", replyTo, server);
		break;
	    }
	    client_->SendMessage(msg, replyTo, server);
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

MessageGlue::StringContainerPtr
MessageGlue::ProcessMessageEvent(const std::string& server,
				 const std::string& fromNick,
				 const std::string& fromUser,
				 const std::string& fromHost,
				 const std::string& to,
				 const std::string& message)
{
    StringContainerPtr result(new StringContainer());

    std::string directMessage = message;
    std::string nick = client_->GetNick(server);
    // Message is direct if the to-field does not begin with channel identifier
    bool direct = to.find_first_of("&#") != 0;
    if ( message.find(nick+":") == 0 || message.find(nick+",") == 0 )
    {
	directMessage = message.substr(nick.size()+1);
	boost::algorithm::trim_left(directMessage);
	direct = true;
    }

    for(BlockingCallContainer::iterator blockingCall = blockingCalls_.begin();
	blockingCall != blockingCalls_.end();
	++blockingCall)
    {

	if ( ( direct || !blockingCall->directOnly_ ) &&
	     boost::regex_search(directMessage, blockingCall->regexp_) )
	{
	    result = CallEventHandler(blockingCall->functionStatePair_,
				      server, fromNick, fromUser, fromHost,
				      to, directMessage);
	    if ( result && result->size() > 0 )
	    {
		return result;
	    }
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

MessageGlue::StringContainerPtr
MessageGlue::CallEventHandler(FunctionStatePair functionStatePair,
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
	if ( lua_->FunctionCall(lua, 6, LUA_MULTRET) == 0 )
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
