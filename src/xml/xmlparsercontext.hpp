#pragma once

#include <libxml/parser.h>

class XmlParserContext
{
public:
    /**
     * @throw Exception if context could not be created
     */
    XmlParserContext();
    virtual ~XmlParserContext();

    xmlParserCtxtPtr operator->() { return context_; }
    xmlParserCtxtPtr operator*() { return context_; }

private:
    xmlParserCtxtPtr context_;

};
