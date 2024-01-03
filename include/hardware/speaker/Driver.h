// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Config.h"
#include "ActionError.h"
#include "ChannelState.h"

#include "utils/Time.h"
#include "utils/CustomConstructor.h"

#include <unordered_map>
#include <algorithm>
#include <expected>
#include <utility>
#include <numeric>
#include <future>
#include <thread>
#include <ranges>
#include <map>

namespace ml::speaker
{
    /**
     * @brief A driver for any speaker.
     * @safety Fully exception and thread safe.
     *
     * Serves as an abstraction layer over the amplifier driver providing all the necessary features for the app.
     * Adds prolongation concept and more convenient channels state management.
     *
     * Sessions mechanism:
     * - The user opens the channel
     * - The user prolongs the channel each 1000(ms) otherwise the channel will be closed.
     *
     * Tracks enqueuing mechanism:
     * - The user activates the opened channel when he actually wants to play any tracks ( takes some time ).
     * - The user enqueues tracks.
     * - These tracks are played.
     * - The user releases the channel signalling to the speaker that no tracks will be played ( takes some time ).
     *
     * Main features:
     * - All channels related function fail if the channel isn't active.
     */
    class Driver : public CustomConstructor
    {
        struct Channel
        {
            ChannelState State;
            std::optional<time_t> ExpiresAt;
            std::vector<std::promise<void>> ActivationListeners;
            std::vector<std::promise<void>> DeactivationListeners;
        };

        std::shared_ptr<amplifier::Driver> Amplifier_;
        std::map<std::string, uint> ChannelsMap_;

        std::vector<Channel> Channels_;
        mutable std::recursive_mutex ChannelsLock_;

        std::jthread Mainloop_;

    public:
        template <typename T = void> using Result = std::expected<T, ActionError>;

        /** Creates a new driver based on the provided configuration. Acquires a port and an audio-device. */
        static auto Create(const Config& config) noexcept -> std::shared_ptr<Driver>;

        /** Prepares a channel, so it may be activated. */
        auto Open(const std::string& channel) noexcept -> Result<>;

        /** Prolongs a channel, so it won't be automatically closed for the next 1000(ms). */
        auto Prolong(const std::string& channel) noexcept -> Result<>;

        /** Activates the channel, required for the playback throughout the channel. */
        auto Activate(const std::string& channel, bool urgently) noexcept -> Result<std::future<void>>;

        /** Notifies the driver that no music will be played over this channel in the nearest future. Clears the queue. */
        auto Deactivate(const std::string& channel, bool urgently) noexcept -> Result<std::future<void>>;

        /** Appends the audio track to the particular active channel. */
        auto Enqueue(const std::string& channel, const audio::Track& audio) noexcept -> Result<std::future<void>>;

        /** Empties the channel. Channel' playback will be stopped immediately. Doesn't pause the channel. */
        auto Clear(const std::string& channel) noexcept -> Result<>;

        /** Drops the first track in the channel' queue and immediately moves to the next one. */
        auto Skip(const std::string& channel) noexcept -> Result<>;

        /** Determines for how long the channel will continue to play. */
        auto DurationLeft(const std::string& channel) const noexcept -> Result<time_t>;

        /** Returns the state of particular channel. */
        auto State(const std::string& channel) const noexcept -> Result<ChannelState>;

        /** Returns the longest duration that the activation of a channel may take. */
        auto ActivationDuration(bool urgently) const noexcept -> time_t;

        /** Returns the longest duration that the activation of a channel may take. */
        auto DeactivationDuration(bool urgently) const noexcept -> time_t;

        /** Determines for how long the speaker will continue to play. */
        auto DurationLeft() const noexcept -> time_t;

        /** Returns whether the amplifier is working now. */
        auto Working() const noexcept -> bool;

    private:
        void Mainloop(const std::stop_token& token) noexcept;
        auto MapToIndex(const std::string& channel) const noexcept -> Result<uint>;
        auto CountActive() const noexcept -> uint;

        static auto MakeFulfilledFuture() noexcept -> std::future<void>;
        static void FulfillListeners(std::vector<std::promise<void>>& list) noexcept;
        static auto BindDriverError(amplifier::ActionError err) noexcept -> ActionError;
    };
}