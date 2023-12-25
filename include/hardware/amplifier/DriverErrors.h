// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::amplifier
{
    /** An error list for Driver::Enqueue. */
    enum EnqueueError
    {
        EE_Ok = 0, ///< No errors happened.
        EE_Closed = 1, ///< The channel is closed.
        EE_BadTrack = 2 ///< The track can't be properly resampled to fit to the player format.
    };
}