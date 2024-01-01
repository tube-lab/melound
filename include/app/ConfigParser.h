// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Config.h"

#include <SimpleIni/SimpleIni.h>

#include <string>
#include <cstdint>
#include <optional>
#include <vector>

namespace ml::app
{
    /** A parser for the user created config.  */
    class ConfigParser
    {
    public:
        /** Tries to parse the config from ini file. */
        static auto FromIni(const std::string& data) -> std::optional<Config>;
    };
}