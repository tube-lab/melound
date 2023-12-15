// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/Audio.h"
using namespace ml;

Audio::Audio(std::vector<Uint8> buffer, SDL_AudioSpec specs) noexcept
    : Buffer_(std::move(buffer)), Specs_(specs) {}

auto Audio::Buffer() const noexcept -> const std::vector<Uint8>&
{
    return Buffer_;
}

auto Audio::Duration() const noexcept -> time_t
{
    int sampleSize = SDL_AUDIO_BITSIZE(Specs_.format) / 8;
    int channels = Specs_.channels ? Specs_.channels : 1;

    time_t samplesPerChannel = (time_t)Buffer_.size() / (channels*sampleSize);
    return (1000*samplesPerChannel) / Specs_.freq;
}

auto Audio::Specs() const noexcept -> const SDL_AudioSpec&
{
    return Specs_;
}