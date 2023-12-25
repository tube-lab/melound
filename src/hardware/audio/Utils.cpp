// Created by Tube Lab. Part of the meloun project.
#include "hardware/audio/Utils.h"
using namespace ml::audio;

auto Utils::EstimateBufferDuration(size_t bufferLength, SDL_AudioSpec spec) noexcept -> time_t
{
    int sampleSize = SDL_AUDIO_BITSIZE(spec.format) / 8;
    int channels = spec.channels ? spec.channels : 1;

    time_t samplesPerChannel = (time_t)bufferLength / (channels*sampleSize);
    return (1000*samplesPerChannel) / spec.freq;
}

auto Utils::Resample(const Track &original, SDL_AudioSpec spec) noexcept -> std::optional<Track>
{
    // Create a new stream
    SDL_AudioSpec orig = original.Spec();
    auto* stream = SDL_NewAudioStream (
        orig.format, orig.channels, orig.freq,
        spec.format, spec.channels, spec.freq
    );

    if (!stream)
    {
        return std::nullopt;
    }

    // Convert all the data
    uint64_t bytes = sizeof(original.Buffer()[0])*original.Buffer().size();
    SDL_AudioStreamPut(stream, &original.Buffer()[0], (int)bytes);

    std::vector<uint8_t> converted(SDL_AudioStreamAvailable(stream));
    SDL_AudioStreamGet(stream, &converted[0], (int)converted.size());

    // Free the stream and create the new track
    SDL_FreeAudioStream(stream);
    return Track { converted, spec };
}
