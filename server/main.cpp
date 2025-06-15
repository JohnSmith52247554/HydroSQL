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

#include <windows.h>
#include <engine/include/table.hpp>

using namespace YourSQL::Server;

int main()
{
#ifdef WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    std::vector<Engine::Column> cols = {
        Engine::Column{"index", Engine::DataType::INT},
        Engine::Column{"name", Engine::DataType::VARCHAR, 10},
        Engine::Column{"age", Engine::DataType::SMALLINT},
    };

    Engine::Table table("test", std::move(cols));

    std::vector<std::string> keys = {"name", "index", "age" };
    std::vector<std::vector<std::string>> vals = {
        { "张三", "1", "20" },
        { "李四", "2", "21" },
        { "王五", "3", "22" },
        { "zaoliu", "4", "23" }
    };
    std::string result;

    if(!table.insert(keys, vals, result))
    {
        std::cout << "error" << std::endl;
    }

    std::cout << result << std::endl;

    // Engine::Table table("test");

    // std::string result;
    // std::vector<std::string> keys = { "index", "age", "name" };
    // std::vector<std::vector<std::string>> output;

    // if(!table.select(keys, false, Engine::SelectOrder("", false), output, result))
    //     std::cout << "error" << std::endl;
    // std::cout << result << std::endl;

    // for (const auto &row : output)
    // {
    //     for (const auto &val : row)
    //     {
    //         std::cout << val << '\t';
    //     }
    //     std::cout << std::endl;
    // }

    std::vector<Engine::UpdateInfo> update = {
        { "age", "100" }
    };
    //std::string result;

    // Engine::Table table("test");

    std::vector<std::string> keyss = {"index", "age", "name"};
    std::vector<std::vector<std::string>> output;

    if (!table.select(keyss, false, Engine::SelectOrder("", false), output, result))
        std::cout << "error" << std::endl;
    std::cout << result << std::endl;

    for (const auto &row : output)
    {
        for (const auto &val : row)
        {
            std::cout << val << '\t';
        }
        std::cout << std::endl;
    }

    if (!table.update(update, false, result))
        std::cout << "error" << std::endl;

    std::cout << result << std::endl;

    // std::vector<std::string> keyss = { "index", "age", "name" };
    // std::vector<std::vector<std::string>> output;

    if(!table.select(keyss, false, Engine::SelectOrder("", false), output, result))
        std::cout << "error" << std::endl;
    std::cout << result << std::endl;

    for (const auto &row : output)
    {
        for (const auto &val : row)
        {
            std::cout << val << '\t';
        }
        std::cout << std::endl;
    }

    return 0;
}