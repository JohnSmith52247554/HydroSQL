/**
 * @file main.cpp
 * @author username (username52247554@gmail.com)
 * @brief main source file
 * @version 0.1
 * @date 2025-06-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */


#include <network/include/server.hpp>
#include <utils/logger/include/logger.hpp>


int main()
{
#ifdef WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    try
    {
        HydroSQL::Server::Network::Server server(8080, 4);
        server.run();
    }
    catch(std::exception &e)
    {
        HydroSQL::Utils::Logger::Logger::get().error(e.what());
    }
    return 0;
}