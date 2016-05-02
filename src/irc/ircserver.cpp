#include "ircserver.hpp"
#include "ircmessage.hpp"
#include "../logging/logger.hpp"
#include "../exception.hpp"

#include <sstream>
#include <fstream>

#include <boost/bind.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <sys/utsname.h>

const std::string VersionName = "Uno";
const std::string VersionVersion = "$Revision$";
std::string VersionEnvironment = "Unknown";

Irc::IrcServer::IrcServer(const UnicodeString& id,
               const UnicodeString& host,
               unsigned int port,
               const std::string& logDirectory,
               const UnicodeString& nick,
               const UnicodeString& serverPassword)
    : Server(id, host, port, logDirectory, nick, serverPassword)
    , appendingChannelNicks_(false)
{
    struct utsname name;
    if (uname(&name) == 0)
    {
        std::stringstream ss;
        ss << name.sysname << " " << name.release;
        VersionEnvironment = ss.str();
    }

    RegisterSelfAsReceiver();
}

Irc::IrcServer::~IrcServer()
{
    // Do not destroy this class while it's performing callbacks
    boost::lock_guard<boost::mutex> lock(callbackMutex_);
}

void Irc::IrcServer::IrcServer::Connect()
{
    connection_.Connect(GetHostName(), GetPort());
}

void Irc::IrcServer::Send(const std::string& data)
{
    std::vector<char> buffer;
    std::copy(data.begin(), data.end(), std::back_inserter(buffer));
    buffer.push_back('\r');
    buffer.push_back('\n');
    try
    {
        connection_.Send(buffer);
    } catch (Exception& e)
    {
        Log << LogLevel::Error << e.GetMessage();
    }
    Log << LogLevel::Debug << GetHost() << "> " << data;
}

void Irc::IrcServer::Receive(Connection& /*connection*/,
                             const std::vector<char>& data)
{
    boost::lock_guard<boost::mutex> lock(callbackMutex_);
    // Copy everything but '\r' as some irc servers don't comply
    std::remove_copy(data.begin(), data.end(), std::back_inserter(
            receiveBuffer_), '\r');

    for (std::string::size_type pos = receiveBuffer_.find('\n');
         pos != std::string::npos;
         pos = receiveBuffer_.find('\n'))
    {
        OnText(receiveBuffer_.substr(0, pos));
        receiveBuffer_.erase(0, pos + 1);
    }
}

void Irc::IrcServer::OnConnect(Connection& /*connection*/)
{
    const UnicodeString& password = GetServerPassword();
    if (password.length() > 0)
    {
        SendPassString(password);
    }
    ChangeNick(GetNick());
    SendUserString(GetNick(), GetNick());

    JoinAllChannels();
}

void Irc::IrcServer::JoinChannel(const std::string& channel, const UnicodeString& key)
{
    AddChannel(channel, key);
    if (connection_.IsConnected())
    {
        if (key.length() > 0)
        {
            Send("JOIN " + channel + " " + AsUtf8(key));
        }
        else
        {
            Send("JOIN " + channel);
        }
    }
}

void Irc::IrcServer::ChangeNick(const UnicodeString& nick)
{
    Send("NICK " + AsUtf8(nick));
    SetNick(nick);
}

void Irc::IrcServer::SendPassString(const UnicodeString& password)
{
    if (password.indexOf(' ') != -1)
    {
        Send("PASS :" + AsUtf8(password));
    }
    else
    {
        Send("PASS " + AsUtf8(password));
    }
}

void Irc::IrcServer::SendUserString(const UnicodeString& user, const UnicodeString& name)
{
    Send("USER " + AsUtf8(user) + " localhost "
            + AsUtf8(GetHostName()) + " :" + AsUtf8(name));
}

void Irc::IrcServer::SendMessage(const std::string& target, const UnicodeString& message)
{
    std::string scrubbedMessage = AsUtf8(message);
    std::string::size_type pos = std::string::npos;
    if ((pos = scrubbedMessage.find_first_of("\n\r")) != std::string::npos)
    {
        scrubbedMessage = scrubbedMessage.substr(0, pos);
    }

    if (scrubbedMessage.size() > 0)
    {
        Send("PRIVMSG " + target + " :" + scrubbedMessage);

        scrubbedMessage = CleanMessageForDisplay(AsUtf8(GetNick()), scrubbedMessage, scrubbedMessage.find('\1') == 0);

        std::stringstream ss;
        ss.imbue(std::locale::classic());
        ss << time(0) << " " << target << ": <" << AsUtf8(GetNick()) << "> "
                << scrubbedMessage;
        LogMessage(target, ss.str());
    }
}

void Irc::IrcServer::Kick(const std::string& channel, const std::string& user,
        const UnicodeString& message)
{
    Send(std::string("KICK ") + channel + std::string(" ") + user
            + std::string(" :") + (message.isEmpty() ? "" : AsUtf8(message)));
}

template<class T>
bool IsIrcNameStatus(const T& v)
{
    return v == '@' || v == '+';
}

void Irc::IrcServer::OnText(const std::string& text)
{
    Log << LogLevel::Debug << GetHostName() << "< " << text;
    IrcMessage message(text);

    if (message.GetCommand() == Irc::Command::PING)
    {
        Send("PONG " + AsUtf8(GetHostName()));
    }
    else if (message.GetCommand() == Irc::Command::RPL_WELCOME)
    {
        JoinAllChannels();
    }
    else if (message.IsCtcp() &&
             message.size() >= 1 &&
             message[1] == "\1VERSION")
    {
        SendMessage(message.GetPrefix().GetNick(),
                    AsUnicode("\1VERSION " + VersionName + " "
                + VersionVersion + " running on " + VersionEnvironment));
    }
    else if (message.GetCommand() == Irc::Command::PRIVMSG && message.size() >= 2)
    {
        // Log messages
        const std::string& from = message.GetPrefix().GetNick();
        const std::string& to = message[0];
        const std::string& replyTo = to.find_first_of("#&") == 0 ? to : from;
        std::stringstream ss;
        ss.imbue(std::locale::classic());
        ss << time(0) << " " << replyTo << ": <" << from << "> "
           << CleanMessageForDisplay(from, message[1], message.IsCtcp());
        UnicodeString logMessage = AsUnicode(ss.str());
        LogMessage(replyTo, AsUtf8(logMessage));

        // Notify receivers
        NotifyReceiver(message);
    }
    else if (message.GetCommand() == Irc::Command::RPL_NAMREPLY
            && message.size() >= 4)
    {
        std::string channel = message[2];

        if (channel.find_first_of("#&") == 0)
        {
            if (!appendingChannelNicks_)
            {
                ClearAllChannelNicks(channel);
                appendingChannelNicks_ = true;
            }

            std::stringstream stream(message[3]);

            while (stream.good())
            {
                std::string nick;
                stream >> nick;
                if (nick.find_first_of("@+") == 0)
                {
                    nick.erase(0, 1);
                }

                if (nick.size() > 0)
                {
                    AddChannelNick(channel, nick);
                }
            }
        }
    }
    else if (message.GetCommand() == Irc::Command::RPL_ENDOFNAMES)
    {
        appendingChannelNicks_ = false;
    }
    else if (message.GetCommand() == Irc::Command::PART && message.size() >= 1)
    {
        const std::string& nick = message.GetPrefix().GetNick();
        const std::string& channel = message[0];
        RemoveChannelNick(channel, nick);
    }
    else if (message.GetCommand() == Irc::Command::QUIT)
    {
        const std::string& nick = message.GetPrefix().GetNick();
        RemoveChannelNick(nick);
    }
    else if (message.GetCommand() == Irc::Command::JOIN && message.size() >= 1)
    {
        const std::string& nick = message.GetPrefix().GetNick();
        const std::string& channel = message[0];
        AddChannelNick(channel, nick);
    }
    else if (message.GetCommand() == Irc::Command::NICK && message.size() >= 1)
    {
        const std::string& oldNick = message.GetPrefix().GetNick();
        const std::string& newNick = message[0];
        ChangeChannelNick(oldNick, newNick);
    }
    else if (message.GetCommand() == Irc::Command::KICK && message.size() >= 2)
    {
        const std::string& channel = message[0];
        const std::string& victim = message[1];

        RemoveChannelNick(channel, victim);

        if (victim == AsUtf8(GetNick()))
        {
            // We've been kicked
            // Remove channel, we'll get a new list of nicks if we rejoin
            RemoveChannelNickChannel(channel);
            // Rejoin
            const UnicodeString* key = GetChannelKey(channel);
            JoinChannel(channel, key ? *key : "");
        }
    }
}

void Irc::IrcServer::RegisterSelfAsReceiver()
{
    connectionReceiver_ = connection_.RegisterReceiver(boost::bind(
            &Irc::IrcServer::Receive, this, _1, _2));
    onConnectCallback_ = connection_.RegisterOnConnectCallback(boost::bind(
            &Irc::IrcServer::OnConnect, this, _1));
}

std::string Irc::IrcServer::CleanMessageForDisplay(const std::string& nick,
                                           const std::string& message,
                                           bool isCtcp)
{
    std::string result = message;

    const std::string ctcpAction = "ACTION";
    if (isCtcp && result.find(ctcpAction) == 0)
    {
        result.replace(0, ctcpAction.size(), "* " + nick);
        std::string::size_type pos = std::string::npos;
        if ((pos = result.rfind("\1")) != std::string::npos)
        {
            result.erase(pos);
        }
    }
    return result;
}
