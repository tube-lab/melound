// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace ml
{
    struct BellSink
    {
        uint64_t Priority;
        time_t Timeout;
    };
}