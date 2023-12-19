// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "hardware/audio/core/Player.h"
#include "hardware/audio/core/Track.h"

#include <vector>
#include <mutex>
#include <ranges>

namespace ml::audio
{
    /**
     * @brief The simple mixer
     * - Allows to overlay multiple channels.
     * - Each particular channel has the same capabilities as a Player instance.
     * - Overlay system based on priorities. When the channel with id=k is unmuted all channels which id's < k are muted.
     * - Do not support any kind of the channel blending.
     */
    class ChannelsMixer
    {
        std::vector<std::shared_ptr<Player>> Channels_ {};

        std::vector<bool> MutedChannels_ {};
        std::mutex MutedChannelsLock_ {};

    public:
        static auto Create(uint channels, SDL_AudioSpec spec, const std::optional<std::string>& audioDevice = std::nullopt) -> std::shared_ptr<ChannelsMixer>;

        /** Appends the audio track to the particular channel. Doesn't clear the pause state. */
        auto Enqueue(uint channel, const Track& audio) noexcept -> std::future<void>;

        /** Empties the channel. Channel' playback will be stopped immediately. Doesn't pauses the channel. */
        void Clear(uint channel) noexcept;

        /** Drops the first track in the channel' queue and immediately moves to the next one. */
        void Skip(uint channel) noexcept;

        /** Temporary pauses all channels playback. */
        void Pause() noexcept;

        /** Resumes a previously paused playback. */
        void Resume() noexcept;

        /** Mutes the channel. */
        void Mute(uint channel) noexcept;

        /** Unmutes the channel. */
        void Unmute(uint channel) noexcept;

        /** Returns that pause state of the channel. */
        auto Paused(uint channel) const noexcept -> bool;

        /** Returns the mute state of the channel. */
        auto Muted(uint channel) const noexcept -> bool;

        /** Determines for how long the channel player will continue to play. */
        auto DurationLeft(uint channel) const noexcept -> time_t;

        /** Determines the duration of the longest channel. */
        auto DurationLeft() const noexcept -> time_t;

        /** Returns the number of mixer' channels. */
        auto Channels() const noexcept -> size_t;

    private:
        ChannelsMixer(const std::vector<std::shared_ptr<Player>>& channels) noexcept;
        ChannelsMixer(const ChannelsMixer&) noexcept = delete;
        ChannelsMixer(ChannelsMixer&&) noexcept = delete;

        void UpdateChannel(size_t channel, bool muted) noexcept;
    };
}