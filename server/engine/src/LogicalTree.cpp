/**
 * @file LogicalTree.cpp
 * @author username (username52247554@gmail.com)
 * @brief Tree structure to represent logical expression.
 * @version 0.1
 * @date 2025-06-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <LogicalTree.hpp>

namespace HydroSQL::Server::Engine::LT
{
    const LiterType dataTypeToLiteralType(const DataType type)
    {
        switch (type)
        {
        case DataType::INT:
            [[fallthrough]];
        case DataType::SMALLINT:
            [[fallthrough]];
        case DataType::BIGINT:
            [[fallthrough]];
        case DataType::CHAR:
            return LiterType::INT;
        case DataType::BOOLEAN:
            return LiterType::BOOLEAN;
        case DataType::FLOAT:
            [[fallthrough]];
        case DataType::DECIMAL:
            return LiterType::FLOAT;
        case DataType::VARCHAR:
            return LiterType::STR;
        case DataType::DATE:
            return LiterType::DATE;
        case DataType::TIME:
            return LiterType::TIME;
        case DataType::DATETIME:
            return LiterType::DATETIME;
        default:
            return LiterType::null;
        }
    }
    
    const int getOpParaNum(const OpType type)
    {
        switch (type)
        {
        case OpType::NOT:
            [[fallthrough]];
        case OpType::LIKE:
            return 1;
        case OpType::AND:
            [[fallthrough]];
        case OpType::OR:
            [[fallthrough]];
        case OpType::EQUAL:
            [[fallthrough]];
        case OpType::GREATER:
            [[fallthrough]];
        case OpType::GREATER_EQUAL:
            [[fallthrough]];
        case OpType::LESS:
            [[fallthrough]];
        case OpType::LESS_EQUAL:
            return 2;
        case OpType::IN:
            return -1;
        default:
            return -1;
        }
    }

    const bool boolOp(std::shared_ptr<LT> root, RowInfo &info)
    {
        assert(root->type == NodeType::OPERATOR 
            || (root->type == NodeType::LITERAL && root->info.liter.liter_type == LiterType::BOOLEAN)
            || root->type == NodeType::COL);
        
        if (root->type == NodeType::OPERATOR)
        {
            switch (root->info.op_type)
            {
            case OpType::NOT:
                assert(root->children.size() == getOpParaNum(OpType::NOT));
                assert(root->children[0]->type != NodeType::CALCULATION
                    && root->children[1]->type != NodeType::CALCULATION);
                return !boolOp(root->children[0], info);
            case OpType::AND:
                assert(root->children.size() == getOpParaNum(OpType::AND));
                assert(root->children[0]->type != NodeType::CALCULATION
                    && root->children[1]->type != NodeType::CALCULATION);
                return boolOp(root->children[0], info) && boolOp(root->children[1], info);
            case OpType::OR:
                assert(root->children.size() == getOpParaNum(OpType::OR));
                assert(root->children[0]->type != NodeType::CALCULATION
                    && root->children[1]->type != NodeType::CALCULATION);
                return boolOp(root->children[0], info) || boolOp(root->children[1], info);
            case OpType::EQUAL:
                return opEqual(root, info);
            case OpType::NOT_EQUAL:
                return !opEqual(root, info);
            case OpType::GREATER:
                return opGreater(root, info);
            case OpType::GREATER_EQUAL:
                return opGreater(root, info) || opEqual(root, info);
            case OpType::LESS:
                return !(opGreater(root, info) || opEqual(root, info));
            case OpType::LESS_EQUAL:
                return !(opGreater(root, info));
            case OpType::LIKE:
            case OpType::IN:
                // TODO:
            default:
                break;
            }
        }
        else if (root->type == NodeType::LITERAL)
        {
            assert(root->info.liter.liter_type == LiterType::BOOLEAN);
            return std::get<bool>(root->info.liter.liter_info);
        }
        else if (root->type == NodeType::COL)
        {
            assert(root->info.liter.liter_type == LiterType::BOOLEAN);
            bool result;
            if(!getCol<bool>(info, std::get<std::string>(root->info.liter.liter_info), result))
            {
                return 0;
            }
            return result;
        }

        return false;
    }

    const int64_t calInt(std::shared_ptr<LT> root, RowInfo &info)
    {
        assert(getLiterType(root, info) == LiterType::INT);
        assert(root->type != NodeType::OPERATOR);

        if (root->type == NodeType::LITERAL)
        {
            assert(root->info.liter.liter_type == LiterType::INT);
            return std::get<int64_t>(root->info.liter.liter_info);
        }
        else if (root->type == NodeType::CALCULATION)
        {
            switch (root->info.cal_type)
            {
            case CalType::ADD:
                assert(root->children.size() == 2);
                return calInt(root->children[0], info) + calInt(root->children[1], info);
            case CalType::MINUS:
                assert(root->children.size() == 2);
                return calInt(root->children[0], info) - calInt(root->children[1], info);
            case CalType::MULTIPLY:
                assert(root->children.size() == 2);
                return calInt(root->children[0], info) * calInt(root->children[1], info);
            case CalType::DIVIDE:
                assert(root->children.size() == 2);
                return calInt(root->children[0], info) * calInt(root->children[1], info);
            case CalType::MODULO:
                assert(root->children.size() == 2);
                assert(getLiterType(root->children[0], info) == LiterType::INT 
                    && getLiterType(root->children[1], info) == LiterType::INT);
                return calInt(root->children[0], info) % calInt(root->children[1], info);
            default:
                break;
            }
        }
        else if (root->type == NodeType::COL)
        {
            assert(root->info.liter.liter_type == LiterType::STR);
            int64_t result;
            if (!getCol<int64_t>(info, std::get<std::string>(root->info.liter.liter_info), result))
            {
                return 0;
            }
            return result;
        }

        return 0;
    }

    const double calFloat(std::shared_ptr<LT> root, RowInfo &info)
    {
        assert(getLiterType(root, info) == LiterType::FLOAT
            || getLiterType(root, info) == LiterType::INT);
        assert(root->type != NodeType::OPERATOR);

        if (root->type == NodeType::LITERAL)
        {
            assert(root->info.liter.liter_type == LiterType::FLOAT
                || root->info.liter.liter_type == LiterType::INT);
            if (root->info.liter.liter_type == LiterType::FLOAT)
                return std::get<double>(root->info.liter.liter_info);
            else
                return static_cast<double>(std::get<int64_t>(root->info.liter.liter_info));
        }
        else if (root->type == NodeType::CALCULATION)
        {
            switch (root->info.cal_type)
            {
            case CalType::ADD:
                assert(root->children.size() == 2);
                return calFloat(root->children[0], info) + calFloat(root->children[1], info);
            case CalType::MINUS:
                assert(root->children.size() == 2);
                return calFloat(root->children[0], info) - calFloat(root->children[1], info);
            case CalType::MULTIPLY:
                assert(root->children.size() == 2);
                return calFloat(root->children[0], info) * calFloat(root->children[1], info);
            case CalType::DIVIDE:
                assert(root->children.size() == 2);
                return calFloat(root->children[0], info) * calFloat(root->children[1], info);
            default:
                break;
            }
        }
        else if (root->type == NodeType::COL)
        {
            // TODO:
            assert(root->info.liter.liter_type == LiterType::FLOAT);
            double result;
            if(!getCol<double>(info, std::get<std::string>(root->info.liter.liter_info), result))
            {
                return 0.0;
            }
            return result;
        }

        return 0.0;
    }

    const std::string calStr(std::shared_ptr<LT> root, RowInfo &info)
    {
        assert(getLiterType(root, info) == LiterType::STR);
        assert((root->type == NodeType::LITERAL && root->info.liter.liter_type == LiterType::STR)
            || root->type == NodeType::COL);

        if (root->type == NodeType::LITERAL)
        {
            return std::get<std::string>(root->info.liter.liter_info);
        }
        else if (root->type == NodeType::COL)
        {
            assert(root->info.liter.liter_type == LiterType::STR);
            std::string result;
            if(!getCol<std::string>(info, std::get<std::string>(root->info.liter.liter_info), result))
            {
                return "";
            }
            return result;
        }

        return "";
    }

    const LiterType getLiterType(std::shared_ptr<LT> node, RowInfo &info)
    {
        switch (node->type)
        {
        case NodeType::OPERATOR:
            return LiterType::BOOLEAN;
        case NodeType::CALCULATION:
            {
                assert(node->children.size() == 2);
                assert(getLiterType(node->children[0], info) != LiterType::STR
                    && getLiterType(node->children[1], info) != LiterType::STR);
                if (getLiterType(node->children[0], info) == LiterType::FLOAT 
                    || getLiterType(node->children[1], info) == LiterType::FLOAT)
                    return LiterType::FLOAT;
                else
                    return LiterType::INT;
            }
        case NodeType::LITERAL:
            assert(node->children.size() == 0);
            return node->info.liter.liter_type;
        case NodeType::COL:
            {
                auto col = std::find_if(info.begin(), info.end(), [&](ColInfo &c)
                                        { return c.col_name == std::get<std::string>(node->info.liter.liter_info); });
                if (col == info.end())
                {
                    return LiterType::null;
                }
                else
                {
                    return col->liter.liter_type;
                }
            }
        default:
            return LiterType::null;
        }
    }

    

    const bool opEqual(std::shared_ptr<LT> root, RowInfo &info)
    {
        assert(root->children.size() == getOpParaNum(OpType::EQUAL));
        assert(root->children[0]->type == NodeType::LITERAL
            || root->children[0]->type == NodeType::CALCULATION
            || root->children[0]->type == NodeType::COL);
        assert(root->children[1]->type == NodeType::LITERAL
            || root->children[1]->type == NodeType::CALCULATION
            || root->children[1]->type == NodeType::COL);
        const auto type0 = getLiterType(root->children[0], info);
        const auto type1 = getLiterType(root->children[1], info);
        assert(!((type0 == LiterType::STR && type1 != LiterType::STR) 
                || (type0 != LiterType::STR && type1 == LiterType::STR)));
        if (type0 == LiterType::STR && type1 == LiterType::STR)
        {
            return calStr(root->children[0], info) == calStr(root->children[1], info);
        }
        else if (type0 == LiterType::FLOAT || type1 == LiterType::FLOAT)
        {
            double num0 = type0 == LiterType::FLOAT ? calFloat(root->children[0], info) : static_cast<double>(calInt(root->children[0], info));
            double num1 = type1 == LiterType::FLOAT ? calFloat(root->children[1], info) : static_cast<double>(calInt(root->children[1], info));
            return num0 == num1;
        }
        else
        {
            return calInt(root->children[0], info) == calInt(root->children[1], info);
        }

        return false;
    }

    const bool opGreater(std::shared_ptr<LT> root, RowInfo &info)
    {
        assert(root->children.size() == getOpParaNum(OpType::EQUAL));
        assert(root->children[0]->type == NodeType::LITERAL
            || root->children[0]->type == NodeType::CALCULATION
            || root->children[0]->type == NodeType::COL);
        assert(root->children[1]->type == NodeType::LITERAL
            || root->children[1]->type == NodeType::CALCULATION
            || root->children[1]->type == NodeType::COL);
        const auto type0 = getLiterType(root->children[0], info);
        const auto type1 = getLiterType(root->children[1], info);
        assert(!((type0 == LiterType::STR && type1 != LiterType::STR) 
                || (type0 != LiterType::STR && type1 == LiterType::STR)));
        if (type0 == LiterType::STR && type1 == LiterType::STR)
        {
            return calStr(root->children[0], info) > calStr(root->children[1], info);
        }
        else if (type0 == LiterType::FLOAT || type1 == LiterType::FLOAT)
        {
            double num0 = type0 == LiterType::FLOAT ? calFloat(root->children[0], info) : static_cast<double>(calInt(root->children[0], info));
            double num1 = type1 == LiterType::FLOAT ? calFloat(root->children[1], info) : static_cast<double>(calInt(root->children[1], info));
            return num0 > num1;
        }
        else
        {
            return calInt(root->children[0], info) > calInt(root->children[1], info);
        }

        return false;
    }
} // namespace HydroSQL::Server::Engine::LT
