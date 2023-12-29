// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::speaker
{
    enum ActionError
    {
        AE_Inactive = 0,
        AE_BadTrack = 1,
        DE_NotFound = 2,
        DE_AlreadyOpened = 3,
    };
}