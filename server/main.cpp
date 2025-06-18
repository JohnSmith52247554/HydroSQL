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
#include <engine/include/LogicalTree.hpp>

using namespace HydroSQL::Server;

void test1()
{
    std::vector<Engine::Column> cols = {
        Engine::Column{"index", Engine::DataType::INT},
        Engine::Column{"name", Engine::DataType::VARCHAR, 10},
        Engine::Column{"age", Engine::DataType::SMALLINT},
    };
    //cols[0].constraints.emplace_back(Engine::ConstraintType::UNIQUE, ""); 

    Engine::Table table("test", std::move(cols));

    std::vector<std::string> keys = {"index", "name", "age"};

    std::vector<std::vector<std::string>> vals(1000000, std::vector<std::string>{3});
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

    if (!table.insert(keys, vals, result))
    {
        std::cout << "error" << std::endl;
    }

    std::cout << result << std::endl;

    // std::vector<std::vector<std::string>> vals2 = {
    //     {"10", "aaa", "20"}};

    // if (!table.insert(keys, vals2, result))
    // {
    //     std::cout << "error" << std::endl;
    // }

    // std::cout << result << std::endl;

    std::vector<Engine::UpdateInfo> ui = {{"index", "10"}};

    std::shared_ptr<Engine::LT::LT> root3 = std::make_shared<Engine::LT::LT>();
    root3->type = Engine::LT::NodeType::OPERATOR;
    root3->info.op_type = Engine::LT::OpType::EQUAL;
    root3->children.resize(2);
    root3->children[0] = std::make_shared<Engine::LT::LT>();
    root3->children[0]->type = Engine::LT::NodeType::COL;
    root3->children[0]->info.liter.liter_type = Engine::LT::LiterType::STR;
    root3->children[0]->info.liter.liter_info.emplace<std::string>("index");
    root3->children[1] = std::make_shared<Engine::LT::LT>();
    root3->children[1]->type = Engine::LT::NodeType::LITERAL;
    root3->children[1]->info.liter.liter_type = Engine::LT::LiterType::INT;
    root3->children[1]->info.liter.liter_info.emplace<int64_t>(20);

    // if (!table.update(ui, root3, result))
    //     std::cout << "error" << std::endl;
    // std::cout << result << std::endl;
    
    // return;

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
        {"age", "100"}};
    // std::string result;

    // Engine::Table table("test");

    std::vector<std::string> keyss = {"index", "age", "name"};
    std::vector<std::vector<std::string>> output;

    std::shared_ptr<Engine::LT::LT> root = std::make_shared<Engine::LT::LT>();
    root->type = Engine::LT::NodeType::OPERATOR;
    root->info.op_type = Engine::LT::OpType::EQUAL;
    root->children.resize(2);
    root->children[0] = std::make_shared<Engine::LT::LT>();
    root->children[0]->type = Engine::LT::NodeType::COL;
    root->children[0]->info.liter.liter_type = Engine::LT::LiterType::STR;
    root->children[0]->info.liter.liter_info.emplace<std::string>("name");
    root->children[1] = std::make_shared<Engine::LT::LT>();
    root->children[1]->type = Engine::LT::NodeType::LITERAL;
    root->children[1]->info.liter.liter_type = Engine::LT::LiterType::STR;
    root->children[1]->info.liter.liter_info.emplace<std::string>("张三");

    auto order = std::make_shared<Engine::SelectOrder>("index", false);

    if (!table.select(keyss, nullptr, order, output, result))
        std::cout << "error" << std::endl;
    std::cout << result << std::endl;

    // for (const auto &row : output)
    // {
    //     for (const auto &val : row)
    //     {
    //         std::cout << val << '\t';
    //     }
    //     std::cout << std::endl;
    // }

    if (!table.update(update, root, result))
        std::cout << "error" << std::endl;

    std::cout << result << std::endl;

    if (!table.delete_(root3, result))
        std::cout << "error" << std::endl;

    std::cout << result << std::endl;

    // std::vector<std::string> keyss = { "index", "age", "name" };
    // std::vector<std::vector<std::string>> output;

    std::shared_ptr<Engine::LT::LT> root2 = std::make_shared<Engine::LT::LT>();
    root2->type = Engine::LT::NodeType::OPERATOR;
    root2->info.op_type = Engine::LT::OpType::OR;
    root2->children.resize(2);
    root2->children[0] = std::make_shared<Engine::LT::LT>();
    root2->children[0]->type = Engine::LT::NodeType::OPERATOR;
    root2->children[0]->info.op_type = Engine::LT::OpType::EQUAL;
    root2->children[0]->children.resize(2);
    root2->children[1] = std::make_shared<Engine::LT::LT>();
    root2->children[1]->type = Engine::LT::NodeType::OPERATOR;
    root2->children[1]->info.op_type = Engine::LT::OpType::EQUAL;
    root2->children[1]->children.resize(2);
    auto &vec0 = root2->children[0]->children;
    auto &vec1 = root2->children[1]->children;
    vec0[0] = std::make_shared<Engine::LT::LT>();
    vec0[0]->type = Engine::LT::NodeType::COL;
    vec0[0]->info.liter.liter_type = Engine::LT::LiterType::STR;
    vec0[0]->info.liter.liter_info.emplace<std::string>("name");
    vec0[1] = std::make_shared<Engine::LT::LT>();
    vec0[1]->type = Engine::LT::NodeType::LITERAL;
    vec0[1]->info.liter.liter_type = Engine::LT::LiterType::STR;
    vec0[1]->info.liter.liter_info.emplace<std::string>("张三");
    vec1[0] = std::make_shared<Engine::LT::LT>();
    vec1[0]->type = Engine::LT::NodeType::COL;
    vec1[0]->info.liter.liter_type = Engine::LT::LiterType::STR;
    vec1[0]->info.liter.liter_info.emplace<std::string>("name");
    vec1[1] = std::make_shared<Engine::LT::LT>();
    vec1[1]->type = Engine::LT::NodeType::LITERAL;
    vec1[1]->info.liter.liter_type = Engine::LT::LiterType::STR;
    vec1[1]->info.liter.liter_info.emplace<std::string>("李四");

    if (!table.select(keyss, nullptr, order, output, result))
        std::cout << "error" << std::endl;
    std::cout << result << std::endl;

    // for (const auto &row : output)
    // {
    //     for (const auto &val : row)
    //     {
    //         std::cout << val << '\t';
    //     }
    //     std::cout << std::endl;
    // }
}

// void test2()
// {
//     using namespace Engine::LT;
//     std::shared_ptr<LT> root = std::make_shared<LT>();
//     root->type = NodeType::OPERATOR;
//     root->info.op_type = OpType::AND;
//     root->children.resize(2);
//     root->children[0] = std::make_shared<LT>();
//     root->children[0]->type = NodeType::LITERAL;
//     root->children[0]->info.liter.liter_type = LiterType::BOOLEAN;
//     root->children[0]->info.liter.liter_info.boolean = true;
//     root->children[1] = std::make_shared<LT>();
//     root->children[1]->type = NodeType::OPERATOR;
//     root->children[1]->info.op_type = OpType::OR;
//     root->children[1]->children.resize(2);
//     auto &vec = root->children[1]->children;
//     vec[0] = std::make_shared<LT>();
//     vec[0]->type = NodeType::LITERAL;
//     vec[0]->info.liter.liter_type = LiterType::BOOLEAN;
//     vec[0]->info.liter.liter_info.boolean = true;
//     vec[1] = std::make_shared<LT>();
//     vec[1]->type = NodeType::LITERAL;
//     vec[1]->info.liter.liter_type = LiterType::BOOLEAN;
//     vec[1]->info.liter.liter_info.boolean = false;
//     std::cout << boolOp(root);
// }

// void test3()
// {
//     using namespace Engine::LT;
//     std::shared_ptr<LT> root = std::make_shared<LT>(NodeType::OPERATOR);
//     root->info.op_type = OpType::LESS;
//     root->children.resize(2);
//     root->children[0] = std::make_shared<LT>(NodeType::CALCULATION);
//     root->children[0]->info.cal_type = CalType::ADD;
//     root->children[0]->children.resize(2);
//     auto &vec = root->children[0]->children;
//     root->children[1] = std::make_shared<LT>(NodeType::LITERAL);
//     root->children[1]->info.liter.liter_type = LiterType::FLOAT;
//     root->children[1]->info.liter.liter_info.doub = 9.5;
//     vec[0] = std::make_shared<LT>(NodeType::LITERAL);
//     vec[0]->info.liter.liter_type = LiterType::FLOAT;
//     vec[0]->info.liter.liter_info.doub = 7.7;
//     vec[1] = std::make_shared<LT>(NodeType::LITERAL);
//     vec[1]->info.liter.liter_type = LiterType::INT;
//     vec[1]->info.liter.liter_info.int64 = 8;
//     std::cout << boolOp(root) << std::endl;
// }

int main()
{
#ifdef WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    auto start = std::chrono::_V2::steady_clock::now();

    test1();

    auto duration = start - std::chrono::_V2::steady_clock::now();

    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(duration) << std::endl;

    return 0;
}