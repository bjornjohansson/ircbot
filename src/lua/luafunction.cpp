#include "luafunction.hpp"
#include "../exception.hpp"

#include <lualib.h>
#include <lauxlib.h>

LuaFunction::LuaFunction(lua_State* lua)
    : lua_(lua)
{
    int top = lua_gettop(lua);
    if ( !lua_isfunction(lua, top) )
    {
	throw Exception(__FILE__, __LINE__,
			"Value on top of the stack is not a function");
    }
    // Duplicate the value on top of the stack
    lua_pushvalue(lua, lua_gettop(lua));
    // Create a reference to that value (which also pops it) in the registry
    reference_.reset(new Reference(lua));
}

void LuaFunction::Push()
{
    if ( lua_ && lua_status(lua_) == 0 && reference_ )
    {
	lua_rawgeti(lua_, LUA_REGISTRYINDEX, reference_->GetReference());
    }
    else
    {
	throw Exception(__FILE__, __LINE__,
			"Can't push function, invalid lua state or reference");
    }
}

LuaFunction::Reference::Reference(lua_State* lua)
    : lua_(lua),
      reference_(luaL_ref(lua, LUA_REGISTRYINDEX))
{
}

LuaFunction::Reference::~Reference()
{
    if ( lua_ && lua_status(lua_) == 0 )
    {
	luaL_unref(lua_, LUA_REGISTRYINDEX, reference_);
    }
}
