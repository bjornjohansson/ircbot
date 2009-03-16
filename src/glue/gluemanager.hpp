#pragma once

#include <vector>
#include <string>

#include "glue.fwd.hpp"
#include "../lua/lua.fwd.hpp"
#include "../lua/luafunction.fwd.hpp"
#include "../client.fwd.hpp"

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

class GlueManager
{
public:
    static GlueManager& Instance();

    void RegisterGlue(Glue* glue);

    void Reset(boost::shared_ptr<Lua> lua, Client* client);

private:
    GlueManager();
    GlueManager(const GlueManager&);
    GlueManager& operator=(const GlueManager&);

    typedef std::vector<Glue*> GlueContainer;
    GlueContainer glues_;

    boost::shared_ptr<Lua> lua_;
};
