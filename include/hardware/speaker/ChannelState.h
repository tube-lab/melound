// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::speaker
{
    enum ChannelState
    {
        CS_Closed = 0,
        CS_Opened = 1,
        CS_Active = 2,
        CS_PendingActivation = 3,
        CS_PendingTermination = 4,
        CS_PendingDeactivation = 5,
    };
}