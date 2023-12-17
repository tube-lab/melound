// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Utils.h"

#include <optional>
#include <vector>
#include <string_view>

namespace ml::audio
{
    /**
     * @brief A parsed audio data.
     *
     */
    class Track
    {
        std::vector<Uint8> Buffer_;
        SDL_AudioSpec Specs_ {};

    public:
        Track(std::vector<Uint8> buffer, SDL_AudioSpec specs) noexcept;

        auto Buffer() const noexcept -> const std::vector<Uint8>&; ///< Returns the audio buffer.
        auto Specs() const noexcept -> const SDL_AudioSpec&; ///< Returns the audio format info.
        auto Duration() const noexcept -> time_t; ///< Returns the audio duration.
    };
}