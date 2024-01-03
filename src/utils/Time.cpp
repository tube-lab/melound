// Created by Tube Lab. Part of the meloun project.
#include "utils/Time.h"
using namespace ml::utils;

auto Time::Now() noexcept -> time_t
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
