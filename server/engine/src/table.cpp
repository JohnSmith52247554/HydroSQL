/**
 * @file table.hpp
 * @author username (username52247554@gmail.com)
 * @brief defination of the basic structure of a table
 * @version 0.1
 * @date 2025-06-13
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <table.hpp>

namespace YourSQL::Server::Engine
{
    std::ostream &operator<<(std::ostream &os, const Constraint &constraint)
    {
        auto type = enumToBase<decltype(constraint.type)>(constraint.type);
        os.write(reinterpret_cast<const char *>(&type), sizeof(type));
        auto size = constraint.details.size();
        os.write(reinterpret_cast<const char *>(&size), sizeof(size));
        os.write(reinterpret_cast<const char *>(constraint.details.data()), sizeof(*constraint.details.data()) * size);
        return os;
    }

    std::istream &operator>>(std::istream &is, Constraint &constraint)
    {
        is.read(reinterpret_cast<char *>(&constraint.type), sizeof(constraint.type));
        size_t size;
        is.read(reinterpret_cast<char *>(&size), sizeof(size));
        constraint.details.resize(size);
        is.read(reinterpret_cast<char *>(constraint.details.data()), sizeof(*constraint.details.data()) * size);
        return is;
    }
}