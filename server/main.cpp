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

    std::vector<std::string> keys = {"index", "name",  "age" };

    std::vector<std::vector<std::string>> vals(500, std::vector<std::string>{3});
    for (size_t i = 0; i < vals.size(); i += 5)
    {
        vals[i][0] = std::to_string(i);
        vals[i][1] = "张三";
        vals[i][2] = "24";
        vals[i + 1][0] = std::to_string(i + 1);
        vals[i + 1][1] = "李四";
        vals[i + 1][2] = "25";
        vals[i + 2][0] = std::to_string(i + 2);
        vals[i + 2][1] = "王五";
        vals[i + 2][2] = "26";
        vals[i + 3][0] = std::to_string(i + 3);
        vals[i + 3][1] = "Alex";
        vals[i + 3][2] = "27";
        vals[i + 4][0] = std::to_string(i + 4);
        vals[i + 4][1] = "Sam";
        vals[i + 4][2] = "28";
    }

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