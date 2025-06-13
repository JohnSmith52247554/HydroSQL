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

#pragma once

namespace YourSQL::Server::Engine
{
    /**
     * @brief convert a enum type into its base type
     * 
     * @tparam Enum the name of the enum type
     * @return std::underlying_type<Enum>::type 
     */
    template <typename Enum>
    typename std::underlying_type<Enum>::type enumToBase(Enum data)
    {
        return static_cast<std::underlying_type<Enum>::type>(data);
    }

    enum class DataType : char
    {
        INT = 0,
        SMALLINT,
        BIGINT,
        FLOAT,
        DOUBLE,
        DECIMAL,
        CHAR,
        BOOLEAN,
        DATE,
        TIME,
    };

    std::ostream &
    operator<<(std::ostream &os, const DataType &data_type);

    enum class ConstraintType : char
    {
        PRIMARY_KEY = 0,
        NOT_NULL,
        UNIQUE,
        CHECK,
        DEFAULT
    };

    struct Constraint
    {
        ConstraintType type;
        std::string details;

        Constraint() : type(ConstraintType::PRIMARY_KEY)
        {}
        Constraint(const ConstraintType &type_, const char* details_)
            : type(type_), details(details_)
        {}
        Constraint(const ConstraintType &type_, const std::string &details_)
            : type(type_), details(details_)
        {}

        friend std::ostream &operator<<(std::ostream &os, const Constraint &constraint);
        friend std::istream &operator>>(std::istream &is, Constraint &constraint);
    };

    std::ostream &operator<<(std::ostream &os, const Constraint &constraint);
    std::istream &operator>>(std::istream &is, Constraint &constraint);

    struct Column
    {
        std::string name;
        DataType data_type;
        unsigned int length;    // for CHAR
        unsigned int precision; // for DECIMAL
        unsigned int scale;     // for DECIMAL
        std::string defaultValue;
        std::vector<Constraint> constraints;

        Column(const std::string &name_, DataType type_, int len_ = 0)
            : name(name_), data_type(type_), length(len_), precision(0), scale(0)
        {}
    };

    class Table
    {
    private:
        std::string name;
        std::vector<Column> columns;
    };

}