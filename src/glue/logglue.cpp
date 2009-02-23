#include "glue.hpp"
#include "gluemanager.hpp"
#include "../client.hpp"
#include "../exception.hpp"

#include <boost/bind.hpp>

class LogGlue : public Glue
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
    GlueManager::Instance().AddFunction(boost::bind(&LogGlue::GetLogName,
						    this, _1), "GetLogName");
    GlueManager::Instance().AddFunction(boost::bind(&LogGlue::GetLastLine,
						    this, _1), "GetLastLine");
}

int LogGlue::GetLogName(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);

    const char* server = lua_tostring(lua, 1);
    const char* target = lua_tostring(lua, 2);

    std::string logName = client_->GetLogName(server, target);

    lua_pushstring(lua, logName.c_str());
    return 1;
}

int LogGlue::GetLastLine(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TSTRING);
    CheckArgument(lua, 3, LUA_TSTRING);

    const char* server = lua_tostring(lua, 1);
    const char* target = lua_tostring(lua, 2);
    const char* nick = lua_tostring(lua, 3);

    long timestamp = -1;
    std::string logLine;
    try
    {
	logLine = client_->GetLastLine(server, target, nick, timestamp);
    }
    catch ( Exception& )
    {
    }
    if ( timestamp < 0 )
    {
	return 0;
    }

    lua_pushstring(lua, logLine.c_str());
    lua_pushinteger(lua, time(0)-timestamp);
    return 2;
}

