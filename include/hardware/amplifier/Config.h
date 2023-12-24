// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace ml::amplifier
{
    struct Config
    {
        /** Time that the amplifier requires to warm-up. */
        time_t WarmingDuration {};

        /** Time that the amplifier requires to cool down, so it's unusable again. */
        time_t CoolingDuration {};

        /** The port to which the power-switch is connected. */
        std::string PowerControlPort {};

        /** The name of the audio output connect to the amplifier. */
        std::optional<std::string> AudioDevice {};

        /** The number of the amplifier channels. */
        uint64_t Channels {};
    };
}