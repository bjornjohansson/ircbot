#include "glue.hpp"
#include "gluemanager.hpp"
#include "../client.hpp"
#include "../exception.hpp"

#include <boost/bind.hpp>
#include <unicode/unistr.h>
#include <converter.hpp>

class LogGlue: public Glue
{
public:
	LogGlue();

	int GetLogName(lua_State* lua);
	int GetLastLine(lua_State* lua);
private:
	void AddFunctions();
};

LogGlue logGlue;

LogGlue::LogGlue()
{
	GlueManager::Instance().RegisterGlue(this);
}

void LogGlue::AddFunctions()
{
	AddFunction(boost::bind(&LogGlue::GetLogName, this, _1), "GetLogName");
	AddFunction(boost::bind(&LogGlue::GetLastLine, this, _1), "GetLastLine");
}

int LogGlue::GetLogName(lua_State* lua)
{
	UnicodeString server;
	std::string target;
	int argumentCount = lua_gettop(lua);
	if (argumentCount >= 1)
	{
		CheckArgument(lua, 1, LUA_TSTRING);
		target = lua_tostring(lua, 1);
		if (argumentCount >= 2)
		{
			CheckArgument(lua, 2, LUA_TSTRING);
			server = ConvertString(lua_tostring(lua, 2));
		}
	}

	std::string logName = client_->GetLogName(target, server);

	lua_pushstring(lua, logName.c_str());
	return 1;
}

int LogGlue::GetLastLine(lua_State* lua)
{
	UnicodeString server;
	std::string target, nick;
	int argumentCount = lua_gettop(lua);
	CheckArgument(lua, 1, LUA_TSTRING);
	nick = lua_tostring(lua, 1);
	if (argumentCount >= 2)
	{
		CheckArgument(lua, 2, LUA_TSTRING);
		target = lua_tostring(lua, 2);
		if (argumentCount >= 3)
		{
			CheckArgument(lua, 3, LUA_TSTRING);
			server = ConvertString(lua_tostring(lua, 3));
		}
	}

	long timestamp = -1;
	UnicodeString logLine;

	try
	{
		logLine = client_->GetLastLine(nick, timestamp, target, server);
	} catch (Exception&)
	{
	}
	if (timestamp < 0)
	{
		return 0;
	}

	lua_pushstring(lua, AsUtf8(logLine).c_str());
	lua_pushinteger(lua, time(0) - timestamp);
	return 2;
}

