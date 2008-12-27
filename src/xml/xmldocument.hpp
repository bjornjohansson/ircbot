#pragma once

#include "xmlparsercontext.hpp"

#include <string>

#include <libxml/tree.h>

class XmlDocument
{
public:
    /**
     * @throw Exception if document could not be created
     */
    XmlDocument(const std::string& filename);
    virtual ~XmlDocument();

    xmlDocPtr operator*() { return document_; }
    xmlDocPtr operator->() { return document_; }

private:
    XmlParserContext context_;
    xmlDocPtr document_;
};
