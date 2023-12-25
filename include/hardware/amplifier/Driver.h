// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Config.h"
#include "DriverErrors.h"
#include "hardware/audio/ChannelsMixer.h"
#include "hardware/relay/Driver.h"

#include "utils/Time.h"

#include <algorithm>
#include <optional>
#include <expected>
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
     * @safety Fully exception and thread safe.
     *
     * Technically copies an API of ChannelsMixer, but with some changes:
     * - A couple of function are async because the amplifier needs time to prepare itself.
     * - Enabling/disabling logic is replaced with Activation/Deactivation.
     * - When the channel is disabled - it is reset to its initial state ( cleared, paused, unmuted, etc ).
     *   We do this because driver follows the logic of "sessions": channels will be used sequentially by different clients
     *   and any of then mustn't be affected by the previous ones.
     * - All channels related function fail if the channel isn't active.
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
            bool Enabled;
            bool Urgent;
            std::promise<void> Listener;
        };

        using ChannelEventsList = std::list<ChannelStateChange>;
        using EventsList = std::vector<ChannelEventsList>;

        Config Config_;
        std::shared_ptr<relay::Driver> Relay_;
        std::shared_ptr<audio::ChannelsMixer> Mixer_;

        // Inter-thread communication
        EventsList ControllerEvents_;
        std::mutex CommLock_;

        // Device state
        bool Working_;
        std::vector<bool> ActiveChannels_;
        time_t PoweringDuration_ {};
        time_t LastWorkingMoment_ {};
        mutable std::mutex StateLock_;

        std::jthread Controller_;

    public:
        /** Creates a new driver based on the provided configuration. Acquires a port and an audio-device. */
        static auto Create(const Config& config) noexcept -> std::shared_ptr<Driver>;

        /** Temporary pauses the playback in all channels, doesn't turn off the device. */
        void Pause() noexcept;

        /** Resumes a previously paused playback. */
        void Resume() noexcept;

        /** Activates the channel, required for the access to the channel. After the future completion the channel may be still not activated. */
        auto Activate(uint channel, bool urgent) noexcept -> std::future<void>;

        /** Disables the channel and completely resets it ( including the pause state ). */
        auto Deactivate(uint channel) noexcept -> std::future<void>;

        /** Appends the audio track to the particular active channel. Doesn't clear the pause state. */
        auto Enqueue(uint channel, const audio::Track& audio) noexcept -> std::expected<std::future<void>, EnqueueError>;

        /** Empties the channel. Channel' playback will be stopped immediately. Doesn't pause the channel. */
        auto Clear(uint channel) noexcept -> bool;

        /** Drops the first track in the channel' queue and immediately moves to the next one. */
        auto Skip(uint channel) noexcept -> bool;

        /** Temporary pauses the playback in channel, but do not disables it. */
        auto Pause(uint channel) noexcept -> bool;

        /** Resumes a previously paused playback in the channel. */
        auto Resume(uint channel) noexcept -> bool;

        /** Mutes the channel. */
        auto Mute(uint channel) noexcept -> bool;

        /** Unmutes the channel. */
        auto Unmute(uint channel) noexcept -> bool;

        /** Returns that pause state of the channel. */
        auto Paused(uint channel) const noexcept -> std::optional<bool>;

        /** Returns the mute state of the channel. */
        auto Muted(uint channel) const noexcept -> std::optional<bool>;

        /** Determines for how long the channel will continue to play. */
        auto DurationLeft(uint channel) const noexcept -> std::optional<time_t>;

        /** Determines the duration of the longest channel. Fails if no channels are active. */
        auto DurationLeft() const noexcept -> std::optional<time_t>;

        /** Returns whenever the channel is active. */
        auto Active(uint channel) const noexcept -> bool;

        /** Returns the number of the driver' activated channels. */
        auto CountActive() const noexcept -> size_t;

        /** Returns the number of amplifier' channels. */
        auto Channels() const noexcept -> size_t;

        /** Returns whenever the amplifier is working. */
        auto Working() const noexcept -> bool;

        /** Returns the longest duration that the activation of channel may take. */
        auto ActivationDuration() const noexcept -> time_t;

    private:
        Driver(Config config, std::shared_ptr<relay::Driver> sw, std::shared_ptr<audio::ChannelsMixer> mixer) noexcept;
        Driver(const Driver&) noexcept = delete;
        Driver(Driver&&) noexcept = delete;

        static void ControllerLoop(const std::stop_token& token, Driver* self) noexcept;
        static auto Warmed(time_t time, time_t workingMoment, time_t poweringInterval, bool urgent, bool working, const Config& config) noexcept -> bool;
        static void FulfillEvents(ChannelEventsList::iterator begin, ChannelEventsList::iterator end, ChannelEventsList& list) noexcept;

        void EnablePowerRelay() noexcept;
        void DisablePowerRelay() noexcept;

        void EnableMixerChannel(uint i) noexcept;
        void DisableMixerChannel(uint i) noexcept;

        auto DoIfChannelEnabled(uint i, const std::function<void()>& f) noexcept -> bool;
    };
}