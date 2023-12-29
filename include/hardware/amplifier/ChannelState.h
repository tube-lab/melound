// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::amplifier
{
    /** The list of possible amplifier channel' states. */
    enum ChannelState
    {
        CS_Closed = 0, ///< The channel is turned off.
        CS_Opened = 1, ///< The channel is opened, it tries to capture the amplifier output.
        CS_Active = 2, ///< The channel is actually ready to play something to the amplifier output.
        CS_UrgentActive = 3 ///< The channel is in extreme need to play something to the amplifier output.
    };
}