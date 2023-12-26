// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Config.h"
#include "ChannelState.h"
#include "DriverError.h"

#include "hardware/audio/ChannelsMixer.h"
#include "hardware/relay/Driver.h"

#include "utils/Time.h"

#include <functional>
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
     * Features:
     * -
     * - When the channel is disabled - it is queue is emptied.
     * - All channels related function fail if the channel isn't active.
     *
     * Timings:
     * - The main idea is that amplifier is working while at least 1 channel is active, but immediately shuts down when no channels are active.
     * - If the amplifier has been turned off less than "cool-down duration"(ms) ago, then it the channel will be activated instantly.
     * - If the task is so urgent that the loses from the bad sound quality are negligible then the amplifier can start to play immediately.
     */
    class Driver : public CustomConstructor
    {
        struct ChannelInfo
        {
            ChannelState State;
            bool UrgentActivation;
            std::vector<std::promise<bool>> ActivationListeners;
        };

        Config Config_;
        std::shared_ptr<relay::Driver> Relay_;
        std::shared_ptr<audio::ChannelsMixer> Mixer_;

        // Device state
        bool Powered_ {};
        std::pair<time_t, time_t> PreviousPoweredInterval_ {};
        std::pair<time_t, time_t> PoweredInterval_ {};
        std::vector<ChannelInfo> Channels_;
        mutable std::recursive_mutex StateLock_;

        std::jthread Controller_;

    public:
        /** Creates a new driver based on the provided configuration. Acquires a port and an audio-device. */
        static auto Create(const Config& config) noexcept -> std::shared_ptr<Driver>;

        /** Temporary pauses the playback in all channels, doesn't turn off the device. */
        void Pause() noexcept;

        /** Resumes a previously paused playback. */
        void Resume() noexcept;

        /** Prepares a channel, so it may be activated. */
        void Open(uint channel) noexcept;

        /** Deactivates and closes a channel. */
        void Close(uint channel) noexcept;

        /** Activates the channel, required for the access to the channel. Fails if the channel state != opened. */
        auto Activate(uint channel, bool urgent) noexcept -> std::optional<std::future<bool>>;

        /** Disables the channel and empties its queue. Fails if the channel state != active. */
        auto Deactivate(uint channel) noexcept -> bool;

        /** Appends the audio track to the particular active channel. Doesn't clear the pause state. */
        auto Enqueue(uint channel, const audio::Track& audio) noexcept -> std::expected<std::future<void>, EnqueueError>;

        /** Empties the channel. Channel' playback will be stopped immediately. Doesn't pause the channel. */
        auto Clear(uint channel) noexcept -> bool;

        /** Drops the first track in the channel' queue and immediately moves to the next one. */
        auto Skip(uint channel) noexcept -> bool;

        /** Determines for how long the channel will continue to play. */
        auto DurationLeft(uint channel) const noexcept -> std::optional<time_t>;

        /** Determines the duration of the longest channel. Returns nothing if no channels are active. */
        auto DurationLeft() const noexcept -> std::optional<time_t>;

        /** Returns whenever the channel is active. */
        auto State(uint channel) const noexcept -> ChannelState;

        /** Returns the number of amplifier' channels. */
        auto Channels() const noexcept -> size_t;

        /** Returns whenever the amplifier is working. */
        auto Powered() const noexcept -> bool;

        /** Returns the longest duration that the activation of channel may take. */
        auto ActivationDuration() const noexcept -> time_t;

    private:
        void ControllerLoop(const std::stop_token& token) noexcept;
        auto Warm(time_t time, bool urgent) const noexcept -> bool;

        void FulfillActivationListeners(uint channel, bool result) noexcept;

        void EnablePowerRelay() noexcept;
        void DisablePowerRelay() noexcept;

        void EnableMixerChannel(uint i) noexcept;
        void PutMixerChannelOnHold(uint i) noexcept;
        void DisableMixerChannel(uint i) noexcept;
    };
}