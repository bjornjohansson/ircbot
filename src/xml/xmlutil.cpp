#include "xmlutil.hpp"
#include "../exception.hpp"

#include <vector>

// This is a class that merely calls xmlCleanupParser on destruction.
// Create one global object of this class and it'll call the cleanup
// on application exit.
class XmlParserCleaner
{
public:
    XmlParserCleaner() {}
    ~XmlParserCleaner() { xmlCleanupParser(); }
};
XmlParserCleaner xmlParserCleaner;

std::string ConvertUtf8ToLat1(const std::string& text)
{
    std::vector<unsigned char> buffer(text.size()+1, 0);
    int bufferSize = buffer.size();
    int textSize = text.size();

    int res = UTF8Toisolat1(&buffer[0], &bufferSize,
		  reinterpret_cast<const unsigned char*>(text.c_str()),
			    &textSize);
    if ( res < 0 )
    {
	throw Exception(__FILE__, __LINE__,
			"Could not convert string from UTF-8 to ISO-8859-1");
    }

    std::string lat1Text = reinterpret_cast<char*>(&buffer[0]);

    return lat1Text;
}

std::string GetXmlNodeAttribute(xmlNodePtr node, const std::string& name)
{
    std::string result;
    xmlChar* attribute = 
	xmlGetProp(node, reinterpret_cast<const xmlChar*>(name.c_str()));
    if ( attribute )
    {
	result = reinterpret_cast<const char*>(attribute);
	result = ConvertUtf8ToLat1(result);
	xmlFree(attribute);
    }
    else
    {
	throw Exception(__FILE__, __LINE__,
			std::string("Could not get attribute '")+name+
			std::string("' for node '")+
			reinterpret_cast<const char*>(node->name)+
			std::string("'"));
    }
    return result;
}

std::string GetXmlNodeTextContent(xmlNodePtr node)
{
    std::string result;

    if ( node->children && node->children->type == XML_TEXT_NODE )
    {
	result = reinterpret_cast<const char*>(node->children->content);
	result = ConvertUtf8ToLat1(result);
    }
    else
    {
	throw Exception(__FILE__, __LINE__, "Node has no text content");
    }
    return result;
}
