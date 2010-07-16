#pragma once

#include "luafunction.fwd.hpp"

#include <string>
#include <map>
#include <list>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/thread.hpp>

#include <unicode/unistr.h>

#ifdef LUA_EXTERN
extern "C" {
#include <lua.h>
}
#else
#include <lua.h>
#endif


class Lua
{
public:
    Lua(const UnicodeString& scriptsDirectory);
    virtual ~Lua();

    void LoadScripts();

    typedef boost::function<int (lua_State*)> LuaFunc;
    typedef boost::shared_ptr<LuaFunc> LuaFunctionHandle;
    LuaFunctionHandle RegisterFunction(const UnicodeString& name,
				       LuaFunc f,
				       bool replace = true);

    int FunctionCall(lua_State* lua, int argCount, int resultCount);

private:
    static int CallDispatch(lua_State* lua);

    /**
     * @throw Exception if file cannot be loaded
     */
    LuaFunction LoadFile(const std::string& filename);

    static void LuaHook(lua_State* lua, lua_Debug* debug);

    typedef std::map<lua_State*, time_t> StateCallTimeMap;
    static StateCallTimeMap stateCallTimes_;
    static boost::shared_mutex callTimeMutex_;

    lua_State* lua_;
    UnicodeString scriptsDirectory_;

    typedef boost::weak_ptr<LuaFunc> LuaFunctionWeakPtr;
    typedef std::pair<LuaFunctionWeakPtr,lua_State*> FunctionStatePair;
    typedef std::map<UnicodeString, FunctionStatePair> LuaFunctionsMap;
    static LuaFunctionsMap luaFunctions_;
    typedef std::list<UnicodeString> StringContainer;
    StringContainer myRegisteredFunctions_;
};
