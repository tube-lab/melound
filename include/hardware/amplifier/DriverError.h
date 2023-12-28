// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "ChannelState.h"

#include "hardware/audio/Track.h"
#include "utils/CustomConstructor.h"

#include <future>
#include <vector>
#include <thread>
#include <sys/types.h>

namespace ml::amplifier
{
    enum DriverError
    {
        DE_Inactive = 0,
        DE_BadTrack = 1
    };
}