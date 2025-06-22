/**
 * @file authority.cpp
 * @author username (username52247554@gmail.com)
 * @brief Authority manager.
 * @version 0.1
 * @date 2025-06-22
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <authority.hpp>

namespace HydroSQL::Server::Authority
{
    AuthManager::AuthManager()
    {
        if (!std::filesystem::exists(USER_CONFIG_PATH))
        {
            std::ofstream ofile(USER_CONFIG_PATH, std::ios::binary);
            if (!ofile.is_open())
                throw std::runtime_error("[ERROR] Unable to create user configuration file.");
            size_t size = 0;
            for (size_t i = 0; i < 2; i++)
                ofile.write(reinterpret_cast<const char *>(&size), sizeof(size));
            ofile.close();
        }
    }

    const bool AuthManager::examinePasswordHash(const std::string &username, const std::string &password_hash)
    {
        std::vector<std::string> username_vec;
        std::vector<std::string> password_hash_vec;
        std::vector<std::string> table_name_vec;

        {
            std::shared_lock lock(shared_mutex);
            auto header_size = read(username_vec, password_hash_vec, table_name_vec);
        }
        auto name = std::find(username_vec.begin(), username_vec.end(), username);
        if (name == username_vec.end())
            throw std::runtime_error("[ERROR] Username not found");
        
        return password_hash == password_hash_vec[name - username_vec.begin()];
    }

    const AuthLevel AuthManager::getLevel(const std::string &username, const std::string tablename)
    {
        std::vector<std::string> username_vec;
        std::vector<std::string> password_hash_vec;
        std::vector<std::string> table_name_vec;

        std::shared_lock lock(shared_mutex);
        auto header_size = read(username_vec, password_hash_vec, table_name_vec);

        auto col_num = username_vec.size();
        auto row_num = table_name_vec.size();

        auto user = std::find(username_vec.begin(), username_vec.end(), username);
        assert(user != username_vec.end());
        auto table = std::find(table_name_vec.begin(), table_name_vec.end(), tablename);
        assert(table != table_name_vec.end());

        std::streamoff coord_x = user - username_vec.begin();
        std::streamoff coord_y = table - table_name_vec.begin();

        std::ifstream ifile(USER_CONFIG_PATH, std::ios::binary);
        if (!ifile.is_open())
            throw std::runtime_error("[ERROR] Unable to open user configuration file.");

        // ifile.seekg(0, std::ios::end);
        // auto end = ifile.tellg();

        ifile.seekg(static_cast<std::streamoff>(header_size) + coord_y * static_cast<std::streamoff>(sizeof(AuthLevel)) * static_cast<std::streamoff>(col_num) + coord_x * static_cast<std::streamoff>(sizeof(AuthLevel)), std::ios::beg);

        // auto cur = ifile.tellg();

        AuthLevel level;
        ifile.read(reinterpret_cast<char *>(&level), sizeof(level));

        return level;
    }

    const int AuthManager::addUser(const std::string &username, const std::string &password_hash)
    {
        std::vector<std::string> username_vec;
        std::vector<std::string> password_hash_vec;
        std::vector<std::string> table_name_vec;
        std::vector<std::vector<AuthLevel>> level_vec;

        {
            std::shared_lock lock(shared_mutex);
            read(username_vec, password_hash_vec, table_name_vec, level_vec);
        }

        auto user = std::find(username_vec.begin(), username_vec.end(), username);
        if (user != username_vec.end())
            throw std::runtime_error("The username has been occupied. Please choose another.");

        username_vec.push_back(username);
        password_hash_vec.push_back(password_hash);

        for (auto &row : level_vec)
        {
            row.emplace_back(AuthLevel::null);
        }


        std::unique_lock u_lock(shared_mutex);

        if (!write(username_vec, password_hash_vec, table_name_vec, level_vec))
            throw std::runtime_error("[ERROR] Write user configuration failed.");

        return 1;
    }

    const int AuthManager::addTable(const std::string &table_name, const std::string &creator)
    {
        std::vector<std::string> username_vec;
        std::vector<std::string> password_hash_vec;
        std::vector<std::string> table_name_vec;
        std::vector<std::vector<AuthLevel>> level_vec;

        {
            std::shared_lock lock(shared_mutex);
            read(username_vec, password_hash_vec, table_name_vec, level_vec);
        }
        auto table_it = std::find(table_name_vec.begin(), table_name_vec.end(), table_name);
        if (table_it != table_name_vec.end())
            throw std::runtime_error("[FAILED] The table name has been occupied. Please choose another.");
        table_name_vec.push_back(table_name);
        auto user = std::find(username_vec.begin(), username_vec.end(), creator);
        assert(user != username_vec.end());
        std::vector<AuthLevel> new_row(username_vec.size(), AuthLevel::null);
        // The one who create the table will become administrator automatically.
        new_row[user - username_vec.begin()] = AuthLevel::ADMIN;
        level_vec.push_back(std::move(new_row));

        std::unique_lock lock(shared_mutex);
        if (!write(username_vec, password_hash_vec, table_name_vec, level_vec))
            throw std::runtime_error("[ERROR] Write user configuration failed.");

        return 1;
    }

    const int AuthManager::setUserAuth(const std::string &username, const std::string &table_name, const AuthLevel level)
    {
        std::vector<std::string> username_vec;
        std::vector<std::string> password_hash_vec;
        std::vector<std::string> table_name_vec;

        std::streampos header_size;
        {
            std::shared_lock lock(shared_mutex);
            header_size = read(username_vec, password_hash_vec, table_name_vec);
        }
        auto col_num = username_vec.size();
        auto row_num = table_name_vec.size();

        auto user = std::find(username_vec.begin(), username_vec.end(), username);
        if (user == username_vec.end())
        {
            throw std::runtime_error("[FAILED] User not found.");
        }
        auto table = std::find(table_name_vec.begin(), table_name_vec.end(), table_name);
        if (table == table_name_vec.end())
        {
            throw std::runtime_error("[FAILED] Table not found.");
        }

        std::streamoff coord_x = user - username_vec.begin();
        std::streamoff coord_y = table - table_name_vec.begin();

        std::unique_lock u_lock(shared_mutex);
        std::fstream file(USER_CONFIG_PATH, std::ios::binary | std::ios::out | std::ios::in);
        if (!file.is_open())
            throw std::runtime_error("[ERROR] Unable to open user configuration file.");

        file.seekg(static_cast<std::streamoff>(header_size) + coord_y * static_cast<std::streamoff>(sizeof(AuthLevel)) * static_cast<std::streamoff>(col_num) + coord_x * static_cast<std::streamoff>(sizeof(AuthLevel)), std::ios::beg);

        file.write(reinterpret_cast<const char *>(&level), sizeof(level));

        return 1;
    }

    void AuthManager::saveStr(std::ostream &os, const std::string &str)
    {
        const auto size = str.size();
        os.write(reinterpret_cast<const char *>(&size), sizeof(size));
        os.write(reinterpret_cast<const char *>(str.data()), sizeof(char) * size);
    }

    void AuthManager::loadStr(std::istream &is, std::string &str)
    {
        size_t size;
        is.read(reinterpret_cast<char *>(&size), sizeof(size));
        str.resize(size);
        is.read(reinterpret_cast<char *>(str.data()), sizeof(char) * size);
    }

    void AuthManager::saveVector(std::ostream &os, const std::vector<std::string> &vec)
    {
        const size_t size = vec.size();
        os.write(reinterpret_cast<const char *>(&size), sizeof(size));
        for (const auto &str : vec)
        {
            saveStr(os, str);
        }
    }

    void AuthManager::loadVector(std::istream &is, std::vector<std::string> &vec)
    {
        size_t size;
        is.read(reinterpret_cast<char *>(&size), sizeof(size));
        vec.resize(size);
        for (auto &str : vec)
        {
            loadStr(is, str);
        }
    }

    void AuthManager::saveVectorWithoutSize(std::ostream &os, const std::vector<std::string> &vec)
    {
        for (const auto &str : vec)
        {
            saveStr(os, str);
        }
    }

    void AuthManager::loadVectorWithoutSize(std::istream &is, std::vector<std::string> &vec, const size_t &size)
    {
        vec.resize(size);
        for (auto &str : vec)
        {
            loadStr(is, str);
        }
    }

    void AuthManager::saveVecLevel(std::ostream &os, const std::vector<AuthLevel> &vec)
    {
        os.write(reinterpret_cast<const char *>(vec.data()), sizeof(AuthLevel) * vec.size());
    }

    void AuthManager::loadVecLevel(std::istream &is, std::vector<AuthLevel> &vec, const size_t &size)
    {
        vec.resize(size);
        is.read(reinterpret_cast<char *>(vec.data()), sizeof(AuthLevel) * vec.size());
    }

    const std::streampos AuthManager::read(std::vector<std::string> &username, std::vector<std::string> &password_hash, std::vector<std::string> &table_name) const
    {
        std::ifstream ifile(USER_CONFIG_PATH, std::ios::binary);
        if (!ifile.is_open())
            throw std::runtime_error("[ERROR] Unable to open user configuration file.");

        loadVector(ifile, username);
        loadVectorWithoutSize(ifile, password_hash, username.size());
        loadVector(ifile, table_name);

        auto size = ifile.tellg();
        ifile.close();

        return size;
    }

    void AuthManager::read(std::vector<std::string> &username, std::vector<std::string> &password_hash, std::vector<std::string> &table_name, std::vector<std::vector<AuthLevel>> &level) const
    {
        std::ifstream ifile(USER_CONFIG_PATH, std::ios::binary);
        if (!ifile.is_open())
            throw std::runtime_error("[ERROR] Unable to open user configuration file.");

        loadVector(ifile, username);
        loadVectorWithoutSize(ifile, password_hash, username.size());
        loadVector(ifile, table_name);

        auto col_num = username.size();
        auto row_num = table_name.size();
        level.resize(row_num);
        for (auto &row : level)
        {
            loadVecLevel(ifile, row, col_num);
        }
    }

    const int AuthManager::write(const std::vector<std::string> &username, const std::vector<std::string> &password_hash, const std::vector<std::string> &table_name, const std::vector<std::vector<AuthLevel>> &level)
    {
        std::ofstream ofile(USER_CONFIG_PATH, std::ios::binary);
        if (!ofile.is_open())
            throw std::runtime_error("[ERROR] Unable to open user configuration file.");

        assert(username.size() == password_hash.size());
        assert(level.size() == table_name.size());

        saveVector(ofile, username);
        saveVectorWithoutSize(ofile, password_hash);
        saveVector(ofile, table_name);
        for (const auto &row : level)
        {
            saveVecLevel(ofile, row);
        }

        ofile.close();

        return 1;
    }
} // namespace HydroSQL::Server::Authority