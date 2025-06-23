/**
 * @file authority.hpp
 * @author username (username52247554@gmail.com)
 * @brief Authority manager.
 * @version 0.1
 * @date 2025-06-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <pch.hpp>

namespace HydroSQL::Server::Authority
{
    enum class AuthLevel : char
    {
        null,       // can not read or write
        READONLY,   // only can read
        MODIFY,     // read and write
        ADMIN       // be able to remove the database and manager the authority level of other user
    };

    class AuthManager
    {
    private:
        std::shared_mutex shared_mutex;

        AuthManager();
        ~AuthManager() = default;

    public:
        static AuthManager &get()
        {
            static AuthManager instance;
            return instance;
        }

        /**
         * The document UserConfig.bin is a two dimension sheet.
         *              username1       username2       username3       ...
         *              password_hash1  password_hash2  password_hash3  ...
         * table_nameA  auth_levelA1    auth_levelA2    auth_levelA3
         * table_nameB  auth_levelB1    auth_levelB2    auth_levelB3
         * table_nameC  auth_levelC1    auth_levelC2    auth_levelC3
         * ...
         *
         * When saving, the sheet will be save in a linear structure as following.
         * USERNAME_SIZE username1, username2, username3, ...
         * password_hash1, password_hash2, password_hash3, ...
         * TABLE_NAME_SIZE table_nameA, table_nameB, table_nameC, ...
         * auth_levelA1, auth_levelA2, auth_levelA3, ...
         * auth_levelB1, auth_levelB2, auth_levelB3, ...
         * auth_levelC1, auth_levelC2, auth_levelC3, ...
         * ...
         * 
         */

        [[deprecated]] const bool examinePasswordHash(const std::string &username, const std::string &password_hash);

        [[nodiscard]] const std::string getPasswordHash(const std::string &username);

        [[nodiscard]] const AuthLevel getLevel(const std::string &username, const std::string tablename);

        [[nodiscard]] const int addUser(const std::string &username, const std::string &password_hash);

        [[nodiscard]] const int addTable(const std::string &table_name, const std::string &creator);

        [[nodiscard]] const int removeTable(const std::string &table_name);

        // note: It necessary to examine whether the caller has administrator authority before calling this function.
        [[nodiscard]] const int setUserAuth(const std::string &username, const std::string &table_name, const AuthLevel level);

        [[nodiscard]] const int setUserAuth(const std::vector<std::string> &usernames, const std::string &table_name, const AuthLevel level);

    private:
        // @return the size of header.
        const std::streampos read(std::vector<std::string> &username, std::vector<std::string> &password_hash, std::vector<std::string> &table_name) const;

        void read(std::vector<std::string> &username, std::vector<std::string> &password_hash, std::vector<std::string> &table_name, std::vector<std::vector<AuthLevel>> &level) const;

        [[nodiscard]] const int write(const std::vector<std::string> &username, const std::vector<std::string> &password_hash, const std::vector<std::string> &table_name, const std::vector<std::vector<AuthLevel>> &level);

        static void saveStr(std::ostream &os, const std::string &str);
        static void loadStr(std::istream &is, std::string &str);

        static void saveVector(std::ostream &os, const std::vector<std::string> &vec);
        static void loadVector(std::istream &is, std::vector<std::string> &vec);

        static void saveVectorWithoutSize(std::ostream &os, const std::vector<std::string> &vec);
        static void loadVectorWithoutSize(std::istream &is, std::vector<std::string> &vec, const size_t &size);

        static void saveVecLevel(std::ostream &os, const std::vector<AuthLevel> &vec);
        static void loadVecLevel(std::istream &is, std::vector<AuthLevel> &vec, const size_t &size);
    };

    class Authoriser
    {
    private:
        std::string username;

    public:
        Authoriser(const std::string &un)
            : username(un)
        {}

        const bool authorise(const std::string &table_name, const AuthLevel level) const;

        inline const std::string &getUsername() const
        {
            return username;
        }
    };
};