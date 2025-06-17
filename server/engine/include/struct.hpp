#pragma once

namespace HydroSQL::Server::Engine
{
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
            CALCULATION
        };

        enum class OpType : char
        {
            null = 0,
            AND,
            OR,
            NOT,

            EQUAL,
            NOT_EQUAL,
            GREATER,
            LESS,
            GREATER_EQUAL,
            LESS_EQUAL,

            LIKE,
            IN
        };

        enum class CalType : char
        {
            null = 0,
            ADD,
            MINUS,
            MULTIPLY,
            DIVIDE,
            MODULO
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
            std::variant<bool, int64_t, double, std::string> liter_info;
        };

        union Info
        {
            OpType op_type;
            CalType cal_type;
            Literal liter;

            Info() {}
            ~Info() {}
        };

        struct LT
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
    } // namespace LT
    
} // namespace HydroSQL::Server::Engine
