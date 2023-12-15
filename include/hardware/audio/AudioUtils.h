// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <SDL2/SDL.h>

namespace ml
{
    /** Small utils for common audio processing tasks. */
    class AudioUtils
    {
    public:
        /** Estimates the duration of the decoded audio buffer played with given specs. */
        static auto EstimateBufferDuration(size_t bufferLength, SDL_AudioSpec spec) noexcept -> time_t;
    };
}