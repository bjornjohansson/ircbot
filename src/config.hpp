#pragma once

#include <string>
#include <vector>

#include <libxml/tree.h>

class Config
{
public:
    Config(const std::string& path);

    class Server
    {
    public:
	Server();
	Server(const std::string& id,
	       const std::string& host,
	       unsigned int port,
	       const std::string& nick);

	void AddChannel(const std::string& name, const std::string& key);

	const std::string& GetId() const { return id_; }
	const std::string& GetHost() const { return host_; }
	unsigned int GetPort() const { return port_; }
	const std::string& GetNick() const { return nick_; }

	typedef std::pair<std::string, std::string> ChannelAndKey;
	typedef std::vector<ChannelAndKey> ChannelContainer;
	typedef ChannelContainer::const_iterator ChannelIterator;

	ChannelIterator GetChannelsBegin() const { return channels_.begin(); }
	ChannelIterator GetChannelsEnd() const { return channels_.end(); }

    private:
	std::string id_;
	std::string host_;
	unsigned int port_;
	std::string nick_;
	ChannelContainer channels_;
    };

    typedef std::vector<Server> ServerContainer;
    typedef ServerContainer::const_iterator ServerIterator;

    ServerIterator GetServersBegin() const { return servers_.begin(); }
    ServerIterator GetServersEnd() const { return servers_.end(); }

    const std::string& GetScriptsDirectory() const {return scriptsDirectory_;}
    const std::string& GetLogsDirectory() const { return logsDirectory_; }
    const std::string& GetRegExpsFilename() const { return regExpsFilename_; }
    const std::string& GetRemindersFilename() const {return remindersFilename_;}

private:
    void ParseGeneral(xmlNode* node);
    void ParseServers(xmlNode* node);

    std::string path_;
    std::string scriptsDirectory_;
    std::string logsDirectory_;
    std::string regExpsFilename_;
    std::string remindersFilename_;
    std::vector<Server> servers_;
};
