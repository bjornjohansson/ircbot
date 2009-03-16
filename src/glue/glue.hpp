#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <utility>
#include <list>

#ifdef LUA_EXTERN
extern "C" {
#include <lua.h>
}
#else
#include <lua.h>
#endif

class Client;

#include "../lua/lua.hpp"

class Glue
{
public:
    virtual void Reset(boost::shared_ptr<Lua>,
		       Client*);
protected:
    virtual void AddFunctions() = 0;
    void CheckArgument(lua_State* lua, int argumentNumber, int expectedType);

    typedef boost::function<int (lua_State*)> GlueFunction;
    void AddFunction(GlueFunction, const std::string& name);

    typedef std::list<Lua::LuaFunctionHandle> FunctionHandleContainer;
    FunctionHandleContainer functionHandles_;

    boost::shared_ptr<Lua> lua_;
    Client* client_;
};
