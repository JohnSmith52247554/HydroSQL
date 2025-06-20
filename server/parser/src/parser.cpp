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
                        throw std::runtime_error("Unclosed singel quote mark.");
                    }
                    if (len > 1)
                    {
                        throw std::runtime_error("Invalid character.");
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
                        throw std::runtime_error("Unclosed double quote mark.");
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
                }

                std::string str(start_it, it + 1);
                if (exist_dash && exist_colon && !exist_dot)
                {
                    // date time
                    if (!datetimeValideExamine(str))
                    {
                        throw std::runtime_error("Invalid datetime.");
                    }
                    token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::DATETIME, datetimeStrToNum(str)});
                }
                else if (exist_colon && !exist_dot && !exist_dash)
                {
                    // time
                    if (!timeValideExamine(str))
                    {
                        throw std::runtime_error("Invalid time.");
                    }
                    token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::TIME, timeStrToNum(str)});
                }
                else if (exist_dash && !exist_dot && !exist_colon)
                {
                    // date
                    if (!dateValideExamine(str))
                    {
                        throw std::runtime_error("Invalid date.");
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
                    throw std::runtime_error("Invalid number");
                }

            }
            // keyword & column name & table name
            else if ((*it >= 'a' && *it <= 'z') || (*it >= 'A' && *it <= 'Z'))
            {
                auto start_it = it;
                while (true)
                {
                    if (it + 1 == command.end())
                        break;
                    it++;
                    if (!(((*it >= 'a' && *it <= 'z') || (*it >= 'A' && *it <= 'Z') || (*it >= '0' && *it <= '9'))))
                    {
                        it--;
                        break;
                    }
                }

                std::string str(start_it, it + 1);

                auto kw = std::find_if(keywords.begin(), keywords.end(),
                                       [&](const std::pair<std::string, KeywordE> &k)
                                       {
                                           return k.first == str;
                                       });

                if (kw != keywords.end())
                {
                    // keyword
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
                                throw std::runtime_error("Invalid number.");
                            }
                        }
                        std::string num(len_start, it);
                        token_list.emplace_back(TokenT::LITERAL, LiteralInfo{LiteralT::INT, stoll(num)});
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
};