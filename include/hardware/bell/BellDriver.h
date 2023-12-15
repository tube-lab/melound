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

        auto Startup(bool urgent, int inactiveTimeout) noexcept -> std::future<uint>;
        auto Shutdown() noexcept -> std::future<void>;

        auto Play(const std::vector<bool>& audio) noexcept -> std::future<bool>;
        auto Stop() const noexcept -> std::optional<size_t>;

        auto StartupDuration() noexcept -> uint;

    private:
        BellDriver(const BellConfig& config) noexcept;
        BellDriver(const BellDriver&) noexcept = delete;
        BellDriver(BellDriver&&) noexcept = delete;
    };
}