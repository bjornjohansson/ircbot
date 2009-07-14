#include "logsink.hpp"

#include <boost/date_time.hpp>

LogSink::LogSink()
{
}

LogSink::~LogSink()
{
}


std::string LogSink::GetPrefix() const
{
    using namespace boost::posix_time;
    ptime t(microsec_clock::local_time());

    return to_iso_extended_string(t) + " ";
}

std::string LogSink::GetSuffix() const
{
    return std::string();
}
