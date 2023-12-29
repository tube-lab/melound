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
    driver->Amplifier_ = config.Controller;
    driver->Channels_ = channels;

    return driver;
}

auto Driver::Open(const std::string& channel) noexcept -> Result<>
{

}

auto Driver::Close(const std::string& channel) noexcept -> Result<>
{

}

auto Driver::Prolong(const std::string& channel) noexcept -> Result<>
{

}

auto Driver::Activate(const std::string& channel, bool urgent) noexcept -> Result<std::future<void>>
{

}

auto Driver::Deactivate(const std::string &channel) noexcept -> Result<std::future<void>>
{
    return ml::speaker::Driver::Result<std::future<void>>();
}

auto Driver::Enqueue(const std::string& channel, const audio::Track& audio) noexcept -> Result<std::future<void>>
{
    std::lock_guard _ { StateLock_ };
    return RequireActive(channel).and_then([&](const Channel* info) -> Result<std::future<void>>
    {
        auto promise = Amplifier_->Enqueue(info->Index, audio);
        if (!promise)
        {
            return std::unexpected { DE_BadTrack };
        }

        return std::move(*promise);
    });
}

auto Driver::Clear(const std::string &channel) noexcept -> Result<>
{
    std::lock_guard _ { StateLock_ };
    return RequireActive(channel).transform([&](const Channel* info)
    {
        Amplifier_->Clear(info->Index);
    });
}

auto Driver::Skip(const std::string& channel) noexcept -> Result<>
{
    std::lock_guard _ { StateLock_ };
    return RequireActive(channel).transform([&](const Channel* info)
    {
        Amplifier_->Skip(info->Index);
    });
}

auto Driver::DurationLeft(const std::string& channel) const noexcept -> Result<time_t>
{
    std::lock_guard _ { StateLock_ };
    return RequireActive(channel).transform([&](const Channel* info)
    {
        return Amplifier_->DurationLeft(info->Index);
    });
}

auto Driver::DurationLeft() const noexcept -> Result<time_t>
{
    std::lock_guard _ { StateLock_ };

    time_t best = LONG_MAX;
    for (const auto& [name, info] : Channels_)
    {
        if (info.State == C_Active)
        {
            best = std::max(best == LONG_MAX ? 0 : best, Amplifier_->DurationLeft(info.Index));
        }
    }

    return best == LONG_MAX ? std::unexpected { DE_NotActive } : Result<time_t> { best };
}

auto Driver::State(const std::string &channel) const noexcept -> Result<ChannelState>
{
    std::lock_guard _ { StateLock_ };
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
        std::lock_guard _ { StateLock_ };
        {
            auto time = TimeNow();
            std::vector<amplifier::ChannelState> states;

            for (const auto& [name, info] : Channels_)
            {
                states.emplace_back(info.State);
            }

            for (auto& [name, info] : Channels_)
            {
                if (!Amplifier_->Active() && info.State == C_PendingDeactivation)
                {
                    info.State = CS_Opened;
                }

                if (Amplifier_->Active() && info.State == C_PendingActivation)
                {
                    info.State = CS_Active;
                }
            }
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

void Driver::SendChannelsToAmplifier() noexcept
{
    std::vector<amplifier::ChannelState> states;
    for (const auto& [name, info] : Channels_)
    {

    }
}
