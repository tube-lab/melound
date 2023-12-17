// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Transition.h"
#include "hardware/audio/core/Track.h"

#include <optional>
#include <utility>
#include <string>
#include <future>
#include <mutex>
#include <deque>

namespace ml::audio
{
    // TODO: Support for the blending
    /**
     * @brief The simple mixer
     * - Allows to mix multiple channels into the single one.
     * - Each particular channel has the same capabilities as a Player instance.
     * - Mixing system based on priorities. When the channel with id=k is un-paused all channels which id's < k are muted,
     *   this works even of the channel with id=k is muted.
     * - Supports customizable channels transition blending ( based on Bezier curves )
     */
    class Mixer
    {
        struct Entry
        {
            std::vector<uint8_t> Data;
            std::promise<void> Listener;
            size_t Idx;
        };

        struct Channel
        {
            std::deque<Entry> Buffer;
            size_t BufferLength {};

            std::atomic<bool> Paused;
            std::atomic<bool> Muted;

            mutable std::mutex Lock;
        };

        Transition Transition_ {};
        SDL_AudioSpec Spec_ {};
        SDL_AudioDeviceID Out_ {};
        std::vector<Channel> Channels_ {};

        std::atomic<size_t> Active_ { UINT64_MAX };
        std::atomic<size_t> PreviousActive_ { UINT64_MAX };

    public:
        static auto Create(uint channels, Transition func, SDL_AudioSpec spec,
                           const std::optional<std::string>& audioDevice = std::nullopt)
                           -> std::shared_ptr<Mixer>;
        ~Mixer();

        /** Appends the audio track to the particular channel. Doesn't clear the pause state. */
        auto Enqueue(uint channel, const Track& audio) noexcept -> std::future<void>;

        /** Empties the channel. Channel' playback will be stopped immediately. Doesn't pauses the channel. */
        void Clear(uint channel) noexcept;

        /** Drops the first track in the channel' queue and immediately moves to the next one. */
        void Skip(uint channel) noexcept;

        /** Temporary pauses a playback. */
        void Pause(uint channel) noexcept;

        /** Resumes a previously paused playback. */
        void Resume(uint channel) noexcept;

        /** Mutes the channel.*/
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

        /** Returns the mixer transition ease function. */
        auto TransitionEffect() const noexcept -> Transition;

    private:
        Mixer() noexcept = default;
        Mixer(const Mixer&) noexcept = delete;
        Mixer(Mixer&&) noexcept = delete;

        static void AudioSupplier(void* userdata, Uint8* stream, int len) noexcept;

        void SyncPauseState() noexcept;
        auto TakeAudioData(uint channel, uint64_t len) noexcept -> std::vector<uint8_t>;
    };
}