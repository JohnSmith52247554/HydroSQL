/**
 * @file stack.hpp
 * @author username (username52247554@gmail.com)
 * @brief Stack using link list
 * @version 0.1
 * @date 2025-06-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <list>

namespace HydroSQL::Utils::DataStructure
{
    template <typename T>
    class Stack
    {
    private:
        std::list<T> list;
        
    public:
        Stack() = default;
        ~Stack() = default;

        void push(T data)
        {
            list.push_back(data);
        }


        T pop()
        {
            if (list.empty())
                throw std::runtime_error("[ERROR] Try to pop an empty stack.");
            T top = list.back();
            list.pop_back();
            return top;
        }

        const T &top() const
        {
            if (list.empty())
                throw std::runtime_error("[ERROR] Try to pop an empty stack.");
            return list.back();
        }

        inline const bool empty() const
        {
            return list.empty();
        }

        inline const size_t size() const
        {
            return list.size();
        }
    };
}