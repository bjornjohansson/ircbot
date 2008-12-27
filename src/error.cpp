#include <string.h>
#include <vector>
#include <string>

std::string GetErrorDescription(int error)
{
    std::string desc;

    std::vector<char> buf(1024, 0);

// GNU has a broken implementation of strerror_r
// (thank you for your incompetence) that returns char*
#ifdef _GNU_SOURCE
    char* res = strerror_r(error, &buf[0], static_cast<size_t>(buf.size()));
    if ( res )
    {
	desc = res;
    }
#else
    int res = strerror_r(error, &buf[0], static_cast<size_t>(buf.size()));
    if ( res == 0 )
    {
	description = &buffer[0];
    }
#endif
    
    return desc;
}
