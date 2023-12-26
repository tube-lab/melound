// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Track.h"
#include "Utils.h"

#include "utils/CustomConstructor.h"

#include <optional>
#include <utility>
#include <string>
#include <future>
#include <mutex>
#include <deque>
#include <unordered_map>

namespace ml::audio
{
    /**
     * @brief A simple audio thread-safe player.
     * @safety Fully exception and thread safe.
     *
     * Main features:
     * - Built on top of SDL audio subsystem.
     * - Provides pause/resume methods.
     * - Provides mute/unmute methods.
     * - Supports queue, so it is fully suitable for VoIP applications.
     *
     * Important features:
     * - The player is paused by default.
     */
    class Player : public CustomConstructor
    {
        struct Entry
        {
            std::vector<uint8_t> Data;
            std::promise<void> Listener;
            size_t Idx;
        };

        SDL_AudioSpec Spec_ {};
        SDL_AudioDeviceID Out_ {};

        std::atomic<bool> Paused_;
        std::atomic<bool> Muted_;

        std::deque<Entry> Buffer_;
        size_t BufferLength_ {};
        mutable std::mutex BufferLock_;

    public:
        /** Creates a player that utilizes the given audio output. */
        static auto Create(const std::optional<std::string>& device = std::nullopt) -> std::shared_ptr<Player>;

        /** Stops playback and closes the audio device. */
        ~Player();

        /** Plays the audio track. Doesn't clear the pause state. Fails if the track can't be resampled properly. */
        auto Enqueue(const Track& audio) noexcept -> std::optional<std::future<void>>;

        /** Empties the queue. Playback will be stopped immediately. Doesn't clear the pause state. */
        void Clear() noexcept;

        /** Drops the first track in the queue and immediately moves to the next one. */
        void Skip() noexcept;

        /** Temporary pauses a playback. */
        void Pause() noexcept;

        /** Resumes a previously paused playback. */
        void Resume() noexcept;

        /** Mutes the player. */
        void Mute() noexcept;

        /** Unmutes the player. */
        void Unmute() noexcept;

        /** Returns the pause state. */
        auto Paused() const noexcept -> bool;

        /** Returns the mute state. */
        auto Muted() const noexcept -> bool;

        /** Determines for how long the player will continue to play. */
        auto DurationLeft() const noexcept -> time_t;

    private:
        static void AudioSupplier(void* userdata, uint8_t* stream, int len) noexcept;
        void DropFirstEntry() noexcept;
        void ReviseDevicePause() noexcept;
    };
}