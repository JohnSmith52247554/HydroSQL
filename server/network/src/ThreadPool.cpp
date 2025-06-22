/**
 * @file ThreadPool.cpp
 * @author username (username52247554@gmail.com)
 * @brief Basic thread pool
 * @version 0.1
 * @date 2025-06-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <ThreadPool.hpp>

namespace HydroSQL::Server::Network
{
    ThreadPool::ThreadPool(const size_t thread_num)
        : working_counter(0)
    {
        for (size_t i = 0u; i < thread_num; i++)
        {
            this->workers.emplace_back([this] -> void
                                        { work(); });
        }
    }

    ThreadPool::~ThreadPool()
    {
        this->tasks.cancel();
        for (auto &thread : workers)
        {
            if (thread.joinable())
                thread.join();
        }
    }

    void ThreadPool::work()
    {
        while (true)
        {
            std::function<void()> task;
            if (!tasks.pop(task))
                break;
            working_counter++;
            task();
            working_counter--;
        }
    }
}