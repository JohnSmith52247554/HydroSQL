/**
 * @file parser.cpp
 * @author username (username52247554@gmail.com)
 * @brief parser
 * @version 0.1
 * @date 2025-06-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <parser.hpp>
#include <engine/include/LogicalTree.hpp>
#include <utils/DataStructure/include/stack.hpp>

namespace HydroSQL::Server::Parser
{
    const char escapeCharacter(std::string::const_iterator &it, const std::string::const_iterator end)
    {
        char ch;
        if (it != end)
        {
            it++;
            switch (*it)
            {
            case 'a':
                ch = '\a';
                break;
            case 'b':
                ch = '\b';
                break;
            case 'f':
                ch = '\f';
                break;
            case 'n':
                ch = '\n';
                break;
            case 'r':
                ch = '\r';
                break;
            case 't':
                ch = '\t';
                break;
            case 'v':
                ch = '\v';
                break;
            case '\\':
                ch = '\\';
                break;
            case '?':
                ch = '\?';
                break;
            case '\'':
                ch = '\'';
                break;
            case '"':
                ch = '\"';
                break;
            case '0':
                ch = '\0';
                break;
            default:
                throw std::runtime_error("Invalid escape character.");
                break;
            }
        }
        return ch;
    }

    const bool continuousNumExamination(const std::string &str, const size_t begin, const size_t length)
    {
        if (begin + length > str.size())
            return false;
        for (size_t i = begin; i < begin + length; i++)
        {
            if (str[i] < '0' || str[i] > '9')
                return false;
        }
        return true;
    }

    const bool dateValideExamine(const std::string &str)
    {
        if (str.size() != 10)
            return false;
        if (!continuousNumExamination(str, 0, 4))
            return false;
        if (str[4] != '-')
            return false;
        if (!continuousNumExamination(str, 5, 2))
            return false;
        if (str[7] != '-')
            return false;
        if (!continuousNumExamination(str, 8, 2))
            return false;
        return true;
    }

    const bool timeValideExamine(const std::string &str)
    {
        if (str.size() != 8)
            return false;
        if (!continuousNumExamination(str, 0, 2))
            return false;
        if (str[2] != ':')
            return false;
        if (!continuousNumExamination(str, 3, 2))
            return false;
        if (str[5] != ':')
            return false;
        if (!continuousNumExamination(str, 6, 2))
            return false;
        return true;
    }

    const bool datetimeValideExamine(const std::string &str)
    {
        if (str.size() != 19)
            return false;
        if (!continuousNumExamination(str, 0, 4))
            return false;
        if (str[4] != '-')
            return false;
        if (!continuousNumExamination(str, 5, 2))
            return false;
        if (str[7] != '-')
            return false;
        if (!continuousNumExamination(str, 8, 2))
            return false;
        if (str[10] != '-')
            return false;
        if (!continuousNumExamination(str, 11, 2))
            return false;
        if (str[13] != ':')
            return false;
        if (!continuousNumExamination(str, 14, 2))
            return false;
        if (str[16] != ':')
            return false;
        if (!continuousNumExamination(str, 17, 2))
            return false;
        return true;
    }

    const int64_t dateStrToNum(const std::string &str)
    {
        int64_t buffer = 0;
        int64_t num = 0;
        buffer = std::stoi(str.substr(8, 2));
        num += buffer * 1e0;
        buffer = std::stoi(str.substr(5, 2));
        num += buffer * 1e2;
        buffer = std::stoi(str.substr(0, 4));
        num += buffer * 1e4;
        return num;
    }

    const int64_t timeStrToNum(const std::string &str)
    {
        int64_t buffer = 0;
        int64_t num = 0;
        buffer = std::stoi(str.substr(6, 2));
        num += buffer * 1e0;
        buffer = std::stoi(str.substr(3, 2));
        num += buffer * 1e2;
        buffer = std::stoi(str.substr(0, 2));
        num += buffer * 1e4;
        return num;
    }

    const int64_t datetimeStrToNum(const std::string &str)
    {
        int64_t buffer = 0;
        int64_t num = 0;
        buffer = std::stoi(str.substr(17, 2));
        num += buffer * 1e0;
        buffer = std::stoi(str.substr(14, 2));
        num += buffer * 1e2;
        buffer = std::stoi(str.substr(11, 2));
        num += buffer * 1e4;
        buffer = std::stoi(str.substr(8, 2));
        num += buffer * 1e6;
        buffer = std::stoi(str.substr(5, 2));
        num += buffer * 1e8;
        buffer = std::stoi(str.substr(0, 4));
        num += buffer * 1e10;
        return num;
    }

    const std::list<Token> tokenize(const std::string &command)
    {
        std::list<Token> token_list;
        for (auto it = command.begin(); it != command.end(); it++)
        {
            // white space
            if (*it == ' ' || *it == '\t' || *it == '\n' || *it == ';')
                continue;
            // single letter
            else if (*it == ',')
            {
                token_list.emplace_back(TokenT::COMMA);
            }
            // bool operator
            else if (*it == '=')
            {
                token_list.emplace_back(TokenT::BOOL_OPERATOR, BoolOp::EQUAL);
            }
            else if (*it == '>')
            {
                if (it != command.end() && *(it + 1) == '=')
                {
                    token_list.emplace_back(TokenT::BOOL_OPERATOR, BoolOp::GREATER_EQUAL);
                    it += 1;
                }
                else
                    token_list.emplace_back(TokenT::BOOL_OPERATOR, BoolOp::GREATER);
            }
            else if (*it == '<')
            {
                if (it != command.end() && *(it + 1) == '=')
                {
                    token_list.emplace_back(TokenT::BOOL_OPERATOR, BoolOp::LESS_EQUAL);
                    it += 1;
                }
                else
                    token_list.emplace_back(TokenT::BOOL_OPERATOR, BoolOp::LESS);
            }
            else if (*it == '!' && it != command.end() && *(it + 1) == '=')
            {
                token_list.emplace_back(TokenT::BOOL_OPERATOR, BoolOp::NOT_EQUAL);
            }
            // calculation operator
            else if (*it == '+')
            {
                token_list.emplace_back(TokenT::CALCULATION_OPERATOR, CalOp::ADD);
            }
            else if (*it == '-')
            {
                if (it != command.end() && *(it + 1) >= '0' && *(it + 1) < '9')
                {
                    // minus number
                    auto start_it = it;
                    bool exist_dot = false;
                    while (true)
                    {
                        it++;
                        if (it == command.end())
                        {
                            it--;
                            break;
                        }
                        if (!((*it >= '0' && *it <= '9') || *it == '.' || *it == ':' || *it == '-'))
                        {
                            it--;
                            break;
                        }
                        if (*it == '.')
                            exist_dot = true;
                    }
                    std::string str(start_it, it);
                    if (!exist_dot)
                    {
                        token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::INT, std::stoll(str)});
                    }
                    else
                    {
                        token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::FLOAT, std::stod(str)});
                    }
                }
                else
                    token_list.emplace_back(TokenT::CALCULATION_OPERATOR, CalOp::MINUS);
            }
            else if (*it == '*')
            {
                token_list.emplace_back(TokenT::CALCULATION_OPERATOR, CalOp::MULTIPLY);
            }
            else if (*it == '/')
            {
                token_list.emplace_back(TokenT::CALCULATION_OPERATOR, CalOp::DIVIDE);
            }
            else if (*it == '%')
            {
                token_list.emplace_back(TokenT::CALCULATION_OPERATOR, CalOp::MODULO);
            }
            // bracket
            else if (*it == '(')
            {
                token_list.emplace_back(TokenT::LBRACKET);
            }
            else if (*it == ')')
            {
                token_list.emplace_back(TokenT::RBRACKET);
            }
            // literal
            // char
            else if (*it == '\'')
            {
                char ch;
                if (it + 1 != command.end())
                {
                    it++;

                    // escape character
                    if (*it == '\\')
                    {
                        ch = escapeCharacter(it, command.end());
                    }
                    else
                    {
                        ch = *it;
                    }
                }
                short len = 0;
                do
                {
                    len++;
                    if (it == command.end())
                    {
                        throw std::runtime_error("[FAILED] Unclosed singel quote mark.");
                    }
                    if (len > 1)
                    {
                        throw std::runtime_error("[FAILED] Invalid character.");
                    }
                    it++;
                } while (*it != '\'');
                token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::INT, ch});
            }
            // string
            else if (*it == '"')
            {
                std::stringstream buffer;
                while (true)
                {
                    if (it == command.end())
                    {
                        throw std::runtime_error("[FAILED] Unclosed double quote mark.");
                        break;
                    }
                    it++;
                    if (*it == '"')
                        break;
                    else if (*it == '\\')
                    {
                        // escape character
                        buffer << escapeCharacter(it, command.end());
                    }
                    else
                    {
                        buffer << *it;
                    }
                }
                token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::STR, buffer.str()});
            }
            // number & date & time & datetime
            else if (*it >= '0' && *it <= '9')
            {
                auto start_it = it;
                bool exist_colon = false;
                bool exist_dash = false;
                bool exist_dot = false;
                while (true)
                {
                    it++;
                    if (it == command.end())
                    {
                        it--;
                        break;
                    }
                    if (!((*it >= '0' && *it <= '9') || *it == '.' || *it == ':' || *it == '-'))
                    {
                        it--;
                        break;
                    }
                    if (*it == ':')
                        exist_colon = true;
                    else if (*it == '-')
                        exist_dash = true;
                    else if (*it == '.')
                        exist_dot = true;
                }

                std::string str(start_it, it + 1);
                if (exist_dash && exist_colon && !exist_dot)
                {
                    // date time
                    if (!datetimeValideExamine(str))
                    {
                        throw std::runtime_error("[FAILED] Invalid datetime.");
                    }
                    token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::DATETIME, datetimeStrToNum(str)});
                }
                else if (exist_colon && !exist_dot && !exist_dash)
                {
                    // time
                    if (!timeValideExamine(str))
                    {
                        throw std::runtime_error("[FAILED] Invalid time.");
                    }
                    token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::TIME, timeStrToNum(str)});
                }
                else if (exist_dash && !exist_dot && !exist_colon)
                {
                    // date
                    if (!dateValideExamine(str))
                    {
                        throw std::runtime_error("[FAILED] Invalid date.");
                    }
                    token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::DATE, dateStrToNum(str)});
                }
                else if (exist_dot && !exist_dash && !exist_colon)
                {
                    // float
                    token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::FLOAT, std::stod(str)});
                }
                else if (!exist_colon && !exist_dot && !exist_dash)
                {
                    // int
                    token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::INT, std::stoll(str)});
                }
                else
                {
                    // invalid
                    throw std::runtime_error("[FAILED] Invalid number");
                }

            }
            // keyword & AND OR NOT & column name & table name
            else if ((*it >= 'a' && *it <= 'z') || (*it >= 'A' && *it <= 'Z'))
            {
                auto start_it = it;
                while (true)
                {
                    if (it + 1 == command.end())
                        break;
                    it++;
                    if (!(((*it >= 'a' && *it <= 'z') || (*it >= 'A' && *it <= 'Z') || (*it >= '0' && *it <= '9') || (*it == '_'))))
                    {
                        it--;
                        break;
                    }
                }

                std::string str(start_it, it + 1);

                auto kw = keywords.find(str);

                if (kw != keywords.end())
                {
                    // keyword
                    if (static_cast<char>(kw->second) < static_cast<char>(KeywordE::AND))
                    {
                        token_list.emplace_back(TokenT::KEYWORD, kw->second);

                        if (kw->second == KeywordE::VARCHAR && it + 2 != command.end())
                        {
                            // get length
                            it += 2;
                            auto len_start = it;
                            while (true)
                            {
                                if (it == command.end())
                                    break;
                                it++;
                                if (*it == ')')
                                    break;
                                if (!(*it >= '0' && *it <= '9'))
                                {
                                    throw std::runtime_error("[FAILED] Invalid number.");
                                }
                            }
                            std::string num(len_start, it);
                            token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::INT, stoll(num)});
                        }
                    }
                    else if (static_cast<char>(kw->second) < static_cast<char>(KeywordE::AUTH_NULL))
                    {
                        // AND OR NOT
                        token_list.emplace_back(TokenT::BOOL_OPERATOR, static_cast<BoolOp>(static_cast<char>(kw->second) - static_cast<char>(KeywordE::AND) + static_cast<char>(BoolOp::AND)));
                    }
                    else
                    {
                        // Authority
                        token_list.emplace_back(TokenT::AUTH_LEVEL, static_cast<AuthLevel>(static_cast<char>(kw->second) - static_cast<char>(KeywordE::AUTH_NULL) + static_cast<char>(AuthLevel::null)));
                    }
                }
                else
                {
                    // column name / table name
                    token_list.emplace_back(TokenT::COLANDTABLE, ColTableName(str));
                }
            }

            if (it == command.end())
                break;
        }

        return token_list;
    }

    using Stack = Utils::DataStructure::Stack<std::shared_ptr<Engine::LT::LT>>;

    void opStackPopOnce(Stack &li_stack, Stack &op_stack)
    {
        auto orig_top = op_stack.pop();

        int liStackPopTime = 0;
        if (orig_top->type == Engine::LT::NodeType::CALCULATION)
            liStackPopTime = 2;
        else if (orig_top->type == Engine::LT::NodeType::OPERATOR)
            liStackPopTime = Engine::LT::getOpParaNum(orig_top->info.op_type);

        orig_top->children.resize(liStackPopTime);

        for (; liStackPopTime > 0; liStackPopTime--)
        {
            auto child = li_stack.pop();
            orig_top->children[liStackPopTime - 1] = child;
        }

        li_stack.push(orig_top);
    }

    void opStackPop(std::shared_ptr<Engine::LT::LT> &op, Stack &li_stack, Stack &op_stack)
    {
        /**
         * Firstly, op_stack pop.
         * Secondly, li_stack pop. The time of popping is resolved by the type of the original top of op_stack.
         * Thirdly, what li_stack has popped become the children of the original top of op_stack.
         * Fourthly, the original top of op_stack, with its children, will be push into the li_stack.
         * Lastly, op will be push into op_stack if op isn't right bracket.
         *
         * If op is not right bracket, the aforesaid steps will be applied only once.
         * If it is, the steps will be applied repeatedly until one left bracket is pop.
         */

        if (op->type != Engine::LT::NodeType::RBRACKET)
        {
            opStackPopOnce(li_stack, op_stack);
            op_stack.push(op);
        }
        else
        {
            while (op_stack.top()->type != Engine::LT::NodeType::LBRACKET)
            {
                opStackPopOnce(li_stack, op_stack);
            }
            op_stack.pop();
        }
    }

    void stackOp(std::shared_ptr<Engine::LT::LT> &op, Stack &li_stack, Stack &op_stack)
    {
        /**
         * Left bracket always push, while right bracket always pop.
         * Otherwise, only when the stack_top has a higher or equal priority than the op
         * will op_stack pop.
         */

        if (op_stack.empty())
        {
            op_stack.push(op);
            return;
        }

        auto stack_top = op_stack.top();
        switch (op->type)
        {
        case Engine::LT::NodeType::LBRACKET:
            op_stack.push(op);
            return;
        case Engine::LT::NodeType::RBRACKET:
            opStackPop(op, li_stack, op_stack);
            return;
        case Engine::LT::NodeType::CALCULATION:
        {
            switch (stack_top->type)
            {
            case Engine::LT::NodeType::OPERATOR:
                op_stack.push(op);
                return;
            case Engine::LT::NodeType::CALCULATION:
                if (static_cast<char>(op->info.cal_type) >= static_cast<char>(stack_top->info.cal_type))
                {
                    opStackPop(op, li_stack, op_stack);
                }
                else
                {
                    op_stack.push(op);
                }
                return;
            case Engine::LT::NodeType::LBRACKET:
                op_stack.push(op);
                return;
            default:
                throw std::runtime_error("[ERROR] Parsing expression failed.");
            }
        }
            return;
        case Engine::LT::NodeType::OPERATOR:
        {
            switch (stack_top->type)
            {
            case Engine::LT::NodeType::OPERATOR:
                if (static_cast<char>(op->info.op_type) >= static_cast<char>(stack_top->info.op_type))
                {
                    opStackPop(op, li_stack, op_stack);
                }
                else
                {
                    op_stack.push(op);
                }
                return;
            case Engine::LT::NodeType::CALCULATION:
                opStackPop(op, li_stack, op_stack);
                return;
            case Engine::LT::NodeType::LBRACKET:
                op_stack.push(op);
                return;
            default:
                throw std::runtime_error("[ERROR] Parsing expression failed.");
            }
        }
            return;
        default:
            throw std::runtime_error("[ERROR] Parsing expression failed.");
        }
    }

    std::shared_ptr<Engine::LT::LT> parseExpr(std::list<Token>::const_iterator start, std::list<Token>::const_iterator end)
    {
        /**
         * Priority:
         * bracket > calculation operator > bool operator > literal / column
         *
         * In calculation operator & bool operator,
         * the smaller one has higher priority.
         */

        Stack literal;
        Stack operation;

        while (true)
        {
            switch (start->type)
            {
            case TokenT::KEYWORD:
                throw std::runtime_error("Invalid expression arguement KEYWORD.");
                break;
            case TokenT::LITERAL:
            {
                auto node = std::make_shared<Engine::LT::LT>();
                node->type = Engine::LT::NodeType::LITERAL;
                node->info.liter.liter_type = std::get<LiteralInfo>(start->info).type;
                node->info.liter.liter_info = std::get<LiteralInfo>(start->info).data;
                literal.push(node);
            }
            break;
            case TokenT::COLANDTABLE:
            {
                auto node = std::make_shared<Engine::LT::LT>();
                node->type = Engine::LT::NodeType::COL;
                node->info.liter.liter_type = Engine::LT::LiterType::STR;
                node->info.liter.liter_info.emplace<std::string>(std::get<std::string>(start->info));
                literal.push(node);
            }
            break;
            case TokenT::BOOL_OPERATOR:
            {
                auto node = std::make_shared<Engine::LT::LT>();
                node->type = Engine::LT::NodeType::OPERATOR;
                node->info.op_type = std::get<BoolOp>(start->info);
                stackOp(node, literal, operation);
            }
            break;
            case TokenT::CALCULATION_OPERATOR:
            {
                auto node = std::make_shared<Engine::LT::LT>();
                node->type = Engine::LT::NodeType::CALCULATION;
                node->info.cal_type = std::get<CalOp>(start->info);
                stackOp(node, literal, operation);
            }
            break;
            case TokenT::LBRACKET:
            {
                auto node = std::make_shared<Engine::LT::LT>();
                node->type = Engine::LT::NodeType::LBRACKET;
                stackOp(node, literal, operation);
            }
            break;
            case TokenT::RBRACKET:
            {
                auto node = std::make_shared<Engine::LT::LT>();
                node->type = Engine::LT::NodeType::RBRACKET;
                stackOp(node, literal, operation);
            }
            break;
            default:
                throw std::runtime_error("[ERROR] Parse expression failed.");
            }

            if (start == end)
                break;
            start++;
        }

        while (literal.size() != 1)
        {
            opStackPopOnce(literal, operation);
        }

        return literal.top();
    }

    std::unique_ptr<Affair> parseCreate(std::list<Token>::const_iterator start, std::list<Token>::const_iterator end)
    {
        if (start == end || !(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::TABLE))
        {
            throw std::runtime_error("[FAILED] Command not found. Do you mean CREATE TABLE?");
            return nullptr;
        }
        start++;
        if (start == end || !(start->type == TokenT::COLANDTABLE))
        {
            throw std::runtime_error("[FAILED] CREATE TABLE should be followed with the table name");
            return nullptr;
        }
        std::string table_name = std::get<ColTableName>(start->info);

        // column definations
        size_t column_num = 0;
        std::vector<Engine::Column> columns;
        if (start != end)
        {
            start++;
            for (; start != end; start++, column_num++)
            {
                Engine::Column col;

                // column name
                if (start == end || !(start->type == TokenT::COLANDTABLE))
                {
                    throw std::runtime_error("[FAILED] Invalid column name.");
                }
                col.name = std::get<ColTableName>(start->info);
                start++;

                // data type
                if (start == end || start->type != TokenT::KEYWORD)
                {
                    throw std::runtime_error("[FAILED] Invalid data type.");
                }

                char type = static_cast<char>(std::get<KeywordE>(start->info)) - static_cast<char>(KeywordE::INT);
                if (type < static_cast<char>(Engine::DataType::INT) || type > static_cast<char>(Engine::DataType::DATETIME))
                    throw std::runtime_error("[FAILED] Invalid data type.");

                col.data_type = static_cast<Engine::DataType>(type);

                if (col.data_type == Engine::DataType::VARCHAR)
                {
                    // length
                    start++;
                    if (start == end || !(start->type == TokenT::LITERAL && std::get<LiteralInfo>(start->info).type == LiteralT::INT))
                    {
                        throw std::runtime_error("[FAILED] Could not found the length of VARCHAR.");
                    }
                    col.length = std::get<int64_t>(std::get<LiteralInfo>(start->info).data);
                }

                // TODO: constraint
                // for (; start != end; start++)
                // {
                //     if (start->type == TokenT::COLANDTABLE)
                //         break;

                // }

                columns.push_back(col);
            }
        }
        if (column_num == 0)
            throw std::runtime_error("[FAILED] Unable to create table with zero column");

        return static_cast<std::unique_ptr<Affair>>(std::make_unique<CreateTableA>(std::move(table_name), std::move(columns)));
    }

    std::unique_ptr<Affair> parseInsert(std::list<Token>::const_iterator start, std::list<Token>::const_iterator end)
    {
        if (start == end || !(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::INTO))
        {
            throw std::runtime_error("[FAILED] Command not found. Do you mean INSERT INTO?");
            return nullptr;
        }
        start++;
        if (start == end || !(start->type == TokenT::COLANDTABLE))
        {
            throw std::runtime_error("[FAILED] CREATE TABLE should be followed with the table name");
            return nullptr;
        }
        std::string table_name = std::get<ColTableName>(start->info);

        if (start == end)
            return nullptr;
        start++;
        if (start->type != TokenT::LBRACKET)
            throw std::runtime_error("[FAILED] The column list should be enclosed with brackets.");
        if (start == end)
            return nullptr;
        start++;
        std::vector<std::string> keys;
        if (start != end)
        {
            for (; start->type != TokenT::RBRACKET; start++)
            {
                if (start->type == TokenT::COLANDTABLE)
                {
                    keys.push_back(std::get<std::string>(start->info));
                }
            }
        }
        if (start == end)
            throw std::runtime_error("[FAILED] The column list should be followed with VALUES.");
        start++;
        if (!(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::VALUES))
            throw std::runtime_error("[FAILED] The column list should be followed with VALUES.");

        if (start == end)
            return nullptr;
        start++;
        std::vector<std::vector<std::shared_ptr<Engine::LT::LT>>> values;
        for (; start != end; start++)
        {
            if (start->type == TokenT::LBRACKET)
            {
                if (start == end)
                    break;
                start++;
                std::vector<std::shared_ptr<Engine::LT::LT>> rows;
                for (; start != end && start->type != TokenT::RBRACKET; start++)
                {
                    auto expr_start = start;
                    while (start != end && start->type != TokenT::RBRACKET && start->type != TokenT::COMMA)
                    {
                        start++;
                    }
                    rows.push_back(parseExpr(expr_start, --start));
                    start++;
                    if (start == end || start->type == TokenT::RBRACKET)
                        break;
                }
                values.push_back(std::move(rows));
            }
        }

        return static_cast<std::unique_ptr<Affair>>(std::make_unique<InsertA>(std::move(table_name), std::move(keys), std::move(values)));
    }

    std::unique_ptr<Affair> parseSelect(std::list<Token>::const_iterator start, std::list<Token>::const_iterator end)
    {
        std::vector<std::string> keys;
        if (start->type == TokenT::LBRACKET)
        {
            // column list
            if (start == end)
                throw std::runtime_error("[FAILED] Could not found column list.");
            start++;
            for (; start != end && start->type != TokenT::RBRACKET; start++)
            {
                if (start->type == TokenT::COLANDTABLE)
                {
                    keys.push_back(std::get<ColTableName>(start->info));
                }
            }
            start++;
        }
        else if (start->type == TokenT::CALCULATION_OPERATOR && std::get<CalOp>(start->info) == CalOp::MULTIPLY)
        {
            if (start == end)
                throw std::runtime_error("[FAILED] Command not found. Do you mean Select From?");
            start++;
        }
        else
        {
            throw std::runtime_error("[FAILED] Could not found column list.");
        }

        if (!(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::FROM))
            throw std::runtime_error("[FAILED] Command not found. Do you mean Select From?");
        if (start == end)
            throw std::runtime_error("[FAILED] Could not found table name.");
        start++;
        if (!(start->type == TokenT::COLANDTABLE))
            throw std::runtime_error("[FAILED] Could not found table name.");
        std::string table_name = std::get<ColTableName>(start->info);

        std::shared_ptr<Engine::LT::LT> requirements = nullptr;
        std::shared_ptr<Engine::SelectOrder> order = nullptr;
        for (; start != end; start++)
        {
            if (start->type == TokenT::KEYWORD)
            {
                if (std::get<KeywordE>(start->info) == KeywordE::WHERE)
                {
                    // WHERE
                    if (start == end)
                        break;
                    start++;
                    auto where_start = start;
                    for (; start != end && start->type != TokenT::KEYWORD; start++)
                    {
                    }
                    requirements = parseExpr(where_start, --start);
                }
                else if (std::get<KeywordE>(start->info) == KeywordE::ORDER)
                {
                    // ORDER BY
                    if (start == end)
                        break;
                    start++;
                    if (!(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::BY))
                        throw std::runtime_error("[FAILED] Command not found. Do you mean ORDER BY?");
                    if (start == end)
                        throw std::runtime_error("[FAILED] ORDER BY should be followed with column name.");
                    start++;
                    if (start->type != TokenT::COLANDTABLE)
                        throw std::runtime_error("[FAILED] ORDER BY should be followed with column name.");
                    order = std::make_shared<Engine::SelectOrder>();
                    order->key = std::get<ColTableName>(start->info);
                    if (start == end)
                        throw std::runtime_error("[FAILED] Could not found ASC or DESC.");
                    start++;
                    if (start->type == TokenT::KEYWORD)
                    {
                        if (std::get<KeywordE>(start->info) == KeywordE::ASC)
                        {
                            order->ascending = true;
                        }
                        else if (std::get<KeywordE>(start->info) == KeywordE::DESC)
                        {
                            order->ascending = false;
                        }
                        else
                        {
                            throw std::runtime_error("[FAILED] Could not found ASC or DESC.");
                        }
                    }
                    else
                    {
                        throw std::runtime_error("[FAILED] Could not found ASC or DESC.");
                    }
                }
            }
        }

        return static_cast<std::unique_ptr<Affair>>(std::make_unique<SelectA>(std::move(table_name), std::move(keys), std::move(requirements), std::move(order)));
    }

    std::unique_ptr<Affair> parseUpdate(std::list<Token>::const_iterator start, std::list<Token>::const_iterator end)
    {
        if (start->type != TokenT::COLANDTABLE)
            throw std::runtime_error("[FAILED] UPDATE should be followed with a table name.");

        std::string table_name = std::get<ColTableName>(start->info);

        if (start == end)
            throw std::runtime_error("[FAILED] Could not found SET.");
        start++;
        if (!(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::SET))
            throw std::runtime_error("[FAILED] Could not found SET.");

        std::vector<std::string> keys;
        std::vector<std::shared_ptr<Engine::LT::LT>> expr;
        std::shared_ptr<Engine::LT::LT> requirements;
        for (; start != end; start++)
        {
            if (start->type == TokenT::COLANDTABLE)
            {
                keys.push_back(std::get<ColTableName>(start->info));
                if (start == end)
                    throw std::runtime_error("[FAILED] Could not found '='.");
                start++;
                if (!(start->type == TokenT::BOOL_OPERATOR && std::get<BoolOp>(start->info) == BoolOp::EQUAL))
                    throw std::runtime_error("[FAILED] Could not found '='.");
                if (start == end)
                    throw std::runtime_error("[FAILED] Could not found expression.");
                auto expr_start = ++start;
                for (; start != end && start->type != TokenT::COMMA && start->type != TokenT::KEYWORD; start++)
                {
                }
                expr.push_back(parseExpr(expr_start, --start));
                // start++;
            }
            else if (start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::WHERE)
            {
                // where
                if (start == end)
                    break;
                start++;
                auto where_start = start;
                for (; start != end && start->type != TokenT::KEYWORD; start++)
                {
                }
                requirements = parseExpr(where_start, --start);
                break;
            }
        }

        return static_cast<std::unique_ptr<Affair>>(std::make_unique<UpdateA>(std::move(table_name), std::move(keys), std::move(expr), std::move(requirements)));
    }

    std::unique_ptr<Affair> parseDelete(std::list<Token>::const_iterator start, std::list<Token>::const_iterator end)
    {
        if (!(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::FROM))
            throw std::runtime_error("[FAILED] Command not found. Do you mean DELETE FROM?");
        if (start == end)
            throw std::runtime_error("[FAILED] DELETE FROM should be followed with a table name.");
        start++;
        if (start->type != TokenT::COLANDTABLE)
            throw std::runtime_error("[FAILED] DELETE FROM should be followed with a table name.");
        std::string table_name = std::get<ColTableName>(start->info);

        if (start == end)
            throw std::runtime_error("[FAILED] Could not found WHERE statement.");
        start++;
        if (!(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::WHERE))
            throw std::runtime_error("[FAILED] Could not found WHERE statement.");
        if (start == end)
            throw std::runtime_error("[FAILED] WHERE should be followed with an expression");
        start++;
        auto where_start = start;
        for (; start != end && start->type != TokenT::KEYWORD; start++)
        {
        }
        auto requirements = parseExpr(where_start, --start);

        return static_cast<std::unique_ptr<Affair>>(std::make_unique<DeleteA>(std::move(table_name), std::move(requirements)));
    }

    std::unique_ptr<Affair> parseDrop(std::list<Token>::const_iterator start, std::list<Token>::const_iterator end)
    {
        if (!(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::TABLE))
        {
            throw std::runtime_error("[FAILED] Command not found. Do you mean DROP TABLE?");
        }

        if (start == end)
            throw std::runtime_error("[FAILED] DROP TABLE should be followed with a table name");
        start++;
        if (!(start->type == TokenT::COLANDTABLE))
        {
            throw std::runtime_error("[FAILED] DROP TABLE should be followed with a table name");
        }
        std::string table_name = std::get<ColTableName>(start->info);

        return static_cast<std::unique_ptr<Affair>>(std::make_unique<DropA>(std::move(table_name)));
    }

    std::unique_ptr<Affair> parseGrant(std::list<Token>::const_iterator start, std::list<Token>::const_iterator end)
    {
        if (start->type != TokenT::AUTH_LEVEL)
        {
            throw std::runtime_error("[FAILED] GRANT should be followed with an authority level");
        }

        auto level = std::get<AuthLevel>(start->info);

        if (start == end)
            throw std::runtime_error("[FAILED] Command not found. Do you meant GRANT ... ON ..TO?");
        start++;

        if (!(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::ON))
            throw std::runtime_error("[FAILED] Command not found. Do you meant GRANT ... ON ..TO?");

        if (start == end)
            throw std::runtime_error("[FAILED] ON should be followed with a table name.");
        start++;
        if (start->type != TokenT::COLANDTABLE)
            throw std::runtime_error("[FAILED] ON should be followed with a table name.");
        std::string table_name = std::get<ColTableName>(start->info);

        if (start == end)
            throw std::runtime_error("[FAILED] Command not found. Do you meant GRANT ... ON ..TO?");
        start++;
        if (!(start->type == TokenT::KEYWORD && std::get<KeywordE>(start->info) == KeywordE::TO))
            throw std::runtime_error("[FAILED] Command not found. Do you meant GRANT ... ON ..TO?");

        if (start == end)
            throw std::runtime_error("[FAILED] TO should be followed with a list of username.");
        start++;

        std::vector<std::string> user_list;

        for (; start != end; start++)
        {
            if (start->type == TokenT::COLANDTABLE)
                user_list.push_back(std::get<ColTableName>(start->info));
        }
        if (user_list.size() == 0)
            throw std::runtime_error("[FAILED] TO should be followed with a list of username.");

        return static_cast<std::unique_ptr<Affair>>(std::make_unique<GrantA>(std::move(table_name), std::move(level), std::move(user_list)));
    }

    std::unique_ptr<Affair> parse(const std::list<Token> &token)
    {
        if (token.size() == 0 || token.front().type != TokenT::KEYWORD)
            return nullptr;

        switch (std::get<KeywordE>(token.front().info))
        {
        case KeywordE::CREATE:
            return parseCreate(++token.begin(), token.end());
        case KeywordE::INSERT:
            return parseInsert(++token.begin(), token.end());
        case KeywordE::SELECT:
            return parseSelect(++token.begin(), token.end());
        case KeywordE::UPDATE:
            return parseUpdate(++token.begin(), token.end());
        case KeywordE::DELETE_:
            return parseDelete(++token.begin(), token.end());
        case KeywordE::DROP:
            return parseDrop(++token.begin(), token.end());
        case KeywordE::GRANT:
            return parseGrant(++token.begin(), token.end());
        default:
            throw std::runtime_error("[FAILED] Command not found.");
            return nullptr;
        }
    }


};