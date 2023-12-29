// Created by Tube Lab. Part of the meloun project.
#include "hardware/amplifier/Driver.h"
using namespace ml::amplifier;

auto Driver::NotifyAboutChannelsChange(const std::vector<ChannelState>& channels) noexcept -> bool
{
    std::lock_guard _ { ChannelsLock_ };
    {
        if (ChannelsNum_ != channels.size())
        {
            return false;
        }

        Channels_ = channels;
        return true;
    }
}

auto Driver::Active() const noexcept -> bool
{
    return Active_;
}

auto Driver::Channels() const noexcept -> size_t
{
    return ChannelsNum_;
}

auto Driver::StartupDuration() const noexcept -> time_t
{
    return StartupDuration_;
}

auto Driver::ShutdownDuration() const noexcept -> time_t
{
    return ShutdownDuration_;
}

Driver::Driver(time_t startupDuration, time_t shutdownDuration, time_t tickInterval, size_t channelsNum) noexcept
    : StartupDuration_(startupDuration), ShutdownDuration_(shutdownDuration),
      TickInterval_(tickInterval), ChannelsNum_(channelsNum)
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

auto Driver::ShouldBeActive() const noexcept -> std::expected<void, ActionError>
{
    if (!Active_)
    {
        return std::unexpected { AE_Inactive };
    }

    return {};
}
