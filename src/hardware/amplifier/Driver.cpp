// Created by Tube Lab. Part of the meloun project.
#include "hardware/amplifier/Driver.h"
using namespace ml::amplifier;

auto Driver::Enqueue(uint channel, const audio::Track& track) -> std::expected<std::future<void>, ActionError>
{
    std::lock_guard _ { DeviceStateLock_ };
    return ActionWrapper(channel).and_then([&]() -> std::expected<std::future<void>, ActionError>
    {
        auto p = DoEnqueue(channel, track);
        if (!p)
        {
            return std::unexpected { AE_IncompatibleTrack };
        }

        return std::move(*p);
    });
}

auto Driver::Skip(uint channel) noexcept -> std::expected<void, ActionError>
{
    std::lock_guard _ { DeviceStateLock_ };
    return ActionWrapper(channel).and_then([&]() -> std::expected<void, ActionError>
    {
        DoSkip(channel);
        return {};
    });
}

auto Driver::Clear(uint channel) noexcept -> std::expected<void, ActionError>
{
    std::lock_guard _ { DeviceStateLock_ };
    return ActionWrapper(channel).and_then([&]() -> std::expected<void, ActionError>
    {
        DoClear(channel);
        return {};
    });
}

auto Driver::DurationLeft(uint channel) const noexcept -> std::expected<time_t, ActionError>
{
    std::lock_guard _ { DeviceStateLock_ };
    return ActionWrapper(channel).and_then([&]() -> std::expected<time_t, ActionError>
    {
        return DoDurationLeft(channel);
    });
}

auto Driver::StartUp(bool urgently) noexcept -> std::future<void>
{
    std::lock_guard _ { DeviceStateLock_ };
    {
        DesiredWorking_ = true;
        UrgentStateChange_ = urgently;
        ActivationListeners_.emplace_back();
        return ActivationListeners_.back().get_future();
    }
}

auto Driver::ShutDown(bool urgently) noexcept -> std::future<void>
{
    std::lock_guard _ { DeviceStateLock_ };
    {
        DesiredWorking_ = false;
        UrgentStateChange_ = urgently;
        DeactivationListeners_.emplace_back();
        return DeactivationListeners_.back().get_future();
    }
}

auto Driver::Open(uint channel) noexcept -> bool
{
    if (!OpenedChannels_[channel])
    {
        DoOpen(channel);
        OpenedChannels_[channel] = true;
        return true;
    }

    return false;
}

auto Driver::Close(uint channel) noexcept -> bool
{
    if (OpenedChannels_[channel])
    {
        DoClose(channel);
        OpenedChannels_[channel] = false;
        return true;
    }

    return false;
}

auto Driver::Opened(uint channel) const noexcept -> bool
{
    return OpenedChannels_[channel];
}

auto Driver::Channels() const noexcept -> size_t
{
    return Channels_;
}

auto Driver::Working() const noexcept -> bool
{
    return Working_;
}

auto Driver::StartupDuration(bool urgently) const noexcept -> time_t
{
    return urgently ? UrgentStartupDuration_ : StartupDuration_;
}

auto Driver::ShutdownDuration(bool urgently) const noexcept -> time_t
{
    return urgently ? UrgentShutdownDuration_ : ShutdownDuration_;
}

Driver::Driver(const Config& config) noexcept
    : StartupDuration_(config.StartupDuration), ShutdownDuration_(config.ShutdownDuration),
      UrgentStartupDuration_(config.UrgentStartupDuration), UrgentShutdownDuration_(config.UrgentShutdownDuration),
      TickInterval_(config.TickInterval), Channels_(config.Channels)
{
    OpenedChannels_ = std::vector<std::atomic<bool>>(config.Channels);
    Mainloop_ = std::jthread { [&](const auto& token)
    {
        Mainloop(token);
    }};
}

void Driver::Mainloop(const std::stop_token& token) noexcept
{
    auto startTime = TimeNow();
    while (!token.stop_requested())
    {
        std::unique_lock _ { DeviceStateLock_ };
        auto time = TimeNow();

        // Resolve StartUp/ShutDown calls when the device is already active/inactive.
        FulfillListeners(Working_ ? ActivationListeners_ : DeactivationListeners_);

        if (Working_ != DesiredWorking_)
        {
            if (DesiredWorking_ && DoActivation(time, time - startTime, UrgentStateChange_))
            {
                Working_ = true;
                FulfillListeners(ActivationListeners_);
            }

            if (!DesiredWorking_ && DoDeactivation(time, time - startTime, UrgentStateChange_))
            {
                Working_ = false;
                FulfillListeners(DeactivationListeners_);
            }
        }
        else
        {
            startTime = time;
        }

        _.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds { TickInterval_ });
    }
}

auto Driver::ActionWrapper(uint channel) const noexcept -> std::expected<void, ActionError>
{
    std::lock_guard _ { DeviceStateLock_ };
    {
        if (!Working_) return std::unexpected { AE_Shutdown };
        if (!OpenedChannels_[channel]) return std::unexpected { AE_ChannelClosed };
        return {};
    }
}

void Driver::FulfillListeners(std::vector<std::promise<void>>& listeners) noexcept
{
    for (auto& promise : listeners)
    {
        promise.set_value();
    }
    listeners.clear();
}
