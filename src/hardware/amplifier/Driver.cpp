// Created by Tube Lab. Part of the meloun project.
#include "hardware/amplifier/Driver.h"
using namespace ml::amplifier;

auto Driver::Activate() noexcept -> std::future<void>
{
    std::lock_guard _ {DeviceStateLock_ };
    {
        DesiredActive_ = true;
        ActivationListeners_.emplace_back();
        return ActivationListeners_.back().get_future();
    }
}

auto Driver::Deactivate() noexcept -> std::future<void>
{
    std::lock_guard _ {DeviceStateLock_ };
    {
        DesiredActive_ = false;
        DeactivationListeners_.emplace_back();
        return DeactivationListeners_.back().get_future();
    }
}

auto Driver::Open(uint channel) noexcept -> bool
{
    if (!OpenedChannels_[channel])
    {
        DoOpen(channel);
        return true;
    }

    return false;
}

auto Driver::Close(uint channel) noexcept -> bool
{
    if (OpenedChannels_[channel])
    {
        DoClose(channel);
        return true;
    }

    return false;
}

auto Driver::Active() const noexcept -> bool
{
    return Active_;
}

auto Driver::Opened(uint channel) const noexcept -> bool
{
    return OpenedChannels_[channel];
}

auto Driver::Channels() const noexcept -> size_t
{
    return Channels_;
}

auto Driver::StartupDuration() const noexcept -> time_t
{
    return StartupDuration_;
}

auto Driver::ShutdownDuration() const noexcept -> time_t
{
    return ShutdownDuration_;
}

Driver::Driver(time_t startupDuration, time_t shutdownDuration, time_t tickInterval, size_t channels) noexcept
    : StartupDuration_(startupDuration), ShutdownDuration_(shutdownDuration),
      TickInterval_(tickInterval), Channels_(channels)
{
    OpenedChannels_ = std::vector<std::atomic<bool>>(channels);
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
        auto time = TimeNow();
        std::unique_lock _ { DeviceStateLock_ };

        // Resolve Activate/Deactivate calls when the device is already active/inactive.
        FulfillListeners(Active_ ? ActivationListeners_ : DeactivationListeners_);

        if (Active_ != DesiredActive_)
        {
            if (DesiredActive_ && DoActivation(time, time - startTime))
            {
                Active_ = true;
                FulfillListeners(ActivationListeners_);
            }

            if (!DesiredActive_ && DoDeactivation(time, time - startTime))
            {
                Active_ = false;
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

void Driver::FulfillListeners(std::vector<std::promise<void>>& listeners) noexcept
{
    for (auto& promise : listeners)
    {
        promise.set_value();
    }
    listeners.clear();
}

auto Driver::ShouldBeActive(uint channel) const noexcept -> std::expected<void, ActionError>
{
    if (!Active_)
    {
        return std::unexpected { AE_Inactive };
    }

    if (!OpenedChannels_[channel])
    {
        return std::unexpected { AE_ChannelClosed };
    }

    return {};
}
