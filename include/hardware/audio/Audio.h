// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "AudioUtils.h"

#include <optional>
#include <vector>
#include <string_view>

namespace ml
{
    /**
     * @brief A parsed audio data.
     * Can be streamed by AudioStreamer.
     */
    class Audio
    {
        std::vector<Uint8> Buffer_;
        SDL_AudioSpec Specs_ {};

    public:
        Audio(std::vector<Uint8> buffer, SDL_AudioSpec specs) noexcept;

        auto Buffer() const noexcept -> const std::vector<Uint8>&; ///< Returns the audio buffer.
        auto Specs() const noexcept -> const SDL_AudioSpec&; ///< Returns the audio format info.
        auto Duration() const noexcept -> time_t; ///< Returns the audio duration.
    };
}