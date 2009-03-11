#pragma once

#include <vector>
#include <string>

#include "glue.fwd.hpp"
#include "../lua/lua.hpp"
#include "../lua/luafunction.fwd.hpp"

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#ifdef LUA_EXTERN
extern "C"
{
#endif
#include <lua.h>
#ifdef LUA_EXTERN
}
#endif

class Client;

class GlueManager
{
public:
    static GlueManager& Instance();

    void RegisterGlue(Glue* glue);

    void Reset(boost::shared_ptr<Lua> lua, Client* client);

    typedef boost::function<int (lua_State*)> GlueFunction;
    void AddFunction(GlueFunction, const std::string& name);

private:
    GlueManager();
    GlueManager(const GlueManager&);
    GlueManager& operator=(const GlueManager&);

    typedef std::vector<Glue*> GlueContainer;
    GlueContainer glues_;

    boost::shared_ptr<Lua> lua_;

    typedef std::list<Lua::LuaFunctionHandle> FunctionHandleContainer;
    FunctionHandleContainer functionHandles_;
};
