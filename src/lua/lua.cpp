#include "lua.hpp"
#include "../exception.hpp"
#include "luafunction.hpp"
#include "../logging/logger.hpp"

#include <functional>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/bind.hpp>

#ifdef LUA_EXTERN
extern "C"
{
#include <lualib.h>
#include <lauxlib.h>
}
#else
#include <lualib.h>
#include <lauxlib.h>
#endif

Lua::LuaFunctionsMap Lua::luaFunctions_;
Lua::StateCallTimeMap Lua::stateCallTimes_;
boost::shared_mutex Lua::callTimeMutex_;

const int CALL_TIMEOUT = 45;

Lua::Lua(const UnicodeString& scriptsDirectory) :
	lua_(lua_open()), scriptsDirectory_(scriptsDirectory)
{
	if (lua_)
	{
		const luaL_Reg libs[] =
		{
		{ "", luaopen_base },
		{ LUA_TABLIBNAME, luaopen_table },
		{ LUA_STRLIBNAME, luaopen_string },
		{ LUA_MATHLIBNAME, luaopen_math },
		{ 0, 0 } };

		const luaL_Reg* lib = libs;

		for (; lib->func; ++lib)
		{
			lua_pushcfunction(lua_, lib->func);
			lua_pushstring(lua_, lib->name);
			lua_call(lua_, 1, 0);
		}

		lua_sethook(lua_, &Lua::LuaHook, LUA_MASKCOUNT, 1);
	}
}

Lua::~Lua()
{
	for (StringContainer::iterator i = myRegisteredFunctions_.begin(); i
			!= myRegisteredFunctions_.end(); ++i)
	{
		LuaFunctionsMap::iterator f = luaFunctions_.find(*i);
		if (f != luaFunctions_.end())
		{
			if ( LuaFunctionHandle func = f->second.first.lock() )
			{
				if (f->second.second == lua_)
				{
					luaFunctions_.erase(*i);
				}
			}
		}
	}

	if (lua_)
	{
		lua_close(lua_);
	}
}

void Lua::LoadScripts()
{
	std::vector<LuaFunction> functions;

	for (boost::filesystem::directory_iterator i(AsUtf8(scriptsDirectory_)); i
			!= boost::filesystem::directory_iterator(); ++i)
	{
		std::string path = i->string();
		if (!boost::filesystem::is_directory(*i) && path.rfind(".lua")
				== path.size() - strlen(".lua"))
		{
			Log << LogLevel::Info << "Loading '" << path << "'...";
			try
			{
				LuaFunction function = LoadFile(path);
				functions.push_back(function);
				Log << LogLevel::Info << "Successfully loaded '" << path << "'";
			} catch (Exception& e)
			{
				Log << LogLevel::Error << "Loading '" << path << "' failed: '"
						<< e.GetMessage() << "'";
			}
		}
	}

	for (std::vector<LuaFunction>::iterator i = functions.begin(); i
			!= functions.end(); ++i)
	{

		i->Push();
		if (lua_pcall(lua_, 0, 0, 0) != 0)
		{
			std::string message = lua_tostring(lua_, -1);
			lua_pop(lua_, -1);
			Log << LogLevel::Error << "Error: " << message;
		}
	}
}

Lua::LuaFunctionHandle Lua::RegisterFunction(const UnicodeString& name,
		LuaFunc f, bool replace)
{
	if (luaFunctions_.find(name) != luaFunctions_.end() && !replace)
	{
		throw Exception(__FILE__, __LINE__, "Function already registered");
	}

	LuaFunctionHandle result(new LuaFunc(f));

	luaFunctions_[name] = FunctionStatePair(LuaFunctionWeakPtr(result), lua_);
	myRegisteredFunctions_.push_back(name);

	lua_pushstring(lua_, AsUtf8(name).c_str());
	lua_pushcclosure(lua_, &Lua::CallDispatch, 1);
	lua_setglobal(lua_, AsUtf8(name).c_str());

	return result;
}

int Lua::FunctionCall(lua_State* lua, int argCount, int resultCount)
{
	assert(lua == lua_);
	{
		boost::upgrade_lock<boost::shared_mutex> lock(callTimeMutex_);
		stateCallTimes_[lua] = time(0);
	}
	return lua_pcall(lua, argCount, resultCount, 0);
}

int Lua::CallDispatch(lua_State* lua)
{
	UnicodeString name = ConvertString(lua_tostring(lua, lua_upvalueindex(1)));

	LuaFunctionsMap::iterator f = luaFunctions_.find(name);

	if (f != luaFunctions_.end())
	{
		if ( LuaFunctionHandle func = f->second.first.lock() )
		{
			try
			{
				return (*func)(lua);
			} catch (Exception& e)
			{
				return luaL_error(lua, AsUtf8(e.GetMessage()).c_str());
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
	if (result != 0)
	{
		const char* msg = lua_tostring(lua_, -1);
		UnicodeString errorMessage;
		if (msg)
		{
			errorMessage = ConvertString(msg);
		}
		else if (result == LUA_ERRSYNTAX)
		{
			errorMessage = "Syntax error during pre-compilation...";
		}
		else if (result == LUA_ERRMEM)
		{
			errorMessage = "Memory allocation error...";
		}
		else if (result == LUA_ERRFILE)
		{
			errorMessage = "Cannot open or read file...";
		}
		throw Exception(__FILE__, __LINE__, errorMessage);
	}

	// Create a lua function that is the file we just loaded
	// Then remove the function from the stack
	LuaFunction luaFunction(lua_);
	lua_pop(lua_, 1);

	return luaFunction;
}

void Lua::LuaHook(lua_State* lua, lua_Debug* debug)
{
	boost::shared_lock<boost::shared_mutex> lock(callTimeMutex_);

	StateCallTimeMap::iterator stateCallTime = stateCallTimes_.find(lua);
	if (stateCallTime != stateCallTimes_.end())
	{
		if (time(0) - stateCallTime->second > CALL_TIMEOUT)
		{
			luaL_error(lua, "Excessive execution time, aborting.");
		}
	}
}
