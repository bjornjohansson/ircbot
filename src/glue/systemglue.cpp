#include "glue.hpp"
#include "gluemanager.hpp"
#include "../client.hpp"
#include "../exception.hpp"
#include "../forkcommand.hpp"

#include <boost/bind.hpp>
#include <converter.hpp>

const int MAX_COMMAND_RETURN_LINES = 10;

class SystemGlue: public Glue
{
public:
	SystemGlue();

	int RunCommand(lua_State* lua);
	int ConvertString(lua_State* lua);
private:
	void AddFunctions();
};

SystemGlue systemGlue;

SystemGlue::SystemGlue()
{
	GlueManager::Instance().RegisterGlue(this);
}

void SystemGlue::AddFunctions()
{
	AddFunction(boost::bind(&SystemGlue::RunCommand, this, _1), "Execute");
	AddFunction(boost::bind(&SystemGlue::ConvertString, this, _1),
			"ConvertString");
}

int SystemGlue::RunCommand(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);

	const char* command = lua_tostring(lua, 1);

	std::string result = ForkCommand(command);

	std::string::size_type pos = result.find('\n');
	int rows = 1;
	while (pos != std::string::npos && rows < MAX_COMMAND_RETURN_LINES)
	{
		pos = result.find('\n', pos + 1);
		++rows;
	}
	if (pos != std::string::npos)
	{
		// Replace any remaining lines with a single error message
		result.replace(pos + 1, std::string::npos, "Too many lines.");
	}

	lua_pushstring(lua, result.c_str());
	return 1;
}

int SystemGlue::ConvertString(lua_State* lua)
{
	CheckArgument(lua, 1, LUA_TSTRING);

	std::string input = lua_tostring(lua, 1);

    // This used to do character encoding detection and then converting to a
    // unicode string and back to UTF-8. In later versions only UTF-8 is
    // supported so this just returns the same string again. The function is
    // still here to keep backwards compatibility with older scripts.
	lua_pushstring(lua, input.c_str());
	return 1;
}
