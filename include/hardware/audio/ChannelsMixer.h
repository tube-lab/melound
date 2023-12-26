// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "hardware/audio/Player.h"
#include "hardware/audio/Track.h"

#include <algorithm>
#include <vector>
#include <mutex>
#include <ranges>

namespace ml::audio
{
    /**
     * @brief The simple mixer
     * - Allows to overlay multiple channels.
     * - Each particular channel has the same capabilities as a Player instance.
     * - Overlay system based on priorities. When the channel with id=k is enabled all channels which id's < k are muted.
     *   This works even if channel with id=k is muted, so always pay attention to this fact.
     *   Note that even when the channel is disabled it continues to play.
     * - Do not support any kind of the channel blending.
     * Note, that each channel is muted by default. This allows you to upload all the necessary audio into it.
     */
    class ChannelsMixer
    {
        std::vector<std::shared_ptr<Player>> Channels_ {};

        std::vector<bool> EnabledChannels_ {};
        std::vector<bool> MutedChannels_ {};
        mutable std::recursive_mutex ChannelsStatesLock_ {};

    public:
        /** Creates the channel mixer that's bound to some audio-device. */
        static auto Create(uint channels, const std::optional<std::string>& audioDevice = std::nullopt) -> std::shared_ptr<ChannelsMixer>;

        /** Temporary pauses the playback in all channels. */
        void Pause() noexcept;

        /** Resumes a previously paused playback. */
        void Resume() noexcept;

        /** Appends the audio track to the particular channel. Doesn't clear the pause state. */
        auto Enqueue(uint channel, const Track& audio) noexcept -> std::optional<std::future<void>>;

        /** Empties the channel. Channel' playback will be stopped immediately. Doesn't pause the channel. */
        void Clear(uint channel) noexcept;

        /** Drops the first track in the channel' queue and immediately moves to the next one. */
        void Skip(uint channel) noexcept;

        /** Enables the channel. */
        void Enable(uint channel) noexcept;

        /** Disables the channel. */
        void Disable(uint channel) noexcept;

        /** Temporary pauses the playback in channel, however doesn't disable it. */
        void Pause(uint channel) noexcept;

        /** Resumes a previously paused playback in the channel, but doesn't enable it. */
        void Resume(uint channel) noexcept;

        /** Mutes the channel. */
        void Mute(uint channel) noexcept;

        /** Unmutes the channel. */
        void Unmute(uint channel) noexcept;

        /** Returns whenever the channel is enabled. */
        auto Enabled(uint channel) const noexcept -> bool;

        /** Returns that pause state of the channel. */
        auto Paused(uint channel) const noexcept -> bool;

        /** Returns the mute state of the channel. */
        auto Muted(uint channel) const noexcept -> bool;

        /** Determines for how long the channel will continue to play. */
        auto DurationLeft(uint channel) const noexcept -> time_t;

        /** Determines the duration of the longest channel. */
        auto DurationLeft() const noexcept -> time_t;

        /** Returns the number of channels that's currently enabled. */
        auto CountEnabled() const noexcept -> size_t;

        /** Returns the number of mixer' channels. */
        auto Channels() const noexcept -> size_t;

    private:
        ChannelsMixer(const std::vector<std::shared_ptr<Player>>& channels) noexcept;
        ChannelsMixer(const ChannelsMixer&) noexcept = delete;
        ChannelsMixer(ChannelsMixer&&) noexcept = delete;

        void UpdateChannel(size_t channel, std::optional<bool> enabled, std::optional<bool> muted) noexcept;
        void SelectChannel() noexcept;
    };
}