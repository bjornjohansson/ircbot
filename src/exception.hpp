#pragma once

#include <string>

class Exception
{
public:
    Exception(const std::string& location, int line, const std::string& msg);

    const std::string& GetMessage() const { return message_; }

private:
    std::string location_;
    int line_;
    std::string message_;
};
