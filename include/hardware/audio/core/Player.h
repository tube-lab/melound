// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Track.h"
#include "utils/Log.h" // TODO: Debug

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
     * Built on top of SDL audio subsystem.
     * Provides pause/resume methods.
     * Provides mute/unmute methods.
     * Supports queue, so it is fully suitable for VoIP applications.
     */
    class Player
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
        static auto Create(SDL_AudioSpec spec, const std::optional<std::string>& device = std::nullopt) -> std::shared_ptr<Player>;
        ~Player();

        /** Plays the audio track. Doesn't clear the pause state. */
        auto Enqueue(const Track& audio) noexcept -> std::future<void>;

        /** Empties the queue. Playback will be stopped immediately. Doesn't clear the pause state. */
        void Clear() noexcept;

        /** Drops the first track in the queue and immediately moves to the next one. */
        void Skip() noexcept;

        /** Temporary pauses a playback. */
        void Pause() noexcept;

        /** Resumes a previously paused playback. */
        void Resume() noexcept;

        /** Mutes the player.*/
        void Mute() noexcept;

        /** Unmutes the player. */
        void Unmute() noexcept;

        /** Returns that pause state. */
        auto Paused() const noexcept -> bool;

        /** Returns the mute state. */
        auto Muted() const noexcept -> bool;

        /** Determines for how long the player will continue to play. */
        auto DurationLeft() const noexcept -> time_t;

    private:
        Player() noexcept = default;
        Player(const Player&) noexcept = delete;
        Player(Player&&) noexcept = delete;

        static void AudioSupplier(void* userdata, Uint8* stream, int len) noexcept;
        void DropFirstEntry() noexcept;
        void ApplyDevicePause() noexcept;
    };
}