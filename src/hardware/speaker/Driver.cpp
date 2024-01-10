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
    driver->Mainloop_ = std::jthread { [&](const auto& token) { driver->Mainloop(token); } };

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
        Channels_[index].ExpiresAt = utils::Time::Now() + 1000;
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

        Channels_[index].ExpiresAt = utils::Time::Now() + 1000;
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
        FulfillListeners(Channels_[index].DeactivationListeners);

        // If the amplifier is working -> immediately return the result
        if (Amplifier_->Working())
        {
            Channels_[index].State = CS_Active;
            return MakeFulfilledFuture();
        }

        // Otherwise try to start the amplifier up
        Amplifier_->StartUp(urgently);
        Channels_[index].State = CS_PendingActivation;

        return Channels_[index].ActivationListeners.emplace_back().get_future();
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

        // Cancel the listeners waiting for activation
        FulfillListeners(Channels_[index].ActivationListeners);

        // Determine whether the amplifier should shut down
        if (CountActive() > 1)
        {
            Channels_[index].State = CS_Opened;
            return MakeFulfilledFuture();
        }

        // Actually shut the amplifier down
        Amplifier_->ShutDown(urgently);
        Channels_[index].State = CS_PendingDeactivation;
        Channels_[index].ExpiresAt = std::nullopt;

        return Channels_[index].DeactivationListeners.emplace_back().get_future();
    });
}

auto Driver::Enqueue(const std::string& channel, const audio::Track& audio) noexcept -> Result<std::future<void>>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<std::future<void>>
    {
        auto result = Amplifier_->Enqueue(index, audio);
        return result ? Result<std::future<void>> { std::move(result.value()) } : std::unexpected { BindDriverError(result.error()) };
    });
}

auto Driver::Clear(const std::string& channel) noexcept -> Result<>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<>
    {
        auto result = Amplifier_->Skip(index);
        return result ? Result<> {} : std::unexpected { BindDriverError(result.error()) };
    });
}

auto Driver::Skip(const std::string& channel) noexcept -> Result<>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<>
    {
        auto result = Amplifier_->Clear(index);
        return result ? Result<> {} : std::unexpected { BindDriverError(result.error()) };
    });
}

auto Driver::DurationLeft(const std::string& channel) const noexcept -> Result<time_t>
{
    std::lock_guard _ { ChannelsLock_ };
    return MapToIndex(channel).and_then([&](uint index) -> Result<time_t>
    {
        auto result = Amplifier_->DurationLeft(index);
        return result ? Result<time_t> { result.value() } : std::unexpected { BindDriverError(result.error()) };
    });
}

auto Driver::State(const std::string &channel) const noexcept -> Result<ChannelState>
{
    std::lock_guard _ { ChannelsLock_ };
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

auto Driver::DurationLeft() const noexcept -> time_t
{
    std::lock_guard _ { ChannelsLock_ };
    {
        time_t longest = 0;
        for (uint i = 0; i < Channels_.size(); ++i)
        {
            auto result = Amplifier_->DurationLeft(i);
            longest = (result && *result >= longest) ? *result : longest;
        }

        return longest;
    }
}

auto Driver::Working() const noexcept -> bool
{
    return Amplifier_->Working();
}

void Driver::Mainloop(const std::stop_token& token) noexcept
{
    while (!token.stop_requested())
    {
        std::unique_lock _ { ChannelsLock_ };
        auto time = utils::Time::Now();

        // Terminate expired channels
        for (uint64_t i = 0; i < Channels_.size(); ++i)
        {
            if (Channels_[i].ExpiresAt && time >= *Channels_[i].ExpiresAt)
            {
                // Cancel the listeners waiting for activation
                FulfillListeners(Channels_[i].ActivationListeners);

                // Determine whether the amplifier should shut down
                if (CountActive() > 1)
                {
                    Amplifier_->Close(i);
                    Channels_[i].State = CS_Closed;
                    continue;
                }

                // Actually shut the amplifier down
                Amplifier_->ShutDown(false);
                Channels_[i].State = CS_PendingTermination;
                Channels_[i].ExpiresAt = std::nullopt;
            }
        }

        // Update states
        for (uint i = 0; i < Channels_.size(); ++i)
        {
            if (Channels_[i].State == CS_PendingActivation && Amplifier_->Working())
            {
                Channels_[i].State = CS_Active;
                FulfillListeners(Channels_[i].ActivationListeners);
            }

            if (Channels_[i].State == CS_PendingTermination && !Amplifier_->Working())
            {
                Amplifier_->Close(i);
                Channels_[i].State = CS_Closed;
                FulfillListeners(Channels_[i].DeactivationListeners);
            }

            if (Channels_[i].State == CS_PendingDeactivation && !Amplifier_->Working())
            {
                Channels_[i].State = CS_Opened;
                FulfillListeners(Channels_[i].DeactivationListeners);
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

auto Driver::CountActive() const noexcept -> uint
{
    return std::count_if(Channels_.begin(), Channels_.end(), [&](const Channel& ch)
    {
        return ch.State == CS_Active;
    });
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

