#include "glue.hpp"
#include "gluemanager.hpp"
#include "../client.hpp"
#include "../exception.hpp"
#include "../message.hpp"
#include "../lua/luafunction.hpp"
#include "../logging/logger.hpp"

#include <boost/bind.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <unicode/unistr.h>
#include <converter.hpp>
#include <boost/regex/icu.hpp>

#ifdef LUA_EXTERN
extern "C"
{
#include <lauxlib.h>
}
#else
#include <lauxlib.h>
#endif

const int MAX_RECURSIONS = 30;
const unsigned int MAX_SEND_LINES = 10;
const unsigned int MAX_LINE_LENGTH = 420;

class MessageGlue: public Glue
{
public:
    MessageGlue();

    void Reset(boost::shared_ptr<Lua> lua, Client* client);

    int Send(lua_State* lua);
    int RecurseMessage(lua_State* lua);
    int RegisterForEvent(lua_State* lua);
    int RegisterBlockingCall(lua_State* lua);

private:
    void AddFunctions();

    void OnEvent(const UnicodeString& server, const Message& message);

    typedef std::list<UnicodeString> StringContainer;
    typedef boost::shared_ptr<StringContainer> StringContainerPtr;
    StringContainerPtr ProcessMessageEvent(const UnicodeString& server,
            const std::string& fromNick, const UnicodeString& fromUser,
            const UnicodeString& fromHost, const std::string& to,
            const UnicodeString& message);

    typedef std::pair<LuaFunction, lua_State*> FunctionStatePair;
    StringContainerPtr CallEventHandler(FunctionStatePair functionStatePair,
            const UnicodeString& server, const std::string& fromNick,
            const UnicodeString& fromUser, const UnicodeString& fromHost,
            const std::string& to, const UnicodeString& message);

    typedef std::list<FunctionStatePair> FunctionContainer;
    typedef std::map<UnicodeString, FunctionContainer> EventFunctionMap;
    EventFunctionMap eventFunctions_;

    struct BlockingCall
    {
        BlockingCall(boost::u32regex regexp, FunctionStatePair pair, bool direct) :
            regexp_(regexp), functionStatePair_(pair), directOnly_(direct)
        {
        }
        boost::u32regex regexp_;
        FunctionStatePair functionStatePair_;
        bool directOnly_;
    };
    typedef std::list<BlockingCall> BlockingCallContainer;
    BlockingCallContainer blockingCalls_;

    Client::EventReceiverHandle eventHandle_;
    int recursions_;

    UnicodeString lastServer_;
    std::string lastFromNick_;
    UnicodeString lastFromUser_;
    UnicodeString lastFromHost_;
    std::string lastTo_;
};

MessageGlue messageGlue;

MessageGlue::MessageGlue()
{
    GlueManager::Instance().RegisterGlue(this);
}

void MessageGlue::AddFunctions()
{
    AddFunction(boost::bind(&MessageGlue::Send, this, _1), "Send");
    AddFunction(boost::bind(&MessageGlue::RecurseMessage, this, _1),
                "RecurseMessage");
    AddFunction(boost::bind(&MessageGlue::RegisterForEvent, this, _1),
                "RegisterForEvent");
    AddFunction(boost::bind(&MessageGlue::RegisterBlockingCall, this, _1),
                "RegisterBlockingCall");
}

void MessageGlue::Reset(boost::shared_ptr<Lua> lua, Client* client)
{
    eventFunctions_.clear();
    blockingCalls_.clear();
    Glue::Reset(lua, client);
    eventHandle_ = client_->RegisterForEvent(boost::bind(&MessageGlue::OnEvent,
                                                         this, _1, _2));
}

int MessageGlue::Send(lua_State* lua)
{
    try
    {
        UnicodeString server, message;
        std::string target;
        int argumentCount = lua_gettop(lua);
        CheckArgument(lua, 1, LUA_TSTRING);
        message = AsUnicode(lua_tostring(lua, 1));
        if (argumentCount >= 2)
        {
            CheckArgument(lua, 2, LUA_TSTRING);
            target = lua_tostring(lua, 2);
            if (argumentCount >= 3)
            {
                CheckArgument(lua, 3, LUA_TSTRING);
                server = AsUnicode(lua_tostring(lua, 3));
            }
        }
        client_->SendMessage(message, target, server);
    } catch (Exception& e)
    {
        luaL_error(lua, AsUtf8(e.GetMessage()).c_str());
    }

    return 0;
}

int MessageGlue::RecurseMessage(lua_State* lua)
{
    ++recursions_;
    if (recursions_ > MAX_RECURSIONS)
    {
        lua_pushstring(lua, "Too many recursions.");
        return 1;
    }

    UnicodeString server = lastServer_;
    std::string fromNick = lastFromNick_;
    UnicodeString fromUser = lastFromUser_;
    UnicodeString fromHost = lastFromHost_;
    std::string to = lastTo_;
    UnicodeString message;

    int argumentCount = lua_gettop(lua);
    CheckArgument(lua, 1, LUA_TSTRING);
    message = AsUnicode(lua_tostring(lua, 1));
    if (argumentCount >= 2)
    {
        CheckArgument(lua, 2, LUA_TSTRING);
        to = lua_tostring(lua, 2);
        if (argumentCount >= 3)
        {
            CheckArgument(lua, 3, LUA_TSTRING);
            fromNick = lua_tostring(lua, 3);
            if (argumentCount >= 4)
            {
                CheckArgument(lua, 4, LUA_TSTRING);
                fromUser = AsUnicode(lua_tostring(lua, 4));
                if (argumentCount >= 5)
                {
                    CheckArgument(lua, 5, LUA_TSTRING);
                    fromHost = AsUnicode(lua_tostring(lua, 5));
                    if (argumentCount >= 6)
                    {
                        CheckArgument(lua, 6, LUA_TSTRING);
                        server = AsUnicode(lua_tostring(lua, 6));
                    }
                }
            }
        }
    }

    lua_pop(lua, lua_gettop(lua));

    StringContainerPtr lines = ProcessMessageEvent(server, fromNick, fromUser,
            fromHost, to, message);

    std::stringstream concat;

    for(StringContainer::const_iterator line = lines->begin();
        line != lines->end();
        ++line)
    {
        concat<<AsUtf8(*line)<<std::endl;
    }

    lua_pushstring(lua, concat.str().c_str());
    return 1;
}

int MessageGlue::RegisterForEvent(lua_State* lua)
{
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TFUNCTION);

    std::string event = lua_tostring(lua, 1);

    if (std::string("ON_MESSAGE") == event)
    {
        try
        {
            lua_pushvalue(lua, 2);
            FunctionStatePair function(LuaFunction(lua), lua);
            lua_pop(lua, 1);
            eventFunctions_["ON_MESSAGE"].push_back(function);
        } catch (Exception& e)
        {
            return luaL_error(lua, AsUtf8(e.GetMessage()).c_str());
        }
    }
    else
    {
        return luaL_error(lua, "Invalid event");
    }
    return 0;
}

int MessageGlue::RegisterBlockingCall(lua_State* lua)
{
    bool directOnly = false;
    CheckArgument(lua, 1, LUA_TSTRING);
    CheckArgument(lua, 2, LUA_TFUNCTION);
    if (lua_gettop(lua) >= 3)
    {
        CheckArgument(lua, 3, LUA_TBOOLEAN);
        directOnly = lua_toboolean(lua, 3);
    }

    UnicodeString matchString = AsUnicode(lua_tostring(lua, 1));

    try
    {
        boost::u32regex regexp = boost::make_u32regex(matchString, boost::regex::extended);

        // Push function at position 2 on top of stack in case there are
        // other things between the function and the top of the stack
        lua_pushvalue(lua, 2);
        FunctionStatePair function(LuaFunction(lua), lua);
        lua_pop(lua, 1);

        blockingCalls_.push_back(BlockingCall(regexp, function, directOnly));
    } catch (boost::regex_error& e)
    {
        return luaL_error(lua, e.what());
    } catch (Exception& e)
    {
        return luaL_error(lua, AsUtf8(e.GetMessage()).c_str());
    }

    return 0;
}

void MessageGlue::OnEvent(const UnicodeString& server,
        const Message& message)
{
    // Extract nick, user and host from the from-user string (nick!user@host)
    const Prefix& from = message.GetPrefix();
    const std::string& to = message.GetTarget();
    const std::string& fromNick = from.GetNick();
    UnicodeString fromUser = AsUnicode(from.GetUser());
    UnicodeString fromHost = AsUnicode(from.GetHost());
    UnicodeString text = AsUnicode(message.GetText());
    const std::string& replyTo = message.GetReplyTo();

    lastServer_ = server;
    lastFromNick_ = fromNick;
    lastFromUser_ = fromUser;
    lastFromHost_ = fromHost;
    lastTo_ = to;

    recursions_ = 0;
    StringContainerPtr lines = ProcessMessageEvent(server, fromNick, fromUser,
            fromHost, to, text);
    recursions_ = 0;

    try
    {
        unsigned int lineCount = 0;
        for (StringContainer::iterator line = lines->begin(); line
                != lines->end(); ++line)
        {
            std::string msg = AsUtf8(*line);
            while (msg.size() > MAX_LINE_LENGTH && lineCount < MAX_SEND_LINES)
            {
                // Find last space before the line exceeds length
                std::string::size_type pos = msg.rfind(" ", MAX_LINE_LENGTH);
                if (pos == std::string::npos)
                {
                    // If no such space we just split at the limit
                    pos = MAX_LINE_LENGTH;
                    // But we do try to find a good split for UTF-8
                    while (pos > 0 && (msg[pos] & 0xC0) == 0x80)
                        --pos;
                    // But if we can't for some reason then we
                    if (pos == 0)
                        pos = MAX_LINE_LENGTH;
                }
                // Send the limited string, remove it from the line
                // and increase the line count
                client_->SendMessage(AsUnicode(msg.substr(0, pos)), replyTo, server);
                msg.erase(0, pos + 1);
                ++lineCount;
            }

            if (lineCount >= MAX_SEND_LINES)
            {
                client_->SendMessage("Too many lines.", replyTo, server);
                break;
            }
            client_->SendMessage(AsUnicode(msg), replyTo, server);
            ++lineCount;
        }
    } catch (Exception&)
    {
        // This is bad, can't really handle so log and rethrow
        Log << LogLevel::Error
                << "Received event with server tag that matches no server";
        throw;
    }
}

MessageGlue::StringContainerPtr MessageGlue::ProcessMessageEvent(
        const UnicodeString& server, const std::string& fromNick,
        const UnicodeString& fromUser, const UnicodeString& fromHost,
        const std::string& to, const UnicodeString& message)
{
    StringContainerPtr result(new StringContainer());

    UnicodeString directMessage = message;
    UnicodeString nick = client_->GetNick(server);
    // Message is direct if the to-field does not begin with channel identifier
    bool direct = to.find_first_of("#&") != 0;
    if (message.indexOf(nick + ":") == 0 || message.indexOf(nick + ",") == 0)
    {
        static boost::u32regex regex = boost::make_u32regex("^\\s*");
        message.extract(nick.length()+1, INT32_MAX, directMessage);
        directMessage = boost::u32regex_replace(directMessage, regex, "");
        direct = true;
    }

    for (BlockingCallContainer::iterator blockingCall = blockingCalls_.begin(); blockingCall
            != blockingCalls_.end(); ++blockingCall)
    {

        if ((direct || !blockingCall->directOnly_) && boost::u32regex_search(
                directMessage, blockingCall->regexp_))
        {
            result = CallEventHandler(blockingCall->functionStatePair_, server,
                    fromNick, fromUser, fromHost, to, directMessage);
            if (result && result->size() > 0)
            {
                return result;
            }
        }
    }

    for (FunctionContainer::iterator handler =
            eventFunctions_["ON_MESSAGE"].begin(); handler
            != eventFunctions_["ON_MESSAGE"].end(); ++handler)
    {
        StringContainerPtr localResult = CallEventHandler(*handler, server,
                fromNick, fromUser, fromHost, to, message);
        if (localResult && localResult->size() > 0)
        {
            result->splice(result->end(), *localResult);
        }
    }
    return result;
}

MessageGlue::StringContainerPtr MessageGlue::CallEventHandler(
        FunctionStatePair functionStatePair, const UnicodeString& server,
        const std::string& fromNick, const UnicodeString& fromUser,
        const UnicodeString& fromHost, const std::string& to,
        const UnicodeString& message)
{
    StringContainerPtr result(new StringContainer());

    lua_State* lua = functionStatePair.second;
    if (lua && lua_status(lua) == 0)
    {
        functionStatePair.first.Push();

        lua_pushstring(lua, AsUtf8(server).c_str());
        lua_pushstring(lua, fromNick.c_str());
        lua_pushstring(lua, AsUtf8(fromUser).c_str());
        lua_pushstring(lua, AsUtf8(fromHost).c_str());
        lua_pushstring(lua, to.c_str());
        lua_pushstring(lua, AsUtf8(message).c_str());
        if (lua_->FunctionCall(lua, 6, LUA_MULTRET) == 0)
        {
            int resultCount = lua_gettop(lua);

            for (int resultNumber = 1; resultNumber <= resultCount
                    && result->size() <= MAX_SEND_LINES; ++resultNumber)
            {
                const char* message = lua_tostring(lua, resultNumber);
                if (message)
                {
                    result->push_back(AsUnicode(message));
                }
                else if (lua_type(lua, resultNumber) == LUA_TTABLE)
                {
                    size_t tableSize = lua_objlen(lua, resultNumber);
                    for (unsigned int tableIndex = 1; tableIndex <= tableSize
                            && result->size() <= MAX_SEND_LINES; ++tableIndex)
                    {
                        lua_rawgeti(lua, resultNumber, tableIndex);
                        message = lua_tostring(lua, -1);
                        if (message)
                        {
                            result->push_back(AsUnicode(message));
                        }
                        lua_pop(lua, 1);
                    }
                }
            }
            lua_pop(lua, resultCount);
        }
        else
        {
            const char* message = lua_tostring(lua, -1);
            if (message)
            {
                result->push_back(AsUnicode(message));
            }
            lua_pop(lua, lua_gettop(lua));
        }
        assert(lua_gettop(lua) == 0);
    }
    return result;
}
