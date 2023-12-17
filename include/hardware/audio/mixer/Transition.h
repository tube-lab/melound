// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <vector>
#include <cstdint>
#include <cstdlib>

namespace ml::audio
{
    struct Transition
    {
        float a = 0.0;
        float b = 0.0;
        float c = 1.0;
        float d = 1.0;
        float l = 1.0;
    };
}