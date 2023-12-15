// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/AudioLoader.h"

auto ml::AudioLoader::FromWav(const std::vector<char> &wav) noexcept -> std::optional<Audio>
{
    // Try to parse the audio
    SDL_AudioSpec wavSpec;
    Uint32 wavLength;
    Uint8* wavBuffer;

    auto* rw = SDL_RWFromConstMem(&wav[0], wav.size());
    auto* r = SDL_LoadWAV_RW(rw, 1, &wavSpec, &wavBuffer, &wavLength);

    if (!r)
    {
        return std::nullopt;
    }

    // Move the buffer and specs into the audio object
    Audio audio { { wavBuffer, wavBuffer + wavLength }, wavSpec };

    delete wavBuffer;
    return audio;
}

