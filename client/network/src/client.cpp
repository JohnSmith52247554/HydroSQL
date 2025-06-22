/**
 * @file client.cpp
 * @author username (username52247554@gmail.com)
 * @brief TCP client
 * @version 0.1
 * @date 2025-06-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <client.hpp>
#include <utils/logger/include/logger.hpp>

#ifdef WIN32
#include <ws2tcpip.h>
#endif

namespace HydroSQL::Client::Network
{
    using Logger = Utils::Logger::Logger;
    using Sink = Utils::Logger::Sink;
    using ConsoleSink = Utils::Logger::BasicConsoleSink;
    using FileSink = Utils::Logger::BasicFileSink;
    using Level = Utils::Logger::Level;

#ifdef WIN32
    Client::Client(const std::string &server_ip_, int server_port_)
        : server_ip(server_ip_), server_port(server_port_), client_socket(INVALID_SOCKET), connected(false)
    {
        std::unique_ptr<Sink> file_sink = std::make_unique<FileSink>(std::string(PROJECT_PATH) + "/log");
        Logger::get().addSink(std::move(file_sink));
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            Logger::get().error("WSAStartup failed.");
            throw std::runtime_error("WSAStartup failed.");
        }
    }

    Client::~Client()
    {
        disconnect();
        WSACleanup();
    }

    const int Client::connect()
    {
        if (connected)
            return 1;

        client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (client_socket == INVALID_SOCKET)
        {
            std::string msg = "Socket creation failed. Error code: " + std::to_string(WSAGetLastError()) + ".";
            Logger::get().error(msg);
            std::cerr << msg << std::endl;
            return 0;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

        if (::connect(client_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            std::string msg = "Connect failed. Error code: " + std::to_string(WSAGetLastError()) + ".";
            Logger::get().error(msg);
            std::cerr << msg  << std::endl;
            closesocket(client_socket);
            return 0;
        }

        connected = true;

        std::string str = "Connected to server " + server_ip + ":" + std::to_string(server_port) + ".";
        Logger::get().info(str);
        std::cout << str << std::endl;
        return 1;
    }

    const int Client::send(const std::string &data)
    {
        if (!connected)
        {
            std::string msg = "Not connected to server.";
            Logger::get().error(msg);
            std::cerr << msg << std::endl;
            return 0;
        }

        int bytes_sent = ::send(client_socket, data.c_str(), data.size(), 0);
        if (bytes_sent == SOCKET_ERROR)
        {
            std::string msg = "Send failed. Error code: " + std::to_string(WSAGetLastError()) + ".";
            std::cerr << msg << std::endl;
            disconnect();
            return 0;
        }

        std::string msg = std::to_string(bytes_sent) + " bytes sent.";
        Logger::get().info(msg);
        return 1;
    }

    const std::string Client::receive()
    {
        if (!connected)
        {
            std::string msg = "Not connected to server.";
            Logger::get().error(msg);
            std::cerr << msg << std::endl;
            return "";
        }

        char buffer[1024];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received > 0)
        {
            return std::string(buffer, bytes_received);
        }
        else if (bytes_received == 0)
        {
            std::string msg = "Server disconnected.";
            Logger::get().info(msg);
            std::cout << msg << std::endl;
            disconnect();
            return msg;
        }
        else
        {
            std::string msg = "Receive failed. Error code: " + std::to_string(WSAGetLastError()) + ".";
            Logger::get().error(msg);
            std::cerr << msg << std::endl;
            disconnect();
            return msg;
        }
    }

    void Client::disconnect()
    {
        if (connected)
        {
            closesocket(client_socket);
            connected = false;
            Logger::get().info("Disconnected from server.");
            std::cout << "Disconnected from server.";
        }
    }
#endif
}