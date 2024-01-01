// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Driver.h"
#include "LampConfig.h"

#include "hardware/audio/Player.h"
#include "hardware/relay/Driver.h"

namespace ml::amplifier
{
    class LampDriver : public Driver
    {
    public:
        static auto Create(const LampConfig& cfg) noexcept -> std::shared_ptr<LampDriver>;
        ~LampDriver();

    protected:
        auto DoEnqueue(uint channel, const audio::Track &track) -> std::optional<std::future<void>> override;
        void DoSkip(uint channel) noexcept override;
        void DoClear(uint channel) noexcept override;
        auto DoDurationLeft(uint channel) const noexcept -> time_t override;
        void DoOpen(uint channel) noexcept override;
        void DoClose(uint channel) noexcept override;
        bool DoActivation(time_t time, time_t elapsed, bool urgently) noexcept override;
        bool DoDeactivation(time_t time, time_t elapsed, bool urgently) noexcept override;
    };
}