// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/core/Track.h"
using namespace ml::audio;

Track::Track(std::vector<Uint8> buffer, SDL_AudioSpec specs) noexcept
    : Buffer_(std::move(buffer)), Specs_(specs) {}

auto Track::Buffer() const noexcept -> const std::vector<Uint8>&
{
    return Buffer_;
}

auto Track::Duration() const noexcept -> time_t
{
    return Utils::EstimateBufferDuration(Buffer_.size(), Specs_);
}

auto Track::Specs() const noexcept -> const SDL_AudioSpec&
{
    return Specs_;
}