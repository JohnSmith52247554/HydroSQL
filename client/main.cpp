#include <client.hpp>
#include <utils/logger/include/logger.hpp>

const char getLastNon0(const std::string &str)
{
    auto it = str.rbegin();
    for (; it != str.rend(); it++)
    {
        if (*it != '\n')
            return *it;
    }
    return '\n';
}

int main()
{
    using namespace HydroSQL::Client::Network;
    using Logger = HydroSQL::Utils::Logger::Logger;

    try
    {
        std::string server_ip;
        int server_port;
        while (true)
        {
            std::cout << "Connect server: " << std::endl;
            std::cout << "Server IP: ";
            std::getline(std::cin, server_ip);
            server_ip = "127.0.0.1";
            std::cout << "Server Port: ";
            std::string buffer;
            std::getline(std::cin, buffer);
            try
            {
                server_port = std::stoi(buffer);
                break;
            }
            catch(...)
            {
                std::cerr << "Expect an integer." << std::endl;
            }
        }
        
        
        // server_port = 8080;
        // std::cout << std::endl;

        std::string msg = "Connecting server " + server_ip + ":" + std::to_string(server_port) + ".";
        Logger::get().info(msg);
        std::cout << msg << std::endl;

        Client client(server_ip, server_port);

        if (!client.connect())
        {
            return -1;
        }

        // Log In / Sign Up

        while (true)
        {
            std::string str = client.receive();
            std::cout << str << std::endl;

            if (str == "Log in success." || str == "Sign up success.")
                break;

            std::cout << "> ";

            std::getline(std::cin, str);

            if (!client.send(str))
                return -1;
        }

        while (true)
        {
            std::string str;

            std::string buffer;
            str.clear();
            do
            {
                std::cout << "> ";
                std::getline(std::cin, buffer);
                str += buffer + "\n";
            } while (getLastNon0(buffer) != ';');

            if (str == "QUIT;\n")
                break;
            if (!client.send(str))
                return -1;

            str = client.receive();
            std::cout << str << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        Logger::get().error(e.what());
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cin.get();

    return 0;
}