// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/TrackLoader.h"
using namespace ml::audio;

auto TrackLoader::FromWav(const std::vector<char> &wav) noexcept -> std::optional<Track>
{
    // Try to parse the audio
    SDL_AudioSpec wavSpec;
    Uint32 wavLength;
    uint8_t* wavBuffer;

    auto* rw = SDL_RWFromConstMem(&wav[0], wav.size());
    auto* r = SDL_LoadWAV_RW(rw, 1, &wavSpec, &wavBuffer, &wavLength);

    if (!r)
    {
        return std::nullopt;
    }

    // Move the buffer and specs into the audio object
    Track audio {{wavBuffer, wavBuffer + wavLength }, wavSpec };

    delete wavBuffer;
    return audio;
}

