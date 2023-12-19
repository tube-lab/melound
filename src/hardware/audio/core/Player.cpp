// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/core/Player.h"
using namespace ml::audio;

#include <iostream>

auto Player::Create(SDL_AudioSpec spec, const std::optional<std::string>& device) -> std::shared_ptr<Player>
{
    SDL_Init(SDL_INIT_AUDIO);

    // Create the dummy player ( because we need its address in audio-supplier callback )
    auto* player = new Player {};

    // Select the device ( if given )
    const char* name = device ? device->c_str() : nullptr;

    // Inject custom callback into the specs
    SDL_AudioSpec modified = spec;
    modified.callback = &Player::AudioSupplier;
    modified.userdata = player;

    // Create the output
    SDL_AudioDeviceID out = SDL_OpenAudioDevice(name, 0, &modified, nullptr, 0);
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
}

auto Player::Enqueue(const Track& audio) noexcept -> std::future<void>
{
    // Open the audio device ( for this audio )
    std::lock_guard _ { BufferLock_ };
    {
        // Add new track to the queue
        Buffer_.emplace_back(audio.Buffer(), std::promise<void> {}, 0);
        BufferLength_ += audio.Buffer().size();

        // Possibly unpause the audio device
        SDL_PauseAudioDevice(Out_, Paused_);

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

        // Stop the playback until the new audio will be added
        SDL_PauseAudioDevice(Out_, true);
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

void Player::ApplyEffect(const std::shared_ptr<Effect>& effect) noexcept
{
    std::lock_guard _ { EffectLock_ };
    {
        Effects_[effect] = 0;
    }
}

void Player::RemoveEffect(const std::shared_ptr<Effect>& effect) noexcept
{
    std::lock_guard _ { EffectLock_ };
    {
        Effects_.erase(effect);
    }
}

void Player::Pause() noexcept
{
    Paused_ = true;
    SDL_PauseAudioDevice(Out_, true);
}

void Player::Resume() noexcept
{
    Paused_ = false;
    SDL_PauseAudioDevice(Out_, false);
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

void Player::AudioSupplier(void* userdata, Uint8* stream, int len) noexcept
{
    auto* self = (Player*)userdata;
    std::unique_lock lock { self->BufferLock_ };

    // Empty the buffer ( required by SDL docs )
    SDL_memset(stream, len, 0);

    // Skip anything if the buffer is already empty
    if (self->Buffer_.empty())
    {
        return;
    }

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

    // Process the effects
    std::unique_lock effects_lock { self->EffectLock_ };

    const auto duration = Utils::EstimateBufferDuration(len, self->Spec_);
    for (auto& [effect, passed] : self->Effects_)
    {
        // Trim the duration to force the effect to play close to Duration() milliseconds
        const auto trimmedDuration = std::min(effect->Duration() - passed, duration);

        // Create the buffer ( always round up in case when trimmed )
        const auto trimmedLength = (size_t)std::ceil(((float)trimmedDuration / (float)duration) * (float)len);
        std::span<uint8_t> view { stream, trimmedLength };

        // Actually apply the effect
        effect->Apply(view, passed, trimmedDuration);
        passed += trimmedDuration;
    }

    // Remove all the expired effects
    std::erase_if(self->Effects_, [](const auto& t)
    {
        return t.first->Duration() == t.second;
    });

    effects_lock.unlock();

    std::cout << remaining << ' ' << self->Buffer_.size() << ' ' << self->BufferLength_ << ' ' << self->Effects_.size() << "\n"; // TODO: Remove debug
}

void Player::DropFirstEntry() noexcept
{
    Buffer_.front().Listener.set_value();
    Buffer_.pop_front();

    // Nothing to play, wait for a new audio
    if (Buffer_.empty())
    {
        SDL_PauseAudioDevice(Out_, true);
    }
}