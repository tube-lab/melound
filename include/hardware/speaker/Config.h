// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <vector>

namespace ml::speaker
{
    struct Config
    {
        /** Time that the speaker requires to warm-up. */
        time_t WarmingDuration {};

        /** Time that the speaker requires to cool down, so it's unusable again. */
        time_t CoolingDuration {};

        /** The port to which the power-relay is connected. */
        std::string PowerControlPort {};

        /** The name of the audio output connected to the speaker. */
        std::optional<std::string> AudioDevice {};

        /** The speaker channels sorted by priority. */
        std::vector<std::string> Channels {};
    };
}