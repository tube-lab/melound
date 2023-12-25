// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Sink.h"

#include <string>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace ml::speaker
{
    struct Config
    {
        /** Time that the amplifier requires to warm-up. */
        time_t WarmingDuration {};

        /** Time that the amplifier requires to cool down, so it's unusable again. */
        time_t CoolingDuration {};

        /** The port to which the power-relay is connected. */
        std::string PowerControlPort {};

        /** The name of the audio output. */
        std::string AudioDevice {};

        /** The list of the speaker sinks. */
        std::unordered_map<std::string, Sink> Sinks {};
    };
}