#pragma once

#include <boost/shared_ptr.hpp>

#include <utility>

#include <lua.h>

class Client;
class Lua;

class Glue
{
public:
    virtual void Reset(boost::shared_ptr<Lua>,
		       Client*);
protected:
    virtual void AddFunctions() = 0;
    void CheckArgument(lua_State* lua, int argumentNumber, int expectedType);

    boost::shared_ptr<Lua> lua_;
    Client* client_;
};