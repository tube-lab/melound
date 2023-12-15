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
    return AudioUtils::EstimateBufferDuration(Buffer_.size(), Specs_);
}

auto Audio::Specs() const noexcept -> const SDL_AudioSpec&
{
    return Specs_;
}