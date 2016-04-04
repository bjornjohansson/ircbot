#include "glue.hpp"
#include "gluemanager.hpp"
#include "../message.hpp"
#include "../client.hpp"
#include "../lua/luafunction.hpp"
#include "../exception.hpp"

#include <string>
#include <list>
#include <map>

#include <unicode/unistr.h>
#include <converter.hpp>

#ifdef LUA_EXTERN
extern "C"
{
#include <lauxlib.h>
}
#else
#include <lauxlib.h>
#endif

const unsigned int MAX_RESULT_LINES = 10;

class EventGlue: public Glue
{
public:
    EventGlue();

    void Reset(boost::shared_ptr<Lua> lua, Client* client);

private:
    void AddFunctions();

    typedef std::pair<LuaFunction, lua_State*> FunctionStatePair;

    typedef std::list<UnicodeString> StringContainer;
    typedef boost::shared_ptr<StringContainer> StringContainerPtr;

    StringContainerPtr CallEventHandlers(FunctionStatePair function,
            const UnicodeString& server, const Message& message);

    typedef std::map<Irc::Command::Command, Client::EventReceiverHandle>
            EventReceiverHandleContainer;
    EventReceiverHandleContainer eventReceiverHandles_;
};

EventGlue eventGlue;

EventGlue::EventGlue()
{
    GlueManager::Instance().RegisterGlue(this);
}

void EventGlue::Reset(boost::shared_ptr<Lua>, Client*)
{

}

void EventGlue::AddFunctions()
{
}

EventGlue::StringContainerPtr EventGlue::CallEventHandlers(
        FunctionStatePair functionStatePair, const UnicodeString& server,
        const Message& message)
{
    StringContainerPtr result(new StringContainer());

    lua_State* lua = functionStatePair.second;
    if (lua && lua_status(lua) == 0)
    {
        functionStatePair.first.Push();

        int argCount = 4;
        lua_pushstring(lua, AsUtf8(server).c_str());
        lua_pushstring(lua, message.GetPrefix().GetNick().c_str());
        lua_pushstring(lua, message.GetPrefix().GetUser().c_str());
        lua_pushstring(lua, message.GetPrefix().GetHost().c_str());
        for (Message::const_iterator param = message.begin(); param
                != message.end(); ++param, ++argCount)
        {
            lua_pushstring(lua, param->c_str());
        }
        if (lua_->FunctionCall(lua, argCount, LUA_MULTRET) == 0)
        {
            int resultCount = lua_gettop(lua);

            for (int resultNumber = 1; resultNumber <= resultCount
                    && result->size() <= MAX_RESULT_LINES; ++resultNumber)
            {
                const char* message = lua_tostring(lua, resultNumber);
                if (message)
                {
                    result->push_back(message);
                }
                else if (lua_type(lua, resultNumber) == LUA_TTABLE)
                {
                    size_t tableSize = lua_objlen(lua, resultNumber);
                    for (unsigned int tableIndex = 1; tableIndex <= tableSize
                            && result->size() <= MAX_RESULT_LINES; ++tableIndex)
                    {
                        lua_rawgeti(lua, resultNumber, tableIndex);
                        message = lua_tostring(lua, -1);
                        if (message)
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
            if (message)
            {
                result->push_back(message);
            }
            lua_pop(lua, lua_gettop(lua));
        }
        assert(lua_gettop(lua) == 0);
    }
    return result;

}
