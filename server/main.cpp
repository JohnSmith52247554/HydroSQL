/**
 * @file main.cpp
 * @author username (username52247554@gmail.com)
 * @brief main source file
 * @version 0.1
 * @date 2025-06-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <engine/include/table.hpp>

using namespace YourSQL::Server;

int main()
{
    std::cout << "INT" << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::INT, L"1", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::INT, L"1.1", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::INT, L"abc", 0) << std::endl;

    std::cout << "FLOAT" << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::FLOAT, L"1", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::FLOAT, L"1.1", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::FLOAT, L"Abc", 0) << std::endl;

    std::cout << "CHAR" << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::CHAR, L"a", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::CHAR, L"0", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::CHAR, L"Abc", 0) << std::endl;

    std::cout << "BOOLEAN" << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::BOOLEAN, L"true", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::BOOLEAN, L"FALSE", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::BOOLEAN, L"A1", 0) << std::endl;

    std::cout << "DATE" << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::DATE, L"2025-06-13", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::DATE, L"1978-02-30", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::DATE, L"22-2-2", 0) << std::endl;

    std::cout << "TIME" << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::TIME, L"21:19:01", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::TIME, L"20:00:00", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::TIME, L"2:1:1", 0) << std::endl;

    std::cout << "DATATIME" << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::DATETIME, L"2024-06-13-21:19:01", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::DATETIME, L"2222-22-22-20:00:00", 0) << std::endl;
    std::cout << Engine::Table::dataTypeExamination(Engine::DataType::DATETIME, L"21-2-3-2:1:1", 0) << std::endl;

    return 0;
}