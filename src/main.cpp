#include "client.hpp"
#include "exception.hpp"
#include "logging/logger.hpp"

#include <string>
#include <vector>

#ifdef __linux__
#include <sys/time.h>
#include <sys/resource.h>
#endif

int main(int argc, char* argv[])
{
	if ( argc < 2 )
	{
		Log<<LogLevel::Error
		   <<"You must specify the name of a configuration file";
		return 0;
	}

	{
		Log<<LogLevel::Info<<boost::thread::hardware_concurrency()
		   <<" concurrent threads supported.";

#ifdef __linux__
		rlimit limit = { 1<<20, 1<<20 };
		setrlimit(RLIMIT_STACK, &limit);
#endif
		
		Client client(argv[1]);

		Log<<LogLevel::Info<<"Running client";
		client.Run();
	}
	Log<<LogLevel::Info<<"Client exited";
}
