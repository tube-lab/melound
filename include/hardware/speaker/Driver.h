// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Config.h"
#include "hardware/audio/Player.h"
#include "hardware/relay/Driver.h"
#include "utils/Time.h"

#include <optional>
#include <vector>
#include <memory>
#include <future>
#include <thread>
#include <map>

namespace ml::speaker
{
    /**
     * @brief A driver for a speaker driver.
     * @safety Exception safe, but not thread safe.
     */
    class Driver
    {


    public:
        static auto Create(const Config& config) noexcept -> std::shared_ptr<Driver>;
        ~Driver();

        /**
         * @brief Opens the speaker sink, blocks the playback of other sinks with lower priority.
         * Opened sink should be prolonged each 1000(ms) otherwise it will be closed automatically.
         * Fails if think do not exist.
         */
        auto Open(const std::string& sink) noexcept -> bool;

        /**
         * @brief Prolongs the opened sink.
         * Fails only if the sink has already been closed.
         */
        auto Prolong(const std::string& sink) noexcept -> bool;

        /**
         * @brief Actually prepares the sink for the music playback.
         * The returned future is fulfilled when the speaker is ready to play the music.
         * Fails if the targeted sink will be closed in progress or has been closed when the function is called.
         */
        auto Grab(const std::string& sink, bool urgent) noexcept -> std::future<bool>;

        /**
         * @brief Notifies the speaker that no music will be played in the nearest future.
         */
        void Release(const std::string& sink) noexcept;

        auto Play(const audio::Track& audio) noexcept -> std::future<bool>;
        void Stop() noexcept;

        auto BufferDuration() -> time_t;
        auto Opened() const noexcept -> std::optional<std::string>;

        // Bell stats
        auto GrabbingTime(bool urgent) -> std::optional<time_t>;

    private:
        Driver(const Config& config) noexcept;
        Driver(const Driver&) noexcept = delete;
        Driver(Driver&&) noexcept = delete;

        void ControlLoop(const std::stop_token& token) noexcept;
        void CloseExpiredSinks(time_t time) noexcept;
    };
}