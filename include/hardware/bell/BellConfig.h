// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace ml
{
    struct BellConfig
    {
        /** Time that the bell needs to set itself up. */
        time_t StartupDuration {};

        /** The port to which the power-switch is connected. */
        int PowerControlPort {};

        /** The name of the audio output. */
        std::string AudioDevice {};
    };
}