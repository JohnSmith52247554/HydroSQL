/**
 * @file ThreadPool.hpp
 * @author username (username52247554@gmail.com)
 * @brief Thread pool.
 * @version 0.1
 * @date 2025-06-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <pch.hpp>

namespace HydroSQL::Server::Network
{
    template <typename T>
    class BlockingQueue
    {
    private:
        bool non_block;
        std::queue<T> queue;
        std::mutex mutex;
        std::condition_variable not_empty;

    public:
        BlockingQueue(bool non_block_ = false)
            : non_block(non_block_)
        {}
        ~BlockingQueue() = default;

        void push(const T &value)
        {
            std::unique_lock<std::mutex> lock(mutex);
            queue.push(value);
            not_empty.notify_one();
        }

        const bool pop(T &value)
        {
            std::unique_lock<std::mutex> lock(mutex);
            not_empty.wait(lock, [this]()
                           { return !queue.empty() || non_block; });
            if (queue.empty())
                return false;

            value = queue.front();
            queue.pop();
            return true;
        }

        void cancel()
        {
            std::lock_guard<std::mutex> lock(mutex);
            non_block = true;
            not_empty.notify_all();
        }
    };

    class ThreadPool
    {
    private:
        std::vector<std::thread> workers;
        BlockingQueue<std::function<void()>> tasks;
        std::atomic<size_t> working_counter;

    public:
        ThreadPool(const size_t thread_num);
        ~ThreadPool();

        template <typename Callable, typename... Args>
        void post(Callable &&callable, Args &&...args)
        {
            auto task = std::bind(std::forward<Callable>(callable), std::forward<Args>(args)...);
            tasks.push(task);
        }

        const bool busy() const
        {
            return working_counter == workers.size();
        }

    private:
        void work();
    };
}