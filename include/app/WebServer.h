// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "ConfigParser.h"

#include "hardware/amplifier/lamp/LampDriver.h"
#include "hardware/audio/TrackLoader.h"
#include "hardware/speaker/Driver.h"

#include "utils/CustomConstructor.h"

#include <memory>
#include <httplib.h>

namespace ml::app
{
    /**
     * @brief The wrapper that provides speaker-driver methods throughout REST API.
     * @safety Fully exception and thread safe.
     *
     * For API reference look on API.md.
     */
    class WebServer : public utils::CustomConstructor
    {
    public:
        /** Creates and runs the application. Logs everything to the console. */
        static auto Run(const std::string& configPath) noexcept -> bool;

    private:
        static auto Response(int status, const std::string& text) noexcept -> httplib::Response;
        static auto LongPolling(const std::future<void>& f) noexcept -> httplib::Response;
        static auto BindError(speaker::ActionError error) noexcept -> httplib::Response;
        static auto BindState(speaker::ChannelState state) noexcept -> httplib::Response;
    };
}