// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/ChannelsMixer.h"
using namespace ml::audio;

auto ChannelsMixer::Create(uint channels, const std::optional<std::string> &audioDevice)
    -> std::shared_ptr<ChannelsMixer>
{
    auto players = std::vector<std::shared_ptr<Player>>(channels);
    for (auto& player : players)
    {
        player = Player::Create(audioDevice);
        if (!player)
        {
            return nullptr;
        }

        player->Resume();
    }
    
    return std::shared_ptr<ChannelsMixer> {
        new ChannelsMixer {players }
    };
}

void ChannelsMixer::Pause() noexcept
{
    for (auto& channel : Channels_)
    {
        channel->Pause();
    }
}

void ChannelsMixer::Resume() noexcept
{
    for (auto& channel : Channels_)
    {
        channel->Resume();
    }
}

auto ChannelsMixer::Enqueue(uint channel, const Track& audio) noexcept -> std::optional<std::future<void>>
{
    return Channels_[channel]->Enqueue(audio);
}

void ChannelsMixer::Clear(uint channel) noexcept
{
    Channels_[channel]->Clear();
}

void ChannelsMixer::Skip(uint channel) noexcept
{
    Channels_[channel]->Skip();
}

void ChannelsMixer::Enable(uint channel) noexcept
{
    UpdateChannel(channel, true, std::nullopt);
}

void ChannelsMixer::Disable(uint channel) noexcept
{
    UpdateChannel(channel, false, std::nullopt);
}

void ChannelsMixer::Pause(uint channel) noexcept
{
    Channels_[channel]->Pause();
}

void ChannelsMixer::Resume(uint channel) noexcept
{
    Channels_[channel]->Resume();
}

void ChannelsMixer::Mute(uint channel) noexcept
{
    UpdateChannel(channel, std::nullopt, true);
}

void ChannelsMixer::Unmute(uint channel) noexcept
{
    UpdateChannel(channel, std::nullopt, false);
}

auto ChannelsMixer::Enabled(uint channel) const noexcept -> bool
{
    std::lock_guard _ { ChannelsStatesLock_ };
    return EnabledChannels_[channel];
}

auto ChannelsMixer::Paused(uint channel) const noexcept -> bool
{
    return Channels_[channel]->Paused();
}

auto ChannelsMixer::Muted(uint channel) const noexcept -> bool
{
    return Channels_[channel]->Muted();
}

auto ChannelsMixer::DurationLeft(uint channel) const noexcept -> time_t
{
    return Channels_[channel]->DurationLeft();
}

auto ChannelsMixer::DurationLeft() const noexcept -> time_t
{
    time_t longest = 0;
    for (size_t i = 0; i < Channels_.size(); ++i)
    {
        longest = std::max(longest, Channels_[i]->DurationLeft());
    }

    return longest;
}

auto ChannelsMixer::CountEnabled() const noexcept -> size_t
{
    std::lock_guard _ { ChannelsStatesLock_ };
    {
        return std::count(EnabledChannels_.begin(), EnabledChannels_.end(), true);
    }
}

auto ChannelsMixer::Channels() const noexcept -> size_t
{
    return Channels_.size();
}

ChannelsMixer::ChannelsMixer(const std::vector<std::shared_ptr<Player>>& channels) noexcept
    : Channels_(channels)
{
    EnabledChannels_.resize(channels.size(), false);
    MutedChannels_.resize(channels.size(), false);

    SelectChannel(); // reset everything to the initial state
}

void ChannelsMixer::UpdateChannel(size_t channel, std::optional<bool> enabled, std::optional<bool> muted) noexcept
{
    std::lock_guard _ {ChannelsStatesLock_ };
    {
        if (enabled) EnabledChannels_[channel] = *enabled;
        if (muted) MutedChannels_[channel] = *muted;

        SelectChannel();
    }
}

void ChannelsMixer::SelectChannel() noexcept
{
    std::lock_guard _ { ChannelsStatesLock_ };
    {
        // Mute all the channels except the first enabled one
        bool found = false;
        for (size_t i : std::views::iota(0ull, Channels_.size()) | std::views::reverse)
        {
            if (EnabledChannels_[i] && !found)
            {
                if (!MutedChannels_[i])
                {
                    Channels_[i]->Unmute();
                }

                found = true;
            }
            else
            {
                Channels_[i]->Mute();
            }
        }
    }
}
