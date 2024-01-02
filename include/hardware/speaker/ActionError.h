// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::speaker
{
    enum ActionError
    {
        AE_ChannelOpened = 0,
        AE_ChannelClosed = 1,
        AE_ChannelNotFound = 2,
        AE_ChannelInactive = 3,
        AE_IncompatibleTrack = 4,
        AE_AllChannelsClosed = 5
    };
}