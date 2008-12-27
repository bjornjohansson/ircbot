#include "xmldocument.hpp"
#include "../exception.hpp"

#include <libxml/xmlerror.h>

XmlDocument::XmlDocument(const std::string& file)
    :document_(xmlCtxtReadFile(*context_, file.c_str(), 0, 0))
{
    if ( document_ == 0 )
    {
	xmlErrorPtr error = xmlGetLastError();
	throw Exception(__FILE__, __LINE__, error->message);
    }
}

XmlDocument::~XmlDocument()
{
    if ( document_ )
    {
	xmlFreeDoc(document_);
    }
}
