// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <SDL2/SDL.h>

#include <optional>
#include <vector>
#include <string_view>

namespace ml::audio
{
    /**
     * @brief A parsed audio data.
     */
    class Track
    {
        std::vector<uint8_t> Buffer_;
        SDL_AudioSpec Spec_ {};

    public:
        Track(std::vector<uint8_t> buffer, SDL_AudioSpec spec) noexcept;

        auto Buffer() const noexcept -> const std::vector<uint8_t>&; ///< Returns the audio buffer.
        auto Spec() const noexcept -> const SDL_AudioSpec&; ///< Returns the audio format info.
    };
}