// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "ConfigParser.h"

#include "hardware/amplifier/lamp/LampDriver.h"
#include "hardware/audio/TrackLoader.h"
#include "hardware/speaker/Driver.h"

#include "utils/CustomConstructor.h"
#include "utils/TokenMiddleware.h"

#include <memory>
#include <crow/app.h>

namespace ml::app
{
    /**
     * @brief The wrapper that provides speaker-driver methods throughout REST API.
     * @safety Fully exception and thread safe.
     *
     * For API reference look on API.md.
     */
    class WebServer : public CustomConstructor
    {
    public:
        /** Creates and runs the application. Logs everything to the console. */
        static auto Run(const std::string& configPath, uint port) noexcept -> bool;

    private:
        static auto LongPolling(const std::future<void>& f) noexcept -> crow::response;
        static auto BindError(speaker::ActionError error) noexcept -> crow::response;
        static auto BindState(speaker::ChannelState state) noexcept -> crow::response;
        static auto Urgent(const crow::query_string& str) noexcept -> bool;
    };
}