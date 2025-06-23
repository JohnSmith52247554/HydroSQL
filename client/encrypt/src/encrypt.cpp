/**
 * @file encrypt.hpp
 * @author username (username52247554@gmail.com)
 * @brief encrypt
 * @version 0.1
 * @date 2025-06-23
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <sodium.h>

#include <encrypt.hpp>
#include <utils/logger/include/logger.hpp>

#ifdef WIN32
#include <conio.h>
#endif

namespace HydroSQL::Client::Encrypt
{
    using Logger = HydroSQL::Utils::Logger::Logger;

    const std::string secureEnter()
    {
        std::string input;
        char ch;
        while ((ch = _getch()) != '\r') // get character until newline
        {
            if (ch == '\b' && !input.empty())
            { // deal with delete
                input.pop_back();
                std::cout << "\b \b"; // clear console
            }
            else if (ch != '\b')
            {
                input.push_back(ch);
                std::cout << "*";
            }
        }
        std::cout << "\n";

        return input;
    }

    const bool chararcherCheck(const std::string &password)
    {
        std::vector<unsigned char> apper_time(3, 0u);

        for (const auto &c : password)
        {
            if (c >= 'a' && c <= 'z')
                apper_time[0]++;
            else if (c >= 'A' && c <= 'Z')
                apper_time[1]++;
            else if (c >= '0' && c <= '9')
                apper_time[2]++;
        }

        for (const auto &num : apper_time)
        {
            if (num == '\0')
                return false;
        }
        return true;
    }

    const bool consecutiveCheck(const std::string &password, const unsigned char max_repeat_time)
    {
        char flag = '\0';
        unsigned char time = 0;
        for (auto it = password.begin(); it != password.end(); it++)
        {
            if (*it != flag)
            {
                flag = *it;
                time = 0;
                do
                {
                    time++;
                    it++;
                } while (*it == flag && it != password.end());
                if (it == password.end())
                    break;
            }
            if (time >= max_repeat_time)
                return false;
        }
        return true;
    }

    const bool validPassword(const std::string &password, std::string &result)
    {
        if (password.size() < 8)
        {
            result = "The length of the master password shoud be no less than eight characters.";
            return false;
        }
        if (!chararcherCheck(password))
        {
            result = "The password should contain lower characters, upper characters and numbers.";
            return false;
        }
        if (!consecutiveCheck(password, 2))
        {
            result = "There should not be a character appears consecutively more than two times.";
            return false;
        }

        return true;
    }

    const bool validUsername(const std::string &username, std::string &result)
    {
        for (const auto &ch : username)
        {
            if (!((ch >= 'a' && ch <= 'z')
               || (ch >= 'A' && ch <= 'Z')
               || (ch >= '0' && ch <= '9')
               || ch == '_'))
            {
                result = "The username should only contain characters, numbers or '_'.";
                return false;
            }
        }

        return true;
    }

    void hash(std::string &password)
    {
        if (sodium_init() < 0)
        {
            Logger::get().error("Libsodium initialization failed.");
            throw std::runtime_error("Libsodium initialization failed.");
        }

        // Argon2id parameters
        constexpr size_t opslimit = crypto_pwhash_argon2id_OPSLIMIT_MODERATE;
        constexpr size_t memlimit = crypto_pwhash_argon2id_MEMLIMIT_MODERATE;

        std::vector<unsigned char> hash(crypto_pwhash_STRBYTES);
        if (crypto_pwhash_str(
                reinterpret_cast<char *>(hash.data()),
                password.c_str(),
                password.length(),
                opslimit,
                memlimit) != 0)
        {
            Logger::get().error("Hashing password failed.");
            throw std::runtime_error("Hashing password failed.");
        }

        // $argon2id$v=19$m=...,t=...,p=...$salt$hash
        password.assign(hash.begin(), hash.end());
    }

    const bool verifyHash(std::string &&password, const std::string &hash)
    {
        if (sodium_init() == -1)
        {
            Logger::get().error("Sodium initialize failed.");
            throw std::runtime_error("Sodium initialize failed.");
            return false;
        }
        auto result = crypto_pwhash_str_verify(
                   reinterpret_cast<const char *>(hash.data()),
                   password.c_str(),
                   password.length()) == 0;
        sodium_memzero(password.data(), password.size());
        return result;
    }
}
