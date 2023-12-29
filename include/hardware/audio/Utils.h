// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Track.h"
#include <optional>
#include <SDL2/SDL.h>

namespace ml::audio
{
    /**
     * @brief Small utils for common audio processing tasks.
     * @safety Fully exception and thread safe.
     */
    class Utils
    {
    public:
        /** Estimates the duration of the decoded audio buffer played with given specs. */
        static auto EstimateBufferDuration(size_t bufferLength, SDL_AudioSpec spec) noexcept -> time_t;

        /** Resamples the track to fit into the given format. */
        static auto Resample(const Track& original, SDL_AudioSpec spec) noexcept -> std::optional<Track>;
    };
}