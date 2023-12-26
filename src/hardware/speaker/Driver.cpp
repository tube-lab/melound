// Created by Tube Lab. Part of the meloun project.
#include "hardware/speaker/Driver.h"
using namespace ml::speaker;

auto Driver::Create(const Config& config) noexcept -> std::shared_ptr<Driver>
{
    // Initialize subsystems
    auto relay = relay::Driver::Create(config.PowerControlPort);
    if (!relay)
    {
        return nullptr;
    }

    auto mixer = audio::ChannelsMixer::Create(config.Channels.size(), config.AudioDevice);
    if (!mixer)
    {
        return nullptr;
    }

    // Convert channels list into the mapping
    std::unordered_map<std::string, uint> mapping;
    for (uint64_t i = 0; i < config.Channels.size(); ++i)
    {
        mapping[config.Channels[i]] = i;
    }

    // Create the driver
    auto driver = std::make_shared<Driver>();
    driver->Config_ = config;
    driver->Relay_ = relay;
    driver->Mixer_ = mixer;
    driver->ChannelsMapping_ = mapping;

    return driver;
}

void Driver::Pause() noexcept
{
    Mixer_->Pause();
}

void Driver::Resume() noexcept
{
    Mixer_->Resume();
}

auto Driver::Open(const std::string& channel) noexcept -> std::optional<DriverError>
{
    auto id = FindChannelId(channel);
    if (id == UINT_MAX)
    {
        return DE_NotFound;
    }

    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[id].State == C_Closed)
        {
            Channels_[id].State = C_Opened;
            Channels_[id].ExpiresAt = TimeNow() + 1000;
            return std::nullopt;
        }

        return DE_AlreadyOpened;
    }
}

auto Driver::Close(const std::string& channel) noexcept -> bool
{
    auto id = FindChannelId(channel);
    if (id == UINT_MAX)
    {
        return false;
    }

    Close(id);
    return true;
}

auto Driver::Prolong(const std::string& channel) noexcept -> std::optional<DriverError>
{
    auto id = FindChannelId(channel);
    if (id == UINT_MAX)
    {
        return DE_NotFound;
    }

    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[id].State != C_Closed)
        {
            Channels_[id].ExpiresAt = TimeNow() + 1000;
            return std::nullopt;
        }

        return DE_Closed;
    }
}

auto Driver::Enqueue(const std::string& channel, const audio::Track& audio) noexcept -> std::expected<std::future<void>, DriverError>
{
    auto id = FindChannelId(channel);
    if (id == UINT_MAX)
    {
        return std::unexpected { DE_NotFound };
    }

    if (Channels_[id].State != C_Active)
    {
        return std::unexpected { DE_NotActive };
    }

    auto promise = Mixer_->Enqueue(id, audio);
    if (!promise)
    {
        return std::unexpected { DE_BadTrack };
    }

    return std::move(*promise);
}

auto Driver::Clear(const std::string &channel) noexcept -> std::optional<ClearError>
{
    return std::optional<ClearError>();
}

auto Driver::Skip(const std::string &channel) noexcept -> std::optional<SkipError>
{
    return std::optional<SkipError>();
}

void Driver::ControlLoop(const std::stop_token &token) noexcept
{

}

void Driver::CloseExpiredChannels(time_t time) noexcept
{
    std::lock_guard _ { StateLock_ };
    for (auto& [name, id] : ChannelsMapping_)
    {
        if (time >= Channels_[id].ExpiresAt)
        {
            Close(name);
        }
    }
}

void Driver::Close(uint id) noexcept
{
    std::lock_guard _ { StateLock_ };
    {
        Deactivate(id);
        Channels_[id].State = C_Closed;
        Channels_[id].ExpiresAt = 0;
    }
}

auto Driver::Deactivate(uint id) noexcept -> bool
{
    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[id].State == C_PendingActivation || Channels_[id].State == C_Active)
        {
            Channels_[id].State = C_Opened;
            FulfillActivationListeners(id, false);
            return true;
        }

        return false;
    }
}

auto Driver::FindChannelId(const std::string& channel) noexcept -> uint
{
    return ChannelsMapping_.contains(channel) ? ChannelsMapping_[channel] : UINT_MAX;
}

void Driver::FulfillActivationListeners(uint id, bool result) noexcept
{
    std::lock_guard _ { StateLock_ };
    {
        for (auto& listener : Channels_[id].ActivationListeners)
        {
            listener.set_value(result);
        }
        Channels_[id].ActivationListeners.clear();
    }
}
