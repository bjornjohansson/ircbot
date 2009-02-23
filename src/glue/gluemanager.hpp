#pragma once

#include <vector>
#include <string>

#include "glue.hpp"
#include "../lua/lua.hpp"
#include "../lua/luafunction.hpp"
#include "../irc/prefix.hpp"

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

class Client;

class GlueManager
{
public:
    static GlueManager& Instance();

    void RegisterGlue(Glue* glue);

    void Reset(boost::shared_ptr<Lua> lua, Client* client);

    typedef boost::function<int (lua_State*)> GlueFunction;
    void AddFunction(GlueFunction, const std::string& name);

private:
    GlueManager();
    GlueManager(const GlueManager&);
    GlueManager& operator=(const GlueManager&);

    typedef std::vector<Glue*> GlueContainer;
    GlueContainer glues_;

    void CheckArgument(lua_State* lua, int argumentNumber, int expectedType);

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

    boost::shared_ptr<Lua> lua_;
    Client* client_;

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
