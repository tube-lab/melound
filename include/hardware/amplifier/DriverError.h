// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::amplifier
{
    /** An error list for Driver::Enqueue. */
    enum EnqueueError
    {
        EE_Ok = 0, ///< No errors happened.
        EE_BadTrack = 1, ///< The track can't be properly resampled to fit to the player format.
        EE_NotActive = 2, ///< The channel isn't active..
    };
}