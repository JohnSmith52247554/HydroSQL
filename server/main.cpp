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
    // std::vector<Engine::Column> cols = {
    //     Engine::Column{L"index", Engine::DataType::INT},
    //     Engine::Column{L"name", Engine::DataType::VARCHAR, 10},
    //     Engine::Column{L"age", Engine::DataType::SMALLINT},
    // };

    // Engine::Table table(L"test", std::move(cols));

    // std::vector<std::wstring> keys = {L"name", L"index", L"age" };
    // std::vector<std::vector<std::wstring>> vals = {
    //     { L"zhangsan", L"1", L"20" },
    //     { L"lisi", L"2", L"21" },
    //     { L"wangwu", L"3", L"22" },
    //     { L"zaoliu", L"4", L"23" }
    // };
    // std::wstring result;

    // if(!table.insert(keys, vals, result))
    // {
    //     std::cout << "error" << std::endl;
    // }

    // std::wcout << result << std::endl;

    Engine::Table table(L"test");

    std::wstring result;
    std::vector<std::wstring> keys = { L"name" };
    std::vector<std::vector<std::wstring>> output;

    table.select(keys, false, Engine::SelectOrder(L"", false), output, result);

    for (const auto &row : output)
    {
        for (const auto &val : row)
        {
            std::wcout << val << '\t';
        }
        std::cout << std::endl;
    }

    return 0;
}