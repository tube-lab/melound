// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/mixer/ChannelsMixer.h"
using namespace ml::audio;

#include <iostream> // TODO: Debug

auto ChannelsMixer::Create(uint channels, SDL_AudioSpec spec, const std::optional<std::string> &audioDevice)
    -> std::shared_ptr<ChannelsMixer>
{
    auto players = std::vector<std::shared_ptr<Player>>(channels);
    for (auto& player : players)
    {
        player = Player::Create(spec, audioDevice);
        player->Resume();
    }
    
    return std::shared_ptr<ChannelsMixer> {
        new ChannelsMixer {players }
    };
}

auto ChannelsMixer::Enqueue(uint channel, const Track& audio) noexcept -> std::future<void>
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

void ChannelsMixer::Mute(uint channel) noexcept
{
    UpdateChannel(channel, true);
}

void ChannelsMixer::Unmute(uint channel) noexcept
{
    UpdateChannel(channel, false);
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
    for (size_t i = 0; i < Channels(); ++i)
    {
        longest = std::max(longest, Channels_[i]->DurationLeft());
    }

    return longest;
}

auto ChannelsMixer::Channels() const noexcept -> size_t
{
    return Channels_.size();
}

ChannelsMixer::ChannelsMixer(const std::vector<std::shared_ptr<Player>>& channels) noexcept
    : Channels_(channels)
{
    MutedChannels_.resize(channels.size(), true);
}

void ChannelsMixer::UpdateChannel(size_t channel, bool muted) noexcept
{
    std::lock_guard _ {MutedChannelsLock_ };
    MutedChannels_[channel] = muted;

    // Mute all the channels except the first unmuted one
    bool found = false;
    for (size_t i : std::views::iota(0ull, Channels_.size()) | std::views::reverse)
    {
        if (!MutedChannels_[i] && !found)
        {
            Channels_[i]->Unmute();
            found = true;
        }
        else
        {
            Channels_[i]->Mute();
        }
    }
}
