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
#include <LogicalTree.hpp>

namespace HydroSQL::Server::Engine
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
        // case ConstraintType::CHECK:
        //     return "CHECK";
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

    LT::LiterType dataTypeToLiteralType(const DataType type)
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
            [[fallthrough]];
        case DataType::BOOLEAN:
            return LT::LiterType::INT;
        case DataType::FLOAT:
            [[fallthrough]];
        case DataType::DECIMAL:
            return LT::LiterType::FLOAT;
        case DataType::VARCHAR:
            [[fallthrough]];
        case DataType::DATE:
            [[fallthrough]];
        case DataType::TIME:
            [[fallthrough]];
        case DataType::DATETIME:
            return LT::LiterType::STR;
        default:
            return LT::LiterType::null;
        }
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

        buffer_row_num = MAX_BUFFER_SIZE / row_length;
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

        buffer_row_num = MAX_BUFFER_SIZE / row_length;
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
            for (size_t j = 0; j < this->columns.size(); j++)
            {
                const auto &col = this->columns[j];
                bool exist = false;
                for (size_t i = 0; i < keys.size(); i++)
                {
                    if (col.name == keys[i])
                    {
                        column_map.emplace(keys[i], ColAndIndex{&col, i, j});
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

        // expression evaluation
        std::vector<std::vector<std::string>> val_str(values.size(), std::vector<std::string>(this->columns.size()));
        for (const auto &row : val_str) 
        {
            for (size_t i = 0; i < this->columns.size(); i++)
            {

            }
        }

        // unique examine
        {
            std::vector<std::string> unique_row;

            for (size_t i = 0; i < keys.size(); i++)
            {
                const auto &col = column_map.find(keys[i]);
                const auto &con = col->second.col->constraints;
                if (std::find_if(con.begin(), con.end(), [](const Constraint constraint)
                                 { return constraint.type == ConstraintType::UNIQUE || constraint.type == ConstraintType::PRIMARY_KEY; })
                                != con.end())
                {
                    unique_row.emplace_back(keys[i]);
                }
            }

            if (unique_row.size() > 0)
            {
                std::vector<std::vector<std::string>> select;
                std::string str;

                if(!this->select(unique_row, nullptr, nullptr, select, str))
                {
                    result = "[ERROR] Unable to select.";
                    return 0;
                }

                for (size_t i = 0; i < unique_row.size(); i++)
                {
                    std::unordered_map<std::string, bool> unique_check;

                    for (const auto & val : values)
                    {
                        auto &k = val[i];
                        if (unique_check.count(k) == 0)
                            unique_check.emplace(k, false);
                        else
                        {
                            result = "[FAILED] Column " + unique_row[i] + " is UNIQUE or PRIMARY_KEY, but the insert value is not unique.";
                            return 0; 
                        }
                    }

                    for (const auto &row : select)
                    {
                        auto &k = row[i];
                        if (unique_check.count(k) == 0)
                            unique_check.emplace(k, false);
                        else
                        {
                            result = "[FAILED] Column " + unique_row[i] + " is UNIQUE or PRIMARY_KEY, but the insert value is not unique.";
                            return 0;
                        }
                    }
                }
            }
        }

        

        // insert
        std::ofstream ofile(data_path, std::ios::binary | std::ios::app);
        if (!ofile.is_open())
        {
            result = "[ERROR] Can not open data file.";
            return 0;
        }

        // for (const auto &rows : values)
        // {
        //     // delete mark
        //     {
        //         std::vector<char> mark(this->DELETE_MARK_SIZE, 0);
        //         ofile.write(reinterpret_cast<const char *>(mark.data()), sizeof(char) * this->DELETE_MARK_SIZE);
        //     }

        //     for (const auto &col : this->columns)
        //     {
        //         const auto it = column_map.find(col.name);
        //         if (it != column_map.end())
        //         {
        //             const auto &val = rows[it->second.index];
        //             insertVal(ofile, col.data_type, val, col.length);
        //         }
        //         else
        //         {
        //             // fill with default value
        //             const Constraint *default_con = nullptr;
        //             for (const auto &con : col.constraints)
        //             {
        //                 if (con.type == ConstraintType::DEFAULT)
        //                     default_con = &con;
        //             }
        //             if (default_con == nullptr)
        //             {
        //                 if (col.data_type == DataType::VARCHAR)
        //                     insertVal(ofile, col.data_type, "");
        //                 else if (col.data_type == DataType::DATE)
        //                     insertVal(ofile, col.data_type, "0000-00-00");
        //                 else if (col.data_type == DataType::TIME)
        //                     insertVal(ofile, col.data_type, "00:00:00");
        //                 else if (col.data_type == DataType::DATETIME)
        //                     insertVal(ofile, col.data_type, "0000-00-00-00:00:00");
        //                 else
        //                     insertVal(ofile, col.data_type, "0");
        //             }
        //             else
        //             {
        //                 // the DEFAULT constraint
        //             }
        //         }
        //     }
        // }

        //*****************NEW*VERSION*****************/
        //*********************************************/

        size_t remained_rows = values.size();

        std::vector<char> buffer;

        while (remained_rows > 0)
        {
            // std::vector<char> buffer(std::min(this->buffer_row_num, remained_rows) * row_length, 0);
            buffer.resize(std::min(this->buffer_row_num, remained_rows) * row_length);
            auto it = buffer.begin();

            for (size_t i = buffer.size() / this->row_length; i > 0; i--)
            {
                auto &row = values[values.size() - remained_rows];

                // delete mark
                for (size_t i = 0; i < this->DELETE_MARK_SIZE; i++)
                {
                    *it++ = 0;
                }

                for (const auto &col : columns)
                {
                    const auto it_ = column_map.find(col.name);
                    if (it_ != column_map.end())
                    {
                        const auto &val = row[it_->second.key_index];
                        it_insertVal(it, col.data_type, val, col.length);
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
                            // if (col.data_type == DataType::VARCHAR)
                            //     it_insertVal(it, col.data_type, "");
                            // else if (col.data_type == DataType::DATE)
                            //     it_insertVal(it, col.data_type, "0000-00-00");
                            // else if (col.data_type == DataType::TIME)
                            //     it_insertVal(it, col.data_type, "00:00:00");
                            // else if (col.data_type == DataType::DATETIME)
                            //     it_insertVal(it, col.data_type, "0000-00-00-00:00:00");
                            // else
                            //     it_insertVal(it, col.data_type, "0");
                        }
                        else
                        {
                            // TODO: the DEFAULT constraint
                        }
                    }
                }

                remained_rows--;
            }
            ofile.write(reinterpret_cast<const char *>(buffer.data()), sizeof(char) * buffer.size());
        }

        //*********************************************/

        ofile.close();

        result = "[SUCCESS] " + std::to_string(values.size()) + " row(s) inserted.";

        return 1;
    }

    int Table::insertV2(const std::vector<std::string> &keys, const std::vector<std::vector<std::shared_ptr<LT::LT>>> &values, std::string &result)
    {
        // legality examination
        // The amount of values each row should be the same as the keys.
        for (size_t i = 0; i < values.size(); i++)
        {
            if (values[i].size() != keys.size())
            {
                result = "[FAILED] The amount of the values in row " + std::to_string(i) + " is " + std::to_string(values[i].size()) + ", but the amount of the keys is " + std::to_string(keys.size()) + ".";
                return 0;
            }
        }

        // The keys should exist.
        std::map<const std::string, const ColSelect> column_map;
        
        {
            std::vector<const Column *> uninvolved_column;
            std::vector<bool> key_flag(keys.size(), false);
            for (size_t j = 0; j < this->columns.size(); j++)
            {
                const auto &col = this->columns[j];
                bool exist = false;
                for (size_t i = 0; i < keys.size(); i++)
                {
                    if (col.name == keys[i])
                    {
                        column_map.emplace(keys[i], ColSelect{ &col, i, j, true});
                        exist = true;
                        key_flag[i] = true;
                        break;
                    }
                }
                if (!exist)
                {
                    uninvolved_column.push_back(&col);
                    column_map.emplace(col.name, ColSelect{&col, 0, j, false});
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
        }

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
                    if (!expressionTypeExamination(*col, values[j][i]))
                    {
                        result = "[FAILED] The type of column " + keys[i] + " is " + dataTypeStr(col->data_type) + ". Row " + std::to_string(j) + " doesn't match that type.";
                        return 0;
                    }
                }
                else
                {
                    if (!expressionTypeExamination(*col, values[j][i]))
                    {
                        result = "[FAILED] The type of column " + keys[i] + " is " + dataTypeStr(col->data_type) + ", whose maximum length is " + std::to_string(col->length) + ". Row " + std::to_string(j) + " doesn't match that type.";
                        return 0;
                    }
                }
            }
        }

        // Calculate the value of the expression and generate the complete row.
        // using Data = std::variant<bool, int8_t, int16_t, int32_t, int64_t, float, double, std::string>;
        std::vector<std::vector<Data>> insert_data(values.size(), std::vector<Data>(this->columns.size()));

        for (size_t row_index = 0; row_index < values.size(); row_index++)
        {
            auto &row_expr = values[row_index];
            auto &row_data = insert_data[row_index];
            std::vector<std::shared_ptr<LT::LT>> full_expr(this->columns.size());
            for (size_t i = 0; i < this->columns.size(); i++)
            {
                auto &it = column_map.find(this->columns[i].name)->second;
                if (it.selected)
                    full_expr[i] = row_expr[it.key_index];
                else
                    full_expr[i] = nullptr;
            }
            calRowExpr(full_expr, column_map, row_data);
        }

        // unique examine


        // insert
        std::ofstream ofile(data_path, std::ios::binary | std::ios::app);
        if (!ofile.is_open())
        {
            result = "[ERROR] Can not open data file.";
            return 0;
        }

        size_t remained_rows = values.size();

        std::vector<char> buffer;

        while (remained_rows > 0)
        {
            buffer.resize(std::min(this->buffer_row_num, remained_rows) * row_length);
            auto it = buffer.begin();

            for (size_t i = buffer.size() / this->row_length; i > 0; i--)
            {
                auto &row = insert_data[values.size() - remained_rows];

                // delete mark
                for (size_t i = 0; i < this->DELETE_MARK_SIZE; i++)
                {
                    *it++ = 0;
                }

                for (size_t col_index = 0; col_index < this->columns.size(); col_index++)
                {
                    auto &col = this->columns[col_index];
                    auto &data = row[col_index];

                    it_insertVal(it, col.data_type, data, col.length);
                }

                remained_rows--;
            }
            ofile.write(reinterpret_cast<const char *>(buffer.data()), sizeof(char) * buffer.size());
        }

        ofile.close();

        result = "[SUCCESS] " + std::to_string(values.size()) + " row(s) inserted.";

        return 1;
    }

    int Table::select(const std::vector<std::string> &keys, const std::shared_ptr<LT::LT> requirements, const std::shared_ptr<SelectOrder> order, std::vector<std::vector<std::string>> &output, std::string &result) const
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
                col_vec[i].cai.key_index = i;
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
                        col.cai.key_index = i;
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
        
        // order examine
        size_t order_index = 0;
        LT::LiterType order_col_type = LT::LiterType::null;
        if (order != nullptr)
        {
            auto order_it = std::find_if(col_vec.begin(), col_vec.end(),
                                         [&](SelectInfo &si)
                                         { return si.selected && si.cai.col->name == order->key; });
            if (order_it == col_vec.end())
            {
                result = "[FAILED] The result is order by the column " + order->key + ", but that column doesn't exist / haven't been selected.";
                return 0;
            }
            order_index = order_it->cai.key_index;
            order_col_type = dataTypeToLiteralType(order_it->cai.col->data_type);
        }

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
        // while (end_of_file - ifile.tellg() >= row_length)
        // {
        //     // check the delete sign
        //     {
        //         std::vector<char> mark(this->DELETE_MARK_SIZE, 0);
        //         ifile.read(reinterpret_cast<char *>(mark.data()), sizeof(char) * this->DELETE_MARK_SIZE);
        //         if (mark[0] != 0)
        //             continue;
        //     }

        //     output_list.emplace_back(keys.size() == 0 ? col_vec.size() : keys.size());
        //     auto &list_back = output_list.back();
        //     for (size_t i = 0; i < col_vec.size(); i++)
        //     {
        //         auto &col = col_vec[i];
        //         if (!col.selected)
        //         {
        //             ifile.seekg(getColumnSize(*col.cai.col), std::ios::cur);
        //         }
        //         else
        //         {
        //             readVal(ifile, *col.cai.col, list_back[col.cai.index]);
        //         }
        //     }

        //     // TODO requirement
        // }

        //**********************NEW*VERSION***********************/
        //********************************************************/
        std::vector<char> buffer;
        while (end_of_file - ifile.tellg() >= row_length)
        {
            // std::vector<char> buffer(std::min(this->buffer_row_num * this->row_length, static_cast<size_t>(end_of_file - ifile.tellg())), 0);
            buffer.resize(std::min(this->buffer_row_num * this->row_length, static_cast<size_t>(end_of_file - ifile.tellg())));

            ifile.read(reinterpret_cast<char *>(buffer.data()), sizeof(char) * buffer.size());

            size_t row_counter = buffer.size() / this->row_length;
            auto it = buffer.begin();

            for (; row_counter > 0; row_counter--)
            {
                // check delete mark
                if (*it != 0)
                {
                    // skip one row
                    it += this->row_length;
                }
                else
                {
                    it += this->DELETE_MARK_SIZE;

                    using ColInfo = LT::ColInfo;
                    using RowInfo = LT::RowInfo;
                    std::unique_ptr<RowInfo> row_info;
                    if (requirements != nullptr)
                    {
                        row_info = std::make_unique<RowInfo>();
                        setRowInfo(*row_info, it);

                        if (!LT::boolOp(requirements, *row_info))
                        {
                            continue;
                        }
                        else
                        {
                            it -= this->row_length - static_cast<std::streampos>(this->DELETE_MARK_SIZE);
                        }
                    }

                    output_list.emplace_back(keys.size() == 0 ? col_vec.size() : keys.size());
                    auto &list_back = output_list.back();
                    for (size_t i = 0; i < col_vec.size(); i++)
                    {
                        auto &col = col_vec[i];
                        if (!col.selected)
                        {
                            it += getColumnSize(*col.cai.col);
                        }
                        else
                        {
                            it_readVal(it, *col.cai.col, list_back[col.cai.key_index]);
                        }
                    }
                }
            }
        }
        //********************************************************/


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

        // sort
        if (order != nullptr)
        {
            std::sort(output.begin() + 1, output.end(),
                      [&](std::vector<std::string> &vec1, std::vector<std::string> &vec2) {
                        switch (order_col_type)
                        {
                            case LT::LiterType::STR:
                            {
                                if (order->ascending)
                                    return vec1[order_index] < vec2[order_index];
                                else
                                    return vec1[order_index] > vec2[order_index];
                            }
                            case LT::LiterType::FLOAT:
                            {
                                double num1 = std::stod(vec1[order_index]);
                                double num2 = std::stod(vec2[order_index]);
                                if (order->ascending)
                                    return num1 < num2;
                                else
                                    return num1 > num2;
                            }
                            case LT::LiterType::INT:
                            {
                                int64_t num1 = std::stoll(vec1[order_index]);
                                int64_t num2 = std::stoll(vec2[order_index]);
                                if (order->ascending)
                                    return num1 < num2;
                                else
                                    return num1 > num2;
                            }
                            default:
                                return true;
                        }
                      });
        }


        result = "[SUCCESS] " + std::to_string(output.size() - 1) + " row(s) selected.";

        return 1;
    }

    int Table::update(const std::vector<UpdateInfo> &info, const std::shared_ptr<LT::LT> requirements, std::string &result)
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

        // unique examine
        {
            std::vector<std::string> unique_row;
            std::vector<size_t> unique_row_index;

            for (size_t i = 0; i < col_vec.size(); i++)
            {
                if (!col_vec[i].should_update)
                    continue;

                const auto &con = col_vec[i].col->constraints;
                if (std::find_if(con.begin(), con.end(), [](const Constraint constraint)
                                 { return constraint.type == ConstraintType::UNIQUE || constraint.type == ConstraintType::PRIMARY_KEY; }) != con.end())
                {
                    unique_row.emplace_back(col_vec[i].col->name);
                    unique_row_index.emplace_back(col_vec[i].index);
                }
            }
            
            if (unique_row.size() > 0)
            {
                std::vector<std::vector<std::string>> select;
                std::string str;

                if (!this->select(unique_row, requirements, nullptr, select, str))
                {
                    result = "[ERROR] Unable to select.";
                    return 0;
                }
                if (select.size() > 2)
                {
                    result = "[FAILED] Multiple rows meet the requirement. But column " + unique_row[0] + " is UNIQUE or PRIMARY_KEY.";
                    return 0;
                }

                if (!this->select(unique_row, nullptr, nullptr, select, str))
                {
                    result = "[ERROR] Unable to select.";
                    return 0;
                }

                for (size_t i = 0; i < unique_row.size(); i++)
                {
                    std::unordered_map<std::string, bool> unique_check;

                    {
                        auto &k = info[unique_row_index[i]].set;
                        if (unique_check.count(k) == 0)
                            unique_check.emplace(k, false);
                        else
                        {
                            result = "[FAILED] Column " + unique_row[i] + " is UNIQUE or PRIMARY_KEY, but the update value is not unique.";
                            return 0;
                        }
                    }

                    for (const auto &row : select)
                    {
                        auto &k = row[i];
                        if (unique_check.count(k) == 0)
                            unique_check.emplace(k, false);
                        else
                        {
                            result = "[FAILED] Column " + unique_row[i] + " is UNIQUE or PRIMARY_KEY, but the update value is not unique.";
                            return 0;
                        }
                    }
                }
            }


        }

        // update
        std::fstream file(data_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            result = "[FAILED] Can not open the data file.";
            return 0;
        }

        file.seekp(0, std::ios::end);
        auto end_of_file = file.tellp();

        file.seekp(header_length, std::ios::beg);
        // auto cur = ofile.tellp();
        size_t changed_num = 0;

        // while (end_of_file - file.tellp() >= this->row_length)
        // {
        //     // check the delete sign
        //     {
        //         std::vector<char> mark(this->DELETE_MARK_SIZE, 0);
        //         file.read(reinterpret_cast<char *>(mark.data()), sizeof(char) * this->DELETE_MARK_SIZE);
        //         if (mark[0] != 0)
        //             continue;
        //     }

        //     // TODO requirements check

        //     for (const auto &col : col_vec)
        //     {
        //         if (!col.should_update)
        //         {
        //             file.seekp(getColumnSize(*col.col), std::ios::cur);
        //             // cur = ofile.tellp();
        //             continue;
        //         }
        //         else
        //         {
        //             if (col.col->data_type != DataType::VARCHAR)
        //             {
        //                 this->insertVal(file, col.col->data_type, info[col.index].set);
        //             }
        //             else
        //             {
        //                 this->insertVal(file, col.col->data_type, info[col.index].set, col.col->length);
        //             }
        //             // cur = ofile.tellp();
        //         }
        //     }
        //     // cur = ofile.tellp();
        //     changed_num++;
        // }

        //***************************NEW*VERSION****************************/
        //******************************************************************/
        std::vector<char> buffer;
        while(end_of_file - file.tellp() >= this->row_length)
        {
            // std::vector<char> buffer(std::min(this->buffer_row_num * this->row_length, static_cast<size_t>(end_of_file - file.tellp())), 0);
            buffer.resize(std::min(this->buffer_row_num * this->row_length, static_cast<size_t>(end_of_file - file.tellp())));

            file.read(reinterpret_cast<char *>(buffer.data()), sizeof(char) * buffer.size());

            auto it = buffer.begin();

            bool update = false;

            for (size_t row_counter = buffer.size() / this->row_length; row_counter > 0; row_counter--)
            {
                // check delete sign
                if (*it != 0)
                {
                    it += this->row_length;
                    continue;
                }
                else
                {
                    it += 1;
                }

                // requirements check
                using ColInfo = LT::ColInfo;
                using RowInfo = LT::RowInfo;
                std::unique_ptr<RowInfo> row_info;
                if (requirements != nullptr)
                {
                    row_info = std::make_unique<RowInfo>();
                    setRowInfo(*row_info, it);

                    if (!LT::boolOp(requirements, *row_info))
                    {
                        continue;
                    }
                    else
                    {
                        it -= this->row_length - static_cast<std::streampos>(this->DELETE_MARK_SIZE);
                    }
                }

                for (const auto &col : col_vec)
                {
                    if (!col.should_update)
                    {
                        it += getColumnSize(*col.col);
                        continue;
                    }
                    else
                    {
                        if (col.col->data_type != DataType::VARCHAR)
                        {
                            this->it_insertVal(it, col.col->data_type, info[col.index].set);
                        }
                        else
                        {
                            this->it_insertVal(it, col.col->data_type, info[col.index].set, col.col->length);
                        }

                        update = true;
                    }
                }
                changed_num++;
            }

            if (update)
            {
                file.seekp(-sizeof(char) * buffer.size(), std::ios::cur);
                file.write(reinterpret_cast<const char *>(buffer.data()), sizeof(char) * buffer.size());
            }
        }

        //******************************************************************/

        file.close();

        result = "[SUCCESS] " + std::to_string(changed_num) + " row(s) updated.";

        return 1;
    }

    int Table::updateV2(const std::vector<std::string> &keys, const std::vector<std::shared_ptr<LT::LT>> &expr, const std::shared_ptr<LT::LT> requirements, std::string &result)
    {
        // legality examination
        // The size of expression each row should be the same as the size of keys.
        
        if (expr.size() != keys.size())
        {
            result = "[FAILED] The amount of the values is " + std::to_string(expr.size()) + ", but the amount of the keys is " + std::to_string(keys.size()) + ".";
            return 0;
        }
        

        // The keys should exist.
        std::map<const std::string, const ColSelect> column_map;
        {
            std::vector<bool> key_flag(keys.size(), false);
            for (size_t j = 0; j < this->columns.size(); j++)
            {
                const auto &col = this->columns[j];
                bool exist = false;
                for (size_t i = 0; i < keys.size(); i++)
                {
                    if (col.name == keys[i])
                    {
                        column_map.emplace(keys[i], ColSelect{&col, i, j, true});
                        exist = true;
                        key_flag[i] = true;
                        break;
                    }
                }
                if (!exist)
                {
                    column_map.emplace(col.name, ColSelect{&col, 0, j, false});
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

        // The type of the values should match the type of the keys.
        for (size_t i = 0; i < keys.size(); i++)
        {
            
            auto &col = column_map[keys[i]].col;
            if (col == nullptr)
            {
                result = "[ERROR] nullptr";
                return 0;
            }
            if (col->data_type != DataType::VARCHAR)
            {
                if (!expressionTypeExamination(*col, expr[i]))
                {
                    result = "[FAILED] The type of column " + keys[i] + " is " + dataTypeStr(col->data_type) + ". The value doesn't match that type.";
                    return 0;
                }
            }
            else
            {
                if (!expressionTypeExamination(*col, expr[i]))
                {
                    result = "[FAILED] The type of column " + keys[i] + " is " + dataTypeStr(col->data_type) + ", whose maximum length is " + std::to_string(col->length) + ". The value doesn't match that type.";
                    return 0;
                }
            }
            
        }

        // TODO: unique examine

        // update
        std::fstream file(data_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            result = "[FAILED] Can not open the data file.";
            return 0;
        }

        file.seekp(0, std::ios::end);
        auto end_of_file = file.tellp();

        file.seekp(header_length, std::ios::beg);
        size_t changed_num = 0;

        std::vector<char> buffer;
        while (end_of_file - file.tellp() >= this->row_length)
        {
            buffer.resize(std::min(this->buffer_row_num * this->row_length, static_cast<size_t>(end_of_file - file.tellp())));

            file.read(reinterpret_cast<char *>(buffer.data()), sizeof(char) * buffer.size());

            auto it = buffer.begin();

            bool update = false;

            for (size_t row_counter = buffer.size() / this->row_length; row_counter > 0; row_counter--)
            {
                // check delete sign
                if (*it != 0)
                {
                    it += this->row_length;
                    continue;
                }
                else
                {
                    it += 1;
                }

                // requirements check
                using ColInfo = LT::ColInfo;
                using RowInfo = LT::RowInfo;
                std::unique_ptr<RowInfo> row_info;
                if (requirements != nullptr)
                {
                    row_info = std::make_unique<RowInfo>();
                    setRowInfo(*row_info, it);

                    if (!LT::boolOp(requirements, *row_info))
                    {
                        continue;
                    }
                    else
                    {
                        it -= this->row_length - static_cast<std::streampos>(this->DELETE_MARK_SIZE);
                    }
                }

                // get data
                std::vector<Data> row_data(this->columns.size());
                for (size_t i = 0; i < this->columns.size(); i++)
                {
                    row_data[i] = row_info->at(i).liter.liter_info;
                }
                // update row
                // Calculate the value of the expression and generate the complete row.
                updateRowExpr(expr, column_map, row_data);
                
                // rewrite row
                for (size_t i = 0; i < this->columns.size(); i++)
                {
                    auto &col = this->columns[i];
                    this->it_insertVal(it, col.data_type, row_data[i], col.length);
                }

                update = true;
                changed_num++;
            }

            if (update)
            {
                file.seekp(-sizeof(char) * buffer.size(), std::ios::cur);
                file.write(reinterpret_cast<const char *>(buffer.data()), sizeof(char) * buffer.size());
            }
        }

        file.close();

        result = "[SUCCESS] " + std::to_string(changed_num) + " row(s) updated.";

        return 1;
    }

    int Table::delete_(const std::shared_ptr<LT::LT> requirements, std::string &result)
    {
        if (requirements == nullptr)
        {
            result = "[FAILED] The DELETE requires a WHERE statement.";
            return 0;
        }

        std::fstream file(this->data_path, std::ios::binary | std::ios::in | std::ios::out);
        if (!file.is_open())
        {
            result = "[FAILED] Can not open the data file.";
            return 0;
        }

        file.seekp(0, std::ios::end);
        auto end_of_file = file.tellp();

        file.seekp(header_length, std::ios::beg);;

        size_t delete_num = 0u;

        std::vector<char> buffer;

        while (end_of_file - file.tellp() >= this->row_length)
        {
            buffer.resize(std::min(this->buffer_row_num * this->row_length, static_cast<size_t>(end_of_file - file.tellp())));

            file.read(reinterpret_cast<char *>(buffer.data()), sizeof(char) * buffer.size());

            auto it = buffer.begin();

            bool write = false;

            for (size_t i = buffer.size() / this->row_length; i > 0; i--)
            {
                // check delete sign
                if (*it != 0)
                {
                    it += this->row_length;
                    continue;
                }
                it += this->DELETE_MARK_SIZE;

                // get row
                LT::RowInfo row_info;
                setRowInfo(row_info, it);

                if (!LT::boolOp(requirements, row_info))
                {
                    continue;
                }

                *(it - this->row_length) = 255;

                delete_num++;
                write = true;
            }

            if (write)
            {
                file.seekp(-sizeof(char) * buffer.size(), std::ios::cur);
                file.write(reinterpret_cast<const char *>(buffer.data()), sizeof(char) * buffer.size());
            }
        }

        file.close();

        result = "[SUCCESS] " + std::to_string(delete_num) + " row(s) deleted.";

        return 1;
    }

    int Table::drop(std::string &result)
    {
        std::filesystem::remove(data_path);
        result = "[SUCCESS] 1 table dropped.";
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

    const bool Table::expressionTypeExamination(const Column &col, const std::shared_ptr<LT::LT> root) const
    {
        try
        {
            switch (getLiterType(root))
            {
            case LT::LiterType::INT:
                return col.data_type == DataType::CHAR ||
                       col.data_type == DataType::INT ||
                       col.data_type == DataType::SMALLINT ||
                       col.data_type == DataType::BIGINT ||
                       col.data_type == DataType::FLOAT ||
                       col.data_type == DataType::DECIMAL;
            case LT::LiterType::FLOAT:
                return col.data_type == DataType::FLOAT ||
                       col.data_type == DataType::DECIMAL;
            case LT::LiterType::STR:
                return col.data_type == DataType::VARCHAR &&
                       col.length >= std::get<std::string>(root->info.liter.liter_info).size();
            case LT::LiterType::DATE:
                {
                    auto &str = std::get<std::string>(root->info.liter.liter_info);
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
                    return col.data_type == DataType::DATE;
                }
            case LT::LiterType::TIME:
                {
                    auto &str = std::get<std::string>(root->info.liter.liter_info);
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
                    return col.data_type == DataType::TIME;
                }
            case LT::LiterType::DATETIME:
                {
                    auto &str = std::get<std::string>(root->info.liter.liter_info);
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
                    return col.data_type == DataType::DATETIME;
                }
            default:
                return false;
            }
        }
        catch(...)
        {
            return false;
        }
    }

    const LT::LiterType Table::getLiterType(std::shared_ptr<LT::LT> node) const
    {
        switch (node->type)
        {
        case LT::NodeType::OPERATOR:
            return LT::LiterType::BOOLEAN;
        case LT::NodeType::CALCULATION:
        {
            assert(node->children.size() == 2);
            assert(getLiterType(node->children[0]) != LT::LiterType::STR && getLiterType(node->children[1]) != LT::LiterType::STR);
            assert(getLiterType(node->children[0]) != LT::LiterType::DATE && getLiterType(node->children[1]) != LT::LiterType::DATE);
            assert(getLiterType(node->children[0]) != LT::LiterType::TIME && getLiterType(node->children[1]) != LT::LiterType::TIME);
            assert(getLiterType(node->children[0]) != LT::LiterType::DATETIME && getLiterType(node->children[1]) != LT::LiterType::DATETIME);
            if (getLiterType(node->children[0]) == LT::LiterType::FLOAT || getLiterType(node->children[1]) == LT::LiterType::FLOAT)
                return LT::LiterType::FLOAT;
            else
                return LT::LiterType::INT;
        }
        case LT::NodeType::LITERAL:
            assert(node->children.size() == 0);
            try
            {
                switch (node->info.liter.liter_type)
                {
                case LT::LiterType::INT:
                    std::get<int64_t>(node->info.liter.liter_info);
                    break;
                case LT::LiterType::FLOAT:
                    std::get<double>(node->info.liter.liter_info);
                    break;
                case LT::LiterType::BOOLEAN:
                    std::get<bool>(node->info.liter.liter_info);
                case LT::LiterType::STR:
                    [[fallthrough]];
                case LT::LiterType::DATE:
                    [[fallthrough]];
                case LT::LiterType::TIME:
                    [[fallthrough]];
                case LT::LiterType::DATETIME:
                    std::get<std::string>(node->info.liter.liter_info);
                default:
                    break;
                }
            }
            catch(...)
            {
                return LT::LiterType::null;
            }
            
            return node->info.liter.liter_type;
        case LT::NodeType::COL:
        {
            auto col = std::find_if(this->columns.begin(), this->columns.end(), [&](const Column &c)
                                    { return c.name == std::get<std::string>(node->info.liter.liter_info); });
            if (col == this->columns.end())
            {
                return LT::LiterType::null;
            }
            else
            {
                return dataTypeToLiteralType(col->data_type);
            }
        }
        default:
            return LT::LiterType::null;
        }
    }

    // void Table::rowExpressionStr(std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColAndIndex> &column_map, std::vector<std::string> &result) const
    // {
    //     std::vector<std::optional<std::string>> buffer(this->columns.size());
    //     std::vector<bool> lock(this->columns.size());
    //     for (size_t index = 0; index < this->columns.size(); index++)
    //     {
    //         if (!buffer[index].has_value())
    //         {

    //             expressionStr(index, row, column_map, buffer, lock);
    //         }
    //     }

    //     result.resize(buffer.size());
    //     for (size_t i = 0; i < result.size(); i++)
    //     {
    //         result[i] = buffer[i].value();
    //     }
    // }

    // const std::string Table::expressionStr(size_t index, std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColAndIndex> &column_map, std::vector<std::optional<std::string>> &buffer, std::vector<bool> &lock) const
    // {
    //     if (buffer[index].has_value())
    //         return buffer[index].value();
    //     if (lock[index])
    //     {
    //         throw std::runtime_error("Circular dependence.");
    //     }
    //     lock[index] = true;

    //     auto it = column_map.find(this->columns[index].name);
    //     auto &str = buffer[index];
        
    //     if (it != column_map.end())
    //     {
    //         auto &val = row[it->second.index];
    //         switch(val->type)
    //         {
    //             case LT::NodeType::LITERAL:
    //                 switch (val->info.liter.liter_type)
    //                 {
    //                 case LT::LiterType::INT:
    //                     str = std::to_string(std::get<int64_t>(val->info.liter.liter_info));
    //                     break;
    //                 case LT::LiterType::FLOAT:
    //                     str = std::to_string(std::get<double>(val->info.liter.liter_info));
    //                     break;
    //                 case LT::LiterType::BOOLEAN:
    //                     str = std::get<bool>(val->info.liter.liter_info) ? "true" : "false";
    //                     break;
    //                 case LT::LiterType::STR:
    //                     [[fallthrough]];
    //                 case LT::LiterType::DATE:
    //                     [[fallthrough]];
    //                 case LT::LiterType::TIME:
    //                     [[fallthrough]];
    //                 case LT::LiterType::DATETIME:
    //                     str = std::get<std::string>(val->info.liter.liter_info);
    //                     break;
    //                 default:
    //                     str = "";
    //                     break;
    //                 }
    //                 break;
    //             case LT::NodeType::OPERATOR:
    //                 {
    //                     bool result = false;
    //                     switch (val->info.op_type)
    //                     {
    //                     case LT::OpType::EQUAL:
                            
    //                         break;
                        
    //                     default:
    //                         break;
    //                     }
    //                 }
    //                 break;
    //             case LT::NodeType::CALCULATION:
    //                 break;
    //             case LT::NodeType::COL:

    //                 break;
    //             default:
    //                 break;
    //         }
    //     }
    //     else
    //     {
    //         // fill with default value;
    //         auto &constraints = this->columns[index].constraints;
    //         auto default_con = std::find_if(constraints.begin(), constraints.end(), [&](Constraint &c)
    //                                         { c.type == ConstraintType::DEFAULT; });
    //         if (default_con != constraints.end())
    //         {
    //             str = default_con->details;
    //         }
    //         else
    //         {
    //             switch (dataTypeToLiteralType(this->columns[index].data_type))
    //             {
    //                 case LT::LiterType::INT:
    //                     str = "0";
    //                     break;
    //                 case LT::LiterType::FLOAT:
    //                     str = "0.0";
    //                     break;
    //                 case LT::LiterType::STR:
    //                     str = "";
    //                     break;
    //                 case LT::LiterType::DATE:
    //                     str = "0000-00-00";
    //                     break;
    //                 case LT::LiterType::TIME:
    //                     str = "00:00:00";
    //                     break;
    //                 case LT::LiterType::DATETIME:
    //                     str = "0000-00-00-00:00:00";
    //                     break;
    //                 default:
    //                     str = "";
    //                     break;
    //             }
    //         }
    //     }

    //     lock[index] = false;
    //     return str.value();
    // }

    void Table::calRowExpr(const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &result) const
    {
        std::vector<std::optional<Data>> buffer(this->columns.size());
        std::vector<bool> lock(this->columns.size(), false);

        for (size_t col_index = 0; col_index < this->columns.size(); col_index++)
        {
            if (!buffer[col_index].has_value())
            {
                calExpr(col_index, row[col_index], row, column_map, buffer, lock, true);
            }
        }

        result.resize(this->columns.size());
        for (size_t i = 0; i < this->columns.size(); i++)
        {
            result[i] = buffer[i].value(); 
        }
    }

    void Table::updateRowExpr(const std::vector<std::shared_ptr<LT::LT>> &row_expr, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &row_data) const
    {
        std::vector<Data> new_row(this->columns.size());
        for (size_t col_index = 0; col_index < this->columns.size(); col_index++)
        {
            auto it = column_map.find(this->columns[col_index].name)->second;
            if (it.selected)
            {
                updateExpr(col_index, row_expr[it.key_index], row_expr, column_map, row_data, new_row, true);
            }
            else
            {
                new_row[col_index] = row_data[col_index];
            }
        }

        row_data = std::move(new_row);
    }

    const Data Table::calExpr(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<std::optional<Data>> &buffer, std::vector<bool> &lock, const bool is_root) const
    {
        assert(buffer.size() == this->columns.size());
        assert(lock.size() == this->columns.size());
        assert(col_index < this->columns.size());

        if (is_root && buffer[col_index].has_value())
            return buffer[col_index].value();

        Data data;
        if (node == nullptr)
        {
            // fill with default value
            auto &constraints = this->columns[col_index].constraints;
            auto default_con = std::find_if(constraints.begin(), constraints.end(), [&](const Constraint &c)
                                           { return c.type == ConstraintType::DEFAULT; });
            if (default_con != constraints.end())
            {
                auto &detail = default_con->details;
                switch (dataTypeToLiteralType(this->columns[col_index].data_type))
                {
                case LT::LiterType::INT:
                    data.emplace<int64_t>(std::stoll(detail));
                    break;
                case LT::LiterType::FLOAT:
                    data.emplace<double>(std::stod(detail));
                    break;
                case LT::LiterType::STR:
                    data.emplace<std::string>(detail);
                    break;
                case LT::LiterType::DATE:
                    {
                        int32_t num;
                        dateStrToNum(detail, num);
                        data.emplace<int64_t>(num);
                    }
                    break;
                case LT::LiterType::TIME:
                    {
                        int32_t num;
                        timeStrToNum(detail, num);
                        data.emplace<int64_t>(num);
                    }
                    break;
                case LT::LiterType::DATETIME:
                    {
                        int64_t num;
                        datetimeStrToNum(detail, num);
                        data.emplace<int64_t>(num);
                    }
                break;
                default:
                    break;
                }
            }
            else
            {
                switch (dataTypeToLiteralType(this->columns[col_index].data_type))
                {
                    case LT::LiterType::INT:
                        data.emplace<int64_t>(0);
                        break;
                    case LT::LiterType::FLOAT:
                        data.emplace<double>(0.0);
                        break;
                    case LT::LiterType::STR:
                        data.emplace<std::string>("");
                        break;
                    case LT::LiterType::DATE:
                        [[fallthrough]];
                    case LT::LiterType::TIME:
                        [[fallthrough]];
                    case LT::LiterType::DATETIME:
                        data.emplace<int64_t>(0);
                        break;
                    default:
                        break;
                }
            }
        }
        else
        {
            switch (node->type)
            {
            case LT::NodeType::LITERAL:
                {
                    switch (node->info.liter.liter_type)
                    {
                    case LT::LiterType::INT:
                        {
                            data.emplace<int64_t>(std::get<int64_t>(node->info.liter.liter_info));
                        }
                        break;
                    case LT::LiterType::BOOLEAN:
                        {
                            data.emplace<bool>(std::get<bool>(node->info.liter.liter_info));
                        }
                        break;
                    case LT::LiterType::FLOAT:
                        {
                            data.emplace<double>(std::get<double>(node->info.liter.liter_info));
                        }
                        break;
                    case LT::LiterType::STR:
                        {
                            data.emplace<std::string>(std::get<std::string>(node->info.liter.liter_info));
                        }
                        break;
                    case LT::LiterType::DATE:
                        {
                            int32_t num;
                            dateStrToNum(std::get<std::string>(node->info.liter.liter_info), num);
                            data.emplace<int64_t>(num);
                        }
                        break;
                    case LT::LiterType::TIME:
                        {
                            int32_t num;
                            timeStrToNum(std::get<std::string>(node->info.liter.liter_info), num);
                            data.emplace<int64_t>(num);
                        }
                        break;
                    case LT::LiterType::DATETIME:
                        {
                            int64_t num;
                            datetimeStrToNum(std::get<std::string>(node->info.liter.liter_info), num);
                            data.emplace<int64_t>(num);
                        }
                        break;
                    default:
                        break;
                    }
                }
                break;
            case LT::NodeType::OPERATOR:
                {
                    switch (node->info.op_type)
                    {
                    case LT::OpType::NOT:
                        {
                            assert(node->children.size() == 1);
                            assert(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN);
                            data.emplace<bool>(!std::get<bool>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)));
                        }
                        break;
                    case LT::OpType::AND:
                        {
                            assert(node->children.size() == 2);
                            assert(node->children.size() == 2);
                            assert(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN
                                && getLiterType(node->children[1]) == LT::LiterType::BOOLEAN);
                            data.emplace<bool>(std::get<bool>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false))
                                            && std::get<bool>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                        }
                        break;
                    case LT::OpType::OR:
                        {
                            assert(node->children.size() == 2);
                            assert(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN
                                && getLiterType(node->children[1]) == LT::LiterType::BOOLEAN);
                            data.emplace<bool>(std::get<bool>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false))
                                            || std::get<bool>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                        }
                        break;
                    case LT::OpType::EQUAL:
                        {
                            data.emplace<bool>(calEqual(col_index, node, row, column_map, buffer, lock));
                        }
                        break;
                    case LT::OpType::NOT_EQUAL:
                        {
                            data.emplace<bool>(!calEqual(col_index, node, row, column_map, buffer, lock));
                        }
                        break;
                    case LT::OpType::GREATER:
                        {
                            data.emplace<bool>(calGreater(col_index, node, row, column_map, buffer, lock));
                        }
                        break;
                    case LT::OpType::LESS:
                        {
                            data.emplace<bool>(!(calGreater(col_index, node, row, column_map, buffer, lock) 
                                            || calEqual(col_index, node, row, column_map, buffer, lock)));
                        }
                        break;
                    case LT::OpType::GREATER_EQUAL:
                        {
                            data.emplace<bool>(calGreater(col_index, node, row, column_map, buffer, lock) 
                                            || calEqual(col_index, node, row, column_map, buffer, lock));
                        }
                        break;
                    case LT::OpType::LESS_EQUAL:
                        {
                            data.emplace<bool>(!calGreater(col_index, node, row, column_map, buffer, lock));
                        }
                        break;
                    default:
                        break;
                    }
                }
                break;
            case LT::NodeType::CALCULATION:
                {
                    assert(node->children.size() == 2);
                    assert(getLiterType(node->children[0]) == LT::LiterType::INT
                        || getLiterType(node->children[0]) == LT::LiterType::FLOAT);
                    assert(getLiterType(node->children[1]) == LT::LiterType::INT
                        || getLiterType(node->children[1]) == LT::LiterType::FLOAT);
                    switch (node->info.cal_type)
                    {
                    case LT::CalType::ADD:
                        {
                            auto type0 = getLiterType(node->children[0]);
                            auto type1 = getLiterType(node->children[1]);
                            if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
                            {
                                double a;
                                double b;
                                if (type0 == LT::LiterType::FLOAT)
                                    a = std::get<double>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false));
                                else
                                    a = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)));
                                if (type1 == LT::LiterType::FLOAT)
                                    b = std::get<double>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
                                else
                                    b = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                                data.emplace<double>(a + b);
                            }
                            else
                            {
                                data.emplace<int64_t>(
                                    std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)) 
                                + std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                            }
                        }
                        break;
                    case LT::CalType::MINUS:
                        {
                            auto type0 = getLiterType(node->children[0]);
                            auto type1 = getLiterType(node->children[1]);
                            if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
                            {
                                double a;
                                double b;
                                if (type0 == LT::LiterType::FLOAT)
                                    a = std::get<double>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false));
                                else
                                    a = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)));
                                if (type1 == LT::LiterType::FLOAT)
                                    b = std::get<double>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
                                else
                                    b = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                                data.emplace<double>(a - b);
                            }
                            else
                            {
                                data.emplace<int64_t>(
                                    std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)) 
                                - std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                            }
                        }
                        break;
                    case LT::CalType::MULTIPLY:
                        {
                            auto type0 = getLiterType(node->children[0]);
                            auto type1 = getLiterType(node->children[1]);
                            if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
                            {
                                double a;
                                double b;
                                if (type0 == LT::LiterType::FLOAT)
                                    a = std::get<double>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false));
                                else
                                    a = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)));
                                if (type1 == LT::LiterType::FLOAT)
                                    b = std::get<double>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
                                else
                                    b = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                                data.emplace<double>(a * b);
                            }
                            else
                            {
                                data.emplace<int64_t>(
                                    std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)) 
                                * std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                            }
                        }
                        break;
                    case LT::CalType::DIVIDE:
                        {
                            auto type0 = getLiterType(node->children[0]);
                            auto type1 = getLiterType(node->children[1]);
                            if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
                            {
                                double a;
                                double b;
                                if (type0 == LT::LiterType::FLOAT)
                                    a = std::get<double>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false));
                                else
                                    a = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)));
                                if (type1 == LT::LiterType::FLOAT)
                                    b = std::get<double>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
                                else
                                    b = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                                data.emplace<double>(a / b);
                            }
                            else
                            {
                                data.emplace<int64_t>(
                                    std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)) 
                                / std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)));
                            }
                        }
                        break;
                    case LT::CalType::MODULO:
                        {
                            assert(getLiterType(node->children[0]) == LT::LiterType::INT
                                && getLiterType(node->children[1]) == LT::LiterType::INT);
                            
                            data.emplace<int64_t>(
                                std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)) 
                                % std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
                        }
                        break;
                    default:
                        break;
                    }
                }
                break;
            case LT::NodeType::COL:
                {
                    if (lock[col_index])
                    {
                        throw std::runtime_error("Circular dependence.");
                    }
                    lock[col_index] = true;

                    // read / generate col
                    auto new_col_index = column_map.find(std::get<std::string>((node->info.liter.liter_info)))->second.col_index;
                    auto new_col_root = row[new_col_index];
                    data =  calExpr(new_col_index, new_col_root, row, column_map, buffer, lock, true);

                    lock[col_index] = false;
                }
                break;
            default:
                break;
            }
        }

        if (is_root)
            buffer[col_index] = data;
        return data;
    }

    const Data Table::updateExpr(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &origine_row, std::vector<Data> &new_row, const bool is_root) const
    {
        assert(node != nullptr);
        assert(origine_row.size() == this->columns.size());
        assert(new_row.size() == this->columns.size());
        assert(col_index < this->columns.size());

        // if (is_root && buffer[col_index].has_value())
        //     return buffer[col_index].value();

        Data data;
        switch (node->type)
        {
        case LT::NodeType::LITERAL:
        {
            switch (node->info.liter.liter_type)
            {
            case LT::LiterType::INT:
            {
                data.emplace<int64_t>(std::get<int64_t>(node->info.liter.liter_info));
            }
            break;
            case LT::LiterType::BOOLEAN:
            {
                data.emplace<bool>(std::get<bool>(node->info.liter.liter_info));
            }
            break;
            case LT::LiterType::FLOAT:
            {
                data.emplace<double>(std::get<double>(node->info.liter.liter_info));
            }
            break;
            case LT::LiterType::STR:
            {
                data.emplace<std::string>(std::get<std::string>(node->info.liter.liter_info));
            }
            break;
            case LT::LiterType::DATE:
            {
                int32_t num;
                dateStrToNum(std::get<std::string>(node->info.liter.liter_info), num);
                data.emplace<int64_t>(num);
            }
            break;
            case LT::LiterType::TIME:
            {
                int32_t num;
                timeStrToNum(std::get<std::string>(node->info.liter.liter_info), num);
                data.emplace<int64_t>(num);
            }
            break;
            case LT::LiterType::DATETIME:
            {
                int64_t num;
                datetimeStrToNum(std::get<std::string>(node->info.liter.liter_info), num);
                data.emplace<int64_t>(num);
            }
            break;
            default:
                break;
            }
        }
        break;
        case LT::NodeType::OPERATOR:
        {
            switch (node->info.op_type)
            {
            case LT::OpType::NOT:
            {
                assert(node->children.size() == 1);
                assert(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN);
                data.emplace<bool>(!std::get<bool>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)));
            }
            break;
            case LT::OpType::AND:
            {
                assert(node->children.size() == 2);
                assert(node->children.size() == 2);
                assert(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN && getLiterType(node->children[1]) == LT::LiterType::BOOLEAN);
                data.emplace<bool>(std::get<bool>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)) && std::get<bool>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
            }
            break;
            case LT::OpType::OR:
            {
                assert(node->children.size() == 2);
                assert(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN && getLiterType(node->children[1]) == LT::LiterType::BOOLEAN);
                data.emplace<bool>(std::get<bool>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)) || std::get<bool>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
            }
            break;
            case LT::OpType::EQUAL:
            {
                data.emplace<bool>(updateEqual(col_index, node, row, column_map, origine_row, new_row));
            }
            break;
            case LT::OpType::NOT_EQUAL:
            {
                data.emplace<bool>(!updateEqual(col_index, node, row, column_map, origine_row, new_row));
            }
            break;
            case LT::OpType::GREATER:
            {
                data.emplace<bool>(updateGreater(col_index, node, row, column_map, origine_row, new_row));
            }
            break;
            case LT::OpType::LESS:
            {
                data.emplace<bool>(!(updateGreater(col_index, node, row, column_map, origine_row, new_row) || updateEqual(col_index, node, row, column_map, origine_row, new_row)));
            }
            break;
            case LT::OpType::GREATER_EQUAL:
            {
                data.emplace<bool>(updateGreater(col_index, node, row, column_map, origine_row, new_row) || updateEqual(col_index, node, row, column_map, origine_row, new_row));
            }
            break;
            case LT::OpType::LESS_EQUAL:
            {
                data.emplace<bool>(!updateGreater(col_index, node, row, column_map, origine_row, new_row));
            }
            break;
            default:
                break;
            }
        }
        break;
        case LT::NodeType::CALCULATION:
        {
            assert(node->children.size() == 2);
            assert(getLiterType(node->children[0]) == LT::LiterType::INT || getLiterType(node->children[0]) == LT::LiterType::FLOAT);
            assert(getLiterType(node->children[1]) == LT::LiterType::INT || getLiterType(node->children[1]) == LT::LiterType::FLOAT);
            switch (node->info.cal_type)
            {
            case LT::CalType::ADD:
            {
                auto type0 = getLiterType(node->children[0]);
                auto type1 = getLiterType(node->children[1]);
                if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
                {
                    double a;
                    double b;
                    if (type0 == LT::LiterType::FLOAT)
                        a = std::get<double>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false));
                    else
                        a = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)));
                    if (type1 == LT::LiterType::FLOAT)
                        b = std::get<double>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false));
                    else
                        b = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
                    data.emplace<double>(a + b);
                }
                else
                {
                    data.emplace<int64_t>(
                        std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)) + std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
                }
            }
            break;
            case LT::CalType::MINUS:
            {
                auto type0 = getLiterType(node->children[0]);
                auto type1 = getLiterType(node->children[1]);
                if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
                {
                    double a;
                    double b;
                    if (type0 == LT::LiterType::FLOAT)
                        a = std::get<double>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false));
                    else
                        a = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)));
                    if (type1 == LT::LiterType::FLOAT)
                        b = std::get<double>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false));
                    else
                        b = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
                    data.emplace<double>(a - b);
                }
                else
                {
                    data.emplace<int64_t>(
                        std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)) - std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
                }
            }
            break;
            case LT::CalType::MULTIPLY:
            {
                auto type0 = getLiterType(node->children[0]);
                auto type1 = getLiterType(node->children[1]);
                if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
                {
                    double a;
                    double b;
                    if (type0 == LT::LiterType::FLOAT)
                        a = std::get<double>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false));
                    else
                        a = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)));
                    if (type1 == LT::LiterType::FLOAT)
                        b = std::get<double>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false));
                    else
                        b = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
                    data.emplace<double>(a * b);
                }
                else
                {
                    data.emplace<int64_t>(
                        std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)) * std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
                }
            }
            break;
            case LT::CalType::DIVIDE:
            {
                auto type0 = getLiterType(node->children[0]);
                auto type1 = getLiterType(node->children[1]);
                if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
                {
                    double a;
                    double b;
                    if (type0 == LT::LiterType::FLOAT)
                        a = std::get<double>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false));
                    else
                        a = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)));
                    if (type1 == LT::LiterType::FLOAT)
                        b = std::get<double>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false));
                    else
                        b = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
                    data.emplace<double>(a / b);
                }
                else
                {
                    data.emplace<int64_t>(
                        std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)) / std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
                }
            }
            break;
            case LT::CalType::MODULO:
            {
                assert(getLiterType(node->children[0]) == LT::LiterType::INT && getLiterType(node->children[1]) == LT::LiterType::INT);

                data.emplace<int64_t>(
                    std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, origine_row, new_row, false)) % std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, origine_row, new_row, false)));
            }
            break;
            default:
                break;
            }
        }
        break;
        case LT::NodeType::COL:
        {
            // read col
            auto new_col_index = column_map.find(std::get<std::string>((node->info.liter.liter_info)))->second.col_index;
            data = origine_row[new_col_index];
        }
        break;
        default:
            break;
        }

        if (is_root)
            new_row[col_index] = data;

        return data;
    }

    const bool Table::calEqual(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<std::optional<Data>> &buffer, std::vector<bool> &lock) const
    {
        assert(node != nullptr);
        assert(node->children.size() == 2);
        assert(!((getLiterType(node->children[0]) == LT::LiterType::STR && getLiterType(node->children[1]) != LT::LiterType::STR) || (getLiterType(node->children[0]) != LT::LiterType::STR && getLiterType(node->children[1]) == LT::LiterType::STR)));
        assert(!(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN || getLiterType(node->children[1]) == LT::LiterType::BOOLEAN));
        const auto type0 = getLiterType(node->children[0]);
        const auto type1 = getLiterType(node->children[1]);
        if (type0 == LT::LiterType::STR && type1 == LT::LiterType::STR)
        {
            return std::get<std::string>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)) 
                == std::get<std::string>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
        }
        else if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
        {
            double a, b;
            if (type0 == LT::LiterType::FLOAT)
                a = std::get<double>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false));
            else
                a = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)));
            if (type1 == LT::LiterType::FLOAT)
                b = std::get<double>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
            else
                b = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
            return a == b;
        }
        else
        {
            return std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false))
                == std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
        }
    }

    const bool Table::calGreater(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<std::optional<Data>> &buffer, std::vector<bool> &lock) const
    {
        assert(node != nullptr);
        assert(node->children.size() == 2);
        assert(!((getLiterType(node->children[0]) == LT::LiterType::STR && getLiterType(node->children[1]) != LT::LiterType::STR) || (getLiterType(node->children[0]) != LT::LiterType::STR && getLiterType(node->children[1]) == LT::LiterType::STR)));
        assert(!(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN || getLiterType(node->children[1]) == LT::LiterType::BOOLEAN));
        const auto type0 = getLiterType(node->children[0]);
        const auto type1 = getLiterType(node->children[1]);
        if (type0 == LT::LiterType::STR && type1 == LT::LiterType::STR)
        {
            return std::get<std::string>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)) 
                > std::get<std::string>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
        }
        else if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
        {
            double a, b;
            if (type0 == LT::LiterType::FLOAT)
                a = std::get<double>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false));
            else
                a = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false)));
            if (type1 == LT::LiterType::FLOAT)
                b = std::get<double>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
            else
                b = static_cast<double>(std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false)));
            return a > b;
        }
        else
        {
            return std::get<int64_t>(calExpr(col_index, node->children[0], row, column_map, buffer, lock, false))
                 > std::get<int64_t>(calExpr(col_index, node->children[1], row, column_map, buffer, lock, false));
        }
    }

    const bool Table::updateEqual(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &original_row, std::vector<Data> &new_row) const
    {
        assert(node != nullptr);
        assert(node->children.size() == 2);
        assert(!((getLiterType(node->children[0]) == LT::LiterType::STR && getLiterType(node->children[1]) != LT::LiterType::STR) || (getLiterType(node->children[0]) != LT::LiterType::STR && getLiterType(node->children[1]) == LT::LiterType::STR)));
        assert(!(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN || getLiterType(node->children[1]) == LT::LiterType::BOOLEAN));
        const auto type0 = getLiterType(node->children[0]);
        const auto type1 = getLiterType(node->children[1]);
        if (type0 == LT::LiterType::STR && type1 == LT::LiterType::STR)
        {
            return std::get<std::string>(updateExpr(col_index, node->children[0], row, column_map, original_row, new_row, false)) == std::get<std::string>(updateExpr(col_index, node->children[1], row, column_map, original_row, new_row, false));
        }
        else if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
        {
            double a, b;
            if (type0 == LT::LiterType::FLOAT)
                a = std::get<double>(updateExpr(col_index, node->children[0], row, column_map, original_row, new_row, false));
            else
                a = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, original_row, new_row, false)));
            if (type1 == LT::LiterType::FLOAT)
                b = std::get<double>(updateExpr(col_index, node->children[1], row, column_map, original_row, new_row, false));
            else
                b = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, original_row, new_row, false)));
            return a == b;
        }
        else
        {
            return std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, original_row, new_row, false)) == std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, original_row, new_row, false));
        }
    }

    const bool Table::updateGreater(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &original_row, std::vector<Data> &new_row) const
    {
        assert(node != nullptr);
        assert(node->children.size() == 2);
        assert(!((getLiterType(node->children[0]) == LT::LiterType::STR && getLiterType(node->children[1]) != LT::LiterType::STR) || (getLiterType(node->children[0]) != LT::LiterType::STR && getLiterType(node->children[1]) == LT::LiterType::STR)));
        assert(!(getLiterType(node->children[0]) == LT::LiterType::BOOLEAN || getLiterType(node->children[1]) == LT::LiterType::BOOLEAN));
        const auto type0 = getLiterType(node->children[0]);
        const auto type1 = getLiterType(node->children[1]);
        if (type0 == LT::LiterType::STR && type1 == LT::LiterType::STR)
        {
            return std::get<std::string>(updateExpr(col_index, node->children[0], row, column_map, original_row, new_row, false)) > std::get<std::string>(updateExpr(col_index, node->children[1], row, column_map, original_row, new_row, false));
        }
        else if (type0 == LT::LiterType::FLOAT || type1 == LT::LiterType::FLOAT)
        {
            double a, b;
            if (type0 == LT::LiterType::FLOAT)
                a = std::get<double>(updateExpr(col_index, node->children[0], row, column_map, original_row, new_row, false));
            else
                a = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, original_row, new_row, false)));
            if (type1 == LT::LiterType::FLOAT)
                b = std::get<double>(updateExpr(col_index, node->children[1], row, column_map, original_row, new_row, false));
            else
                b = static_cast<double>(std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, original_row, new_row, false)));
            return a > b;
        }
        else
        {
            return std::get<int64_t>(updateExpr(col_index, node->children[0], row, column_map, original_row, new_row, false)) > std::get<int64_t>(updateExpr(col_index, node->children[1], row, column_map, original_row, new_row, false));
        }
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
        std::streampos len = this->DELETE_MARK_SIZE;
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

    void Table::it_insertVal(std::vector<char>::iterator &it, const DataType type, const std::string &val, const size_t len)
    {
        switch (type)
        {
        case DataType::INT:
        {
            int32_t num = std::stoi(val);
            for (size_t i = 0; i < sizeof(num) * 8; i += 8)
            {
                *it++ = static_cast<char>((num >> i) & 0xFF);
            }
        }
        break;
        case DataType::SMALLINT:
        {
            int16_t num = static_cast<int16_t>(std::stoi(val));
            for (size_t i = 0; i < sizeof(num) * 8; i += 8)
            {
                *it++ = static_cast<char>((num >> i) & 0xFF);
            }
        }
        break;
        case DataType::BIGINT:
        {
            int64_t num = std::stoll(val);
            for (size_t i = 0; i < sizeof(num) * 8; i += 8)
            {
                *it++ = static_cast<char>((num >> i) & 0xFF);
            }
        }
        break;
        case DataType::FLOAT:
        {
            float num = std::stof(val);
            char *bytes = reinterpret_cast<char *>(&num);
            for (size_t i = 0; i < sizeof(float); ++i)
                *it++ = bytes[i];
        }
        break;
        case DataType::DECIMAL:
        {
            // TODO: costumize the presision and scale of the decimal
            double num = std::stod(val);
            char *bytes = reinterpret_cast<char *>(&num);
            for (size_t i = 0; i < sizeof(double); ++i)
                *it++ = bytes[i];
        }
        break;
        case DataType::CHAR:
        {
            *it++ = val[0];
        }
        break;
        case DataType::VARCHAR:
        {
            for (size_t i = 0; i < len; i++)
            {
                if (i <  val.size())
                    *it++ = val[i];
                else
                    *it++ = '\0';
            }
            
        }
        break;
        case DataType::BOOLEAN:
        {
            if (val == "true" || val == "TRUE" || val == "1")
                *it++ = 0xFF;
            else if (val == "false" || val == "FALSE" || val == "0")
                *it++ = 0x00;
        }
        break;
        case DataType::DATE:
        {
            int32_t date;
            dateStrToNum(val, date);
            for (size_t i = 0; i < sizeof(date) * 8; i += 8)
            {
                *it++ = static_cast<char>((date >> i) & 0xFF);
            }
        }
        break;
        case DataType::TIME:
        {
            int32_t time;
            timeStrToNum(val, time);
            for (size_t i = 0; i < sizeof(time) * 8; i += 8)
            {
                *it++ = static_cast<char>((time >> i) & 0xFF);
            }
        }
        break;
        case DataType::DATETIME:
        {
            int64_t datetime = 0;
            datetimeStrToNum(val, datetime);
            for (size_t i = 0; i < sizeof(datetime) * 8; i += 8)
            {
                *it++ = static_cast<char>((datetime >> i) & 0xFF);
            }
        }
        break;
        default:
            break;
        }
    }

    void Table::it_insertVal(std::vector<char>::iterator &it, const DataType type, const Data &val, const size_t len)
    {
        switch (type)
        {
        case DataType::INT:
        {
            int32_t num = static_cast<decltype(num)>(std::get<int64_t>(val));
            for (size_t i = 0; i < sizeof(num) * 8; i += 8)
            {
                *it++ = static_cast<char>((num >> i) & 0xFF);
            }
        }
        break;
        case DataType::SMALLINT:
        {
            int16_t num = static_cast<decltype(num)>(std::get<int64_t>(val));
            for (size_t i = 0; i < sizeof(num) * 8; i += 8)
            {
                *it++ = static_cast<char>((num >> i) & 0xFF);
            }
        }
        break;
        case DataType::BIGINT:
        {
            int64_t num = static_cast<decltype(num)>(std::get<int64_t>(val));
            for (size_t i = 0; i < sizeof(num) * 8; i += 8)
            {
                *it++ = static_cast<char>((num >> i) & 0xFF);
            }
        }
        break;
        case DataType::FLOAT:
        {
            float num = static_cast<decltype(num)>(std::get<double>(val));
            char *bytes = reinterpret_cast<char *>(&num);
            for (size_t i = 0; i < sizeof(float); ++i)
                *it++ = bytes[i];
        }
        break;
        case DataType::DECIMAL:
        {
            // TODO: costumize the presision and scale of the decimal
            double num = static_cast<decltype(num)>(std::get<double>(val));
            char *bytes = reinterpret_cast<char *>(&num);
            for (size_t i = 0; i < sizeof(double); ++i)
                *it++ = bytes[i];
        }
        break;
        case DataType::CHAR:
        {
            *it++ = static_cast<char>(std::get<int64_t>(val));
        }
        break;
        case DataType::VARCHAR:
        {
            auto &str = std::get<std::string>(val);
            for (size_t i = 0; i < len; i++)
            {
                if (i < str.size())
                    *it++ = str[i];
                else
                    *it++ = '\0';
            }
        }
        break;
        case DataType::BOOLEAN:
        {
            if (std::get<bool>(val))
                *it++ = 0xFF;
            else
                *it++ = 0x00;
        }
        break;
        case DataType::DATE:
        {
            int32_t date = static_cast<int32_t>(std::get<int64_t>(val));
            for (size_t i = 0; i < sizeof(date) * 8; i += 8)
            {
                *it++ = static_cast<char>((date >> i) & 0xFF);
            }
        }
        break;
        case DataType::TIME:
        {
            int32_t time = static_cast<int32_t>(std::get<int64_t>(val));
            for (size_t i = 0; i < sizeof(time) * 8; i += 8)
            {
                *it++ = static_cast<char>((time >> i) & 0xFF);
            }
        }
        break;
        case DataType::DATETIME:
        {
            int64_t datetime = static_cast<int64_t>(std::get<int64_t>(val));
            for (size_t i = 0; i < sizeof(datetime) * 8; i += 8)
            {
                *it++ = static_cast<char>((datetime >> i) & 0xFF);
            }
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

    void Table::it_readVal(std::vector<char>::iterator &it, const Column &col, std::string &output)
    {
        switch (col.data_type)
        {
        case DataType::INT:
        {
            int32_t buffer = 0;
            for (size_t i = 0; i < sizeof(buffer) * 8; i += 8)
            {
                buffer = buffer | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
            output = std::to_string(buffer);
        }
        break;
        case DataType::SMALLINT:
        {
            int16_t buffer = 0;
            for (size_t i = 0; i < sizeof(buffer) * 8; i += 8)
            {
                buffer = buffer | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
            output = std::to_string(buffer);
        }
        break;
        case DataType::BIGINT:
        {
            int64_t buffer = 0;
            for (size_t i = 0; i < sizeof(buffer) * 8; i += 8)
            {
                buffer = buffer | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
            output = std::to_string(buffer);
        }
        break;
        case DataType::FLOAT:
        {
            float buffer = 0.f;
            char *bytes = reinterpret_cast<char *>(&buffer);
            for (size_t i = 0; i < sizeof(buffer); ++i)
            {
                bytes[i] = *it++;
            }
            output = std::to_string(buffer);
        }
        break;
        case DataType::DECIMAL:
        {
            // TODO: costumize the presision and scale of the decimal
            double buffer = 0;
            char *bytes = reinterpret_cast<char *>(&buffer);
            for (size_t i = 0; i < sizeof(buffer); ++i)
            {
                bytes[i] = *it++;
            }
            output = std::to_string(buffer);
        }
        break;
        case DataType::CHAR:
        {
            output = std::to_string(*it++);
        }
        break;
        case DataType::VARCHAR:
        {
            std::vector<char> buffer(it, it + col.length);
            auto end = buffer.rbegin();
            while (*end == '\0' && end != buffer.rend())
            {
                end++;
            }
            output.assign(buffer.begin(), end.base());
            it += col.length;
        }
        break;
        case DataType::BOOLEAN:
        {
            if (*it++ == 0x00)
                output = "false";
            else
                output = "true";
        }
        break;
        case DataType::DATE:
        {
            int32_t buffer = 0;
            for (size_t i = 0; i < sizeof(buffer) * 8; i += 8)
            {
                buffer = buffer | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
            dateNumToStr(buffer, output);
        }
        break;
        case DataType::TIME:
        {
            int32_t buffer = 0;
            for (size_t i = 0; i < sizeof(buffer) * 8; i += 8)
            {
                buffer = buffer | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
            timeNumToStr(buffer, output);
        }
        break;
        case DataType::DATETIME:
        {
            int64_t buffer = 0;
            for (size_t i = 0; i < sizeof(buffer) * 8; i += 8)
            {
                buffer = buffer | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
            datetimeNumToStr(buffer, output);
        }
        break;
        default:
            break;
        }
    }

    void Table::it_rawReadInt(std::vector<char>::iterator &it, const Column &col, int64_t &output)
    {
        output = 0;
        switch (col.data_type)
        {
        case DataType::INT:
        {
            for (size_t i = 0; i < sizeof(int32_t) * 8; i += 8)
            {
                output = output | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
        }
        break;
        case DataType::SMALLINT:
        {
            for (size_t i = 0; i < sizeof(int16_t) * 8; i += 8)
            {
                output = output | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
        }
        break;
        case DataType::BIGINT:
        {
            for (size_t i = 0; i < sizeof(int64_t) * 8; i += 8)
            {
                output = output | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
        }
        break;
        case DataType::CHAR:
        {
            output = *it++;
        }
        break;
        case DataType::DATE:
        {
            for (size_t i = 0; i < sizeof(int32_t) * 8; i += 8)
            {
                output = output | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
        }
        break;
        case DataType::TIME:
        {
            for (size_t i = 0; i < sizeof(int32_t) * 8; i += 8)
            {
                output = output | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
        }
        break;
        case DataType::DATETIME:
        {
            for (size_t i = 0; i < sizeof(int64_t) * 8; i += 8)
            {
                output = output | ((static_cast<decltype(buffer_row_num)>(*it++) & 0xFF) << i);
            }
        }
        break;
        default:
            assert(false);
            break;
        }
    }
    void Table::it_rawReadFloat(std::vector<char>::iterator &it, const Column &col, double &output)
    {
        switch (col.data_type)
        {
        case DataType::FLOAT:
        {
            float buffer = 0.f;
            char *bytes = reinterpret_cast<char *>(&buffer);
            for (size_t i = 0; i < sizeof(buffer); ++i)
            {
                bytes[i] = *it++;
            }
            output = static_cast<double>(buffer);
        }
        break;
        case DataType::DECIMAL:
        {
            // TODO: costumize the presision and scale of the decimal
            output = 0;
            char *bytes = reinterpret_cast<char *>(&output);
            for (size_t i = 0; i < sizeof(output); ++i)
            {
                bytes[i] = *it++;
            }
        }
        default:
            assert(false);
            break;
        }
    }
    void Table::it_rawReadStr(std::vector<char>::iterator &it, const Column &col, std::string &output)
    {
        if (col.data_type == DataType::VARCHAR)
        {
            std::vector<char> buffer(it, it + col.length);
            auto end = buffer.rbegin();
            while (*end == '\0' && end != buffer.rend())
            {
                end++;
            }
            output.assign(buffer.begin(), end.base());
            it += col.length;
        }
        else
        {
            assert(false);
        }
    }

    void Table::it_rawReadBool(std::vector<char>::iterator &it, const Column &col, bool &output)
    {
        if (col.data_type == DataType::BOOLEAN)
        {
            if (*it++ == 0x00)
                output = false;
            else
                output = true;
        }
        else
        {
            assert(false);
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

    void Table::datetimeStrToNum(const std::string &str, int64_t &num)
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

    void Table::datetimeNumToStr(const int64_t &num, std::string &str)
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

    void Table::setRowInfo(LT::RowInfo &row_info, std::vector<char>::iterator &it) const
    {
        row_info.resize(this->columns.size());
        for (size_t i = 0; i < this->columns.size(); i++)
        {
            row_info.at(i).col_name = columns[i].name;
            row_info.at(i).col_type = columns[i].data_type;
            switch (columns[i].data_type)
            {
            case DataType::INT:
                [[fallthrough]];
            case DataType::SMALLINT:
                [[fallthrough]];
            case DataType::BIGINT:
                [[fallthrough]];
            case DataType::CHAR:
                [[fallthrough]];
            case DataType::DATE:
                [[fallthrough]];
            case DataType::TIME:
                [[fallthrough]];
            case DataType::DATETIME:
            {
                row_info.at(i).liter.liter_type = LT::LiterType::INT;
                int64_t temp;
                it_rawReadInt(it, columns[i], temp);
                row_info.at(i).liter.liter_info.emplace<int64_t>(temp);
            }
            break;
            case DataType::FLOAT:
                [[fallthrough]];
            case DataType::DECIMAL:
            {
                row_info.at(i).liter.liter_type = LT::LiterType::FLOAT;
                double temp;
                it_rawReadFloat(it, columns[i], temp);
                row_info.at(i).liter.liter_info.emplace<double>(temp);
            }
            break;
            case DataType::VARCHAR:
            {
                row_info.at(i).liter.liter_type = LT::LiterType::STR;
                std::string temp;
                it_rawReadStr(it, columns[i], temp);
                row_info.at(i).liter.liter_info.emplace<std::string>(std::move(temp));
            }
            break;
            case DataType::BOOLEAN:
            {
                row_info.at(i).liter.liter_type = LT::LiterType::BOOLEAN;
                bool temp;
                it_rawReadBool(it, columns[i], temp);
                row_info.at(i).liter.liter_info.emplace<bool>(temp);
            }
            break;
            default:
                break;
            }
        }
    }
}