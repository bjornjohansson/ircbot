#include "lua.hpp"
#include "../exception.hpp"
#include "luafunction.hpp"

#include <iostream>
#include <functional>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/bind.hpp>

#include <lualib.h>
#include <lauxlib.h>

Lua::LuaFunctionsMap Lua::luaFunctions_;

Lua::Lua(const std::string& scriptsDirectory)
    : lua_(lua_open()),
      scriptsDirectory_(scriptsDirectory)
{
    if ( lua_ )
    {
	const luaL_Reg libs[] = {
	    {"", luaopen_base},
	    {LUA_TABLIBNAME, luaopen_table},
	    {LUA_STRLIBNAME, luaopen_string},
	    {LUA_MATHLIBNAME, luaopen_math},
	    {0, 0}
	};

	const luaL_Reg* lib = libs;

	for(;lib->func; ++lib)
	{
	    lua_pushcfunction(lua_, lib->func);
	    lua_pushstring(lua_, lib->name);
	    lua_call(lua_, 1, 0);
	}
    }
}

Lua::~Lua()
{
    for(StringContainer::iterator i = myRegisteredFunctions_.begin();
	i != myRegisteredFunctions_.end();
	++i)
    {
	LuaFunctionsMap::iterator f = luaFunctions_.find(*i);
	if ( f != luaFunctions_.end() )
	{
	    if ( LuaFunctionHandle func = f->second.first.lock() )
	    {
		if ( f->second.second == lua_ )
		{
		    luaFunctions_.erase(*i);
		}
	    }
	}
    }

    if ( lua_ )
    {
	lua_close(lua_);
    }
}

void Lua::LoadScripts()
{
    std::vector<LuaFunction> functions;

    for(boost::filesystem::directory_iterator i(scriptsDirectory_);
	i != boost::filesystem::directory_iterator();
	++i)
    {
	std::string path = i->string();
	if ( !boost::filesystem::is_directory(*i) && 
	     path.rfind(".lua") == path.size()-strlen(".lua") )
	{
	    std::cout<<"Loading '"<<path<<"'..."<<std::flush;
	    try
	    {
		LuaFunction function = LoadFile(path);
		functions.push_back(function);
		std::cout<<"done"<<std::endl;
	    }
	    catch ( Exception& e )
	    {
		std::cout<<"failed: '"<<e.GetMessage()<<"'"<<std::endl;
	    }
	}
    }

    for(std::vector<LuaFunction>::iterator i = functions.begin();
	i != functions.end();
	++i)
    {

	i->Push();
	if ( lua_pcall(lua_, 0, 0, 0) != 0 )
	{
	    std::string message = lua_tostring(lua_, -1);
	    lua_pop(lua_, -1);
	    std::cerr<<"Error: "<<message<<std::endl;
	}
    }
}

Lua::LuaFunctionHandle Lua::RegisterFunction(const std::string& name,
					     LuaFunc f,
					     bool replace)
{
    if ( luaFunctions_.find(name) != luaFunctions_.end() && !replace )
    {
	throw Exception(__FILE__, __LINE__, "Function already registered");
    }

    LuaFunctionHandle result(new LuaFunc(f));

    luaFunctions_[name] = FunctionStatePair(LuaFunctionWeakPtr(result), lua_);
    myRegisteredFunctions_.push_back(name);

    lua_pushstring(lua_, name.c_str());
    lua_pushcclosure(lua_, &Lua::CallDispatch, 1);
    lua_setglobal(lua_, name.c_str());

    return result;
}


int Lua::CallDispatch(lua_State* lua)
{
    std::string name = lua_tostring(lua, lua_upvalueindex(1));

    LuaFunctionsMap::iterator f = luaFunctions_.find(name);

    if ( f != luaFunctions_.end() )
    {
	if ( LuaFunctionHandle func = f->second.first.lock() )
	{
	    try
	    {
		return (*func)(lua);
	    }
	    catch(Exception& e)
	    {
		return luaL_error(lua, e.GetMessage().c_str());
	    }
	}
	else
	{
	    luaFunctions_.erase(f);
	    return luaL_error(lua, "This function no longer exists");
	}
    }

    // No matching object for this name
    return luaL_error(lua, "This function does not exist");
}

LuaFunction Lua::LoadFile(const std::string& filename)
{
    int result = luaL_loadfile(lua_, filename.c_str());
    if ( result != 0 )
    {
        const char* msg = lua_tostring(lua_, -1);
	std::string errorMessage;
	if ( msg )
	{
	    errorMessage = msg;
	}
	else if ( result == LUA_ERRSYNTAX )
	{
	    errorMessage = "Syntax error during pre-compilation...";
	}
	else if ( result == LUA_ERRMEM )
	{
	    errorMessage = "Memory allocation error...";
	}
	else if ( result == LUA_ERRFILE )
	{
	    errorMessage = "Cannot open or read file...";
	}
	throw Exception(__FILE__, __LINE__, errorMessage);
    }

    // Create a lua function that is the file we just loaded
    // Then remove the function from the stack
    LuaFunction luaFunction(lua_);
    lua_pop(lua_,1);

    return luaFunction;
}
