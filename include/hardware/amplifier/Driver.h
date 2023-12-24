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
#include <list>
#include <map>

namespace ml::amplifier
{
    /**
     * @brief A driver for a lamp amplifier.
     * Technically copies an API of ChannelsMixer, but with some changes:
     * - A couple of function are async because the amplifier needs time to prepare itself.
     * - Enabling/disabling logic is replaced with Activation/Deactivation.
     * - When the channel is disabled - it is reset to its initial state ( cleared, paused, unmuted, etc ).
     *   We do this because driver follows the logic of "sessions": channels will be used sequentially by different clients
     *   and any of then mustn't be affected by the previous ones.
     * - Enqueue function fails if the channel isn't active.
     *
     * Timings:
     * - The main idea is that amplifier is working while at least 1 channel is active, but immediately shuts down when no channels are active.
     * - If the amplifier has been turned off less than "cool-down duration"(ms) ago, then it the channel will be activated instantly.
     * - If the task is so urgent that the loses from the bad sound quality are negligible then the amplifier can start to play immediately.
     */
    class Driver
    {
        struct ChannelStateChange
        {
            uint Channel;
            bool Enabled;
            bool Urgent;
            std::promise<bool> Listener;
        };

        Config Config_;
        std::shared_ptr<SwitchDriver> Switch_;
        std::shared_ptr<audio::ChannelsMixer> Mixer_;

        // Thread communication
        std::list<ChannelStateChange> ControllerEvents_;
        std::mutex CommLock_;

        // Controller state
        std::atomic<bool> Working_;
        std::atomic<time_t> LastWorkingTime_;
        std::atomic<time_t> WarmingStartTime_;

        std::jthread Controller_;

    public:
        static auto Create(const Config& config) noexcept -> std::shared_ptr<Driver>;

        /** Temporary pauses the playback in all channels, doesn't turn off the device. */
        void Pause() noexcept;

        /** Resumes a previously paused playback. */
        void Resume() noexcept;

        /** Appends the audio track to the particular active channel. Doesn't clear the pause state. */
        auto Enqueue(uint channel, const audio::Track& audio) noexcept -> std::optional<std::future<void>>;

        /** Empties the channel. Channel' playback will be stopped immediately. Doesn't pauses the channel. */
        void Clear(uint channel) noexcept;

        /** Drops the first track in the channel' queue and immediately moves to the next one. */
        void Skip(uint channel) noexcept;

        /** Activates the channel, required for the access to the channel. Fails if the channel is deactivated before the finish. */
        auto Activate(uint channel, bool urgent) noexcept -> std::future<bool>;

        /** Disables the channel and completely resets it ( including the pause state ). */
        void Deactivate(uint channel) noexcept;

        /** Temporary pauses the playback in channel, but do not disables it. */
        void Pause(uint channel) noexcept;

        /** Resumes a previously paused playback in the channel. */
        void Resume(uint channel) noexcept;

        /** Mutes the channel. */
        void Mute(uint channel) noexcept;

        /** Unmutes the channel. */
        void Unmute(uint channel) noexcept;

        /** Returns whenever the channel is active. */
        auto Enabled(uint channel) const noexcept -> bool;

        /** Returns that pause state of the channel. */
        auto Paused(uint channel) const noexcept -> bool;

        /** Returns the mute state of the channel. */
        auto Muted(uint channel) const noexcept -> bool;

        /** Determines for how long the channel will continue to play. */
        auto DurationLeft(uint channel) const noexcept -> time_t;

        /** Determines the duration of the longest channel. */
        auto DurationLeft() const noexcept -> time_t;

        /** Returns the number of amplifier' channels. */
        auto Channels() const noexcept -> size_t;

        /** Returns whenever the amplifier is working. */
        auto Working() const noexcept -> bool;

        /** Returns the longest duration that the activation of channel may take. */
        auto ActivationDuration() const noexcept -> time_t;

    private:
        Driver(Config config, std::shared_ptr<SwitchDriver> sw, std::shared_ptr<audio::ChannelsMixer> mixer) noexcept;
        Driver(const Driver&) noexcept = delete;
        Driver(Driver&&) noexcept = delete;

        static void ControllerLoop(const std::stop_token& token, Driver* self) noexcept;
        static auto MayTurnOn(time_t time, time_t workingMoment, time_t warmingStart, bool urgent, const Config& config) noexcept -> bool;

        void EnableMixerChannel(uint i) noexcept;
        void DisableMixerChannel(uint i) noexcept;
    };
}