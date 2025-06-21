/**
 * @file LogicalTree.hpp
 * @author username (username52247554@gmail.com)
 * @brief Tree structure to represent logical expression.
 * @version 0.1
 * @date 2025-06-17
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <struct.hpp>

namespace HydroSQL::Server::Engine::LT
{
    HYDROSQL_ENGINE_API [[nodiscard]] const int getOpParaNum(const OpType type);

    [[nodiscard]] const bool boolOp(std::shared_ptr<LT> root, RowInfo &info);
    [[nodiscard]] const int64_t calInt(std::shared_ptr<LT> root, RowInfo &info);
    [[nodiscard]] const double calFloat(std::shared_ptr<LT> root, RowInfo &info);
    [[nodiscard]] const std::string calStr(std::shared_ptr<LT> root, RowInfo &info);

    [[nodiscard]] const LiterType getLiterType(std::shared_ptr<LT> node, RowInfo &info);

    [[nodiscard]] const bool opEqual(std::shared_ptr<LT> root, RowInfo &info);
    [[nodiscard]] const bool opGreater(std::shared_ptr<LT> root, RowInfo &info);

    template <typename T,
              typename std::enable_if_t<std::is_same_v<T, int64_t> || std::is_same_v<T, double> || std::is_same_v<T, std::string> || std::is_same_v<T, bool>, int> = 0>
    [[nodiscard]] const int getCol(const RowInfo &info, const std::string &col_name, T &result)
    {
        for (const auto &col : info)
        {
            if (col.col_name == col_name)
            {
                using DataType = HydroSQL::Server::Engine::DataType;
                DataType type;
                if constexpr (std::is_same_v<T, int64_t>)
                {
                    if (col.col_type == DataType::INT
                     || col.col_type == DataType::SMALLINT
                     || col.col_type == DataType::BIGINT
                     || col.col_type == DataType::CHAR
                     || col.col_type == DataType::DATE
                     || col.col_type == DataType::TIME
                     || col.col_type == DataType::DATETIME)
                    {
                        result = static_cast<T>(std::get<int64_t>(col.liter.liter_info));
                    }
                    else
                    {
                        throw std::runtime_error("[ERROR] The type of column " + col_name + "doesn't meet the requirement of the expression.");
                    }
                }
                else if constexpr (std::is_same_v<T, double>)
                {
                    if (col.col_type == DataType::FLOAT || col.col_type == DataType::DECIMAL)
                    {
                        result = static_cast<T>(std::get<double>(col.liter.liter_info));
                    }
                    else
                    {
                        throw std::runtime_error("[ERROR] The type of column " + col_name + "doesn't meet the requirement of the expression.");
                    }
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    if (col.col_type == DataType::VARCHAR)
                    {
                        result = static_cast<T>(std::get<std::string>(col.liter.liter_info));
                    }
                    else
                    {
                        throw std::runtime_error("[ERROR] The type of column " + col_name + "doesn't meet the requirement of the expression.");
                    }
                }
                else if constexpr (std::is_same_v<T, bool>)
                {
                    if (col.col_type == DataType::BOOLEAN)
                    {
                        result = static_cast<T>(std::get<bool>(col.liter.liter_info));
                    }
                    else
                    {
                        throw std::runtime_error("[ERROR] The type of column " + col_name + "doesn't meet the requirement of the expression.");
                    }
                }
                return 1;
            }

            
            continue;
        }
        return 0;
    }

    

} // namespace YourSQL::E

