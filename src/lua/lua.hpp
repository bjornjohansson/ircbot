#pragma once

#include "luafunction.hpp"

#include <string>
#include <map>
#include <list>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <lua.h>

class Lua
{
public:
    Lua(const std::string& scriptsDirectory);
    virtual ~Lua();

    void LoadScripts();

    typedef boost::function<int (lua_State*)> LuaFunc;
    typedef boost::shared_ptr<LuaFunc> LuaFunctionHandle;
    LuaFunctionHandle RegisterFunction(const std::string& name,
				       LuaFunc f,
				       bool replace = true);

    static int CallDispatch(lua_State* lua);

    int FunctionCall(lua_State* lua);

private:
    /**
     * @throw Exception if file cannot be loaded
     */
    LuaFunction LoadFile(const std::string& filename);

    lua_State* lua_;
    std::string scriptsDirectory_;

    typedef boost::weak_ptr<LuaFunc> LuaFunctionWeakPtr;
    typedef std::pair<LuaFunctionWeakPtr,lua_State*> FunctionStatePair;
    typedef std::map<std::string, FunctionStatePair> LuaFunctionsMap;
    static LuaFunctionsMap luaFunctions_;
    typedef std::list<std::string> StringContainer;
    StringContainer myRegisteredFunctions_;
};
