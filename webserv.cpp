#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <csignal>
#include "config.hpp"
#include "handleClients.hpp"
#include "HttpServer.hpp"

volatile sig_atomic_t g_running = 1;

void signalHandler(int signum)
{
    (void)signum;
    g_running = 0;
    std::cout << "\nShutting down server..." << std::endl;
}

int main(int argc , char **argv)
{
    if(argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " [configuration file]" << std::endl;
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    try
    {
        std::string configPath = (argc == 2) ? argv[1] : "default.conf";
        std::vector<HttpServer> servers = parser(configPath);
        
        if (servers.empty())
        {
            std::cerr << "Error: No servers configured" << std::endl;
            return 1;
        }
        
        for (size_t i = 0; i < servers.size(); i++)
        {
            servers[i].normalize();
        }
        
        multiple(servers);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
