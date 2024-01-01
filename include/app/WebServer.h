// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "ConfigParser.h"

#include "hardware/amplifier/LampDriver.h"
#include "hardware/audio/TrackLoader.h"
#include "hardware/speaker/Driver.h"

#include "utils/CustomConstructor.h"
#include "utils/TokenMiddleware.h"

#include <memory>
#include <crow/app.h>

namespace ml::app
{
    class WebServer : public CustomConstructor
    {
    public:
        static auto Run(const std::string& configPath, uint port) noexcept -> bool;

    private:
        static auto LongPolling(const std::future<void>& f) noexcept -> crow::response;
        static auto BindError(speaker::ActionError error) noexcept -> crow::response;
        static auto BindState(speaker::ChannelState state) noexcept -> crow::response;
    };
}