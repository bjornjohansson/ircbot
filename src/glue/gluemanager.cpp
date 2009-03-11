#include "gluemanager.hpp"
#include "glue.hpp"

#include <boost/bind.hpp>
#include <algorithm>

#include "../client.hpp"
#include "../lua/lua.hpp"

GlueManager& GlueManager::Instance()
{
    static GlueManager instance_;
    return instance_;
}

void GlueManager::RegisterGlue(Glue* glue)
{
    glues_.push_back(glue);
}

void GlueManager::Reset(boost::shared_ptr<Lua> lua, Client* client)
{
    lua_ = lua;

    for(GlueContainer::iterator glue = glues_.begin();
	glue != glues_.end();
	++glue)
    {
	(*glue)->Reset(lua, client);
    }
}

void GlueManager::AddFunction(GlueManager::GlueFunction f,
			      const std::string& name)
{
    functionHandles_.push_back(lua_->RegisterFunction(name, f));
}


GlueManager::GlueManager()
{

}
