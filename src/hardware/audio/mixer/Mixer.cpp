// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/mixer/Mixer.h"
using namespace ml::audio;

#include <iostream>

auto Mixer::Create(uint channels, Transition func, SDL_AudioSpec spec, const std::optional<std::string> &audioDevice)
    -> std::shared_ptr<Mixer>
{
    auto players = std::vector<std::shared_ptr<Player>>(channels);
    for (size_t i = 0; i < players.size(); ++i)
    {
        players[i] = Player::Create(spec, audioDevice);
    }
    
    return std::shared_ptr<Mixer> {
        new Mixer { func, players }
    };
}

Mixer::~Mixer()
{
    for (auto& ch : Channels_)
    {
        ch->Clear();
    }
}

auto Mixer::Enqueue(uint channel, const Track& audio) noexcept -> std::future<void>
{
    return Channels_[channel]->Enqueue(audio);
}

void Mixer::Clear(uint channel) noexcept
{
    Channels_[channel]->Clear();
}

void Mixer::Skip(uint channel) noexcept
{
    Channels_[channel]->Skip();
}

void Mixer::Pause(uint channel) noexcept
{
    Channels_[channel]->Pause();
}

void Mixer::Resume(uint channel) noexcept
{
    Channels_[channel]->Resume();

    for (int64_t i = channel - 1; i >= 0; ++i)
    {
        Channels_[i]->Pause();
    }
}

void Mixer::Mute(uint channel) noexcept
{
    Channels_[channel]->Mute();
}

void Mixer::Unmute(uint channel) noexcept
{
    Channels_[channel]->Unmute();
}

auto Mixer::Paused(uint channel) const noexcept -> bool
{
    return Channels_[channel]->Paused();
}

auto Mixer::Muted(uint channel) const noexcept -> bool
{
    return Channels_[channel]->Muted();
}

auto Mixer::DurationLeft(uint channel) const noexcept -> time_t
{
    return Channels_[channel]->DurationLeft();
}

auto Mixer::DurationLeft() const noexcept -> time_t
{
    time_t longest = 0;
    for (size_t i = 0; i < Channels(); ++i)
    {
        longest = std::max(longest, Channels_[i]->DurationLeft());
    }

    return longest;
}

auto Mixer::Channels() const noexcept -> size_t
{
    return Channels_.size();
}

auto Mixer::TransitionEffect() const noexcept -> Transition
{
    return Transition_;
}

Mixer::Mixer(Transition func, const std::vector<std::shared_ptr<Player>>& channels) noexcept
    : Transition_(func), Channels_(channels)
{
    //ChannelStates_.push_back(false);
}
