/**
 * @file server.cpp
 * @author username (username52247554@gmail.com)
 * @brief Muti-thread TCP server
 * @version 0.1
 * @date 2025-06-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <server.hpp>
#include <authority/include/authority.hpp>
#include <parser/include/parser.hpp>
#include <parser/include/affairs.hpp>

#ifdef WIN32
#include <ws2tcpip.h>
#endif

namespace HydroSQL::Server::Network
{
    using Logger = Utils::Logger::Logger;
    using Sink = Utils::Logger::Sink;
    using ConsoleSink = Utils::Logger::BasicConsoleSink;
    using FileSink = Utils::Logger::BasicFileSink;
    using Level = Utils::Logger::Level;
    using Authoriser = Authority::Authoriser;

#ifdef WIN32
    Server::Server(const int port_, const size_t thread_num)
        : port(port_), thread_pool(thread_num), running(false)
    {
        std::unique_ptr<Sink> console_sink = std::make_unique<ConsoleSink>(Level::INFO, "hh:mm:ss.SSS");
        Logger::get().addSink(std::move(console_sink));
        std::unique_ptr<Sink> file_sink = std::make_unique<FileSink>(std::string(PROJECT_PATH) + "/log");
        Logger::get().addSink(std::move(file_sink));
        Logger::get().setLevel(this->log_level);
    }

    Server::~Server()
    {
        if (!running)
            return;

        running = false;

        closesocket(this->server_socket);
        WSACleanup();

        Logger::get().info("Server stopped.");
    }

    const int Server::run()
    {
        // initialize winsock
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            Logger::get().error("WSAStartup failed.");
            return 0;
        }

        // create server socket
        this->server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket == INVALID_SOCKET)
        {
            WSACleanup();
            Logger::get().error("Create socket failed.");
            return 0;
        }

        // set SO_REUSEADDR
        int opt = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
                       (char *)&opt, sizeof(opt)) == SOCKET_ERROR)
        {
            closesocket(server_socket);
            WSACleanup();
            Logger::get().error("Setsockopt failed.");
            return 0;
        }

        // bind address and port
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
        {
            closesocket(server_socket);
            WSACleanup();
            Logger::get().error("Bind failed.");
            return 0;
        }

        // start listening
        if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR)
        {
            closesocket(server_socket);
            WSACleanup();
            Logger::get().error("Listen failed.");
            return 0;
        }

        Logger::get().info("Start listening on port " + std::to_string(this->port) + ".");

        running = true;

        while (running)
        {
            sockaddr_in client_addr;
            int client_addr_size = sizeof(client_addr);

            SOCKET client_socket = accept(server_socket,
                                         (sockaddr *)&client_addr,
                                         &client_addr_size);

            if (client_socket == INVALID_SOCKET)
            {
                if (running)
                {
                    Logger::get().error("Accept failed. Error code: " + std::to_string(WSAGetLastError()) + ".");
                }
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

            Logger::get().info("New connection from " + std::string(client_ip) + ":" + std::to_string(ntohs(client_addr.sin_port)));


            if (!thread_pool.busy())
                thread_pool.post(std::bind(handleClient, this, client_socket, std::string(client_ip) + ":" + std::to_string(ntohs(client_addr.sin_port))));
            else
            {
                Logger::get().warning("Server busy. Reject connection from"+ std::string(client_ip) + ":" + std::to_string(ntohs(client_addr.sin_port)));

                std::string response = "Server busy. Please try again latter.";
                if (::send(client_socket, response.c_str(), response.size(), 0) == SOCKET_ERROR)
                {
                    Logger::get().error("Send reponse failed.");
                }
            }
        }

        return 1;
    }

    const int Server::send(std::string &msg, const SOCKET client_socket, const std::string &username)
    {
        if (::send(client_socket, msg.c_str(), msg.size(), 0) == SOCKET_ERROR)
        {
            Logger::get().error(username + ": Send reponse failed.");
            return 0;
        }
        return 1;
    }

    const int Server::recieve(std::string &msg, const SOCKET client_socket, const std::string &username)
    {
        char buffer[4096];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received == 0)
        {
            Logger::get().info(username + ": Client disconnected.");
            return 0;
        }
        else if (bytes_received < 0)
        {
            Logger::get().error(username + ": Recv failed.");
            return 0;
        }

        msg = std::string(buffer, bytes_received);
        return 1;
    }

    void Server::handleClient(const SOCKET client_socket, const std::string client_addr)
    {
        try
        {
            std::string msg = "(L)Log In\n(S)Sign Up";
            std::string username = client_addr;
            while (true)
            {
                if (!send(msg, client_socket, username))
                    return;

                if (!recieve(msg, client_socket, username))
                    return;
                if (msg == "L" || msg == "l" || msg == "Log In")
                {
                    // log in
                    msg = "Username: ";
                    if (!send(msg, client_socket, username))
                        return;

                    if (!recieve(username, client_socket, username))
                        return;

                    msg = "Password: ";
                    if (!send(msg, client_socket, username))
                        return;

                    std::string buffer;
                    while (true)
                    {
                        if (!recieve(buffer, client_socket, username))
                            return;
                        if (buffer == "Get stored password hash.")
                            break;
                    }
                    try
                    {
                        auto password_hash = Authority::AuthManager::get().getPasswordHash(username);
                        if (!send(password_hash, client_socket, username))
                            return;

                        if (!recieve(msg, client_socket, username))
                            return;

                        if (msg == "Password correct.")
                        {
                            msg = "Log in success.";
                            Logger::get().info(username + ": " + msg);
                            if (!send(msg, client_socket, username))
                                return;
                            break;
                        }
                        else
                        {
                            msg = "Password incorrect.\n(L)Log In\n(S)Sign Up";
                        }

                        // if (Authority::AuthManager::get().examinePasswordHash(username, password_hash))
                        // {
                        //     msg = "Log in success.";
                        //     Logger::get().info(username + ": " + msg);
                        //     if (!send(msg, client_socket, username))
                        //         return;
                        //     break;
                        // }
                        // else
                        // {
                        //     msg = "Password incorrect.\n(L)Log In\n(S)Sign Up";
                        // }
                    }
                    catch (std::exception &e)
                    {
                        msg = std::string(e.what()) + "\n(L)Log In\n(S)Sign Up";
                    }
                    
                }
                else if (msg == "S" || msg == "s" || msg == "Sign Up")
                {
                    // sign up
                    msg = "Username: ";
                    if (!send(msg, client_socket, username))
                        return;

                    if (!recieve(username, client_socket, username))
                        return;

                    msg = "Password: ";
                    if (!send(msg, client_socket, username))
                        return;

                    std::string password_hash;
                    if (!recieve(password_hash, client_socket, username))
                        return;

                    try
                    {
                        if (Authority::AuthManager::get().addUser(username, password_hash))
                        {
                            msg = "Sign up success.";
                            Logger::get().info(username + ": " + msg);
                            if (!send(msg, client_socket, username))
                                return;
                            break;
                        }
                    }
                    catch (std::exception &e)
                    {
                        msg = std::string(e.what()) + "\n(L)Log In\n(S)Sign Up";
                    }
                }
                else
                    msg = "Command not found.\n(L)Log In\n(S)Sign Up";
            }

            while (true)
            {
                try
                {
                    if (!recieve(msg, client_socket, username))
                        return;

                    Logger::get().info(username + ": " + msg);

                    auto tokens = Parser::tokenize(msg);
                    auto affair = Parser::parse(tokens);
                    if (affair == nullptr)
                    {
                        msg = "Parse failed.";
                        if (!send(msg, client_socket, username))
                            return;
                        continue;
                    }

                    auto auth = std::make_unique<Authoriser>(username);
                    std::string result;
                    affair->execute(std::move(auth), result);

                    if (!send(result, client_socket, username))
                        return;
                }
                catch(const std::exception& e)
                {
                    static const std::string warning_pref = "[FAILED]";
                    std::string err = e.what();
                    if (err.substr(0, warning_pref.size()) == warning_pref)
                        Logger::get().warning(username + ": " + err);
                    else
                        Logger::get().error(username + ": " + err);
                    if (!send(err, client_socket, username))
                        return;
                }
            }

        }
        catch(const std::exception& e)
        {
            Logger::get().error(e.what());
        }
        

        closesocket(client_socket);
    }

#endif
}