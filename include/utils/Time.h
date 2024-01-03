// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <chrono>

namespace ml::utils
{
    /**
     * @brief Utils for working with the time.
     * @safety Fully exception and thread safe.
     */
    class Time
    {
    public:
        /** Returns the timestamp based on the system time. */
        static auto Now() noexcept -> time_t;
    };
}