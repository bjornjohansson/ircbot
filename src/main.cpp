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

    Client client(argv[1]);

    ConnectionManager& connectionManager = ConnectionManager::Instance();

    connectionManager.Loop();

/*
    try
    {
      Server server(host);

      while(true)
      {
	  sleep(30000);
//	  server.Wait();
      }
    }
    catch ( Exception& e )
    {
	std::cout<<e.GetMessage()<<std::endl;
    }
    return 0;
*/
}
