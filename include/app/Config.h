// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <vector>

namespace ml::app
{
    struct Config
    {
        /** Port on which the web-server will run. */
        uint16_t Port = 8080;

        /** The application' API token. */
        std::string Token = "meloun";

        /** Time that the speaker requires to warm-up. */
        time_t WarmingDuration = 0;

        /** Time that the speaker requires to cool down, so it's unusable again. */
        time_t CoolingDuration = 0;

        /** Path to the port which is connected to the power-relay. */
        std::string PowerPort = "/dev/ttyS0";

        /** The name of the audio output connected to the speaker. */
        std::optional<std::string> AudioDevice = std::nullopt;

        /** The speaker channels sorted by priority. */
        std::vector<std::string> Channels = { "default" };
    };
}