// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/mixer/Mixer.h"
using namespace ml::audio;

#include <iostream>

auto Mixer::Create(uint channels, Transition func, SDL_AudioSpec spec, const std::optional<std::string> &audioDevice)
    -> std::shared_ptr<Mixer>
{
    SDL_Init(SDL_INIT_AUDIO);

    // Create the dummy mixer ( we need its address in audio-supplier callback )
    auto* mixer = new Mixer {};

    // Select the device ( if given )
    const char* device = audioDevice ? audioDevice->c_str() : nullptr;

    // Inject custom callback into the specs
    SDL_AudioSpec modified = spec;
    modified.callback = &Mixer::AudioSupplier;
    modified.userdata = mixer;

    // Create the output
    SDL_AudioDeviceID out = SDL_OpenAudioDevice(device, 0, &modified, nullptr, 0);
    if (!out)
    {
        delete mixer;
        return nullptr;
    }

    // Instantly unmute the audio device
    SDL_PauseAudioDevice(out, false);

    mixer->Spec_ = spec;
    mixer->Out_ = out;
    mixer->Channels_ = std::vector<Channel>(channels);

    return std::shared_ptr<Mixer> {
        mixer
    };
}

Mixer::~Mixer()
{
    for (size_t i = 0; i < Channels(); ++i)
    {
        Clear(i);
    }
}

auto Mixer::Enqueue(uint channel, const Track& audio) noexcept -> std::future<void>
{
    std::lock_guard _ { Channels_[channel].Lock };
    {
        auto& ch = Channels_[channel];

        // Add new track to the queue
        ch.Buffer.emplace_back(audio.Buffer(), std::promise<void> {}, 0);
        ch.BufferLength += audio.Buffer().size();

        return ch.Buffer.back().Listener.get_future();
    }
}

void Mixer::Clear(uint channel) noexcept
{
    std::lock_guard _ { Channels_[channel].Lock };
    {
        auto& buf = Channels_[channel].Buffer;
        while (!buf.empty())
        {
            buf.front().Listener.set_value();
            buf.pop_front();
        }
    }
}

void Mixer::Skip(uint channel) noexcept
{
    std::lock_guard _ { Channels_[channel].Lock };
    {
        auto& buf = Channels_[channel].Buffer;
        if (!buf.empty())
        {
            buf.front().Listener.set_value();
            buf.pop_front();
        }
    }
}

void Mixer::Pause(uint channel) noexcept
{
    std::lock_guard _ { Channels_[channel].Lock };
    Channels_[channel].Paused = false;
    SyncPauseState();
}

void Mixer::Resume(uint channel) noexcept
{
    std::lock_guard _ { Channels_[channel].Lock };
    Channels_[channel].Paused = true;
    SyncPauseState();
}

void Mixer::Mute(uint channel) noexcept
{
    std::lock_guard _ { Channels_[channel].Lock };
    Channels_[channel].Muted = true;
}

void Mixer::Unmute(uint channel) noexcept
{
    std::lock_guard _ { Channels_[channel].Lock };
    Channels_[channel].Muted = false;
}

auto Mixer::Paused(uint channel) const noexcept -> bool
{
    std::lock_guard _ { Channels_[channel].Lock };
    return Channels_[channel].Paused;
}

auto Mixer::Muted(uint channel) const noexcept -> bool
{
    std::lock_guard _ { Channels_[channel].Lock };
    return Channels_[channel].Muted;
}

auto Mixer::DurationLeft(uint channel) const noexcept -> time_t
{
    std::lock_guard _ { Channels_[channel].Lock };
    return Channels_[channel].BufferLength;
}

auto Mixer::DurationLeft() const noexcept -> time_t
{
    time_t longest = 0;
    for (size_t i = 0; i < Channels(); ++i)
    {
        longest = std::max(longest, DurationLeft(i));
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

void Mixer::AudioSupplier(void* userdata, Uint8* stream, int len) noexcept
{
    auto* self = (Mixer*)userdata;
    SDL_memset(stream, len, 0);

    // Find the first active channel
    for (int64_t i = self->Channels_.size() - 1; i >= 0; --i)
    {
        std::lock_guard _ { self->Channels_[i].Lock };
        if (self->Channels_[i].Paused)
        {
            continue;
        }

        // Take the data from it
        auto first = self->TakeAudioData(i, len);
        SDL_memcpy(stream, &first[0], len);

        break;
    }

    // Select the first active channel
    // Blend it with the previous one ( if it exists )
    //
}

void Mixer::SyncPauseState() noexcept
{
    // Unpause output if there is at least 1 playing channel
    for (size_t i = 0; i < Channels(); ++i)
    {
        if (!Paused(i))
        {
            SDL_PauseAudioDevice(Out_, false);
            return;
        }
    }

    // All channels are paused, so we can actually pause the output
    SDL_PauseAudioDevice(Out_, true);
}

auto Mixer::TakeAudioData(uint channel, uint64_t len) noexcept -> std::vector<uint8_t>
{
    std::vector<uint8_t> result;
    result.reserve(len);

    std::lock_guard _ { Channels_[channel].Lock };
    auto& ch = Channels_[channel];

    // Feed audio data into the stream
    size_t remaining = std::min(ch.BufferLength, (size_t)len);
    while (remaining)
    {
        auto& front = ch.Buffer.front();
        long chunk = (long)std::min(front.Data.size() - front.Idx, remaining);

        if (!ch.Muted)
        {
            result.insert(result.end(),front.Data.begin() + front.Idx, front.Data.begin() + front.Idx + chunk);
        }

        front.Idx += chunk;
        remaining -= chunk;
        ch.BufferLength -= chunk;

        // If the page ended - invoke the listener
        if (front.Idx == front.Data.size())
        {
            front.Listener.set_value();
            ch.Buffer.pop_front();
        }
    }

    return result;
}
