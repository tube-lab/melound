// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Config.h"
#include "DriverError.h"
#include "ChannelState.h"

#include "hardware/audio/ChannelsMixer.h"
#include "hardware/relay/Driver.h"

#include "utils/Time.h"
#include "utils/CustomConstructor.h"

#include <unordered_map>
#include <algorithm>
#include <expected>
#include <thread>
#include <list>

namespace ml::speaker
{
    /**
     * @brief A driver for a lamp speaker.
     * @safety Fully exception and thread safe.
     *
     * Sessions mechanism:
     * - The user opens the channel
     * - The user prolongs the channel each 1000(ms) otherwise the channel will be closed.
     *
     * Tracks enqueuing mechanism:
     * - The user activates the channel when it actually wants to play any tracks ( takes some time ).
     * - The user enqueues tracks.
     * - These tracks are played.
     * - The user releases the channel notifying the speaker that no tracks will be played ( may take some time ).
     *
     * Main features:
     * - All channels related function fail if the channel isn't active.
     *
     * Timings:
     * - The main idea is that speaker is working while at least 1 channel is active, but immediately shuts down when no channels are active.
     * - If the speaker has been turned off less than "cool-down duration"(ms) ago, then it the channel will be activated instantly.
     * - If the task is so urgent that the loses from the bad sound quality are negligible then the speaker can start to play immediately.
     */
    class Driver : public CustomConstructor
    {
        struct ChannelInfo
        {
            ChannelState State;
            time_t ExpiresAt;
            std::list<std::promise<bool>> ActivationListeners;
        };

        Config Config_;
        std::shared_ptr<relay::Driver> Relay_;
        std::shared_ptr<audio::ChannelsMixer> Mixer_;
        std::unordered_map<std::string, uint> ChannelsMapping_;

        time_t PoweringDuration_ {};
        time_t LastWorkingMoment_ {};
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
        auto Open(const std::string& channel) noexcept -> std::optional<DriverError>;

        /** Manually deactivates and closes a channel. Fails if the channel do not exist. */
        auto Close(const std::string& channel) noexcept -> bool;

        /** Prolongs a channel, so it won't be automatically closed for the next 1000(ms). */
        auto Prolong(const std::string& channel) noexcept -> std::optional<DriverError>;

        /** Activates the channel, required for the playback throughout the channel. */
        auto Activate(const std::string& channel, bool urgent) noexcept -> std::future<bool>;

        /** Notifies the driver that no music will be played over this channel in the nearest future. Clears the queue. */
        auto Deactivate(const std::string& channel) noexcept -> std::expected<std::optional<void>, DriverError>;

        /** Appends the audio track to the particular active channel. */
        auto Enqueue(const std::string& channel, const audio::Track& audio) noexcept -> std::expected<std::future<void>, DriverError>;

        /** Empties the channel. Channel' playback will be stopped immediately. Doesn't pause the channel. */
        auto Clear(const std::string& channel) noexcept -> std::optional<DriverError>;

        /** Drops the first track in the channel' queue and immediately moves to the next one. */
        auto Skip(const std::string& channel) noexcept -> std::optional<DriverError>;

        /** Determines for how long the channel will continue to play. */
        auto DurationLeft(const std::string& channel) const noexcept -> std::optional<time_t>;

        /** Determines the duration of the longest channel. Fails if no channels are active. */
        auto DurationLeft() const noexcept -> std::optional<time_t>;

        /** Returns the state of particular channel. */
        auto State(const std::string& channel) const noexcept -> bool;

        /** Returns the number of speaker' channels. */
        auto Channels() const noexcept -> size_t;

        /** Returns whenever the speaker is powered. */
        auto Powered() const noexcept -> bool;

        /** Returns the longest duration that the activation of channel may take. */
        auto ActivationDuration() const noexcept -> time_t;

    private:
        void ControlLoop(const std::stop_token& token) noexcept;

        void CloseExpiredChannels(time_t time) noexcept;

        void Close(uint id) noexcept;
        auto Deactivate(uint id) noexcept -> bool;

        auto FindChannelId(const std::string& channel) noexcept -> uint;
        void FulfillActivationListeners(uint id, bool result) noexcept;
    };
}