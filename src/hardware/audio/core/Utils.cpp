// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/core/Utils.h"
using namespace ml::audio;

auto Utils::EstimateBufferDuration(size_t bufferLength, SDL_AudioSpec spec) noexcept -> time_t
{
    int sampleSize = SDL_AUDIO_BITSIZE(spec.format) / 8;
    int channels = spec.channels ? spec.channels : 1;

    time_t samplesPerChannel = (time_t)bufferLength / (channels*sampleSize);
    return (1000*samplesPerChannel) / spec.freq;
}