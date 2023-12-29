// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "hardware/amplifier/Driver.h"

#include <string>
#include <memory>
#include <vector>

namespace ml::speaker
{
    struct Config
    {
        /** The speaker amplifier subsystem implementation. */
        std::shared_ptr<amplifier::Driver> Amplifier;

        /** The speaker channels sorted by priority. */
        std::vector<std::string> Channels {};
    };
}