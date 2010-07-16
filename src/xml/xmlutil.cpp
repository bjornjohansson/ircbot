#include "xmlutil.hpp"
#include "../exception.hpp"

#include <vector>
#include <converter.hpp>

// This is a class that merely calls xmlCleanupParser on destruction.
// Create one global object of this class and it'll call the cleanup
// on application exit.
class XmlParserCleaner
{
public:
	XmlParserCleaner()
	{
	}
	~XmlParserCleaner()
	{
		xmlCleanupParser();
	}
};
XmlParserCleaner xmlParserCleaner;

std::string GetXmlNodeAttribute(xmlNodePtr node, const std::string& name)
{
	std::string result;
	xmlChar* attribute = xmlGetProp(node,
			reinterpret_cast<const xmlChar*> (name.c_str()));
	if (attribute)
	{
		result = reinterpret_cast<const char*> (attribute);
		xmlFree(attribute);
	}
	else
	{
		throw Exception(__FILE__, __LINE__, AsUnicode("Could not get attribute '" + name
				+ "' for node '" + reinterpret_cast<const char*> (node->name) + "'"));
	}
	return result;
}

std::string GetXmlNodeTextContent(xmlNodePtr node)
{
	std::string result;

	if (node->children && node->children->type == XML_TEXT_NODE)
	{
		result = reinterpret_cast<const char*> (node->children->content);
	}
	else
	{
		throw Exception(__FILE__, __LINE__, "Node has no text content");
	}
	return result;
}
