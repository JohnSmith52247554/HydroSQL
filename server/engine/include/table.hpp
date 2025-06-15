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

namespace YourSQL::Server::Engine
{
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

    void saveStr(std::ostream &os, const std::wstring &str);
    void loadStr(std::istream &is, std::wstring &str);
    
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

    enum class DataType : char
    {
        INT = 0,
        SMALLINT,
        BIGINT,
        FLOAT,
        DECIMAL,
        CHAR,
        VARCHAR,
        BOOLEAN,
        DATE,
        TIME,
        DATETIME,
    };

    /**
     * @brief get the name of a data type as string
     * 
     * @return const char* 
     */
    const wchar_t *dataTypeStr(const DataType type);

    enum class ConstraintType : char
    {
        PRIMARY_KEY = 0,
        NOT_NULL,
        UNIQUE,
        CHECK,
        DEFAULT
    };

    /**
     * @brief get the name of a constraint type as string
     *
     * @return const char*
     */
    const wchar_t *constraintTypeStr(const ConstraintType type);

    struct Constraint
    {
        ConstraintType type;
        std::wstring details;

        Constraint() : type(ConstraintType::PRIMARY_KEY)
        {}
        Constraint(const ConstraintType &type_, const wchar_t *details_)
            : type(type_), details(details_)
        {}
        Constraint(const ConstraintType &type_, const std::wstring &details_)
            : type(type_), details(details_)
        {}

        // Constraint(const Constraint &other) = delete;
        // Constraint(const Constraint &&other) = delete;
        void operator=(const Constraint &other) = delete;

        friend std::ostream &operator<<(std::ostream &os, const Constraint &constraint);
        friend std::istream &operator>>(std::istream &is, Constraint &constraint);
    };

    std::ostream &operator<<(std::ostream &os, const Constraint &constraint);
    std::istream &operator>>(std::istream &is, Constraint &constraint);

    struct Column
    {
        std::wstring name;
        DataType data_type;
        unsigned int length;    // for VARCHAR
        unsigned int precision; // for DECIMAL
        unsigned int scale;     // for DECIMAL
        std::wstring default_value;
        std::vector<Constraint> constraints;

        Column() : data_type(DataType::INT), length(1), precision(0), scale(0)
        {}
        Column(const std::wstring &name_, DataType type_, int len_ = 1)
            : name(name_), data_type(type_), length(len_), precision(0), scale(0)
        {}

        // Column(const Column &other) = delete;
        // Column(const Column &&other) = delete;
        void operator=(const Column &other) = delete;

        friend std::ostream &operator<<(std::ostream &os, const Column &col);
        friend std::istream &operator>>(std::istream &is, Column &col);
    };

    std::ostream &operator<<(std::ostream &os, const Column &col);
    std::istream &operator>>(std::istream &is, Column &col);

    struct SelectOrder
    {
        std::wstring key;
        bool ascending;
    };

    struct UpdateInfo
    {
        std::wstring col_name;
        std::wstring set;
    };

    class Table
    {
    private:
        struct ColAndIndex
        {
            const Column *col;
            size_t index;
        };
        
        std::wstring name;
        std::vector<Column> columns;

        // the header stores the name of the table and infomations of columns
        std::streampos header_length;

        // the length of one row
        std::streampos row_length;

        std::filesystem::path data_path;

    public:
        Table(const std::wstring &name_);
        Table(const std::wstring &name_, const std::vector<Column> &&columns_);

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
        [[nodiscard]] int insert(const std::vector<std::wstring> &keys, const std::vector<std::vector<std::wstring>> &values, std::wstring &result);

        /**
         * @brief select some rows in the table
         *
         * @param keys The columns which will be displayed. An empty vector represent "*", which  means all columns.
         * @param requirements TODO: The requirements that the rows should meet. A tree.
         * @param order The order of the output rows.
         * @param output The selected rows in the right order.
         * @param result The result (or error message) to be outputed.
         * @return int 0 for failed and 1 for succeeded
         */
        [[nodiscard]] int select(const std::vector<std::wstring> &keys, const bool &requirements, const SelectOrder &order, std::vector<std::vector<std::wstring>> &output, std::wstring &result) const;

        [[nodiscard]] int update(const std::vector<UpdateInfo> &info, const bool &requirements, std::wstring &result);

        [[nodiscard]] int delete_();

    private:
        [[nodiscard]] static const bool dataTypeExamination(const DataType type, const std::wstring &str, const size_t varchar_length);

        [[nodiscard]] static const bool continuousNumExamination(const std::wstring &str, const size_t begin, const size_t length);

        [[nodiscard]] static const size_t getDataTypeSize(const DataType type);
        [[nodiscard]] static const size_t getColumnSize(const Column &col);

        [[nodiscard]] const std::streampos calRowLen() const;

        static void insertVal(std::ostream &os, const DataType type, const std::wstring &val, const size_t len = 1);

        static void readVal(std::istream &is, const Column &col, std::wstring &output);

        // WARNING: The following six fuctions will not examine the legality of the input in order to be faster. Illegal input may cause the program to CRASH. Use a try and catch block if your are not sure whether the input is legal.
        static void dateStrToNum(const std::wstring &str, int32_t &num);
        static void dateNumToStr(const int32_t &num, std::wstring &str);
        static void timeStrToNum(const std::wstring &str, int32_t &num);
        static void timeNumToStr(const int32_t &num, std::wstring &str);
        static void datetimeStrToNum(const std::wstring &str, int32_t &num);
        static void datetimeNumToStr(const int32_t &num, std::wstring &str);
    };
}