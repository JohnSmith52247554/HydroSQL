/**
 * @file parser.hpp
 * @author username (username52247554@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-06-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

/**
 * CREATE_TABLE_STMT = CREATE TABLE table_name
 *                     (column_definition (, column_definition)*
 *
 * SELECT_STMT = SELECT select_list FROM table_name
 *               [WHERE where_condition]
 *               [ORDER BY order_by_condition]
 *
 * INSERT_STMT = INSERT INTO table_name (column_list)
 *               VALUE (value_list)
 *
 * UPDATE_STMT = UPDATE table_name
 *               SET column_name = expression (, column_name = literal)*  TODO:
 *               [WHERE where_condition]
 *
 * DELETE_STMT = DELETE FROM table_name
 *               WHERE where_condition
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

namespace HydroSQL::Server::Parser
{
    
} // namespace HydroSQL::Server::Parser
