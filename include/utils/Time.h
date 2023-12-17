// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <chrono>

// TODO: Rewrite and move code to cpp file.
namespace ml
{
    inline auto TimeNow() noexcept -> time_t
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }
}