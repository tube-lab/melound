// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <optional>
#include <cstdlib>
#include <cstdint>
#include <span>

namespace ml::audio
{
    /**
     * @brief Base class for audio-player effect.
     * The effect directly modifies the audio buffer.
     * The effect shouldn't have any internal state, in other words Process function should be pure.
     * If the effect is automatically removed from the player once the duration passes.
     * The effect may play for 1(ms) longer that expected due to rounding of real numbers.
     */
    struct Effect
    {
        virtual ~Effect() = default;

        /** Applies the effect to the given buffer. Beginning time-point is counted from the effect application. */
        virtual void Apply(std::span<uint8_t>& buffer, time_t beginning, time_t duration) const noexcept = 0;

        /** Returns the duration of the effect in (ms) or INT64_MAX if the effect is infinite. */
        virtual auto Duration() const noexcept -> time_t = 0;
    };
}