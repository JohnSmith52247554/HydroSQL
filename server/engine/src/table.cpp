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
    void saveStr(std::ostream &os, const std::wstring &str)
    {
        size_t size = str.size();
        os.write(reinterpret_cast<const char *>(&size), sizeof(size));
        os.write(reinterpret_cast<const char *>(str.data()), sizeof(*str.data()) * size);
    }

    void loadStr(std::istream &is, std::wstring &str)
    {
        size_t size;
        is.read(reinterpret_cast<char *>(&size), sizeof(size));
        str.resize(size);
        is.read(reinterpret_cast<char *>(str.data()), sizeof(*str.data()) * size);
    }

    const wchar_t *dataTypeStr(const DataType type)
    {
        switch (type)
        {
        case DataType::INT:
            return L"INT";
        case DataType::SMALLINT:
            return L"SMALLINT";
        case DataType::BIGINT:
            return L"BIGINT";
        case DataType::FLOAT:
            return L"FLOAT";
        case DataType::DECIMAL:
            return L"DECIMAL";
        case DataType::CHAR:
            return L"CHAR";
        case DataType::VARCHAR:
            return L"VARCHAR";
        case DataType::BOOLEAN:
            return L"BOOLEAN";
        case DataType::DATE:
            return L"DATE";
        case DataType::TIME:
            return L"TIME";
        case DataType::DATETIME:
            return L"DATETIME";
        default:
            return L"UNDEFINED";
        }
    }

    const wchar_t *constraintTypeStr(const ConstraintType type)
    {
        switch (type)
        {
        case ConstraintType::PRIMARY_KEY:
            return L"PRIMARY_KEY";
        case ConstraintType::NOT_NULL:
            return L"NOT_NULL";
        case ConstraintType::UNIQUE:
            return L"UNIQUE";
        case ConstraintType::CHECK:
            return L"CHECK";
        case ConstraintType::DEFAULT:
            return L"DEFAULT";
        default:
            return L"UNDEFINED";
        }
    }

    std::ostream &operator<<(std::ostream &os, const Constraint &constraint)
    {
        auto type = enumToBase<decltype(constraint.type)>(constraint.type);
        os.write(reinterpret_cast<const char *>(&type), sizeof(type));
        saveStr(os, constraint.details);
        return os;
    }

    std::istream &operator>>(std::istream &is, Constraint &constraint)
    {
        is.read(reinterpret_cast<char *>(&constraint.type), sizeof(constraint.type));
        loadStr(is, constraint.details);
        return is;
    }

    std::ostream &operator<<(std::ostream &os, const Column &col)
    {
        saveStr(os, col.name);
        auto data_type = enumToBase<decltype(col.data_type)>(col.data_type);
        os.write(reinterpret_cast<const char *>(&data_type), sizeof(data_type));
        os.write(reinterpret_cast<const char *>(&col.length), sizeof(col.length));
        os.write(reinterpret_cast<const char *>(&col.precision), sizeof(col.precision));
        os.write(reinterpret_cast<const char *>(&col.scale), sizeof(col.scale));
        saveStr(os, col.default_value);
        saveVector<Constraint>(os, col.constraints);
        return os;
    }

    std::istream &operator>>(std::istream &is, Column &col)
    {
        loadStr(is, col.name);
        is.read(reinterpret_cast<char *>(&col.data_type), sizeof(col.data_type));
        is.read(reinterpret_cast<char *>(&col.length), sizeof(col.length));
        is.read(reinterpret_cast<char *>(&col.precision), sizeof(col.precision));
        is.read(reinterpret_cast<char *>(&col.scale), sizeof(col.scale));
        loadStr(is, col.default_value);
        loadVector<Constraint>(is, col.constraints);
        return is;
    }

    int Table::init()
    {
        std::filesystem::path path(PROJECT_PATH);
        path = path / (this->name + L".dat");
        std::ifstream ifile(path, std::ios::binary);
        if (!ifile.is_open())
            return 0;

        size_t col_num;
        ifile.read(reinterpret_cast<char *>(&col_num), sizeof(col_num));
        this->columns.resize(col_num);
        loadVector<Column>(ifile, this->columns);

        this->header_length = ifile.tellg();

        return 1;
    }

    int Table::insert(const std::vector<std::wstring> &keys, const std::vector<std::vector<std::wstring>> &values, std::wstring &result)
    {
        // legality examination
        // The amount of values each row should be the same as the keys.
        for (size_t i = 0; i < values.size(); i++)
        {
            if (values[i].size () != keys.size())
            {
                result = L"[ERROR] The amount of the values in row " + std::to_wstring(i) + L" is " + std::to_wstring(values[i].size()) + L", but the amount of the keys is " + std::to_wstring(keys.size()) + L".";
                return 0;
            }
        }

        // The keys should exist.
        std::map<std::wstring, const Column *> column_map;
        for (const auto &key : keys)
        {
            bool exist = false;

            for (const auto &col : this->columns)
            {
                if (col.name == key)
                {
                    column_map.emplace(key, &col);
                    exist = true;
                    break;
                }
            }

            if (!exist)
            {
                result = L"[ERROR] The key " + key + L" doesn't exist in the table.";
                return 0;
            }
        }

        // The type of the values should match the type of the keys.
        for (size_t i = 0; i < keys.size(); i++)
        {
            for (size_t j = 0; j < values.size(); j++)
            {
                auto &col = column_map[keys[i]];
                if (col == nullptr)
                {
                    result = L"[ERROR] nullptr";
                    return 0;
                }
                if (!dataTypeExamination(col->data_type, values[j][i], col->data_type == DataType::VARCHAR ? col->length : 0))
                {
                    result = L"[ERROR] The type of column " + keys[i] + L" is " + dataTypeStr(col->data_type) + L". Row " + std::to_wstring(j) + L" doesn't match that type.";
                    return 0;
                }
            }
        }

        return 1;
    }

    bool Table::dataTypeExamination(const DataType type, const std::wstring &str, const size_t varchar_length)
    {
        // INT, SMALLINT, FLOAT... should could be interpret as a number.
        // CHAR should be a string with size equal one.
        // BOOLEAN should be whether true or false.
        // DATE should match the format as YYYY-MM-DD
        // TIME should match the format as hh:mm:ss
        // DATETIME should be YYYY-MM-DD-hh:mm:ss
        // The size of VARCHAR should be lower than the maximum length.

        switch (type)
        {
        case DataType::INT:
            [[fallthrough]];
        case DataType::SMALLINT:
            try
            {
                std::stoi(str);
            }
            catch (const std::invalid_argument &e)
            {
                return false;
            }
            break;
        case DataType::BIGINT:
            try
            {
                std::stol(str);
            }
            catch (const std::invalid_argument &e)
            {
                return false;
            }
            break;
        case DataType::FLOAT:
            try
            {
                std::stof(str);
            }
            catch (const std::invalid_argument &e)
            {
                return false;
            }
            break;
        case DataType::DECIMAL:
            try
            {
                std::stod(str);
            }
            catch (const std::invalid_argument &e)
            {
                return false;
            }
            break;
        case DataType::CHAR:
            return str.size() == 1;
        case DataType::BOOLEAN:
            return str == L"true" || str == L"false" || str == L"TRUE" || str == L"FALSE" || str == L"1" || str == L"0";
        case DataType::DATE:
            if (str.size() != 10)
                return false;
            if (!continuousNumExamination(str, 0, 4))
                return false;
            if (str[4] != L'-')
                return false;
            if (!continuousNumExamination(str, 5, 2))
                return false;
            if (str[7] != L'-')
                return false;
            if (!continuousNumExamination(str, 8, 2))
                return false;
            break;
        case DataType::TIME:
            if (str.size() != 8)
                return false;
            if (!continuousNumExamination(str, 0, 2))
                return false;
            if (str[2] != L':')
                return false;
            if (!continuousNumExamination(str, 3, 2))
                return false;
            if (str[5] != L':')
                return false;
            if (!continuousNumExamination(str, 6, 2))
                return false;
            break;
        case DataType::DATETIME:
            if (str.size() != 19)
                return false;
            if (!continuousNumExamination(str, 0, 4))
                return false;
            if (str[4] != L'-')
                return false;
            if (!continuousNumExamination(str, 5, 2))
                return false;
            if (str[7] != L'-')
                return false;
            if (!continuousNumExamination(str, 8, 2))
                return false;
            if (str[10] != L'-')
                return false;
            if (!continuousNumExamination(str, 11, 2))
                return false;
            if (str[13] != L':')
                return false;
            if (!continuousNumExamination(str, 14, 2))
                return false;
            if (str[16] != L':')
                return false;
            if (!continuousNumExamination(str, 17, 2))
                return false;
            break;
        case DataType::VARCHAR:
            return str.size() <= varchar_length;
        default:
            break;
        }

        return true;
    }

    bool Table::continuousNumExamination(const std::wstring &str, const size_t begin, const size_t length)
    {
        if (begin + length > str.size())
            return false;
        for (size_t i = begin; i < begin + length; i++)
        {
            if (str[i] < L'0' || str[i] > L'9')
                return false;
        }
        return true;
    }
}