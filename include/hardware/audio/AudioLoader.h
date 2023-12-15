// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Audio.h"

namespace ml
{
    /** A bunch of audio parsers. Directly works with the data from the memory. */
    class AudioLoader
    {
    public:
        /** Tries to parse audio encoded as wav. */
        static auto FromWav(const std::vector<char> &wav) noexcept -> std::optional<Audio>;
    };
}