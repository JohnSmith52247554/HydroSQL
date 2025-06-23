/**
 * @file server.hpp
 * @author username (username52247554@gmail.com)
 * @brief Muti-thread TCP server.
 * @version 0.1
 * @date 2025-06-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <utils/logger/include/logger.hpp>
#include <ThreadPool.hpp>

#ifdef WIN32
#include <winsock2.h>
#endif

namespace HydroSQL::Server::Network
{
    /**
     * @brief TCP server
     * 
     */
    class Server
    {
    private:
        const Utils::Logger::Level log_level = Utils::Logger::Level::INFO;

        int port;
        ThreadPool thread_pool;
        std::atomic<bool> running;

#ifdef WIN32
        SOCKET server_socket;
#endif
#ifdef _linux

#endif

    public:
        Server(const int port, const size_t thread_num);
        ~Server();

        const int run();

    private:
        const int send(std::string &msg, const SOCKET client_socket, const std::string &username);
        const int recieve(std::string &msg, const SOCKET client_socket, const std::string &username);

#ifdef WIN32
        // The function to be push into the thread pool.
        void handleClient(const SOCKET client_socket, const std::string client_addr);
#endif
    };
}