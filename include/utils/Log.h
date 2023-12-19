// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <iostream>
#include <mutex>
#include <sstream>
#include <utility>
#include <map>

namespace ml
{
    // TODO: Move implementation to cpp, possibly wiping out the whole class
    class LogWriter
    {
        std::stringstream Stream_;
        std::ostream& Out_;
        std::string Origin_;
        std::mutex& Lock_;

    public:
        LogWriter(std::ostream& stream, std::string origin, std::mutex& lock)
                : Out_(stream), Origin_(std::move(origin)), Lock_(lock)
        {

        }

        ~LogWriter()
        {
            std::lock_guard _ { Lock_ };
            Out_ << Origin_ << ": " << Stream_.str();
        }

        auto operator<<(const auto& s) noexcept -> LogWriter&
        {
            Stream_ << s;
            return *this;
        }
    };

    class LogType
    {
        std::map<const void*, std::string> Names_;

    public:
        __attribute__((used))
        void Name(const void* ptr, const std::string& name)
        {
            Names_[ptr] = name;
        }

        auto Info(const void* ptr) -> LogWriter
        {
            static std::mutex lock;
            return LogWriter { std::cout, GetName(ptr), lock };
        }

    private:
        auto GetName(const void* ptr) -> std::string
        {
            if (Names_.contains(ptr))
            {
                return Names_[ptr];
            }

            std::stringstream str;
            str << (const void*)ptr;
            return str.str();
        }
    };

    inline LogType Log;
}