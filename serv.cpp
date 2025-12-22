#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "config.hpp"
#include "handleClients.hpp"
#include "HttpServer.hpp"

int main(int argc , char **argv)
{
    if(argc > 2)
    {
        return 0;
    }

    std::vector<HttpServer> servers = parser(std::string(argv[1]));
    multiple(servers);

    return 0;
}
