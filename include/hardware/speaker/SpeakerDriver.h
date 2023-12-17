// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "SpeakerConfig.h"
#include "hardware/audio/core/Player.h"
#include "hardware/switch/SwitchDriver.h"
#include "utils/Time.h"

#include <optional>
#include <vector>
#include <memory>
#include <future>
#include <thread>
#include <map>

namespace ml
{

    /**
     * A driver for a speaker driver.
     * Exception safe, but not thread safe.
     */
    class SpeakerDriver
    {
        struct SinkInfo : SpeakerSink
        {
            std::shared_ptr<audio::Player> Player;
        };

        struct SinkStats
        {
            std::promise<bool> GrabPromise;
            time_t ExpiresAt {};
            bool Opened {};
        };

        // Protocol constants
        static constexpr time_t ProlongationDuration = 1000;

        // Initial configuration
        const SpeakerConfig Config_;
        const std::shared_ptr<SwitchDriver> Switch_;
        const std::unordered_map<std::string, SinkInfo> Sinks_;

        std::unordered_map<std::string, SinkStats> SinksStats_;
        std::mutex SinksStatsLock_;

        std::atomic<bool> DesiredAmplifierReady_;
        std::atomic<bool> AmplifierReady_;
        std::atomic<time_t> LastAmplifierActiveTime_;

        std::atomic_bool TurnedOn_;
        std::atomic_int Last_;
        std::atomic_int LastActiveTime_;

        std::jthread Controller_;

    public:
        static auto Create(const SpeakerConfig& config) noexcept -> std::shared_ptr<SpeakerDriver>;
        ~SpeakerDriver();

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
        SpeakerDriver(const SpeakerConfig& config) noexcept;
        SpeakerDriver(const SpeakerDriver&) noexcept = delete;
        SpeakerDriver(SpeakerDriver&&) noexcept = delete;

        void ControlLoop(const std::stop_token& token) noexcept;
        void CloseExpiredSinks(time_t time) noexcept;
        void ChoosePlayerByPriority() noexcept;

        auto FindSinkInfo(const std::string& name) -> const SinkInfo&;
    };
}