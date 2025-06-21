/**
 * @file logger.cpp
 * @author username (username52247554@gmail.com)
 * @brief basic logger
 * @version 0.1
 * @date 2025-06-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <chrono>

#include "../include/logger.hpp"

namespace HydroSQL::Utils::Logger
{
    Sink::Sink(const Level level_, const std::string &time_format_)
        : level(level_)
    {
        setTimeFormat(time_format_);
    }

    void Sink::setTimeFormat(const std::string &time_format_)
    {
        for (auto it = time_format_.begin(); it != time_format_.end();)
        {
            std::vector<char> pool = {'Y', 'M', 'D', 'h', 'm', 's'};
            for (const auto &ch : pool)
            {
                if (*it == ch)
                {
                    if (!(time_format_.end() - it > 1 && *(it + 1) == ch))
                    {
                        throw std::runtime_error("Invalid time format.");
                        return;
                    }
                    else
                    {
                        it += 1;
                        break;
                    }
                }
            }
            if (*it == 'S')
            {
                if (!(time_format_.end() - it > 2 && *(it + 1) == 'S' && *(it + 2) == 'S'))
                {
                    throw std::runtime_error("Invalid time format.");
                    return;
                }
                else
                {
                    it += 2;
                    break;
                }
            }

            it++;
        }

        this->time_format = time_format_;
    }

    const std::string Sink::getTimeStr(const TimeStamp &timestamp, const std::string &format)
    {
        std::stringstream ss;

        for (auto it = format.begin(); it != format.end(); it++)
        {
            switch (*it)
            {
            case 'Y':
                if (*(it + 3) == 'Y')
                {
                    ss << std::setw(4) << std::setfill('0') << timestamp.year;
                    it += 3;
                }
                else
                {
                    ss << std::setw(2) << std::setfill('0') << timestamp.year % 100;
                    it += 1;
                }
                break;
            case 'M':
                ss << std::setw(2) << std::setfill('0') << static_cast<int>(timestamp.month);
                it += 1;
                break;
            case 'D':
                ss << std::setw(2) << std::setfill('0') << static_cast<int>(timestamp.day);
                it += 1;
                break;
            case 'h':
                ss << std::setw(2) << std::setfill('0') << static_cast<int>(timestamp.hour);
                it += 1;
                break;
            case 'm':
                ss << std::setw(2) << std::setfill('0') << static_cast<int>(timestamp.minute);
                it += 1;
                break;
            case 's':
                ss << std::setw(2) << std::setfill('0') << static_cast<int>(timestamp.second);
                it += 1;
                break;
            case 'S':
                ss << std::setw(3) << std::setfill('0') << timestamp.millisecond;
                it += 2;
                break;
            default:
                ss << *it;
                break;
            }
        }

        return ss.str();
    }

    const std::string Sink::getTimeStr(const TimeStamp &timestamp) const
    {
        return getTimeStr(timestamp, this->time_format);
    }

    BasicConsoleSink::BasicConsoleSink(const Level level_, const std::string &time_format_)
        : Sink(level_, time_format_)
    {}

    void BasicConsoleSink::info(const TimeStamp &timestamp, const std::string &log) const
    {
        if (level <= Level::INFO)
            std::cout << getTimeStr(timestamp) << " [INFO] " + log << std::endl;
    }

    void BasicConsoleSink::warning(const TimeStamp &timestamp, const std::string &log) const
    {
        if (level <= Level::WARNING)
            std::cout << getTimeStr(timestamp) << " [WARNING] " + log << std::endl;
    }

    void BasicConsoleSink::error(const TimeStamp &timestamp, const std::string &log) const
    {
        if (level <= Level::ERROR)
            std::cout << getTimeStr(timestamp) << " [ERROR] " + log << std::endl;
    }

    BasicFileSink::BasicFileSink(std::filesystem::path log_path_, const Level level_, const std::string &time_format_)
        : Sink(level_, time_format_), log_path(log_path_)
    {}

    BasicFileSink::~BasicFileSink()
    {
        if (ofile != nullptr)
            ofile->close();
    }

    void BasicFileSink::info(const TimeStamp &timestamp, const std::string &log) const
    {
        if (level <= Level::INFO)
        {
            updateOfile(timestamp);
            std::lock_guard<std::mutex> lock(file_mutex);
            (*ofile) << getTimeStr(timestamp) << " [INFO] " + log << std::endl;
        }
    }

    void BasicFileSink::warning(const TimeStamp &timestamp, const std::string &log) const
    {
        if (level <= Level::WARNING)
        {
            updateOfile(timestamp);
            std::lock_guard<std::mutex> lock(file_mutex);
            (*ofile) << getTimeStr(timestamp) << " [WARNING] " + log << std::endl;
        }
    }

    void BasicFileSink::error(const TimeStamp &timestamp, const std::string &log) const
    {
        if (level <= Level::ERROR)
        {
            updateOfile(timestamp);
            std::lock_guard<std::mutex> lock(file_mutex);
            (*ofile) << getTimeStr(timestamp) << " [ERROR] " + log << std::endl;
        }
    }

    std::filesystem::path BasicFileSink::getFilePath(const TimeStamp &timestamp) const
    {
        return this->log_path / getTimeStr(timestamp, "YY-MM-DD") / (getTimeStr(timestamp, "hh") + ".log");
    }

    void BasicFileSink::updateOfile(const TimeStamp &timestamp) const
    {
        std::lock_guard<std::mutex> lock(file_mutex);
        std::filesystem::path path = getFilePath(timestamp);
        if (path != file_path || ofile == nullptr)
        {
            if (!std::filesystem::exists(path.parent_path()))
            {
                std::filesystem::create_directories(path.parent_path());
            }

            file_path = path;

            if (ofile != nullptr)
                ofile->close();
            ofile = std::make_unique<std::ofstream>(file_path, std::ios::app);
        }
    }

    Logger::Logger()
        : stop_flag(false)
    {
        log_thread = std::make_unique<std::thread>([this](){
            while (true)
            {
                std::unique_lock<std::mutex> lock(buffer_mutex);
                wait_for_log.wait(lock, [this]()
                                { return !buffer.empty() || stop_flag; });
                auto time = getSystemTimeStamp();
                while (!buffer.empty())
                {
                    auto &log = buffer.front();
                    for (const auto &sink : sinks)
                    {
                        switch (log.first)
                        {
                        case Level::INFO:
                            sink->info(time, log.second);
                            break;
                        case Level::WARNING:
                            sink->warning(time, log.second);
                            break;
                        case Level::ERROR:
                            sink->error(time, log.second);
                            break;
                        default:
                            break;
                        }
                        
                    }
                    buffer.pop_front();
                }
                lock.unlock();
                if (stop_flag)
                    break;
            }
        });
    } 

    Logger::~Logger()
    {
        stop_flag = true;
        wait_for_log.notify_one();
        log_thread->join();
    }

    void Logger::info(std::string log)
    {
        std::unique_lock<std::mutex> lock(buffer_mutex);
        buffer.push_back({Level::INFO, std::move(log)});
        lock.unlock();
        wait_for_log.notify_one();
    
    }

    void Logger::warning(std::string log)
    {
        std::unique_lock<std::mutex> lock(buffer_mutex);
        buffer.push_back({Level::WARNING, std::move(log)});
        lock.unlock();
        wait_for_log.notify_one();
    }
    void Logger::error(std::string log)
    {
        std::unique_lock<std::mutex> lock(buffer_mutex);
        buffer.push_back({Level::ERROR, std::move(log)});
        lock.unlock();
        wait_for_log.notify_one();
    }

    const TimeStamp Logger::getSystemTimeStamp()
    {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

        std::tm *now_tm = std::localtime(&now_time_t);

        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        TimeStamp timestamp;
        timestamp.year = now_tm->tm_year + 1900;
        timestamp.month = now_tm->tm_mon + 1;
        timestamp.day = now_tm->tm_mday;
        timestamp.hour = now_tm->tm_hour;
        timestamp.minute = now_tm->tm_min;
        timestamp.second = now_tm->tm_sec;
        timestamp.millisecond = static_cast<int16_t>(now_ms.count());

        return timestamp;
    }
}

int main()
{
    using namespace HydroSQL::Utils::Logger;

    try
    {
        // Logger::get().setLevel(Level::WARNING);
        Logger::get().addSink(std::move(std::make_unique<BasicConsoleSink>(Level::WARNING, "hh:mm:ss.SSS")));
        Logger::get().addSink(std::move(std::make_unique<BasicFileSink>("log")));
        for (size_t i = 0; i < 10000; i++)
        {
            Logger::get().info(std::to_string(i));
            Logger::get().warning(std::to_string(2 * i));
            Logger::get().error(std::to_string(i * i));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}