#pragma once

#ifdef LUA_EXTERN
extern "C" {
#include <lua.h>
}
#else
#include <lua.h>
#endif

#include <boost/shared_ptr.hpp>

/**
 * Keep a reference to the function at the top of the stack
 */
class LuaFunction
{
public:
    /**
     * Does not pop the stack, the function is left on top of the stack
     * @throw Exception if value on top of lua stack is not a function
     */
    LuaFunction(lua_State* lua);

    /**
     * @throw Exception if the lua state or function reference is invalid
     */
    void Push();

private:
    class Reference
    {
    public:
	Reference(lua_State* lua);
	virtual ~Reference();

	int GetReference() const { return reference_; }

    private:
	lua_State* lua_;
	int reference_;
    };

    lua_State* lua_;
    typedef boost::shared_ptr<Reference> ReferencePtr;
    ReferencePtr reference_;
};
