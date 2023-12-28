// Created by Tube Lab. Part of the meloun project.
#include "hardware/amplifier/Driver.h"
using namespace ml::amplifier;

void Driver::NotifyAboutChannelsChange(const std::vector<ChannelState>& channels) noexcept
{
    std::lock_guard _ { ChannelsLock_ };
    Channels_ = channels;
}

auto Driver::Active() const noexcept -> bool
{
    return Active_;
}

auto Driver::StartupDuration() const noexcept -> time_t
{
    return StartupDuration_;
}

auto Driver::ShutdownDuration() const noexcept -> time_t
{
    return ShutdownDuration_;
}

Driver::Driver(time_t startupDuration, time_t shutdownDuration, time_t tickInterval) noexcept
    : StartupDuration_(startupDuration), ShutdownDuration_(shutdownDuration), TickInterval_(tickInterval)
{
    Mainloop_ = std::jthread { [&](const auto& token)
    {
        Mainloop(token);
    }};
}

void Driver::Mainloop(const std::stop_token& token) noexcept
{
    auto lTime = TimeNow();
    while (!token.stop_requested())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds { TickInterval_ });
        std::lock_guard _ { ChannelsLock_ };

        auto time = TimeNow();
        Active_ = Tick(time, time - lTime, Channels_);
        lTime = time;
    }
}

auto Driver::ShouldBeActive() const noexcept -> std::expected<void, DriverError>
{
    if (!Active_)
    {
        return std::unexpected { DE_Inactive };
    }

    return {};
}
