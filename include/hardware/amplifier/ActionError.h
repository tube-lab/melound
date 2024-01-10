// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "hardware/audio/Track.h"
#include "utils/CustomConstructor.h"

#include <future>
#include <vector>
#include <thread>
#include <sys/types.h>

namespace ml::amplifier
{
    /** // TODO: Write docs */
    enum ActionError
    {
        AE_Shutdown = 0, ///<
        AE_ChannelClosed = 1, ///<
        AE_IncompatibleTrack = 2 ///<
    };
}