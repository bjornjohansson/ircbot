#pragma once
#include "logsink.hpp"

class StdOutSink : public LogSink
{
public:
    StdOutSink();

    StdOutSink& operator<<(const std::string& rhs);
};
