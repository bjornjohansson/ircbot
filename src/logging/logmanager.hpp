#pragma once
#include "loglevel.hpp"
#include "stdoutsink.hpp"
#include "nullsink.hpp"
#include "logsink.hpp"

class LogManager
{
public:
    static LogManager& Instance();

    LogSinkPtr GetSink(const LogLevel::LogLevel logLevel)
    {
    switch(logLevel)
    {
    case LogLevel::Trace:
    case LogLevel::Debug:
#ifdef DEBUG
        return stdOutSink_;
#else
        break;
#endif
    case LogLevel::Info:
    case LogLevel::Warning:
    case LogLevel::Error:
    case LogLevel::Critical:
    default:
        return stdOutSink_;
        break;
    }
    return nullSink_;
    }


private:
    LogManager();
    LogManager(const LogManager&);
    LogManager& operator=(const LogManager&);

    LogSinkPtr stdOutSink_;
    LogSinkPtr nullSink_;
};
