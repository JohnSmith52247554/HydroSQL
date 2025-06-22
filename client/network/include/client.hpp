/**
 * @file client.hpp
 * @author username (username52247554@gmail.com)
 * @brief TCP client
 * @version 0.1
 * @date 2025-06-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <pch.hpp>

#ifdef WIN32
#include <winsock.h>
#endif

namespace HydroSQL::Client::Network
{
    class Client
    {
    private:
        SOCKET client_socket;
        std::string server_ip;
        int server_port;
        bool connected;

    public:
        Client(const std::string &server_ip_, int server_port_);
        ~Client();

        const int connect();

        const int send(const std::string &data);

        const std::string receive();

    private:
        void disconnect();
    };


} // namespace HydroSQL::Client::Network