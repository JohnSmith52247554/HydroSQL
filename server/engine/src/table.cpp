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
    void saveStr(std::ostream &os, const std::string &str)
    {
        size_t size = str.size();
        os.write(reinterpret_cast<const char *>(&size), sizeof(size));
        os.write(reinterpret_cast<const char *>(str.data()), sizeof(*str.data()) * size);
    }

    void loadStr(std::istream &is, std::string &str)
    {
        size_t size;
        is.read(reinterpret_cast<char *>(&size), sizeof(size));
        str.resize(size);
        is.read(reinterpret_cast<char *>(str.data()), sizeof(*str.data()) * size);
    }

    const char *dataTypeStr(const DataType type)
    {
        switch (type)
        {
        case DataType::INT:
            return "INT";
        case DataType::SMALLINT:
            return "SMALLINT";
        case DataType::BIGINT:
            return "BIGINT";
        case DataType::FLOAT:
            return "FLOAT";
        case DataType::DECIMAL:
            return "DECIMA";
        case DataType::CHAR:
            return "CHAR";
        case DataType::VARCHAR:
            return "VARCHAR";
        case DataType::BOOLEAN:
            return "BOOLEAN";
        case DataType::DATE:
            return "DATE";
        case DataType::TIME:
            return "TIME";
        case DataType::DATETIME:
            return "DATETIME";
        default:
            return "UNDEFINED";
        }
    }

    const char *constraintTypeStr(const ConstraintType type)
    {
        switch (type)
        {
        case ConstraintType::PRIMARY_KEY:
            return "PRIMARY_KEY";
        case ConstraintType::NOT_NULL:
            return "NOT_NUL";
        case ConstraintType::UNIQUE:
            return "UNIQUE";
        case ConstraintType::CHECK:
            return "CHECK";
        case ConstraintType::DEFAULT:
            return "DEFAULT";
        default:
            return "UNDEFINED";
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

    Table::Table(const std::string &name_)
        : name(name_)
    {
        std::filesystem::path path(DATA_PATH);
        this->data_path = path / (this->name + ".dat");
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

    Table::Table(const std::string &name_, const std::vector<Column> &&columns_)
        : name(name_), columns(columns_)
    {
        std::filesystem::path path(DATA_PATH);
        this->data_path = path / (this->name + ".dat");
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

    int Table::insert(const std::vector<std::string> &keys, const std::vector<std::vector<std::string>> &values, std::string &result)
    {
        // legality examination
        // The amount of values each row should be the same as the keys.
        for (size_t i = 0; i < values.size(); i++)
        {
            if (values[i].size () != keys.size())
            {
                result = "[FAILED] The amount of the values in row " + std::to_string(i) + " is " + std::to_string(values[i].size()) + ", but the amount of the keys is " + std::to_string(keys.size()) + ".";
                return 0;
            }
        }

        // The keys should exist.    
        std::map<const std::string, const ColAndIndex> column_map;
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
                    result = "[FAILED] The key " + keys[i] + " doesn't exist in the table.";
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
        //         result = "[ERROR] The key " + key + " doesn't exist in the table.";
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
                    result = "[ERROR] nullptr";
                    return 0;
                }
                if (col->data_type != DataType::VARCHAR)
                {
                    if (!dataTypeExamination(col->data_type, values[j][i], 0))
                    {
                        result = "[FAILED] The type of column " + keys[i] + " is " + dataTypeStr(col->data_type) + ". Row " + std::to_string(j) + " doesn't match that type.";
                        return 0;
                    }
                }
                else
                {
                    if (!dataTypeExamination(col->data_type, values[j][i], col->length))
                    {
                        result = "[FAILED] The type of column " + keys[i] + " is " + dataTypeStr(col->data_type) + ", whose maximum length is " + std::to_string(col->length) + ". Row " + std::to_string(j) + " doesn't match that type.";
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
                result = "[FAILED] Column " + col->name + " is not null and doesn't have a default value.";
                return 0;
            }
        }

        // TODO:
        // unique examine

        

        // insert
        std::ofstream ofile(data_path, std::ios::binary | std::ios::app);
        if (!ofile.is_open())
        {
            result = "[ERROR] Can not open data file.";
            return 0;
        }

        for (const auto &rows : values)
        {
            for (const auto &col : this->columns)
            {
                const auto it = column_map.find(col.name);
                if (it != column_map.end())
                {
                    const auto &val = rows[it->second.index];
                    insertVal(ofile, col.data_type, val, col.length);
                }
                else
                {
                    // fill with default value
                    const Constraint *default_con = nullptr;
                    for (const auto &con : col.constraints)
                    {
                        if (con.type == ConstraintType::DEFAULT)
                            default_con = &con;
                    }
                    if (default_con == nullptr)
                    {
                        if (col.data_type == DataType::VARCHAR)
                            insertVal(ofile, col.data_type, "");
                        else if (col.data_type == DataType::DATE)
                            insertVal(ofile, col.data_type, "0000-00-00");
                        else if (col.data_type == DataType::TIME)
                            insertVal(ofile, col.data_type, "00:00:00");
                        else if (col.data_type == DataType::DATETIME)
                            insertVal(ofile, col.data_type, "0000-00-00-00:00:00");
                        else
                            insertVal(ofile, col.data_type, "0");
                    }
                    else
                    {
                        // TODO: the DEFAULT constraint
                    }
                }
            }
        }

        ofile.close();

        result = "[SUCCESS] " + std::to_string(values.size()) + " row(s) affected.";

        return 1;
    }

    int Table::select(const std::vector<std::string> &keys, const bool &requirements, const SelectOrder &order, std::vector<std::vector<std::string>> &output, std::string &result) const
    {
        // legality examination
        // The keys should exist.
        struct SelectInfo
        {
            ColAndIndex cai;
            bool selected;
        };
        
        std::vector<SelectInfo> col_vec(this->columns.size());
        for (size_t i = 0; i < col_vec.size(); i++)
        {
            col_vec[i].cai.col = &this->columns[i];
            col_vec[i].selected = false;
        }

        if (keys.size() == 0)
        {
            for (size_t i = 0; i < col_vec.size(); i++)
            {
                col_vec[i].selected = true;
                col_vec[i].cai.index = i;
            }
        }
        else
        {
            for (size_t i = 0; i < keys.size(); i++)
            {
                bool exist = false;
                for (auto &col : col_vec)
                {
                    if (keys[i] == col.cai.col->name)
                    {
                        col.selected = true;
                        col.cai.index = i;
                        exist = true;
                        break;
                    }
                }
                if (!exist)
                {
                    result = "[FAILED] The key " + keys[i] + " doesn't exist.";
                    return 0;
                }
            }
        }
        // TODO: requirement examine
        
        // TODO: order examine

        // Select
        std::ifstream ifile(data_path, std::ios::binary);
        if (!ifile.is_open())
        {
            result = "[ERROR] Can not open data file.";
            return 0;
        }

        ifile.seekg(0, std::ios::end);
        auto end_of_file = ifile.tellg();

        ifile.seekg(header_length, std::ios::beg);

        std::list<std::vector<std::string>> output_list;
        while (end_of_file - ifile.tellg() >= row_length)
        {
            output_list.emplace_back(keys.size() == 0 ? col_vec.size() : keys.size());
            auto &list_back = output_list.back();
            for (size_t i = 0; i < col_vec.size(); i++)
            {
                auto &col = col_vec[i];
                if (!col.selected)
                {
                    ifile.seekg(getColumnSize(*col.cai.col), std::ios::cur);
                }
                else
                {
                    readVal(ifile, *col.cai.col, list_back[col.cai.index]);
                }
            }

            // TODO: requirement
        }

        output.resize(output_list.size() + 1);
        if (keys.size() == 0)
        {
            output[0].resize(col_vec.size());
            for (size_t i = 0; i < col_vec.size(); i++)
            {
                output[0][i] = col_vec[i].cai.col->name;
            }
        }
        else
        {
            output[0].resize(keys.size());
            for (size_t i = 0; i < keys.size(); i++)
            {
                output[0][i] = keys[i];
            }
        }
        {
            auto i1 = output_list.begin();
            auto i2 = output.begin() + 1;
            while (i1 != output_list.end())
            {
                *i2 = std::move(*i1);
                i1++;
                i2++;
            }
        }

        // TODO: sort

        result = "[SUCCESS] " + std::to_string(output.size() - 1) + " row(s) in set.";

        return 1;
    }

    int Table::update(const std::vector<UpdateInfo> &info, const bool &requirements, std::string &result)
    {
        // legality examination
        // the columns to be update should exist
        struct ColUpdate
        {
            Column *col;
            bool should_update;
            size_t index;
        };
        std::vector<ColUpdate> col_vec(this->columns.size(), ColUpdate(nullptr, false));
        for (size_t i = 0; i < col_vec.size(); i++)
        {
            col_vec[i].col = &this->columns[i];
        }

        for (size_t i = 0; i < info.size(); i++)
        {
            bool exist = false;
            for (auto &col_up : col_vec)
            {
                if (info[i].col_name == col_up.col->name)
                {
                    exist = true;
                    col_up.should_update = true;
                    col_up.index = i;
                    break;
                }
            }
            if (!exist)
            {
                result = "[FAILED] The column " + info[i].col_name + " doesn't exist.";
                return 0;
            }
        }

        // the value to be set should match the data type of the column
        for (const auto &col : col_vec)
        {
            if (!col.should_update)
                continue;
            
            if (col.col->data_type == DataType::VARCHAR)
            {
                if(!this->dataTypeExamination(col.col->data_type, info[col.index].set, col.col->length))
                {
                    result = "[FAILED] The data type of column " + col.col->name + " is VARCHAR, whose maximun lenght is " + std::to_string(col.col->length) + ". The set value doesn't match that type.";
                    return 0;
                }
            }   
            else
            {
                if (!this->dataTypeExamination(col.col->data_type, info[col.index].set, 0))
                {
                    result = "[FAILED] The data type of column " + col.col->name + " is " + dataTypeStr(col.col->data_type) + ". The set value doesn't match that type.";
                    return 0;
                }
            } 
        }

        // update
        std::ofstream ofile(data_path, std::ios::binary | std::ios::in);
        if (!ofile.is_open())
        {
            result = "[FAILED] Can not open the data file.";
            return 0;
        }

        ofile.seekp(0, std::ios::end);
        auto end_of_file = ofile.tellp();

        ofile.seekp(header_length, std::ios::beg);
        // auto cur = ofile.tellp();
        size_t changed_num = 0;

        while (end_of_file - ofile.tellp() >= this->row_length)
        {
            // TODO: requirements check

            for (const auto &col : col_vec)
            {
                if (!col.should_update)
                {
                    ofile.seekp(getColumnSize(*col.col), std::ios::cur);
                    // cur = ofile.tellp();
                    continue;
                }
                else
                {
                    if (col.col->data_type != DataType::VARCHAR)
                    {
                        this->insertVal(ofile, col.col->data_type, info[col.index].set);
                    }
                    else
                    {
                        this->insertVal(ofile, col.col->data_type, info[col.index].set, col.col->length);
                    }
                    // cur = ofile.tellp();
                }
            }
            // cur = ofile.tellp();
            changed_num++;
        }

        ofile.close();

        result = "[SUCCESS] " + std::to_string(changed_num) + " row(s) updated.";

        return 1;
    }
    
    const bool Table::dataTypeExamination(const DataType type, const std::string &str, const size_t varchar_length)
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
            return str == "true" || str == "false" || str == "TRUE" || str == "FALSE" || str == "1" || str == "0";
        case DataType::DATE:
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
            break;
        case DataType::TIME:
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
            break;
        case DataType::DATETIME:
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
            break;
        case DataType::VARCHAR:
            return str.size() <= varchar_length;
        default:
            break;
        }

        return true;
    }

    const bool Table::continuousNumExamination(const std::string &str, const size_t begin, const size_t length)
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

    const size_t Table::getDataTypeSize(const DataType type)
    {
        switch (type)
        {
        case DataType::INT:
            return sizeof(int32_t);
        case DataType::SMALLINT:
            return sizeof(int16_t);
        case DataType::BIGINT:
            return sizeof(int64_t);
        case DataType::FLOAT:
            return sizeof(float);
        case DataType::DECIMAL:
            return sizeof(double);
        case DataType::CHAR:
            return sizeof(char);
        case DataType::VARCHAR:
            return sizeof(char);
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
    const size_t Table::getColumnSize(const Column &col)
    {
        size_t len = 0;
        if (col.data_type == DataType::VARCHAR)
        {
            len += getDataTypeSize(col.data_type) * col.length;
        }
        else
        {
            len += getDataTypeSize(col.data_type);
        }
        return len;
    }

    const std::streampos Table::calRowLen() const
    {
        std::streampos len = 0;
        for (const auto &col : this->columns)
        {
            len += getColumnSize(col);
        }
        return len;
    }

    void Table::insertVal(std::ostream &os, const DataType type, const std::string &val, const size_t len)
    {
        switch (type)
        {
        case DataType::INT:
        {
            int32_t num = std::stoi(val);
            os.write(reinterpret_cast<const char *>(&num), sizeof(num));
        }
        break;
        case DataType::SMALLINT:
        {
            int16_t num = static_cast<int16_t>(std::stoi(val));
            os.write(reinterpret_cast<const char *>(&num), sizeof(num));
        }
        break;
        case DataType::BIGINT:
        {
            int64_t num = std::stoll(val);
            os.write(reinterpret_cast<const char *>(&num), sizeof(num));
        }
        break;
        case DataType::FLOAT:
        {
            float num = std::stof(val);
            os.write(reinterpret_cast<const char *>(&num), sizeof(num));
        }
        break;
        case DataType::DECIMAL:
        {
            // TODO: costumize the presision and scale of the decimal
            double num = std::stod(val);
            os.write(reinterpret_cast<const char *>(&num), sizeof(num));
        }
        break;
        case DataType::CHAR:
        {
            char ch = val[0];
            os.write(reinterpret_cast<const char *>(&ch), sizeof(ch));
        }
        break;
        case DataType::VARCHAR:
        {
            std::vector<char> str(len, '\0');
            for (size_t i = 0; i < std::min(len, val.size()); i++)
            {
                str[i] = val[i];
            }
            os.write(reinterpret_cast<const char *>(str.data()), sizeof(char) * str.size());
        }
        break;
        case DataType::BOOLEAN:
        {
            bool boolean;
            if (val == "true" || val == "TRUE" || val == "1")
                boolean = true;
            else if (val == "false" || val == "FALSE" || val == "0")
                boolean = false;
            os.write(reinterpret_cast<const char *>(&boolean), sizeof(boolean));
        }
        break;
        case DataType::DATE:
        {
            int32_t date;
            dateStrToNum(val, date);
            os.write(reinterpret_cast<const char *>(&date), sizeof(date));
        }
        break;
        case DataType::TIME:
        {
            int32_t time;
            timeStrToNum(val, time);
            os.write(reinterpret_cast<const char *>(&time), sizeof(time));
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
            os.write(reinterpret_cast<const char *>(&datetime), sizeof(datetime));
        }
        break;
        default:
            break;
        }
    }

    void Table::readVal(std::istream &is, const Column &col, std::string &output)
    {
        switch (col.data_type)
        {
        case DataType::INT:
        {
            int32_t buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            output = std::to_string(buffer);
        }
        break;
        case DataType::SMALLINT:
        {;
            int16_t buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            output = std::to_string(buffer);
        }
        break;
        case DataType::BIGINT:
        {
            int64_t buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            output = std::to_string(buffer);
        }
        break;
        case DataType::FLOAT:
        {
            float buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            output = std::to_string(buffer);
        }
        break;
        case DataType::DECIMAL:
        {
            // TODO: costumize the presision and scale of the decimal
            double buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            output = std::to_string(buffer);
        }
        break;
        case DataType::CHAR:
        {
            char buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            output = std::to_string(buffer);
        }
        break;
        case DataType::VARCHAR:
        {
            std::vector<char> buffer(col.length, '\0');
            is.read(reinterpret_cast<char *>(buffer.data()), sizeof(char) * buffer.size());
            auto end = buffer.rbegin();
            while (*end == '\0' && end != buffer.rend())
            {
                end++;
            }
            output.assign(buffer.begin(), end.base());
        }
        break;
        case DataType::BOOLEAN:
        {
            bool buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            output = std::to_string(buffer);
        }
        break;
        case DataType::DATE:
        {
            int32_t buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            dateNumToStr(buffer, output);
        }
        break;
        case DataType::TIME:
        {
            int32_t buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            timeNumToStr(buffer, output);
        }
        break;
        case DataType::DATETIME:
        {
            int64_t buffer;
            is.read(reinterpret_cast<char *>(&buffer), sizeof(buffer));
            datetimeNumToStr(buffer, output);
        }
        break;
        default:
            break;
        }
    }

    void Table::dateStrToNum(const std::string &str, int32_t &num)
    {
        int32_t buffer = 0;
        num = 0;
        buffer = std::stoi(str.substr(8, 2));
        num += buffer * 1e0;
        buffer = std::stoi(str.substr(5, 2));
        num += buffer * 1e2;
        buffer = std::stoi(str.substr(0, 4));
        num += buffer * 1e4;
    }
    void Table::dateNumToStr(const int32_t &num, std::string &str)
    {
        int32_t year = num / 10000;
        int32_t month = (num % 10000) / 100;
        int32_t day = num % 100;
        std::stringstream wss;
        wss << std::setw(4) << std::setfill('0') << year << "-"
            << std::setw(2) << std::setfill('0') << month << "-"
            << std::setw(2) << std::setfill('0') << day;
        str = wss.str();
    }
    void Table::timeStrToNum(const std::string &str, int32_t &num)
    {
        int32_t buffer = 0;
        num = 0;
        buffer = std::stoi(str.substr(6, 2));
        num += buffer * 1e0;
        buffer = std::stoi(str.substr(3, 2));
        num += buffer * 1e2;
        buffer = std::stoi(str.substr(0, 2));
        num += buffer * 1e4;
    }
    void Table::timeNumToStr(const int32_t &num, std::string &str)
    {
        int32_t hour = num / 10000;
        int32_t minute = (num % 10000) / 100;
        int32_t second = num % 100;

        std::stringstream wss;
        wss << std::setw(2) << std::setfill('0') << hour << ":"
            << std::setw(2) << std::setfill('0') << minute << ":"
            << std::setw(2) << std::setfill('0') << second;

        str = wss.str();
    }
    void Table::datetimeStrToNum(const std::string &str, int32_t &num)
    {
        int64_t buffer = 0;
        num = 0;
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
    }
    void Table::datetimeNumToStr(const int32_t &num, std::string &str)
    {
        int64_t datePart = num / 1000000;
        int32_t year = datePart / 10000;
        int32_t month = (datePart % 10000) / 100;
        int32_t day = datePart % 100;

        int64_t timePart = num % 1000000;
        int32_t hour = timePart / 10000;
        int32_t minute = (timePart % 10000) / 100;
        int32_t second = timePart % 100;

        std::stringstream wss;
        wss << std::setw(4) << std::setfill('0') << year << "-"
            << std::setw(2) << std::setfill('0') << month << "-"
            << std::setw(2) << std::setfill('0') << day << "-"
            << std::setw(2) << std::setfill('0') << hour << ":"
            << std::setw(2) << std::setfill('0') << minute << ":"
            << std::setw(2) << std::setfill('0') << second;

        str = wss.str();
    }
}