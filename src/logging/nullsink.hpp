#include "logsink.hpp"

class NullSink : public LogSink
{
    LogSink& operator<<(const std::string& rhs)
    {
	return *this;
    }
};
