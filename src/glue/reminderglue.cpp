#include "glue.hpp"
#include "gluemanager.hpp"
#include "../client.hpp"
#include "../exception.hpp"
#include "../remindermanager.hpp"

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <converter.hpp>

class ReminderGlue: public Glue
{
public:
	ReminderGlue();

	void Reset(boost::shared_ptr<Lua> lua, Client* client);

	int AddReminder(lua_State* lua);
	int FindReminder(lua_State* lua);
private:
	void AddFunctions();

	boost::shared_ptr<ReminderManager> reminderManager_;
};

ReminderGlue reminderGlue;

ReminderGlue::ReminderGlue()
{
	GlueManager::Instance().RegisterGlue(this);
}

void ReminderGlue::AddFunctions()
{
	AddFunction(boost::bind(&ReminderGlue::AddReminder, this, _1),
			"ReminderAdd");
	AddFunction(boost::bind(&ReminderGlue::FindReminder, this, _1),
			"ReminderFind");
}

void ReminderGlue::Reset(boost::shared_ptr<Lua> lua, Client* client)
{
	Glue::Reset(lua, client);
	reminderManager_.reset(new ReminderManager(
			client_->GetConfig().GetRemindersFilename(), boost::bind(
					&Client::SendMessage, client_, _1, _2, _3)));
}

int ReminderGlue::AddReminder(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TNUMBER);
	CheckArgument(lua, 2, LUA_TSTRING);
	CheckArgument(lua, 3, LUA_TSTRING);
	CheckArgument(lua, 4, LUA_TSTRING);

	time_t seconds = lua_tointeger(lua, 1);
	UnicodeString server = AsUnicode(lua_tostring(lua, 2));
	std::string channel = lua_tostring(lua, 3);
	UnicodeString message = AsUnicode(lua_tostring(lua, 4));

	reminderManager_->CreateReminder(seconds, server, channel, message);

	return 0;
}

int ReminderGlue::FindReminder(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);
	CheckArgument(lua, 2, LUA_TSTRING);
	CheckArgument(lua, 3, LUA_TSTRING);

	UnicodeString server = AsUnicode(lua_tostring(lua, 1));
	UnicodeString channel = AsUnicode(lua_tostring(lua, 2));
	UnicodeString searchString = AsUnicode(lua_tostring(lua, 3));

	ReminderManager::ReminderIteratorRange reminders =
			reminderManager_->FindReminders(server, channel, searchString);

	int reminderCount = std::distance(reminders.first, reminders.second);
	lua_createtable(lua, reminderCount, 0);
	int mainTableIndex = lua_gettop(lua);

	for (ReminderManager::ReminderIterator reminder = reminders.first; reminder
			!= reminders.second; ++reminder)
	{
		lua_createtable(lua, 2, 0);
		int subTableIndex = lua_gettop(lua);
		lua_pushinteger(lua, reminder->Timestamp - time(0));
		lua_rawseti(lua, subTableIndex, 1);
		lua_pushstring(lua, AsUtf8(reminder->Message).c_str());
		lua_rawseti(lua, subTableIndex, 2);
		int currentMainIndex = std::distance(reminders.first, reminder) + 1;
		lua_rawseti(lua, mainTableIndex, currentMainIndex);
	}
	return 1;
}
