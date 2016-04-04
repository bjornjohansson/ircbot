#include "config.hpp"
#include "exception.hpp"
#include "xml/xmldocument.hpp"
#include "xml/xmlutil.hpp"
#include "logging/logger.hpp"

#include <boost/lexical_cast.hpp>
#include <converter.hpp>

bool operator==(const std::string& lhs, const xmlChar* rhs)
{
    return lhs == reinterpret_cast<const char*> (rhs);
}

Config::Config(const UnicodeString& path) :
    path_(path)
{
    try
    {
        XmlDocument document(AsUtf8(path));

        xmlNode* root = xmlDocGetRootElement(*document);
        if (root && std::string("config") == root->name)
        {
            for (xmlNode* child = root->children; child; child = child->next)
            {
                if (std::string("general") == child->name)
                {
                    ParseGeneral(child);
                }
                else if (std::string("servers") == child->name)
                {
                    ParseServers(child);
                }
            }
        }
    } catch (Exception& e)
    {
        Log << LogLevel::Error << e.GetMessage();
        throw;
    }
}

void Config::ParseGeneral(xmlNode* node)
{
    for (xmlNode* child = node->children; child; child = child->next)
    {
        if (std::string("scripts") == child->name)
        {
            scriptsDirectory_ = AsUnicode(GetXmlNodeTextContent(child));
        }
        else if (std::string("logs") == child->name)
        {
            logsDirectory_ = AsUnicode(GetXmlNodeTextContent(child));
        }
        else if (std::string("regexps") == child->name)
        {
            regExpsFilename_ = AsUnicode(GetXmlNodeTextContent(child));
        }
        else if (std::string("reminders") == child->name)
        {
            remindersFilename_ = AsUnicode(GetXmlNodeTextContent(child));
        }
        else if (std::string("namedpipe") == child->name)
        {
            namedPipeName_ = AsUnicode(GetXmlNodeTextContent(child));
        }
        else if (std::string("locale") == child->name)
        {
            locale_ = AsUnicode(GetXmlNodeTextContent(child));
        }
    }
}

void Config::ParseServers(xmlNode* node)
{
    for (xmlNode* child = node->children; child; child = child->next)
    {
        if (std::string("server") == child->name)
        {
            std::string typeStr;
            Server::Type type;
            std::string rawId;
            UnicodeString id;
            UnicodeString host;
            int port = 0;
            UnicodeString nick;
            UnicodeString password;

            typeStr = GetXmlNodeAttribute(child, "type");
            rawId = GetXmlNodeAttribute(child, "id");
            id = AsUnicode(rawId);
            host = AsUnicode(GetXmlNodeAttribute(child, "host"));
            try
            {
                port = boost::lexical_cast<int>(GetXmlNodeAttribute(child,
                    "port"));
            }
            catch (boost::bad_lexical_cast&)
            {
                throw Exception(__FILE__, __LINE__,
                                ("Invalid port in configuration for server "
                                 + rawId).c_str());
            }
            try
            {
                password = AsUnicode(GetXmlNodeAttribute(child, "password"));
            } catch (Exception&)
            {
                // Ignore missing password attribute
            }

            if (typeStr == "irc")
            {
                type = Server::IrcServer;
            }
            else if (typeStr == "matrix")
            {
                type = Server::MatrixServer;
            }
            else
            {
                throw Exception(__FILE__, __LINE__, ("Invalid server type " + typeStr).c_str());
            }

            Server::ChannelContainer channels;

            for (xmlNodePtr subChild = child->children; subChild; subChild
                    = subChild->next)
            {
                if (std::string("nick") == subChild->name)
                {
                    nick = AsUnicode(GetXmlNodeTextContent(subChild));
                }
                else if (std::string("channels") == subChild->name)
                {
                    for (xmlNodePtr channel = subChild->children; channel; channel
                            = channel->next)
                    {
                        if (std::string("channel") == channel->name)
                        {
                            UnicodeString key;
                            std::string chanName;
                            try
                            {
                                key = AsUnicode(GetXmlNodeAttribute(channel, "key"));
                            } catch (Exception&)
                            {
                            }
                            try
                            {
                                chanName = GetXmlNodeTextContent(channel);
                                channels.push_back(Server::ChannelAndKey(chanName,
                                        key));
                            } catch (Exception&)
                            {
                            }
                        }
                    }
                }
            }
            Server server(type, id, host, port, nick, password);
            for (Server::ChannelContainer::iterator i = channels.begin(); i
                    != channels.end(); ++i)
            {
                server.AddChannel(i->first, i->second);
            }
            servers_.push_back(server);
        }
    }
}

Config::Server::Server() :
    port_(0)
{
}

Config::Server::Server(Type type,
                       const UnicodeString& id,
                       const UnicodeString& host,
                       unsigned int port,
                       const UnicodeString& nick,
                       const UnicodeString& password)
    : type_(type)
    , id_(id)
    , host_(host)
    , port_(port)
    , nick_(nick)
    , password_(password)
{
}

void Config::Server::AddChannel(const std::string& name, const UnicodeString& key)
{
    channels_.push_back(ChannelAndKey(name, key));
}
