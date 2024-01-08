// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "LampConfig.h"
#include "../Driver.h"

#include "hardware/audio/ChannelsMixer.h"
#include "hardware/relay/Driver.h"

namespace ml::amplifier
{
    /**
     * @brief A driver for the lamp based amplifier.
     * @safety Fully exception and thread safe.
     *
     * Due to old nature, such amplifiers always require warmup, so the driver takes care about this.
     */
    class LampDriver : public Driver
    {
        time_t CoolingDuration_;
        std::shared_ptr<audio::ChannelsMixer> Mixer_;
        std::shared_ptr<relay::Driver> PowerRelay_;

        std::atomic<time_t> DeactivatedAt_;

    public:
        /** Creates a new driver instance based on the config. */
        static auto Create(const LampConfig& cfg) noexcept -> std::shared_ptr<LampDriver>;

    private:
        using Driver::Driver;

        auto DoEnqueue(uint channel, const audio::Track &track) -> std::optional<std::future<void>> final;
        void DoSkip(uint channel) noexcept final;
        void DoClear(uint channel) noexcept final;
        auto DoDurationLeft(uint channel) const noexcept -> time_t final;
        void DoOpen(uint channel) noexcept final;
        void DoClose(uint channel) noexcept final;
        bool DoActivation(time_t time, time_t elapsed, bool urgently) noexcept final;
        bool DoDeactivation(time_t time, time_t elapsed, bool urgently) noexcept final;
    };
}