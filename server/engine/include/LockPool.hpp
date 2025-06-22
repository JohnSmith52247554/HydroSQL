/**
 * @file LockPool.hpp
 * @author username (username52247554@gmail.com)
 * @brief The class that offer mutex for each database when there is a write option
 * @version 0.1
 * @date 2025-06-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

namespace HydroSQL::Server::Engine
{
    class LockPool
    {
    private:
        static const size_t POOL_SIZE = 4;
        std::array<std::shared_mutex, POOL_SIZE> locks;
        std::hash<std::string> hash;

        LockPool() = default;
        ~LockPool() = default;

    public:
        static LockPool &get()
        {
            static LockPool instance;
            return instance;
        }

        std::shared_mutex &getLock(const std::string &file_name)
        {
            size_t index = hash(file_name) % POOL_SIZE;
            return locks[index];
        }
    };
}