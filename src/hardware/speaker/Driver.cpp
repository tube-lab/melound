// Created by Tube Lab. Part of the meloun project.
#include "hardware/speaker/Driver.h"
using namespace ml::speaker;

auto Driver::Create(const Config& config) noexcept -> std::shared_ptr<Driver>
{
    // Convert channels list into the map of states
    std::unordered_map<std::string, Channel> channels;
    for (uint i = 0; i < config.Channels.size(); ++i)
    {
        channels[config.Channels[i]] = { .Index = i };
    }

    // Create the driver
    auto driver = std::make_shared<Driver>();
    driver->Amplifier_ = config.Amplifier;
    driver->Channels_ = channels;

    return driver;
}

auto Driver::Open(const std::string& channel) noexcept -> Result<>
{
    return SetDesiredChannelState(channel, CS_Opened);
}

auto Driver::Close(const std::string& channel) noexcept -> Result<>
{
    return SetDesiredChannelState(channel, CS_Closed);
}

auto Driver::Prolong(const std::string& channel) noexcept -> Result<>
{
    return SetDesiredChannelState(channel, CS_);
}

auto Driver::Activate(const std::string& channel, bool urgent) noexcept -> Result<std::future<void>>
{

}

auto Driver::Deactivate(const std::string &channel) noexcept -> Result<std::future<void>>
{

}

auto Driver::Enqueue(const std::string& channel, const audio::Track& audio) noexcept -> Result<std::future<void>>
{
    std::lock_guard _ { ChannelsLock_ };
    return RequireActive(channel).and_then([&](const Channel* info) -> Result<std::future<void>>
    {
        auto result = Amplifier_->Enqueue(info->Index, audio);
        if (!result && result.error() == amplifier::AE_BadTrack)
        {
            return std::unexpected { DE_BadTrack };
        }

        return std::move(*result);
    });
}

auto Driver::Clear(const std::string &channel) noexcept -> Result<>
{
    std::lock_guard _ { ChannelsLock_ };
    return RequireActive(channel).transform([&](const Channel* info)
    {
        (void)Amplifier_->Clear(info->Index);
    });
}

auto Driver::Skip(const std::string& channel) noexcept -> Result<>
{
    std::lock_guard _ { ChannelsLock_ };
    return RequireActive(channel).transform([&](const Channel* info)
    {
        (void)Amplifier_->Skip(info->Index);
    });
}

auto Driver::DurationLeft(const std::string& channel) const noexcept -> Result<time_t>
{
    std::lock_guard _ { ChannelsLock_ };
    return RequireActive(channel).and_then([&](const Channel* info) -> Result<time_t>
    {
        return Amplifier_->DurationLeft(info->Index);
    });
}

auto Driver::DurationLeft() const noexcept -> Result<time_t>
{
    std::lock_guard _ { ChannelsLock_ };

    time_t best = LONG_MAX;
    for (const auto& [name, info] : Channels_)
    {
        if (info.State == CS_Active)
        {
            auto channel = Amplifier_->DurationLeft(info.Index);
            best = std::max(best == LONG_MAX ? 0 : best, );
        }
    }

    return best == LONG_MAX ? std::unexpected { DE_NotActive } : Result<time_t> { best };
}

auto Driver::State(const std::string &channel) const noexcept -> Result<ChannelState>
{
    std::lock_guard _ {ChannelsLock_ };
    return RequireExisting(channel).transform([&](const Channel* info)
    {
        return info->State;
    });
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
        std::this_thread::sleep_for(std::chrono::milliseconds { 20 });
        std::lock_guard _ { ChannelsLock_ };
        {
            for (const auto& )
        }
    }
}

auto Driver::RequireActive(const std::string& channel) const noexcept -> Result<const Channel*>
{
    return RequireExisting(channel).and_then([&](const Channel* info) -> Result<const Channel*>
    {
        if (info->State != CS_Active)
        {
            return std::unexpected { DE_NotActive };
        }

        return info;
    });
}

auto Driver::RequireExisting(const std::string &channel) const noexcept -> Result<const Channel*>
{
    auto it = Channels_.find(channel);
    if (it == Channels_.end())
    {
        return std::unexpected { DE_NotFound };
    }

    return &it->second;
}

void Driver::SetDesiredChannelState(const std::string& channel, ChannelState state) noexcept
{
    std::lock_guard _ { ChannelsLock_ };
    {
        if (Channels_[channel].State != state)
        {
            Channels_[channel].State = state;
            FulfillChannelStateListeners(channel);
            SendChannelsToAmplifier();
        }
    }
}

void Driver::FulfillChannelStateListeners(const std::string &channel)
{
    std::lock_guard _ { ChannelsLock_ };
    {
        auto listeners = Channels_[channel].Listeners | std::views::filter([&](auto& v)
        {

        });

        for (auto& state : listeners)
        {
            promise.set_value();
        }

    }
}

void Driver::SendChannelsToAmplifier() noexcept
{
    std::lock_guard _ { ChannelsLock_ };
    {
        std::vector<amplifier::ChannelState> states;
        for (const auto& [name, channel] : Channels_)
        {
            states.push_back(ToAmplifierChannelState(channel.State));
        }

        Amplifier_->NotifyAboutChannelsChange(states);
    }
}

auto Driver::ToAmplifierChannelState(ChannelState state) noexcept -> amplifier::ChannelState
{
    if (state == CS_Closed) return amplifier::CS_Closed;
    if (state == CS_Opened) return amplifier::CS_Opened;
    if (state == CS_Active) return amplifier::CS_Active;
    if (state == CS_UrgentActive) return amplifier::CS_UrgentActive;
    if (state == CS_PendingActivation) return amplifier::CS_Active;
    if (state == CS_PendingDeactivation) return amplifier::CS_Opened;
}

auto Driver::TranslateError(amplifier::ActionError err) noexcept -> ActionError
{
    if (err == amplifier::AE_Inactive) return AE_Inactive;
    if (err == amplifier::AE_BadTrack) return AE_BadTrack;
}
