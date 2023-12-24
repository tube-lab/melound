// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/core/Track.h"
using namespace ml::audio;

Track::Track(std::vector<uint8_t> buffer, SDL_AudioSpec spec) noexcept
    : Buffer_(std::move(buffer)), Spec_(spec) {}

auto Track::Buffer() const noexcept -> const std::vector<uint8_t>&
{
    return Buffer_;
}

auto Track::Spec() const noexcept -> const SDL_AudioSpec&
{
    return Spec_;
}