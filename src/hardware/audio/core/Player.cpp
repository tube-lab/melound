// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/core/Player.h"
using namespace ml::audio;

auto Player::Create(const std::optional<std::string>& device) -> std::shared_ptr<Player>
{
    SDL_Init(SDL_INIT_AUDIO);

    // Create the dummy player ( because we need its address in audio-supplier callback )
    auto* player = new Player {};

    // Select the device ( if given )
    const char* name = device ? device->c_str() : nullptr;

    // Make default specs
    SDL_AudioSpec spec = {};
    spec.freq = 44100;
    spec.samples = 4096;
    spec.callback = &Player::AudioSupplier;
    spec.userdata = player;

    // Create the output
    SDL_AudioDeviceID out = SDL_OpenAudioDevice(name, 0, &spec, &spec, SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (!out)
    {
        delete player;
        return nullptr;
    }

    player->Spec_ = spec;
    player->Out_ = out;
    player->Paused_ = true;

    return std::shared_ptr<Player> {
        player
    };
}

Player::~Player()
{
    Clear();
    SDL_CloseAudioDevice(Out_);
}

auto Player::Enqueue(const Track& audio) noexcept -> std::optional<std::future<void>>
{
    // Open the audio device ( for this audio )
    std::lock_guard _ { BufferLock_ };
    {
        // Resample the track in order
        auto adjusted = Utils::Resample(audio, Spec_);
        if (!adjusted)
        {
            return std::nullopt;
        }

        // Add new track to the queue
        Buffer_.emplace_back(adjusted->Buffer(), std::promise<void> {}, 0);
        BufferLength_ += adjusted->Buffer().size();

        // Possibly unpause the audio device
        ReviseDevicePause();

        return Buffer_.back().Listener.get_future();
    }
}

void Player::Clear() noexcept
{
    std::lock_guard _ { BufferLock_ };
    {
        while (!Buffer_.empty())
        {
            Buffer_.front().Listener.set_value();
            Buffer_.pop_front();
        }

        ReviseDevicePause();
    }
}

void Player::Skip() noexcept
{
    std::lock_guard _ { BufferLock_ };
    {
        if (!Buffer_.empty())
        {
            DropFirstEntry();
        }
    }
}

void Player::Pause() noexcept
{
    Paused_ = true;
    ReviseDevicePause();
}

void Player::Resume() noexcept
{
    Paused_ = false;
    ReviseDevicePause();
}

void Player::Mute() noexcept
{
    Muted_ = true;
}

void Player::Unmute() noexcept
{
    Muted_ = false;
}

auto Player::Paused() const noexcept -> bool
{
    return Paused_;
}

auto Player::Muted() const noexcept -> bool
{
    return Muted_;
}

auto Player::DurationLeft() const noexcept -> time_t
{
    std::lock_guard _ { BufferLock_ };
    {
        return Utils::EstimateBufferDuration(BufferLength_, Spec_);
    }
}

void Player::AudioSupplier(void* userdata, uint8_t* stream, int len) noexcept
{
    auto* self = (Player*)userdata;
    std::unique_lock lock { self->BufferLock_ };

    // Empty the buffer ( required by SDL docs )
    SDL_memset(stream, 0, len);

    // Feed audio data into the stream
    size_t remaining = std::min(self->BufferLength_, (size_t)len);
    uint8_t* dst = stream;

    while (remaining)
    {
        auto& front = self->Buffer_.front();
        long chunk = (long)std::min(front.Data.size() - front.Idx, remaining);

        // Event if the channel is muted we need to take
        if (!self->Muted_)
        {
            SDL_memcpy(dst, &front.Data[front.Idx], chunk);
            dst += chunk;
        }

        front.Idx += chunk;
        remaining -= chunk;
        self->BufferLength_ -= chunk;

        // If the page ended - invoke the listener
        if (front.Idx == front.Data.size())
        {
            self->DropFirstEntry();
        }
    }

    lock.unlock();

    //#include <iostream>
    //std::cout << remaining << ' ' << self->Buffer_.size() << ' ' << self->BufferLength_
    //                   << " muted=" << self->Muted_ << "\n"; // TODO: Remove debug
}

void Player::DropFirstEntry() noexcept
{
    Buffer_.front().Listener.set_value();
    Buffer_.pop_front();

    ReviseDevicePause();
}

void Player::ReviseDevicePause() noexcept
{
    // Nothing to play, wait for a new audio to be enqueued
    if (Buffer_.empty())
    {
        SDL_PauseAudioDevice(Out_, true);
        return;
    }

    // Use the internal state
    SDL_PauseAudioDevice(Out_, Paused_);
}
