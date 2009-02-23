#include "glue.hpp"

#include <string>
#include <lauxlib.h>

#include <iostream>

void Glue::Reset(boost::shared_ptr<Lua> lua,
		 Client* client)
{
    lua_ = lua;
    client_ = client;
    AddFunctions();
}

void Glue::CheckArgument(lua_State* lua, int argumentNumber, int expectedType)
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

