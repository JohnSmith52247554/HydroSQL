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

    Engine::Table table(L"test");

    std::vector<std::wstring> keys = {L"name", L"index" };
    std::vector<std::vector<std::wstring>> vals = {
        { L"zhangsan", L"1"},
        { L"lisi", L"2"},
        { L"wangwu", L"3" }
    };
    std::wstring result;

    if(!table.insert(keys, vals, result))
    {
        std::cout << "error" << std::endl;
    }

    std::wcout << result << std::endl;

    return 0;
}