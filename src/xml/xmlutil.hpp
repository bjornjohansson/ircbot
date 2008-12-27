#pragma once

#include <string>

#include <libxml/tree.h>

/**
 * @throw Exception if attribute can't be found
 */
std::string GetXmlNodeAttribute(xmlNodePtr node, const std::string& name);

/**
 * @throw Exception if no text content can be found
 */
std::string GetXmlNodeTextContent(xmlNodePtr node);
