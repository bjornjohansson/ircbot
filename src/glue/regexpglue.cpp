#include "glue.hpp"
#include "../lua/luafunction.hpp"
#include "../regexp/regexp.hpp"

#include <converter.hpp>

#ifdef LUA_EXTERN
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}
#else
#include <lua.h>
#include <lauxlib.h>
#endif

#include <boost/bind.hpp>

#include "gluemanager.hpp"
#include "../client.hpp"
#include "../lua/lua.hpp"
#include "../regexp/regexpmanager.hpp"
#include "../exception.hpp"

class RegexpGlue: public Glue
{
public:
	RegexpGlue();

	void Reset(boost::shared_ptr<Lua> lua, Client* client);

	int AddRegExp(lua_State* lua);
	int DeleteRegExp(lua_State* lua);
	int RegExpMatchAndReply(lua_State* lua);
	int RegExpFindMatch(lua_State* lua);
	int RegExpFindRegExp(lua_State* lua);
	int RegExpChangeReply(lua_State* lua);
	int RegExpChangeRegExp(lua_State* lua);
	int RegExpMoveUp(lua_State* lua);
	int RegExpMoveDown(lua_State* lua);

private:
	void AddFunctions();

	UnicodeString RegExpOperation(const UnicodeString& reply,
			const UnicodeString& message, const RegExp& regexp);

	int FillRegExpTable(lua_State* lua,
			RegExpManager::RegExpIteratorRange regExps);

	boost::shared_ptr<RegExpManager> regExpManager_;

	typedef std::pair<LuaFunction, lua_State*> FunctionStatePair;
	typedef boost::shared_ptr<FunctionStatePair> FunctionStatePairPtr;
	FunctionStatePairPtr regExpOperation_;
};

RegexpGlue regexpGlue;

RegexpGlue::RegexpGlue()
{
	GlueManager::Instance().RegisterGlue(this);

}

void RegexpGlue::AddFunctions()
{
	AddFunction(boost::bind(&RegexpGlue::AddRegExp, this, _1), "RegExpAdd");
	AddFunction(boost::bind(&RegexpGlue::DeleteRegExp, this, _1),
			"RegExpDelete");
	AddFunction(boost::bind(&RegexpGlue::RegExpMatchAndReply, this, _1),
			"RegExpMatchAndReply");
	AddFunction(boost::bind(&RegexpGlue::RegExpFindMatch, this, _1),
			"RegExpFindMatch");
	AddFunction(boost::bind(&RegexpGlue::RegExpFindRegExp, this, _1),
			"RegExpFindRegExp");
	AddFunction(boost::bind(&RegexpGlue::RegExpChangeReply, this, _1),
			"RegExpChangeReply");
	AddFunction(boost::bind(&RegexpGlue::RegExpChangeRegExp, this, _1),
			"RegExpChangeRegExp");
	AddFunction(boost::bind(&RegexpGlue::RegExpMoveUp, this, _1),
			"RegExpMoveUp");
	AddFunction(boost::bind(&RegexpGlue::RegExpMoveDown, this, _1),
			"RegExpMoveDown");
}

void RegexpGlue::Reset(boost::shared_ptr<Lua> lua, Client* client)
{
	Glue::Reset(lua, client);
	const Config& config = client_->GetConfig();
	regExpManager_.reset(new RegExpManager(config.GetRegExpsFilename(),
			config.GetLocale()));
}

int RegexpGlue::AddRegExp(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);
	CheckArgument(lua, 2, LUA_TSTRING);

	UnicodeString regexp = AsUnicode(lua_tostring(lua, 1));
	UnicodeString reply = AsUnicode(lua_tostring(lua, 2));

	RegExpManager::RegExpResult res = regExpManager_->AddRegExp(regexp, reply);
	if (res.Success)
	{
		lua_pushboolean(lua, 1);
		return 1;
	}
	lua_pushboolean(lua, 0);
	lua_pushstring(lua, AsUtf8(res.ErrorMessage).c_str());
	return 2;
}

int RegexpGlue::DeleteRegExp(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);

	UnicodeString regexp = AsUnicode(lua_tostring(lua, 1));

	bool result = regExpManager_->RemoveRegExp(regexp);
	lua_pushboolean(lua, result);
	return 1;
}

int RegexpGlue::RegExpMatchAndReply(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);
	CheckArgument(lua, 2, LUA_TFUNCTION);

	UnicodeString message = AsUnicode(lua_tostring(lua, 1));

	try
	{
		lua_pushvalue(lua, 2);
		regExpOperation_.reset(new FunctionStatePair(LuaFunction(lua), lua));
		lua_pop(lua, 1);
	} catch (Exception& e)
	{
		return luaL_error(lua, AsUtf8(e.GetMessage()).c_str());
	}

	UnicodeString reply = regExpManager_->FindMatchAndReply(message,
			boost::bind(&RegexpGlue::RegExpOperation, this, _1, _2, _3));
	lua_pushstring(lua, AsUtf8(reply).c_str());
	return 1;
}

int RegexpGlue::RegExpFindMatch(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);

	UnicodeString message = AsUnicode(lua_tostring(lua, 1));

	RegExpManager::RegExpIteratorRange matches = regExpManager_->FindMatches(
			message);

	return FillRegExpTable(lua, matches);
}

int RegexpGlue::RegExpFindRegExp(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);

	UnicodeString searchString = AsUnicode(lua_tostring(lua, 1));

	RegExpManager::RegExpIteratorRange matches = regExpManager_->FindRegExps(
			searchString);

	return FillRegExpTable(lua, matches);
}

int RegexpGlue::RegExpChangeReply(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);
	CheckArgument(lua, 2, LUA_TSTRING);

	UnicodeString regexp = AsUnicode(lua_tostring(lua, 1));
	UnicodeString reply = AsUnicode(lua_tostring(lua, 2));

	bool success = regExpManager_->ChangeReply(regexp, reply);
	lua_pushboolean(lua, success);
	return 1;
}

int RegexpGlue::RegExpChangeRegExp(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);
	CheckArgument(lua, 2, LUA_TSTRING);

	UnicodeString regexp = AsUnicode(lua_tostring(lua, 1));
	UnicodeString newRegexp = AsUnicode(lua_tostring(lua, 2));

	RegExpManager::RegExpResult res = regExpManager_->ChangeRegExp(regexp,
			newRegexp);
	lua_pushboolean(lua, res.Success);
	if (res.Success)
	{
		return 1;
	}
	lua_pushstring(lua, AsUtf8(res.ErrorMessage).c_str());
	return 2;
}

int RegexpGlue::RegExpMoveUp(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);

	UnicodeString regexp = lua_tostring(lua, 1);

	bool result = regExpManager_->MoveUp(regexp);

	lua_pushboolean(lua, result);
	return 1;
}

int RegexpGlue::RegExpMoveDown(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);

	UnicodeString regexp = lua_tostring(lua, 1);

	bool result = regExpManager_->MoveDown(regexp);

	lua_pushboolean(lua, result);
	return 1;
}

UnicodeString RegexpGlue::RegExpOperation(const UnicodeString& reply,
		const UnicodeString& message, const RegExp& regexp)
{
	UnicodeString result = reply;
	if (regExpOperation_)
	{
		lua_State* lua = regExpOperation_->second;
		if (lua && lua_status(lua) == 0)
		{
			regExpOperation_->first.Push();
			if (lua_isfunction(lua, -1) != 0)
			{
				lua_pushstring(lua, AsUtf8(reply).c_str());
				lua_pushstring(lua, AsUtf8(message).c_str());
				lua_pushstring(lua, AsUtf8(regexp.GetRegExp()).c_str());
				lua_pushstring(lua, AsUtf8(regexp.GetReply()).c_str());
				if (lua_->FunctionCall(lua, 4, 1) == 0)
				{
					if (lua_isstring(lua, -1))
					{
						const char* r = lua_tostring(lua, -1);
						if (r)
						{
							result = AsUnicode(r);
						}
					}
					lua_pop(lua, 1);
				}
				else
				{
					const char* message = lua_tostring(lua, -1);
					if (message != 0)
					{
						result = AsUnicode(message);
					}
					lua_pop(lua, 1);
				}
			}
		}
	}
	return result;
}

int RegexpGlue::FillRegExpTable(lua_State* lua,
		RegExpManager::RegExpIteratorRange regExps)
{
	int count = std::distance(regExps.first, regExps.second);

	lua_createtable(lua, count, 0);
	int mainTableIndex = lua_gettop(lua);

	for (RegExpManager::RegExpIterator regExp = regExps.first; regExp
			!= regExps.second; ++regExp)
	{
		lua_createtable(lua, 3, 0);
		int subTableIndex = lua_gettop(lua);
		lua_pushstring(lua, AsUtf8(regExp->GetRegExp()).c_str());
		lua_rawseti(lua, subTableIndex, 1);
		lua_pushstring(lua, AsUtf8(regExp->GetReply()).c_str());
		lua_rawseti(lua, subTableIndex, 2);
		lua_pushstring(lua, AsUtf8(regExpManager_->Encode(regExp->GetRegExp())).c_str());
		lua_rawseti(lua, subTableIndex, 3);
		int currentMainIndex = std::distance(regExps.first, regExp) + 1;
		lua_rawseti(lua, mainTableIndex, currentMainIndex);
	}
	return 1;
}
