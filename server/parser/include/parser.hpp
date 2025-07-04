/**
 * @file parser.hpp
 * @author username (username52247554@gmail.com)
 * @brief Parser
 * @version 0.1
 * @date 2025-06-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

/**
 * CREATE_TABLE_STMT = CREATE TABLE table_name
 *                     (column_definition (, column_definition)*
 *
 * SELECT_STMT = SELECT select_list FROM table_name
 *               [WHERE where_condition]
 *               [ORDER BY order_by_condition]
 *
 * INSERT_STMT = INSERT INTO table_name (column_list)
 *               VALUES (value_list)
 *
 * UPDATE_STMT = UPDATE table_name
 *               SET column_name = expression (, column_name = expression)*
 *               [WHERE where_condition]
 *
 * DELETE_STMT = DELETE FROM table_name
 *               WHERE where_condition
 * 
 * GRANT_STMT = GRANT authority ON table_name TO username | (, username)*
 * 
 * DROP_STMT = DROP TABLE table_name
 * 
 * authority = NULL | READONLY | MODIFY | ADMIN
 *
 * column_definition = column_name data_type [(, constraint_definition)*)]
 *
 * constraint_definition = PRIMARY KEY | UNIQUE | NOT NULL | DEFAULT expression
 *
 * select_list = * | column_name (, column_name)*
 *
 * where_condition = condition ( (AND | OR) condition )*
 *
 * condition = expression comparison_operator expression
 *           | expression (NOT)? IN ( value_list )  TODO:
 *           | expression (NOT)? BETWEEN expression AND expression
 *           | expression (NOT)? LIKE pattern
 *           | NOT condition
 *
 * comparison_operator = = | != | > | < | >= | <=
 *
 * order_by_list = expression (ASC | DESC)
 *
 * expression = literal_value
 *            | column_name
 *            | expression binary_operator expression
 *            | (expression)
 *
 * value_list = expression (, expression)*  TODO:
 *
 * column_list = column_name (, column_name)*
 *
 * value_list = expression (, expression)*
 */

#pragma once

#include <pch.hpp>
#include <engine/include/struct.hpp>
#include <affairs.hpp>
#include <authority/include/authority.hpp>

namespace HydroSQL::Server::Parser
{
    enum class TokenT : char
    {
        KEYWORD = 0,
        COLANDTABLE,
        BOOL_OPERATOR,
        CALCULATION_OPERATOR,
        LITERAL,
        AUTH_LEVEL,
        LBRACKET,
        RBRACKET,
        COMMA
    };

    enum class KeywordE : char
    {
        CREATE = 0,
        TABLE,
        INSERT,
        INTO,
        VALUES,
        SELECT,
        FROM,
        UPDATE,
        SET,
        DELETE_,
        WHERE,
        ORDER,
        BY,
        ASC,
        DESC,
        DROP,
        GRANT,
        ON,
        TO,

        INT,
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

        AND,
        OR,
        NOT,

        AUTH_NULL,
        READONLY,
        MODIFY,
        ADMIN,
    };

    const std::unordered_map<std::string, KeywordE> keywords = {
        {"CREATE", KeywordE::CREATE},
        {"TABLE", KeywordE::TABLE},
        {"INSERT", KeywordE::INSERT},
        {"INTO", KeywordE::INTO},
        {"VALUES", KeywordE::VALUES},
        {"SELECT", KeywordE::SELECT},
        {"FROM", KeywordE::FROM},
        {"UPDATE", KeywordE::UPDATE},
        {"SET", KeywordE::SET},
        {"DELETE", KeywordE::DELETE_},
        {"WHERE", KeywordE::WHERE},
        {"ORDER", KeywordE::ORDER},
        {"BY", KeywordE::BY},
        {"ASC", KeywordE::ASC},
        {"DESC", KeywordE::DESC},
        {"DROP", KeywordE::DROP},
        {"GRANT", KeywordE::GRANT},
        {"ON", KeywordE::ON},
        {"TO", KeywordE::TO},

        {"INT", KeywordE::INT},
        {"SMALLINT", KeywordE::SMALLINT},
        {"BIGINT", KeywordE::BIGINT},
        {"FLOAT", KeywordE::FLOAT},
        {"DECIMAL", KeywordE::DECIMAL},
        {"CHAR", KeywordE::CHAR},
        {"VARCHAR", KeywordE::VARCHAR},
        {"BOOLEAN", KeywordE::BOOLEAN},
        {"DATE", KeywordE::DATE},
        {"TIME", KeywordE::TIME},
        {"DATETIME", KeywordE::DATETIME},

        {"AND", KeywordE::AND},
        {"OR", KeywordE::OR},
        {"NOT", KeywordE::NOT},

        {"NULL", KeywordE::AUTH_NULL},
        {"READONLY", KeywordE::READONLY},
        {"MODIFY", KeywordE::MODIFY},
        {"ADMIN", KeywordE::ADMIN},
    };

    using BoolOp = HydroSQL::Server::Engine::LT::OpType;
    using CalOp = HydroSQL::Server::Engine::LT::CalType;
    using LiteralT = HydroSQL::Server::Engine::LT::LiterType;
    using ColTableName = std::string;
    using AuthLevel = HydroSQL::Server::Authority::AuthLevel;

    struct LiteralInfo
    {
        LiteralT type;
        HydroSQL::Server::Engine::Data data;
    };

    using TokenInfo = std::variant<BoolOp, CalOp, LiteralInfo, ColTableName, KeywordE, AuthLevel>;

    struct Token
    {
        TokenT type;
        TokenInfo info;
    };

    /**
     * @brief Generate a list of token from command;
     *
     */
    const std::list<Token> tokenize(const std::string &command);

    /**
     * @brief Parse the list of token and generate an affair.
     * 
     * @return An object with method execute. Calling that method will excute the command.
     */
    std::unique_ptr<Affair> parse(const std::list<Token> &token);

} // namespace HydroSQL::Server::Parser
