/**
 * @file encrypt.hpp
 * @author username (username52247554@gmail.com)
 * @brief encrypt
 * @version 0.1
 * @date 2025-06-23
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <pch.hpp>

namespace HydroSQL::Client::Encrypt
{
    const std::string secureEnter();

    const bool validPassword(const std::string &password, std::string &result);

    const bool validUsername(const std::string &username, std::string &result);

    void hash(std::string &password);

    const bool verifyHash(std::string &&password, const std::string &hash);
}