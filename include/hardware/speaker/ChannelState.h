// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::amplifier
{
    enum ChannelState
    {
        C_Closed = 0,
        C_Opened = 1,
        C_Active = 2,
        C_PendingActivation = 3,
        C_PendingDeactivation = 4
    };
}