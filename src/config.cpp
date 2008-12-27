#include "config.hpp"
#include "exception.hpp"
#include "xml/xmldocument.hpp"
#include "xml/xmlutil.hpp"

#include <iostream>

#include <boost/lexical_cast.hpp>

bool operator==(const std::string& lhs, const xmlChar* rhs)
{
    return lhs == reinterpret_cast<const char*>(rhs);
}

Config::Config(const std::string& path)
    : path_(path)
{
    try
    {
	XmlDocument document(path);

	xmlNode* root = xmlDocGetRootElement(*document);
	if ( root && std::string("config") == root->name )
	{
	    for(xmlNode* child = root->children; child; child = child->next)
	    {
		if ( std::string("general") == child->name )
		{
		    ParseGeneral(child);
		}
		else if ( std::string("servers") == child->name )
		{
		    ParseServers(child);
		}
	    }
	}
    }
    catch ( Exception& e )
    {
	std::cerr<<e.GetMessage()<<std::endl;
    }
}

void Config::ParseGeneral(xmlNode* node)
{
    for(xmlNode* child = node->children; child; child = child->next)
    {
	if ( std::string("scripts") == child->name )
	{
	    scriptsDirectory_ = GetXmlNodeTextContent(child);
	}
	else if ( std::string("logs") == child->name )
	{
	    logsDirectory_ = GetXmlNodeTextContent(child);
	}
	else if ( std::string("regexps") == child->name )
	{
	    regExpsFilename_ = GetXmlNodeTextContent(child);
	}
	else if ( std::string("reminders") == child->name )
	{
	    remindersFilename_ = GetXmlNodeTextContent(child);
	}
    }
}

void Config::ParseServers(xmlNode* node)
{
    for(xmlNode* child = node->children; child; child = child->next)
    {
	if ( std::string("server") == child->name )
	{
	    try
	    {
		std::string id;
		std::string host;
		int port = 0;
		std::string nick;

		id = GetXmlNodeAttribute(child, "id");
		host = GetXmlNodeAttribute(child, "host");
		port = boost::lexical_cast<int>(GetXmlNodeAttribute(child,
								    "port"));

		typedef std::pair<std::string, std::string> ChannelAndKey;
		std::vector<ChannelAndKey> channels;

		for(xmlNodePtr subChild = child->children;
		    subChild;
		    subChild = subChild->next)
		{
		    if ( std::string("nick") == subChild->name )
		    {
			nick = GetXmlNodeTextContent(subChild);
		    }
		    else if ( std::string("channels") == subChild->name )
		    {
			for(xmlNodePtr channel = subChild->children;
			    channel;
			    channel = channel->next)
			{
			    if ( std::string("channel") == channel->name )
			    {
				std::string key;
				std::string chanName;
				try
				{
				    key = GetXmlNodeAttribute(channel, "key");
				}
				catch ( Exception& )
				{
				}
				try
				{
				    chanName = GetXmlNodeTextContent(channel);
				    channels.push_back(ChannelAndKey(chanName,
								     key));
				}
				catch ( Exception& )
				{
				}
			    }
			}
		    }
		}
		Server server(id, host, port, nick);
		for(std::vector<ChannelAndKey>::iterator i = channels.begin();
		    i != channels.end();
		    ++i)
		{
		    server.AddChannel(i->first, i->second);
		}
		servers_.push_back(server);
	    }
	    catch ( Exception& e )
	    {
	    }
	    catch ( boost::bad_lexical_cast& )
	    {
	    }
	}
    }
}

Config::Server::Server()
    : port_(0)
{
}

Config::Server::Server(const std::string& id,
		       const std::string& host,
		       unsigned int port,
		       const std::string& nick)
    : id_(id),
      host_(host),
      port_(port),
      nick_(nick)
{
}

void Config::Server::AddChannel(const std::string& name,
				const std::string& key)
{
    channels_.push_back(ChannelAndKey(name, key));
}
