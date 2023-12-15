// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/AudioPlayer.h"
using namespace ml;

#include <iostream>

auto AudioPlayer::Create(SDL_AudioSpec spec, const std::optional<std::string>& audioDevice) -> std::shared_ptr<AudioPlayer>
{
    SDL_Init(SDL_INIT_AUDIO);

    // Create the dummy player ( because we need its address in audio-supplier callback )
    auto* player = new AudioPlayer {};

    // Select the device ( if given )
    const char* device = audioDevice ? audioDevice->c_str() : nullptr;

    // Inject custom callback into the specs
    SDL_AudioSpec modified = spec;
    modified.callback = &AudioPlayer::AudioSupplier;
    modified.userdata = player;

    // Create the output
    SDL_AudioDeviceID out = SDL_OpenAudioDevice(device, 0, &modified, nullptr, 0);
    if (!out)
    {
        delete player;
        return nullptr;
    }

    player->Out_ = out;

    return std::shared_ptr<AudioPlayer> {
        player
    };
}

AudioPlayer::~AudioPlayer()
{
    Clear();
}

auto AudioPlayer::Enqueue(const Audio& audio) noexcept -> std::future<void>
{
    // Open the audio device ( for this audio )
    std::lock_guard _ { BufferLock_ };
    {
        // Add new track to the queue
        Buffer_.emplace_back(audio.Buffer(), std::promise<void> {}, 0);
        BufferLength_ += audio.Buffer().size();

        // We don't queue the audio because we use custom audio-supplier
        SDL_QueueAudio(Out_, &Buffer_.back().Data[0], Buffer_.back().Data.size());
        SDL_PauseAudioDevice(Out_, Paused_);

        return Buffer_.back().Listener.get_future();
    };
}

void AudioPlayer::Clear() noexcept
{
    std::lock_guard _ { BufferLock_ };
    {
        while (!Buffer_.empty())
        {
            Buffer_.front().Listener.set_value();
            Buffer_.pop_front();
        }
    };
}

void AudioPlayer::Skip() noexcept
{
    std::lock_guard _ { BufferLock_ };
    {
        Buffer_.front().Listener.set_value();
        Buffer_.pop_front();
    }
}

void AudioPlayer::Pause() noexcept
{
    Paused_ = true;
    SDL_PauseAudioDevice(Out_, true);
}

void AudioPlayer::Resume() noexcept
{
    Paused_ = false;
    SDL_PauseAudioDevice(Out_, false);
}

void AudioPlayer::Mute() noexcept
{
    Muted_ = true;
}

void AudioPlayer::Unmute() noexcept
{
    Muted_ = false;
}

auto AudioPlayer::Paused() const noexcept -> bool
{
    return Paused_;
}

auto AudioPlayer::Muted() const noexcept -> bool
{
    return Muted_;
}

auto AudioPlayer::QueueSize() const noexcept -> size_t
{
    std::lock_guard _ { BufferLock_ };
    return Buffer_.size();
}

void AudioPlayer::AudioSupplier(void* userdata, Uint8* stream, int len) noexcept
{
    auto* self = (AudioPlayer*)userdata;

    std::unique_lock lock { self->BufferLock_ };

    // Skip anything if the buffer is already empty
    if (self->Buffer_.empty())
    {
        return;
    }

    // Feed audio data into the stream
    size_t remaining = std::min(self->BufferLength_, (size_t)len);
    while (remaining)
    {
        auto& front = self->Buffer_.front();
        long chunk = (long)std::min(front.Data.size() - front.Idx, remaining);

        if (!self->Muted_)
        {
            SDL_memcpy(stream, &front.Data[front.Idx], chunk);
        }

        front.Idx += chunk;
        remaining -= chunk;
        self->BufferLength_ -= chunk;

        // If the page ended - invoke the listener
        if (front.Idx == front.Data.size())
        {
            front.Listener.set_value();
            self->Buffer_.pop_front();
        }
    }

    lock.unlock();
    std::cout << remaining << ' ' << self->Buffer_.size() << ' ' << self->BufferLength_  << "\n"; // TODO: Remove debug
}
