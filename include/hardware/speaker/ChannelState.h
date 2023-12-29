// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::speaker
{
    enum ChannelState
    {
        CS_Closed = 0,
        CS_Opened = 1,
        CS_Active = 2,
        C_PendingActivation = 3,
        C_PendingDeactivation = 4
    };
}