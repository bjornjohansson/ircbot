#include "client.hpp"
#include "exception.hpp"
#include "connection/connectionmanager.hpp"

#include <string>
#include <vector>
#include <iostream>

int main(int argc, char* argv[])
{
    if ( argc < 2 )
    {
	std::cerr<<"You must specify the name of a configuration file"
		 <<std::endl;
	return 0;
    }

    {
        Client client(argv[1]);

	std::cout<<"Running client"<<std::endl;
	client.Run();
    }
    std::cout<<"Client exited"<<std::endl;
}
