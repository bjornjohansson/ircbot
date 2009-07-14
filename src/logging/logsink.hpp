#pragma once

#include <string>

class LogSink
{
public:
    LogSink();
    virtual ~LogSink();

    virtual LogSink& operator<<(const std::string& rhs) = 0;

protected:
    std::string GetPrefix() const;
    std::string GetSuffix() const;

private:
    

};
