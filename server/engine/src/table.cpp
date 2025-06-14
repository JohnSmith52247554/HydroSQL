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

    Table::Table(const std::wstring &name_)
        : name(name_)
    {
        std::filesystem::path path(DATA_PATH);
        this->data_path = path / (this->name + L".dat");
        if (!std::filesystem::exists(this->data_path))
        {
            throw std::runtime_error("[ERROR] Table " + std::string(name.begin(), name.end()) + " doesn't exist.");
            return;
        }
        std::ifstream ifile(this->data_path, std::ios::binary);
        if (!ifile.is_open())
        {
            throw std::runtime_error("[ERROR] Could not load table" + std::string(name.begin(), name.end()) + " .");
            return;
        }

        loadVector<Column>(ifile, this->columns);

        this->header_length = ifile.tellg();

        this->row_length = calRowLen();

        ifile.close();
    }

    Table::Table(const std::wstring &name_, const std::vector<Column> &&columns_)
        : name(name_), columns(columns_)
    {
        std::filesystem::path path(DATA_PATH);
        this->data_path = path / (this->name + L".dat");
        if (std::filesystem::exists(this->data_path))
        {
            throw std::runtime_error("[ERROR] Table " + std::string(name.begin(), name.end()) + " already exist.");
            return;
        }

        std::ofstream ofile(this->data_path, std::ios::binary);
        if (!ofile.is_open())
        {
            throw std::runtime_error("[ERROR] Could not load table" + std::string(name.begin(), name.end()) + " .");
            return;
        }

        saveVector<Column>(ofile, this->columns);

        this->header_length = ofile.tellp();

        this->row_length = calRowLen();

        ofile.close();
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
        struct ColAndIndex
        {
            const Column *col;
            size_t index;
        };
        
        std::map<const std::wstring, const ColAndIndex> column_map;
        std::vector<const Column *> uninvolved_column;
        
        {
            std::vector<bool> key_flag(keys.size(), false);
            for (const auto &col : this->columns)
            {
                bool exist = false;
                for (size_t i = 0; i < keys.size(); i++)
                {
                    if (col.name == keys[i])
                    {
                        column_map.emplace(keys[i], ColAndIndex{&col, i});
                        exist = true;
                        key_flag[i] = true;
                        break;
                    }
                }
                if (!exist)
                {
                    uninvolved_column.push_back(&col);
                }
            }
            for (size_t i = 0; i < key_flag.size(); i++)
            {
                if (!key_flag[i])
                {
                    result = L"[ERROR] The key " + keys[i] + L" doesn't exist in the table.";
                    return 0;
                }
            }
        }

        // for (const auto &key : keys)
        // {
        //     bool exist = false;

        //     for (const auto &col : this->columns)
        //     {
        //         if (col.name == key)
        //         {
        //             column_map.emplace(key, &col);
        //             exist = true;
        //             break;
        //         }
        //     }

        //     if (!exist)
        //     {
        //         result = L"[ERROR] The key " + key + L" doesn't exist in the table.";
        //         return 0;
        //     }
        // }

        // The type of the values should match the type of the keys.
        for (size_t i = 0; i < keys.size(); i++)
        {
            for (size_t j = 0; j < values.size(); j++)
            {
                auto &col = column_map[keys[i]].col;
                if (col == nullptr)
                {
                    result = L"[ERROR] nullptr";
                    return 0;
                }
                if (col->data_type != DataType::VARCHAR)
                {
                    if (!dataTypeExamination(col->data_type, values[j][i], 0))
                    {
                        result = L"[ERROR] The type of column " + keys[i] + L" is " + dataTypeStr(col->data_type) + L". Row " + std::to_wstring(j) + L" doesn't match that type.";
                        return 0;
                    }
                }
                else
                {
                    if (!dataTypeExamination(col->data_type, values[j][i], col->length))
                    {
                        result = L"[ERROR] The type of column " + keys[i] + L" is " + dataTypeStr(col->data_type) + L", whose maximum length is " + std::to_wstring(col->length) + L". Row " + std::to_wstring(j) + L" doesn't match that type.";
                        return 0;
                    }
                }
            }
        }

        // the uninvolved column with constraint NOT_NULL should have default value
        for (const auto &col : uninvolved_column)
        {
            bool has_default = false;
            bool has_not_null = false;
            for (const auto &con : col->constraints)
            {
                if (con.type == ConstraintType::DEFAULT)
                {
                    has_default = true;
                    break;
                }
                if (con.type == ConstraintType::NOT_NULL)
                {
                    has_not_null = true;
                    break;
                }
            }
            if (!has_default && has_not_null)
            {
                result = L"[ERROR] Column " + col->name + L" is not null and doesn't have a default value.";
                return 0;
            }
        }

        // TODO:
        // unique examine

        

        // insert
        std::ofstream ofile(data_path, std::ios::binary | std::ios::app);
        for (const auto &rows : values)
        {
            for (const auto &col : this->columns)
            {
                const auto it = column_map.find(col.name);
                if (it != column_map.end())
                {
                    const auto &val = rows[it->second.index];
                    switch (col.data_type)
                    {
                    case DataType::INT:
                        {
                            int32_t num = std::stoi(val);
                            ofile.write(reinterpret_cast<const char *>(&num), sizeof(num));
                        }
                        break;
                    case DataType::SMALLINT:
                        {
                            int16_t num = static_cast<int16_t>(std::stoi(val));
                            ofile.write(reinterpret_cast<const char *>(&num), sizeof(num));
                        }
                        break;
                    case DataType::BIGINT:
                        {
                            int64_t num = std::stoll(val);
                            ofile.write(reinterpret_cast<const char *>(&num), sizeof(num));
                        }
                        break;
                    case DataType::FLOAT:
                        {
                            float num = std::stof(val);
                            ofile.write(reinterpret_cast<const char *>(&num), sizeof(num));
                        }
                        break;
                    case DataType::DECIMAL:
                        {
                            // TODO: costumize the presision and scale of the decimal
                            double num = std::stod(val);
                            ofile.write(reinterpret_cast<const char *>(&num), sizeof(num));
                        }
                        break;
                    case DataType::CHAR:
                        {
                            char ch = val[0];
                            ofile.write(reinterpret_cast<const char *>(&ch), sizeof(ch));
                        }
                        break;
                    case DataType::VARCHAR:
                        {
                            std::vector<wchar_t> str(col.length, '\0');
                            for (size_t i = 0; i < std::min(static_cast<size_t>(col.length), val.size()); i++)
                            {
                                str[i] = val[i];
                            }
                            ofile.write(reinterpret_cast<const char *>(str.data()), sizeof(wchar_t) * str.size());
                        }
                        break;
                    case DataType::BOOLEAN:
                        {
                            bool boolean;
                            if (val == L"true" || val == L"TRUE" || val == L"1")
                                boolean = true;
                            else if (val == L"false" || val == L"FALSE" || val == L"0")
                                boolean = false;
                            ofile.write(reinterpret_cast<const char *>(&boolean), sizeof(boolean));
                        }
                        break;
                    case DataType::DATE:
                        {
                            int32_t buffer = 0;
                            int32_t date = 0;
                            buffer = std::stoi(val.substr(8, 2));
                            date += buffer * 1e0;
                            buffer = std::stoi(val.substr(5, 2));
                            date += buffer * 1e2;
                            buffer = std::stoi(val.substr(0, 4));
                            date += buffer * 1e4;
                            ofile.write(reinterpret_cast<const char *>(&date), sizeof(date));
                        }
                        break;
                    case DataType::TIME:
                        {
                            int32_t buffer = 0;
                            int32_t time = 0;
                            buffer = std::stoi(val.substr(6, 2));
                            time += buffer * 1e0;
                            buffer = std::stoi(val.substr(3, 2));
                            time += buffer * 1e2;
                            buffer = std::stoi(val.substr(0, 2));
                            time += buffer * 1e4;
                            ofile.write(reinterpret_cast<const char *>(&time), sizeof(time));
                        }
                        break;
                    case DataType::DATETIME:
                        {
                            int64_t buffer = 0;
                            int64_t datetime = 0;
                            buffer = std::stoi(val.substr(17, 2));
                            datetime += buffer * 1e0;
                            buffer = std::stoi(val.substr(14, 2));
                            datetime += buffer * 1e2;
                            buffer = std::stoi(val.substr(11, 2));
                            datetime += buffer * 1e4;
                            buffer = std::stoi(val.substr(8, 2));
                            datetime += buffer * 1e6;
                            buffer = std::stoi(val.substr(5, 2));
                            datetime += buffer * 1e8;
                            buffer = std::stoi(val.substr(0, 4));
                            datetime += buffer * 1e10;
                            ofile.write(reinterpret_cast<const char *>(&datetime), sizeof(datetime));
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    // TODO: fill with default value
                }
            }
        }

        ofile.close();

        result = L"[SUCCESS] " + std::to_wstring(values.size()) + L" row(s) affected.";

        return 1;
    }

    const bool Table::dataTypeExamination(const DataType type, const std::wstring &str, const size_t varchar_length)
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
                std::stoll(str);
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
            return str.size() == 1 && str[0] >= 0 && str[0] <= 255;
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

    const bool Table::continuousNumExamination(const std::wstring &str, const size_t begin, const size_t length)
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

    const size_t Table::getDataTypeSize(const DataType type)
    {
        switch (type)
        {
        case DataType::INT:
            return sizeof(int32_t);
        case DataType::SMALLINT:
            return sizeof(int16_t);
        case DataType::BIGINT:
            return sizeof(int32_t);
        case DataType::FLOAT:
            return sizeof(float);
        case DataType::DECIMAL:
            return sizeof(double);
        case DataType::CHAR:
            return sizeof(char);
        case DataType::VARCHAR:
            return sizeof(wchar_t);
        case DataType::BOOLEAN:
            return sizeof(bool);
        case DataType::DATE:
            return sizeof(int32_t);
        case DataType::TIME:
            return sizeof(int32_t);
        case DataType::DATETIME:
            return sizeof(int64_t);
        default:
            return 0;
        }
    }

    const std::streampos Table::calRowLen() const
    {
        std::streampos len = 0;
        for (const auto &col : this->columns)
        {
            if (col.data_type == DataType::VARCHAR)
            {
                len += this->getDataTypeSize(col.data_type) + col.length;
            }
            else
            {
                len += this->getDataTypeSize(col.data_type);
            }
        }
        return len;
    }
}