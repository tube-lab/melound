// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "BellConfig.h"
#include "hardware/audio/AudioPlayer.h"

#include <optional>
#include <vector>
#include <memory>
#include <future>
#include <map>

namespace ml
{
    /**
     * A driver for a bell driver.
     * Exception safe, but not thread safe.
     */
    class BellDriver
    {
        AudioPlayer Player_;
        std::atomic_int TimeoutImplementation_;

    public:
        static auto Create(const BellConfig& config) noexcept -> std::shared_ptr<BellDriver>;
        ~BellDriver();

        auto Open(std::string_view sink, char mode) noexcept -> bool;
        auto Close(std::string_view sink) noexcept -> bool;

        auto Play(std::string_view sink, const Audio& audio) noexcept -> bool;
        void Stop() noexcept;

        auto BufferDuration(std::string_view sink) -> time_t;
        auto OpeningTime(std::string_view sink, char mode) -> time_t;
        auto Timeout(std::string_view sink) noexcept -> time_t;

    private:
        BellDriver(const BellConfig& config) noexcept;
        BellDriver(const BellDriver&) noexcept = delete;
        BellDriver(BellDriver&&) noexcept = delete;
    };
}