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
    /** */
    enum ActionError
    {
        AE_ChannelClosed = 0, ///<
        AE_Inactive = 1, ///<
        AE_IncompatibleTrack = 2 ///<
    };
}