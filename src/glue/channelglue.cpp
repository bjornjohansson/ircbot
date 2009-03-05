#include "glue.hpp"
#include "gluemanager.hpp"
#include "../exception.hpp"
#include "../client.hpp"

#include <boost/bind.hpp>

#ifdef LUA_EXTERN
extern "C" {
#include <lauxlib.h>
}
#else
#include <lauxlib.h>
#endif
class ChannelGlue : public Glue
{
public:
    ChannelGlue();

    int JoinChannel(lua_State* lua);
    int Kick(lua_State* lua);
    int GetChannelNicks(lua_State* lua);
private:   
    void AddFunctions();
};

ChannelGlue channelGlue;

ChannelGlue::ChannelGlue()
{
    GlueManager::Instance().RegisterGlue(this);
}

void ChannelGlue::AddFunctions()
{
    GlueManager::Instance().AddFunction(boost::bind(&ChannelGlue::JoinChannel,
						    this, _1), "Join");
    GlueManager::Instance().AddFunction(boost::bind(&ChannelGlue::Kick,
						    this, _1), "Kick");
    GlueManager::Instance().AddFunction(boost::bind(
					    &ChannelGlue::GetChannelNicks,
					    this, _1), "GetChannelNicks");
}

int ChannelGlue::JoinChannel(lua_State* lua)
{
    try
    {
	std::string server, channel, key;
	int argumentCount = lua_gettop(lua);
	CheckArgument(lua, 1, LUA_TSTRING);
	channel = lua_tostring(lua, 1);
	if ( argumentCount >= 2 )
	{
	    CheckArgument(lua, 2, LUA_TSTRING);
	    key = lua_tostring(lua, 2);
	    if ( argumentCount >= 3 )
	    {
		CheckArgument(lua, 3, LUA_TSTRING);
		server = lua_tostring(lua, 3);
	    }
	}
	
	client_->JoinChannel(channel, key, server);
    }
    catch ( Exception& e)
    {
	return luaL_error(lua, e.GetMessage().c_str());
    }

    return 0;
}

int ChannelGlue::Kick(lua_State* lua)
{
    try
    {
	std::string user, message, channel, server;
	int argumentCount = lua_gettop(lua);
	CheckArgument(lua, 1, LUA_TSTRING);
	user = lua_tostring(lua, 1);
	if ( argumentCount >= 2 )
	{
	    CheckArgument(lua, 2, LUA_TSTRING);
	    message = lua_tostring(lua, 2);
	    if ( argumentCount >= 3 )
	    {
		CheckArgument(lua, 3, LUA_TSTRING);
		channel = lua_tostring(lua, 3);
		if ( argumentCount >= 4 )
		{
		    CheckArgument(lua, 4, LUA_TSTRING);
		    server = lua_tostring(lua, 4);
		}
	    }
	}
	client_->Kick(user, message, channel, server);
    }
    catch ( Exception& e )
    {
	return luaL_error(lua, e.GetMessage().c_str());
    }
    return 0;
}


int ChannelGlue::GetChannelNicks(lua_State* lua)
{
    std::string server, channel;
    int argumentCount = lua_gettop(lua);
    if ( argumentCount >= 1 )
    {
	CheckArgument(lua, 1, LUA_TSTRING);
	channel = lua_tostring(lua, 1);
	if ( argumentCount >= 2 )
	{
	    CheckArgument(lua, 2, LUA_TSTRING);
	    server = lua_tostring(lua, 2);
	}
    }

    const Server::NickContainer& nicks = client_->GetChannelNicks(channel,
								  server);

    int count = std::distance(nicks.begin(), nicks.end());

    lua_createtable(lua, count, 0);
    int mainTableIndex = lua_gettop(lua);

    for(Server::NickContainer::const_iterator nick = nicks.begin();
	nick != nicks.end();
	++nick)
    {
	lua_pushstring(lua, nick->c_str());
	int currentMainIndex = std::distance(nicks.begin(), nick)+1;
	lua_rawseti(lua, mainTableIndex, currentMainIndex);
    }
    return 1;
}
