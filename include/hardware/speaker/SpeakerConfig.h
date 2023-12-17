// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "SpeakerSink.h"

#include <string>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace ml
{
    struct SpeakerConfig
    {
        /** Time that the amplifier requires to warm-up. */
        time_t WarmingDuration {};

        /** Time that the amplifier requires to cool down, so it's unusable again. */
        time_t CoolingDuration {};

        /** The port to which the power-switch is connected. */
        std::string PowerControlPort {};

        /** The name of the audio output. */
        std::string AudioDevice {};

        /** The list of the speaker sinks. */
        std::unordered_map<std::string, SpeakerSink> Sinks {};
    };
}