/**
 * @file logger.hpp
 * @author username (username52247554@gmail.com)
 * @brief basic logger
 * @version 0.1
 * @date 2025-06-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once

#include <string>
#include <vector>
#include <list>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <fstream>

namespace HydroSQL::Utils::Logger
{
    enum class Level : char
    {
        INFO = 0,
        WARNING,
        ERROR_
    };

    struct TimeStamp
    {
        int16_t year;
        int8_t month;
        int8_t day;
        int8_t hour;
        int8_t minute;
        int8_t second;
        int16_t millisecond;
    };

    constexpr const char *DEFAULT_TIME_FORMAT = "YYYY-MM-DD-hh:mm:ss.SSS";

    class Sink
    {
    protected:
        Level level;
        std::string time_format;

    public:
        Sink(const Level level_ = Level::INFO, const std::string &time_format_ = DEFAULT_TIME_FORMAT);
        virtual ~Sink() {}

        inline void setLevel(const Level level_)
        {
            level = level_;
        }

        void setTimeFormat(const std::string &time_format_);

        virtual void info(const TimeStamp &timestamp, const std::string &log) const = 0;
        virtual void warning(const TimeStamp &timestamp, const std::string &log) const = 0;
        virtual void error(const TimeStamp &timestamp, const std::string &log) const = 0;

    protected:
        // UNSAFE
        static const std::string getTimeStr(const TimeStamp &timestamp, const std::string &format);
        const std::string getTimeStr(const TimeStamp &timestamp) const;
    };

    class BasicConsoleSink : public Sink
    {
    public:
        BasicConsoleSink(const Level level_ = Level::INFO, const std::string &time_format_ = DEFAULT_TIME_FORMAT);
        ~BasicConsoleSink() {}

        virtual void info(const TimeStamp &timestamp, const std::string &log) const override;
        virtual void warning(const TimeStamp &timestamp, const std::string &log) const override;
        virtual void error(const TimeStamp &timestamp, const std::string &log) const override;
    }; 

    class BasicFileSink : public Sink
    {
    private:
        std::filesystem::path log_path;
        mutable std::filesystem::path file_path;
        mutable std::unique_ptr<std::ofstream> ofile;
        mutable std::mutex file_mutex;

    public:
        BasicFileSink(std::filesystem::path log_path_, const Level level_ = Level::INFO, const std::string &time_format_ = DEFAULT_TIME_FORMAT);
        ~BasicFileSink();

        virtual void info(const TimeStamp &timestamp, const std::string &log) const override;
        virtual void warning(const TimeStamp &timestamp, const std::string &log) const override;
        virtual void error(const TimeStamp &timestamp, const std::string &log) const override;

    private:
        std::filesystem::path getFilePath(const TimeStamp &timestamp) const;
        void updateOfile(const TimeStamp &timestamp) const;
    };

    class Logger
    {
    private:
        std::vector<std::unique_ptr<Sink>> sinks;
        // Level level;
        std::list<std::pair<Level, std::string>> buffer;

        std::unique_ptr<std::thread> log_thread;
        std::mutex buffer_mutex;
        std::condition_variable wait_for_log;
        std::atomic<bool> stop_flag;

        Logger();
        ~Logger();

    public:
        static Logger &get()
        {
            static Logger singleton;
            return singleton;
        }

        void info(std::string log);
        void warning(std::string log);
        void error(std::string log);

        void addSink(std::unique_ptr<Sink> &&sink)
        {
            if (sink == nullptr)
                throw std::runtime_error("Sink is nullptr.");
            // sink->setLevel(level);
            sinks.push_back(std::move(sink));
        }

        void setLevel(const Level level_)
        {
            for (auto &sink : sinks)
            {
                sink->setLevel(level_);
            }
        }

        inline decltype(sinks) &getSinks()
        {
            return sinks;
        }

    private:
        static const TimeStamp getSystemTimeStamp();
    };
} // namespace HydroSQL::Utils::Logger
