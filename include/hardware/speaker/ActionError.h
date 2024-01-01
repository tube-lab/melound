// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::speaker
{
    enum ActionError
    {
        AE_ChannelOpened = 0,
        AE_ChannelClosed = 1,
        AE_ChannelInactive = 2,
        AE_IncompatibleTrack = 3,
        AE_ChannelNotFound = 4,
    };
}