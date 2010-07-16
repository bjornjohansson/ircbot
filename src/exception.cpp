#include "exception.hpp"

#include <converter.hpp>

Exception::Exception(const std::string& location,
		     int line,
		     const UnicodeString& message)
    : location_(AsUnicode(location)),
      line_(line),
      message_(message)
{


}
