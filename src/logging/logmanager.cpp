#include "logmanager.hpp"

LogManager& LogManager::Instance()
{
    static LogManager logManager;

    return logManager;
}

LogManager::LogManager()
	: stdOutSink_(new StdOutSink())
	, nullSink_(new NullSink())
{
    

}
