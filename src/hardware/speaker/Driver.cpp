// Created by Tube Lab. Part of the meloun project.
#include "hardware/speaker/Driver.h"
using namespace ml::speaker;

auto Driver::Create(const Config& config) noexcept -> std::shared_ptr<Driver>
{
    // Convert channels list into the map of states
    std::map<std::string, uint> channelsMap;
    for (uint i = 0; i < config.Channels.size(); ++i)
    {
        channelsMap[config.Channels[i]] = i;
    }

    // Create the driver
    auto driver = std::make_shared<Driver>();
    driver->Amplifier_ = config.Amplifier;
    driver->ChannelsMap_ = channelsMap;
    driver->Channels_ = std::vector<Channel>(config.Channels.size());

    return driver;
}

auto Driver::Open(const std::string& channel) noexcept -> Result<>
{
    // Opens the channel on the amplifier
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<>
    {
        if (!Amplifier_->Open(index))
        {
            return std::unexpected { AE_ChannelOpened };
        }

        Channels_[index].State = CS_Opened;
        Channels_[index].ExpiresAt = TimeNow() + 1000;
        return {};
    });
}

auto Driver::Prolong(const std::string& channel) noexcept -> Result<>
{
    // Prolongs the channel opening state by another 1000(ms)
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<>
    {
        if (Channels_[index].State == CS_Closed)
        {
            return std::unexpected { AE_ChannelClosed };
        }

        Channels_[index].ExpiresAt = TimeNow() + 1000;
        return {};
    });
}

auto Driver::Activate(const std::string& channel, bool urgently) noexcept -> Result<std::future<void>>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<std::future<void>>
    {
        if (Channels_[index].State == CS_Closed)
        {
            return std::unexpected { AE_ChannelClosed };
        }

        // Cancel the pending deactivation
        if (Channels_[index].State == CS_PendingDeactivation || Channels_[index].State == CS_PendingTermination)
        {
            FulfillListeners(Channels_[index].DeactivationListeners);
        }

        // If the amplifier is working -> immediately return the result
        if (Amplifier_->Working())
        {
            Channels_[index].State = CS_Active;
            return MakeFulfilledFuture();
        }

        // Otherwise try to use the code
        Amplifier_->StartUp(true);
        Channels_[index].State = CS_PendingActivation;

        Channels_[index].ActivationListeners.emplace_back();
        return Channels_[index].ActivationListeners.back().get_future();
    });
}

auto Driver::Deactivate(const std::string& channel, bool urgently) noexcept -> Result<std::future<void>>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<std::future<void>>
    {
        if (Channels_[index].State != CS_Active)
        {
            return std::unexpected { AE_ChannelInactive };
        }

        return DeactivateChannel(index, urgently, false);
    });
}

auto Driver::Enqueue(const std::string& channel, const audio::Track& audio) noexcept -> Result<std::future<void>>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<std::future<void>>
    {
        auto result = Amplifier_->Enqueue(index, audio);
        return result ? std::unexpected { BindDriverError(result.error()) } : Result<std::future<void>> { std::move(result.value()) };
    });
}

auto Driver::Clear(const std::string& channel) noexcept -> Result<>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<>
    {
        auto result = Amplifier_->Skip(index);
        return result ? std::unexpected { BindDriverError(result.error()) } : Result<> {};
    });
}

auto Driver::Skip(const std::string& channel) noexcept -> Result<>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<>
    {
        auto result = Amplifier_->Clear(index);
        return result ? std::unexpected { BindDriverError(result.error()) } : Result<> {};
    });
}

auto Driver::DurationLeft(const std::string& channel) const noexcept -> Result<time_t>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<time_t>
    {
        auto result = Amplifier_->DurationLeft(index);
        return result ? std::unexpected { BindDriverError(result.error()) } : Result<time_t> { result.value() };
    });
}

auto Driver::DurationLeft() const noexcept -> Result<time_t>
{
    std::lock_guard _ { ChannelsLock_ };
    {
        std::optional<time_t> longest;
        for (uint i = 0; i < Channels_.size(); ++i)
        {
            auto result = Amplifier_->DurationLeft(i);
            longest = result && (!longest || *result >= *longest) ? *result : *longest;
        }

        return longest ? Result < time_t > {*longest } : std::unexpected { AE_AllChannelsClosed };
    }
}

auto Driver::State(const std::string &channel) const noexcept -> Result<ChannelState>
{
    return MapToIndex(channel).and_then([&](uint index) -> Result<ChannelState>
    {
        return Channels_[index].State;
    });
}

auto Driver::ActivationDuration(bool urgently) const noexcept -> time_t
{
    return Amplifier_->StartupDuration(urgently);
}

auto Driver::DeactivationDuration(bool urgently) const noexcept -> time_t
{
    return Amplifier_->ShutdownDuration(urgently);
}

void Driver::Mainloop(const std::stop_token& token) noexcept
{
    while (!token.stop_requested())
    {
        std::unique_lock _ { ChannelsLock_ };
        auto time = TimeNow();

        // Close expired channels
        for (uint64_t i = 0; i < Channels_.size(); ++i)
        {
            if (Channels_[i].ExpiresAt && time >= *Channels_[i].ExpiresAt)
            {
                DeactivateChannel(i, false, true);
            }
        }

        // Update states
        for (auto& ch : Channels_)
        {
            if (ch.State == CS_PendingActivation && Amplifier_->Working())
            {
                ch.State = CS_Active;
                FulfillListeners(ch.ActivationListeners);
            }

            if (ch.State == CS_PendingTermination && !Amplifier_->Working())
            {
                ch.State = CS_Closed;
                FulfillListeners(ch.DeactivationListeners);
            }

            if (ch.State == CS_PendingDeactivation && !Amplifier_->Working())
            {
                ch.State = CS_Opened;
                FulfillListeners(ch.DeactivationListeners);
            }
        }

        _.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds { 20 });
    }
}

auto Driver::MapToIndex(const std::string& channel) const noexcept -> Result<uint>
{
    auto it = ChannelsMap_.find(channel);
    return it == ChannelsMap_.end() ? std::unexpected { AE_ChannelNotFound } : Result<uint> { it->second };
}

auto Driver::DeactivateChannel(uint channel, bool urgently, bool terminate) noexcept -> std::future<void>
{
    // Cancel the listeners waiting for activation
    if (Channels_[channel].State == CS_PendingActivation)
    {
        FulfillListeners(Channels_[channel].ActivationListeners);
    }

    // Determine whether the amplifier should shut down
    auto active = std::count_if(Channels_.begin(), Channels_.end(), [&](const Channel& ch)
    {
        return ch.State == CS_Active;
    });

    if (active > 1)
    {
        Channels_[channel].State = terminate ? CS_Closed : CS_Opened;
        return MakeFulfilledFuture();
    }

    Amplifier_->ShutDown(urgently);
    Channels_[channel].State = terminate ? CS_PendingDeactivation : CS_PendingTermination;

    Channels_[channel].DeactivationListeners.emplace_back();
    return Channels_[channel].DeactivationListeners.back().get_future();
}

auto Driver::MakeFulfilledFuture() noexcept -> std::future<void>
{
    std::promise<void> p;
    p.set_value();
    return p.get_future();
}

void Driver::FulfillListeners(std::vector<std::promise<void>>& list) noexcept
{
    for (auto& v : list)
    {
        v.set_value();
    }
    list.clear();
}

auto Driver::BindDriverError(amplifier::ActionError err) noexcept -> ActionError
{
    if (err == amplifier::AE_Shutdown) return AE_ChannelInactive; // channel is inactive if the amplifier isn't working
    if (err == amplifier::AE_ChannelClosed) return AE_ChannelClosed;
    if (err == amplifier::AE_IncompatibleTrack) return AE_IncompatibleTrack;
    std::unreachable();
}

