#pragma once

#include <unicode/unistr.h>
#include <string>

class Exception
{
public:
    Exception(const std::string& location, int line, const UnicodeString& msg);

    const UnicodeString& GetMessage() const { return message_; }

private:
    UnicodeString location_;
    int line_;
    UnicodeString message_;
};
