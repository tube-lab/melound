// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "BellSink.h"

#include <string>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace ml
{
    struct BellConfig
    {
        /** Time that the amplifier requires to warm-up. */
        time_t Warming {};

        /** Time that the amplifier requires to cool down, so it's unusable again. */
        time_t Cooling {};

        /** The port to which the power-switch is connected. */
        std::string PowerControlPort {};

        /** The name of the audio output. */
        std::string AudioDevice {};

        /** The list of bell sinks. */
        std::unordered_map<std::string, BellSink> Sinks {};
    };
}