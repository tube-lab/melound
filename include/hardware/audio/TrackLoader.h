// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "Track.h"

namespace ml::audio
{
    /**
     * @brief A bunch of audio parsers. Directly works with the data from the memory.
     * @safety Fully exception and thread safe.
     */
    class TrackLoader
    {
    public:
        /** Tries to parse audio encoded as wav. */
        static auto FromWav(const std::vector<char> &wav) noexcept -> std::optional<Track>;
    };
}