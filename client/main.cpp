#include <network/include/client.hpp>
#include <encrypt/include/encrypt.hpp>
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
    using namespace HydroSQL::Client;
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
        

        std::string msg = "Connecting server " + server_ip + ":" + std::to_string(server_port) + ".";
        Logger::get().info(msg);
        std::cout << msg << std::endl;

        Network::Client client(server_ip, server_port);

        if (!client.connect())
        {
            return -1;
        }

        // Log In / Sign Up
        while (true)
        {
            std::string str = client.receive();
            std::cout << str << std::endl;
            if (str == "Server busy. Please try again latter.")
            {
                std::cout << "\nPress any key to exit." << std::endl;
                std::cin.get();
                return 0;
            }

            std::cout << "> ";
            std::getline(std::cin, str);
            if (!client.send(str))
                return -1;

            if (str == "L" || str == "l" || str == "Log In")
            {
                while (true)
                {
                    str = client.receive();
                    std::cout << str << std::endl;
                    if (str == "Log in success.")
                        break;

                        std::cout << "> ";

                    if (str == "Password: ")
                    {
                        auto password = Encrypt::secureEnter();
                        std::string msg = "Get stored password hash.";
                        if (!client.send(msg))  
                            return -1;
                        std::string password_hash = client.receive();
                        if(Encrypt::verifyHash(std::move(password), password_hash))
                        {
                            str = "Password correct.";
                        }
                        else
                            str = "Password incorrecr.";
                    }
                    else
                        std::getline(std::cin, str);

                    if (!client.send(str))
                        return -1;
                }
                break;
            }
            else if (str == "S" || str == "s" || str == "Sign Up")
            {
                while (true)
                {
                    str = client.receive();
                    std::cout << str << std::endl;
                    if (str == "Sign up success.")
                        break;

                        std::cout << "> ";

                    if (str == "Password: ")
                    {
                        std::string result;
                        while (true)
                        {
                            str = Encrypt::secureEnter();
                            if (!Encrypt::validPassword(str, result))
                            {
                                std::cout << result << "\nPassword: \n> ";
                                continue;
                            }
                            std::cout << "Please enter again: \n> ";
                            auto str2 = Encrypt::secureEnter();
                            if (str != str2)
                            {
                                std::cout << "The input is different." << std::endl;
                                std::cout << "Password: \n> ";
                                continue;
                            }
                            break;
                        }
                        Encrypt::hash(str);
                    }
                    else if (str == "Username: ")
                    {
                        std::string result;
                        while (true)
                        {
                            std::getline(std::cin, str);
                            if (Encrypt::validUsername(str, result))
                                break;
                            std::cout << result << "\nUsername: \n> ";
                        }
                    }
                    else
                    {
                        std::getline(std::cin, str);
                    }
                    

                    if (!client.send(str))
                        return -1;
                }
                break;
                
            }
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


    std::cout << "\nPress any key to exit." << std::endl;
    std::cin.get();


    return 0;
}