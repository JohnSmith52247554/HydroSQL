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
    std::string path(PROJECT_PATH);
    path += "/data.bin";
    Engine::Constraint c(Engine::ConstraintType::CHECK, "testtest");
    std::ofstream ofs(path, std::ios::binary);
    ofs << c;
    ofs.close();
    std::ifstream ifs(path, std::ios::binary);
    Engine::Constraint cc;
    ifs >> cc;
    ifs.close();
    std::cout << static_cast<int>(cc.type) << cc.details << std::endl;
    return 0;
}