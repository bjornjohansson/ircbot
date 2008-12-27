#include "exception.hpp"

Exception::Exception(const std::string& location,
		     int line,
		     const std::string& message)
    : location_(location),
      line_(line),
      message_(message)
{


}
