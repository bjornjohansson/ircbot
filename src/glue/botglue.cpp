#include "glue.hpp"
#include "gluemanager.hpp"
#include "../client.hpp"
#include "../exception.hpp"

#include <boost/bind.hpp>

class BotGlue : public Glue
{
public:
    BotGlue();

    int GetMyNick(lua_State* lua);
private:
    void AddFunctions();
};

BotGlue botGlue;

BotGlue::BotGlue()
{
    GlueManager::Instance().RegisterGlue(this);
}

void BotGlue::AddFunctions()
{
    GlueManager::Instance().AddFunction(boost::bind(&BotGlue::GetMyNick,
						    this, _1), "GetMyNick");
}

int BotGlue::GetMyNick(lua_State* lua)
{
    std::string server;
    if ( lua_gettop(lua) >= 1 )
    {
	CheckArgument(lua, 1, LUA_TSTRING);
	server = lua_tostring(lua, 1);
    }

    std::string nick = client_->GetNick(server);

    lua_pushstring(lua, nick.c_str());
    return 1;
}
