#pragma once

#include "lua/luafunction.hpp"
#include "lua/lua.hpp"
#include "regexp/regexpmanager.hpp"
#include "remindermanager.hpp"
#include "irc/prefix.hpp"

#include <string>
#include <list>
#include <map>
#include <vector>

#include <lua.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/regex.hpp>

class Client;
class RegExpManager;

class LuaClientGlue
{
public:
    LuaClientGlue(Lua& lua, Client& client);

    int JoinChannel(lua_State* lua);
    int Send(lua_State* lua);
    int RegisterForEvent(lua_State* lua);
    int GetLogName(lua_State* lua);
    int GetLastLine(lua_State* lua);
    int GetMyNick(lua_State* lua);
    int GetChannelNicks(lua_State* lua);

    int AddRegExp(lua_State* lua);
    int DeleteRegExp(lua_State* lua);
    int RegExpMatchAndReply(lua_State* lua);
    int RegExpFindMatch(lua_State* lua);
    int RegExpFindRegExp(lua_State* lua);
    int RegExpChangeReply(lua_State* lua);
    int RegExpChangeRegExp(lua_State* lua);
    int RegExpMoveUp(lua_State* lua);
    int RegExpMoveDown(lua_State* lua);

    int RunCommand(lua_State* lua);

    int AddReminder(lua_State* lua);
    int FindReminder(lua_State* lua);

    int RecurseMessage(lua_State* lua);

    int RegisterBlockingCall(lua_State* lua);

    void OnMessageEvent(const std::string& server, const Irc::Prefix& from,
			const std::string& to, const std::string& message);

    std::string RegExpOperation(const std::string& reply,
				const std::string& message,
				const RegExp& regexp);

private:
    typedef int (LuaClientGlue::*Function)(lua_State*);
    void AddFunction(Function f, const std::string& name);

    void CheckArgument(lua_State* lua, int argumentNumber, int expectedType);

    int FillRegExpTable(lua_State* lua,
			RegExpManager::RegExpIteratorRange regExps);

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

    Lua& lua_;
    Client& client_;

    typedef std::list<FunctionStatePair> FunctionContainer;
    typedef std::map<std::string,FunctionContainer> EventFunctionMap;
    EventFunctionMap eventFunctions_;

    typedef std::list<Lua::LuaFunctionHandle> FunctionHandleContainer;
    FunctionHandleContainer functionHandles_;

    typedef boost::function<void (const std::string&, //server
				  const Irc::Prefix&, //from
				  const std::string&, //to, message
				  const std::string&)> MessageEventReceiver;
    typedef boost::shared_ptr<MessageEventReceiver> MessageEventReceiverHandle;
    MessageEventReceiverHandle messageHandle_;

    RegExpManager regExpManager_;
    ReminderManager reminderManager_;

    typedef boost::shared_ptr<FunctionStatePair> FunctionStatePairPtr;
    FunctionStatePairPtr regExpOperation_;

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

    int recursions_;
};
