// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Driver.h"

#include "hardware/audio/Player.h"
#include "hardware/relay/Driver.h"

namespace ml::amplifier
{
    class LampAmplifier : public Driver
    {
    public:
        auto Enqueue(uint channel, const audio::Track &track) -> std::expected<std::future<void>, ActionError> override
        {
            return {};
        }

        auto Skip(uint channel) noexcept -> std::expected<time_t, ActionError> override
        {
            return {};
        }

        auto Clear(uint channel) noexcept -> std::expected<time_t, ActionError> override
        {
            return {};
        }

        auto DurationLeft(uint channel) const noexcept -> std::expected<time_t, ActionError> override
        {
            return {};
        }

    protected:
        bool Tick(time_t time, time_t dt, const std::vector<ChannelState> &channels) noexcept override
        {
            return false;
        }
    };
}