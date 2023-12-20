// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Config.h"
#include "hardware/audio/ChannelsMixer.h"
#include "hardware/switch/SwitchDriver.h"
#include "utils/Time.h"

#include <optional>
#include <vector>
#include <memory>
#include <future>
#include <thread>
#include <map>

namespace ml::amplifier
{
    /**
     * A driver for a lamp amplifier.
     * Fully exception and thread safe.
     */
    class Driver
    {
        Config Config_;
        std::shared_ptr<SwitchDriver> Switch_;
        std::shared_ptr<audio::ChannelsMixer> Mixer_;

    public:
        static auto Create(const Config& config) noexcept -> std::shared_ptr<Driver>;
        ~Driver();

        void Activate(uint channel) noexcept;
        void Deactivate(uint channel) noexcept;

        void Enqueue(uint channel) noexcept;

        auto Active(uint channel) const noexcept -> bool;

    private:
        Driver(const Config& config) noexcept;
        Driver(const Driver&) noexcept = delete;
        Driver(Driver&&) noexcept = delete;
    };
}