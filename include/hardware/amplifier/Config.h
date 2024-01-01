// Created by Tube Lab. Part of the meloun project.
#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace ml::amplifier
{
    struct Config
    {
        /** The time which the amplifier needs to start up normally. */
        time_t StartupDuration;

        /** The time which the amplifier needs to start up urgently. */
        time_t UrgentStartupDuration;

        /** The time which the amplifier needs to shut down normally. */
        time_t ShutdownDuration;

        /** The time which the amplifier needs to shut down urgently. */
        time_t UrgentShutdownDuration;

        /** The delay between async action doers invocations. */
        time_t TickInterval;

        /** The number of the amplifier channels. */
        uint Channels {};
    };
}