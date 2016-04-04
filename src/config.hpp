#pragma once

#include <string>
#include <vector>

#include <libxml/tree.h>
#include <unicode/unistr.h>

class Config
{
public:
    Config(const UnicodeString& path);

    class Server
    {
    public:
        enum Type
        {
            IrcServer,
            MatrixServer
        };

        Server();
        Server(Type serverType,
               const UnicodeString& id,
               const UnicodeString& host,
               unsigned int port,
               const UnicodeString& nick,
               const UnicodeString& password);

        void AddChannel(const std::string& name, const UnicodeString& key);

        Type GetServerType() const
        {
            return type_;
        }
        const UnicodeString& GetId() const
        {
            return id_;
        }
        const UnicodeString& GetHost() const
        {
            return host_;
        }
        unsigned int GetPort() const
        {
            return port_;
        }
        const UnicodeString& GetNick() const
        {
            return nick_;
        }
        const UnicodeString& GetPassword() const
        {
            return password_;
        }

        typedef std::pair<std::string, UnicodeString> ChannelAndKey;
        typedef std::vector<ChannelAndKey> ChannelContainer;
        typedef ChannelContainer::const_iterator ChannelIterator;

        ChannelIterator GetChannelsBegin() const
        {
            return channels_.begin();
        }
        ChannelIterator GetChannelsEnd() const
        {
            return channels_.end();
        }

    private:
        Type type_;
        UnicodeString id_;
        UnicodeString host_;
        unsigned int port_;
        UnicodeString nick_;
        UnicodeString password_;
        ChannelContainer channels_;
    };

    typedef std::vector<Server> ServerContainer;
    typedef ServerContainer::const_iterator ServerIterator;

    ServerIterator GetServersBegin() const
    {
        return servers_.begin();
    }
    ServerIterator GetServersEnd() const
    {
        return servers_.end();
    }

    const UnicodeString& GetScriptsDirectory() const
    {
        return scriptsDirectory_;
    }
    const UnicodeString& GetLogsDirectory() const
    {
        return logsDirectory_;
    }
    const UnicodeString& GetRegExpsFilename() const
    {
        return regExpsFilename_;
    }
    const UnicodeString& GetRemindersFilename() const
    {
        return remindersFilename_;
    }
    const UnicodeString& GetNamedPipeName() const
    {
        return namedPipeName_;
    }
    const UnicodeString& GetLocale() const
    {
        return locale_;
    }

private:
    void ParseGeneral(xmlNode* node);
    void ParseServers(xmlNode* node);

    UnicodeString path_;
    UnicodeString scriptsDirectory_;
    UnicodeString logsDirectory_;
    UnicodeString regExpsFilename_;
    UnicodeString remindersFilename_;
    UnicodeString namedPipeName_;
    UnicodeString locale_;
    std::vector<Server> servers_;
};
