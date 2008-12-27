#include "xmlparsercontext.hpp"
#include "../exception.hpp"

XmlParserContext::XmlParserContext()
    : context_(xmlNewParserCtxt())
{
    if ( context_ == 0 )
    {
	throw Exception(__FILE__, __LINE__, "Could not allocate context");
    }
}

XmlParserContext::~XmlParserContext()
{
    if ( context_ )
    {
	xmlFreeParserCtxt(context_);
    }
}
