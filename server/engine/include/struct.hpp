/**
 * @file struct.hpp
 * @author username (username52247554@gmail.com)
 * @brief structure & enum
 * @version 0.1
 * @date 2025-06-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#ifdef WIN32
#ifdef HYDROSQL_ENGINE_EXPORTS
#define HYDROSQL_ENGINE_API __declspec(dllexport)
#else
#define HYDROSQL_ENGINE_API __declspec(dllimport)
#endif
#else
#define HYDROSQL_ENGINE_API
#endif

namespace HydroSQL::Server::Engine
{
    using Data = std::variant<bool, int64_t, double, std::string>;

    enum class DataType : char
    {
        INT = 0,
        SMALLINT,
        BIGINT,
        FLOAT,
        DECIMAL,
        CHAR,
        VARCHAR,
        BOOLEAN,
        DATE,
        TIME,
        DATETIME,
    };
    

    namespace LT
    {
        enum class NodeType : char
        {
            null = 0,
            COL,
            LITERAL,
            OPERATOR,
            CALCULATION,
            LBRACKET,
            RBRACKET
        };

        enum class OpType : char
        {
            null = 0,

            EQUAL,
            NOT_EQUAL,
            GREATER,
            LESS,
            GREATER_EQUAL,
            LESS_EQUAL,

            LIKE,
            IS_IN,

            AND,
            OR,
            NOT
        };

        enum class CalType : char
        {
            null = 0,
            MULTIPLY,
            DIVIDE,
            MODULO,
            ADD,
            MINUS,
        };

        enum class LiterType : char
        {
            null = 0,
            INT,
            FLOAT,
            BOOLEAN,
            STR,
            DATE,
            TIME,
            DATETIME
        };

        struct Literal
        {
            LiterType liter_type;
            Data liter_info;
        };

        union Info
        {
            OpType op_type;
            CalType cal_type;
            Literal liter;

            Info() {}
            ~Info() {}
        };

        /**
         * @brief Logical tree node. After converting an expression into a logical tree, the expression can be calculated recursively from the root node.
         * 
         */
        struct HYDROSQL_ENGINE_API LT
        {
            NodeType type;
            Info info;
            std::vector<std::shared_ptr<LT>> children;

            LT(NodeType type_ = NodeType::null)
                : type(type_)
            {
                info.op_type = OpType::null;
            }

            ~LT() {}
        };

        struct ColInfo
        {
            std::string col_name;
            HydroSQL::Server::Engine::DataType col_type;
            Literal liter;
        };

        using RowInfo = std::vector<ColInfo>;

        [[nodiscard]] const LiterType dataTypeToLiteralType(const DataType type);

    } // namespace LT
    
} // namespace HydroSQL::Server::Engine
