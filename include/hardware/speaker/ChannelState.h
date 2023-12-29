// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::speaker
{
    enum ChannelState
    {
        CS_Closed = 0,
        CS_Opened = 1,
        CS_Active = 2,
        CS_UrgentActive = 3,
        CS_PendingActivation = 4,
        CS_PendingDeactivation = 5
    };
}