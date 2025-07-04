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

#pragma once

#include <struct.hpp>

namespace HydroSQL::Server::Engine
{
    
    enum class ConstraintType : char
    {
        PRIMARY_KEY = 0,
        NOT_NULL,
        UNIQUE,
        DEFAULT
    };

    /**
     * @brief Constraint of column
     * 
     */
    struct Constraint
    {
        ConstraintType type;
        std::string details;

        Constraint() : type(ConstraintType::PRIMARY_KEY)
        {
        }
        Constraint(const ConstraintType &type_, const char *details_)
            : type(type_), details(details_)
        {
        }
        Constraint(const ConstraintType &type_, const std::string &details_)
            : type(type_), details(details_)
        {
        }

        void operator=(const Constraint &other) = delete;

        friend std::ostream &operator<<(std::ostream &os, const Constraint &constraint);
        friend std::istream &operator>>(std::istream &is, Constraint &constraint);
    };

    /**
     * @brief Column of table
     * 
     */
    struct HYDROSQL_ENGINE_API Column
    {
        std::string name;
        DataType data_type;
        unsigned int length;    // for VARCHAR
        unsigned int precision; // TODO: for DECIMAL
        unsigned int scale;     // TODO: for DECIMAL
        std::string default_value;  // for constraint DEFAULT
        std::vector<Constraint> constraints;

        Column() : data_type(DataType::INT), length(1), precision(0), scale(0)
        {
        }
        Column(const std::string &name_, DataType type_, int len_ = 1)
            : name(name_), data_type(type_), length(len_), precision(0), scale(0)
        {
        }

        void operator=(const Column &other) = delete;

        /**
         * @brief save or load a column into/from a stream object(std::fstream)
         * 
         */
        friend std::ostream &operator<<(std::ostream &os, const Column &col);
        friend std::istream &operator>>(std::istream &is, Column &col);
    };

    struct SelectOrder
    {
        std::string key;
        bool ascending; // True: ascending; False: decreasing
    };

    // Abort
    struct UpdateInfo
    {
        std::string col_name;
        std::string set;
    };
    
    /**
     * @brief convert a enum type into its base type
     * 
     * @tparam Enum the name of the enum type
     * @return std::underlying_type<Enum>::type 
     */
    template<typename Enum>
    typename std::underlying_type<Enum>::type enumToBase(Enum data)
    {
        return static_cast<std::underlying_type<Enum>::type>(data);
    }

    /**
     * @brief save a std::string into a stream object (std::fstream)
     * 
     */
    void saveStr(std::ostream &os, const std::string &str);
    /**
     * @brief load a std::string from a stream object (std::fstream)
     *
     */
    void loadStr(std::istream &is, std::string &str);
    
    /**
     * @brief save a std::vector into a stream object (std::fstream)
     * 
     * @tparam T The type that std::vector contain.
     */
    template<typename T>
    void saveVector(std::ostream &os, const std::vector<T> &vec)
    {
        size_t size = vec.size();
        os.write(reinterpret_cast<const char *>(&size), sizeof(size));
        for (const auto &mem : vec)
        {
            os << mem;
        }
    }

    template<typename T>
    void loadVector(std::istream &is, std::vector<T> &vec)
    {
        size_t size;
        is.read(reinterpret_cast<char *>(&size), sizeof(size));
        vec.resize(size);
        for (auto &mem : vec)
        {
            is >> mem;
        }
    }

    /**
     * @brief get the name of a data type as string
     * 
     * @return const char* 
     */
    const char *dataTypeStr(const DataType type);

    /**
     * @brief get the name of a constraint type as string
     *
     * @return const char*
     */
    const char *constraintTypeStr(const ConstraintType type);

    // Save or load a constraint
    std::ostream &operator<<(std::ostream &os, const Constraint &constraint);
    std::istream &operator>>(std::istream &is, Constraint &constraint);

    // Save or load a column
    std::ostream &operator<<(std::ostream &os, const Column &col);
    std::istream &operator>>(std::istream &is, Column &col);

    // Get the literal type of the data type
    LT::LiterType dataTypeToLiteralType(const DataType type);

    /**
     * @brief SQL table
     * 
     */
    class HYDROSQL_ENGINE_API Table
    {
    private:
        struct ColAndIndex
        {
            const Column *col;
            size_t key_index;
            size_t col_index;
        };

        struct ColSelect
        {
            const Column *col;
            size_t key_index;
            size_t col_index;
            bool selected;

            ColSelect(const Column *col_ = nullptr, const size_t ki = 0, const size_t ci = 0, const bool s = false)
                : col(col_), key_index(ki), col_index(ci), selected(s)
            {}
        };

        // table name
        std::string name;
        
        // all columns
        std::vector<Column> columns;

        // the header stores the name of the table and infomations of columns
        std::streampos header_length;

        // the length of one row
        std::streampos row_length;

        // database path
        std::filesystem::path data_path;
    
        static constexpr size_t DELETE_MARK_SIZE = sizeof(char);
        static constexpr size_t MAX_BUFFER_SIZE = 4 * 1024; // 4kb

        size_t buffer_row_num;

    public:
        /**
         * @brief Construct a new Table object and load is columns from database.
         * 
         * @param name_ table name
         * @note The database should exist.
         */
        Table(const std::string &name_);
        /**
         * @brief Construct a new Table object and create a database.
         * 
         */
        Table(const std::string &name_, const std::vector<Column> &&columns_);

        /**
         * @note Table should not be copy/move.
         * 
         */
        Table(const Table &other) = delete;
        Table(const Table &&other) = delete;
        void operator=(const Table &other) = delete;

        /**
         * @brief insert some rows into the table
         *
         * @param keys the columns
         * @param values The values. The inner vector represents a row. The values in a certain row should match the keys.
         * @param result The result (or error message) to be outputed.
         * @return int 0 for failed and 1 for succeeded
         */
        [[nodiscard]] int insert(const std::vector<std::string> &keys, const std::vector<std::vector<std::string>> &values, std::string &result);

        /**
         * @brief A better insert that support expression.
         * 
         */
        [[nodiscard]] int insertV2(const std::vector<std::string> &keys, const std::vector<std::vector<std::shared_ptr<LT::LT>>> &values, std::string &result);

        /**
         * @brief select some rows in the table
         *
         * @param keys The columns which will be displayed. An empty vector represent "*", which  means all columns.
         * @param requirements The requirements that the rows should meet. A tree.
         * @param order The order of the output rows.
         * @param output The selected rows in the right order.
         * @param result The result (or error message) to be outputed.
         * @return int 0 for failed and 1 for succeeded
         */
        [[nodiscard]] int select(const std::vector<std::string> &keys, const std::shared_ptr<LT::LT> requirements, const std::shared_ptr<SelectOrder> order, std::vector<std::vector<std::string>> &output, std::string &result) const;

        /**
         * @brief Update columns in the table.
         *
         * @param info The columns that should be update and the new value of that column.
         * @param requirements The WHERE statement.
         * @param result The result (or error message) to be outputed.
         * @return int 0 for failed and 1 for succeeded
         */
        [[nodiscard]] int update(const std::vector<UpdateInfo> &info, const std::shared_ptr<LT::LT> requirements, std::string &result);

        /**
         * @brief Better update that support expression.
         * 
         */
        [[nodiscard]] int updateV2(const std::vector<std::string> &keys, const std::vector<std::shared_ptr<LT::LT>> &expr, const std::shared_ptr<LT::LT> requirements, std::string &result);

        /**
         * @brief Delete rows in the table.
         *
         * @param requirements The WHERE statement.
         * @param result The result (or error message) to be outputed.
         * @return int 0 for failed and 1 for succeeded
         * 
         * @note Will not actually DELETE the rows, but mark them with a delete mark.
         */
        [[nodiscard]] int delete_(const std::shared_ptr<LT::LT> requirements, std::string &result);

        /**
         * @brief Delete the table. ACTUALLY, FOREVER.
         *
         * @param result The result (or error message) to be outputed.
         * @return int 0 for failed and 1 for succeeded
         */
        [[nodiscard]] int drop(std::string &result);

    private:
        /**
         * @brief Examine whether a string can be interpret into the data type that the column required.
         * 
         */
        [[nodiscard]] static const bool dataTypeExamination(const DataType type, const std::string &str, const size_t varchar_length);

        /**
         * @brief Examine whether an expression can be interpret into the data type that the column required.
         *
         */
        [[nodiscard]] const bool expressionTypeExamination(const Column &col, const std::shared_ptr<LT::LT> root) const;

        /**
         * @brief Deduce the final data type of an expression.
         * 
         * @param node The root of that expression.
         */
        [[nodiscard]] const LT::LiterType getLiterType(std::shared_ptr<LT::LT> node) const;

        /**
         * @brief Calculate all the expression (including default value) and fill a row.
         * 
         * @note Use dynamic programming to reduce the calculation.
         */
        void calRowExpr(const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &result) const;

        /**
         * @brief Calculate one expression.
         *
         * @param lock Used to detect circular dependence. When a circular dependence happen (colA = colB, colB = colA), this function will enter with the lock locked (which means the certain member in the vector is setted as true).
         * @note This function will be lauched by the function calRowExpr.
         */
        const Data calExpr(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<std::optional<Data>> &buffer, std::vector<bool> &lock, const bool is_root) const;

        /**
         * @brief Calculate a equal expression.
         * 
         */
        const bool calEqual(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<std::optional<Data>> &buffer, std::vector<bool> &lock) const;

        /**
         * @brief Calculate a greater-than expression.
         * 
         */
        const bool calGreater(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<std::optional<Data>> &buffer, std::vector<bool> &lock) const;

        /**
         * @brief Calculate all the expression (including default value) and fill a row.
         *
         * @note What's defferent for calRowExpr is that in calRowExpr (called in insertV2), there is an empty row, but in this function, the row has initial valued.
         */
        void updateRowExpr(const std::vector<std::shared_ptr<LT::LT>> &row_expr, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &row_data) const;

        // The following has the similar functionality as the calXX version.
        const Data updateExpr(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &origine_row, std::vector<Data> &new_row, const bool is_root) const;
        const bool updateEqual(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &original_row, std::vector<Data> &new_row) const;
        const bool updateGreater(const size_t col_index, const std::shared_ptr<LT::LT> node, const std::vector<std::shared_ptr<LT::LT>> &row, const std::map<const std::string, const ColSelect> &column_map, std::vector<Data> &original_row, std::vector<Data> &new_row) const;

        /**
         * @brief Examine continuous character in the string.
         */
        [[nodiscard]] static const bool continuousNumExamination(const std::string &str, const size_t begin, const size_t length);

        /**
         * @brief Get the size of each data stucture in bytes.
         * 
         */
        [[nodiscard]] static const size_t getDataTypeSize(const DataType type);

        /**
         * @brief Get the column size in bytes
         * 
         * @note The size of column is consist of its name and its constrains.
         */
        [[nodiscard]] static const size_t getColumnSize(const Column &col);

        /**
         * @brief Calculate the size of a row in bytes.
         * 
         * @note The size of the row is the sum of the size of data type of columns.
         */
        [[nodiscard]] const std::streampos calRowLen() const;

        // [ABORT]
        [[deprecated]] static void insertVal(std::ostream &os, const DataType type, const std::string &val, const size_t len = 1);
        // new version using iterator
        /**
         * @brief Insert a value into a std::vector which act as buffer.
         * 
         */
        static void it_insertVal(std::vector<char>::iterator &it, const DataType type, const std::string &val, const size_t len = 1);
        static void it_insertVal(std::vector<char>::iterator &it, const DataType type, const Data &val, const size_t len = 1);

        // [ABORT]
        [[deprecated]] static void readVal(std::istream &is, const Column &col, std::string &output);
        // new version using iterator
        /**
         * @brief Read a value from a std::vector which act as buffer.
         * 
         */
        static void it_readVal(std::vector<char>::iterator &it, const Column &col, std::string &output);
        static void it_rawReadInt(std::vector<char>::iterator &it, const Column &col, int64_t &output);
        static void it_rawReadFloat(std::vector<char>::iterator &it, const Column &col, double &output);
        static void it_rawReadStr(std::vector<char>::iterator &it, const Column &col, std::string &output);
        static void it_rawReadBool(std::vector<char>::iterator &it, const Column &col, bool &output);

        // WARNING: The following six fuctions will not examine the legality of the input in order to be faster. Illegal input may cause the program to CRASH. Use a try and catch block if your are not sure whether the input is legal.
        static void dateStrToNum(const std::string &str, int32_t &num);
        static void dateNumToStr(const int32_t &num, std::string &str);
        static void timeStrToNum(const std::string &str, int32_t &num);
        static void timeNumToStr(const int32_t &num, std::string &str);
        static void datetimeStrToNum(const std::string &str, int64_t &num);
        static void datetimeNumToStr(const int64_t &num, std::string &str);

        void setRowInfo(LT::RowInfo &row_info, std::vector<char>::iterator &it) const;
    };
}