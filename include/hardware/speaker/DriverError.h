// Created by Tube Lab. Part of the meloun project.
#pragma once

namespace ml::speaker
{
    enum DriverError
    {
        DE_Closed = 0,
        DE_NotFound = 1,
        DE_BadTrack = 2,
        DE_NotActive = 3,
        DE_AlreadyOpened = 4,
        DE_AlreadyActive = 5
    };
}