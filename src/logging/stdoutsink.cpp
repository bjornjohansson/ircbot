#include "stdoutsink.hpp"

#include <iostream>

StdOutSink::StdOutSink()
{

}

StdOutSink& StdOutSink::operator<<(const std::string& rhs)
{
    std::cout<<GetPrefix()<<rhs<<GetSuffix()<<std::endl;
    return *this;
}
