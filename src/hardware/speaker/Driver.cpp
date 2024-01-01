// Created by Tube Lab. Part of the meloun project.
#include "hardware/speaker/Driver.h"
using namespace ml::speaker;

auto Driver::Create(const Config& config) noexcept -> std::shared_ptr<Driver>
{
    // Convert channels list into the map of states
    std::vector<Channel> channels;
    for (uint i = 0; i < config.Channels.size(); ++i)
    {
        channels[i] = { .Index = i, .Name = config.Channels[i] };
    }

    // Create the driver
    auto driver = std::make_shared<Driver>();
    driver->Amplifier_ = config.Amplifier;
    driver->Channels_ = channels;

    return driver;
}

auto Driver::Open(const std::string& channel) noexcept -> Result<>
{
    std::lock_guard _ { ChannelsLock_ };
    return RequireExistingChannel(channel).and_then([&](uint index) -> Result<>
    {
        if (Channels_[index].State != CS_Closed)
        {
            return std::unexpected {AE_ChannelOpened };
        }

        SetState(index, CS_Opened);
        return {};
    });
}

auto Driver::Close(const std::string& channel) noexcept -> Result<>
{
    std::lock_guard _ { ChannelsLock_ };
    return RequireExistingChannel(channel).and_then([&](uint index) -> Result<>
    {
        if (Channels_[index].State == CS_Closed)
        {
            return std::unexpected {AE_ChannelClosed };
        }

        SetState(index, CS_Opened);
        return {};
    });
}

auto Driver::Prolong(const std::string& channel) noexcept -> Result<>
{
    std::lock_guard _ { ChannelsLock_ };
    return RequireExistingChannel(channel).and_then([&](uint index) -> Result<>
    {
        if (Channels_[index].State == CS_Closed)
        {
            return std::unexpected {AE_ChannelClosed };
        }

        Channels_[index].ExpiresAt = TimeNow() + 1000;
        return {};
    });
}

auto Driver::Activate(const std::string& channel, bool urgently) noexcept -> Result<std::future<void>>
{
    std::lock_guard _ { ChannelsLock_ };
    return RequireExistingChannel(channel).and_then([&](uint index) -> Result<std::future<void>>
    {
        if (Channels_[index].State != CS_Closed)
        {
            return std::unexpected {AE_ChannelOpened };
        }

        auto listener = InsertStateListener(index, CS_Active);
        SetState(index, CS_Opened);
        return std::{};
    });
}

auto Driver::Deactivate(const std::string& channel, bool urgently) noexcept -> Result<std::future<void>>
{
    return ml::speaker::Driver::Result<std::future<void>>();
}

auto Driver::Enqueue(const std::string& channel, const audio::Track& audio) noexcept -> Result<std::future<void>>
{
    return ml::speaker::Driver::Result<std::future<void>>();
}

auto Driver::Clear(const std::string& channel) noexcept -> Result<>
{
    return ml::speaker::Driver::Result();
}

auto Driver::Skip(const std::string& channel) noexcept -> Result<>
{
    return ml::speaker::Driver::Result();
}

auto Driver::DurationLeft(const std::string& channel) const noexcept -> Result<time_t>
{

}

auto Driver::DurationLeft() const noexcept -> Result<time_t>
{
    return nullptr;
}

auto Driver::State(const std::string& channel) const noexcept -> Result<ChannelState>
{
    return ml::speaker::Driver::Result<ChannelState>();
}

auto Driver::ActivationDuration() const noexcept -> time_t
{
    return Amplifier_->StartupDuration();
}

auto Driver::DeactivationDuration() const noexcept -> time_t
{
    return Amplifier_->ShutdownDuration();
}

void Driver::Mainloop(const std::stop_token& token) noexcept
{
    while (!token.stop_requested())
    {
        std::unique_lock _ { ChannelsLock_ };
        auto time = TimeNow();
        uint active = CountActiveChannels();

        // Updates states when necessary
        for (auto& channel : Channels_)
        {
            if (time >= channel.ExpiresAt)
            {
                SetState(channel.Index, CS_Closed);
                continue;
            }

            if (channel.State == CS_PendingShutdown && (!Amplifier_->Working() || active > 1))
            {
                SetState(channel.Index, CS_Closed);
                continue;
            }

            if (channel.State == CS_PendingActivation && Amplifier_->Working())
            {
                SetState(channel.Index, CS_Active);
                continue;
            }

            if (channel.State == CS_PendingDeactivation && (!Amplifier_->Working() || active > 0))
            {
                SetState(channel.Index, CS_Opened);
                continue;
            }
        }

        _.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds { 20 });
    }
}

void Driver::SetState(uint channel, ChannelState state) noexcept
{
    std::lock_guard _ { ChannelsLock_ };
    {
        std::vector<ChannelState> fulfillStates;

        if (state == CS_Closed)
        {
            Amplifier_->Close(channel);
            fulfillStates.push_back(CS_Closed);
        }

        if (state == CS_Opened)
        {
            Amplifier_->Open(channel);
            fulfillStates.push_back(CS_Opened);
        }

        if (state == CS_PendingShutdown || state == CS_PendingDeactivation || state == CS_PendingUrgentDeactivation)
        {
            Amplifier_->(channel);
            if (CountActiveChannels() == 1)
            {
                Amplifier_->Shutdown(state == CS_PendingUrgentDeactivation);
            }

            fulfillStates.push_back(state);
        }

        if (state == CS_PendingActivation || state == CS_PendingUrgentActivation)
        {
            Amplifier_->Open(channel);
            if (CountActiveChannels() == 1)
            {
                Amplifier_->Shutdown(state == CS_PendingUrgentActivation);
            }

            fulfillStates.push_back(state);
        }

        // Update the channel state
        Channels_[channel].State = state;

        for (auto s : fulfillStates)
        {
            for (auto& l : Channels_[channel].Listeners[s])
            {
                l.set_value();
            }
            Channels_[channel].Listeners[s].clear();
        }
    }
}
