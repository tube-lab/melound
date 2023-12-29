// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "ChannelState.h"
#include "ActionError.h"

#include "hardware/audio/Track.h"

#include "utils/CustomConstructor.h"
#include "utils/Time.h"

#include <future>
#include <vector>
#include <thread>
#include <expected>
#include <sys/types.h>

namespace ml::amplifier
{
    /**
     * @brief Abstract driver for the amplifier.
     * @safety Fully exceptional and thread safe.
     *
     * Defines a universal API for managing different amplifier.
     * Takes care about internal states and the async mainloop.
     * To get understanding of the channels management take a look on ChannelState docs.
     *
     * Requirements for concrete implementations:
     * 1. When the channel with index=i is opened all the channels where index < i should be muted.
     * 2. All the actions MUST NOT be invocable when the device is inactive.
     * 3. The driver MUST NOT care about the runtime errors or the hardware disconnections.
     * 4. The driver MUST initialize/de-initialize the hardware in response to the channels states changes.
     */
    class Driver : public CustomConstructor
    {
        time_t StartupDuration_;
        time_t ShutdownDuration_;
        time_t TickInterval_;
        size_t ChannelsNum_;

        std::atomic<bool> Active_;

        std::vector<ChannelState> Channels_;
        std::mutex ChannelsLock_;

        std::jthread Mainloop_;

    public:
        /** Appends the track to the channel' queue, requires the device to be active. */
        virtual auto Enqueue(uint channel, const audio::Track& track) -> std::expected<std::future<void>, ActionError> = 0;

        /** Skips the first track in the channel' queue, requires the device to be active. */
        virtual auto Skip(uint channel) noexcept -> std::expected<time_t, ActionError> = 0;

        /** Clears the channel' queue, requires the device to be active. */
        virtual auto Clear(uint channel) noexcept -> std::expected<time_t, ActionError> = 0;

        /** Estimates how much playback time is left for the particular channel, requires the device to be active. */
        virtual auto DurationLeft(uint channel) const noexcept -> std::expected<time_t, ActionError> = 0;

        /** Notifies the driver that channels state has been updated. Fails if the size of vector != to the channels number. */
        auto NotifyAboutChannelsChange(const std::vector<ChannelState>& channels) noexcept -> bool;

        /** Returns whether the driver is active. */
        auto Active() const noexcept -> bool;

        /** Returns the number of amplifier' channels. */
        auto Channels() const noexcept -> size_t;

        /** Returns how much time the device may take in order to start up in the worst case. */
        auto StartupDuration() const noexcept -> time_t;

        /** Returns how much time the device may take in order to shut down in the worst case. */
        auto ShutdownDuration() const noexcept -> time_t;

    protected:
        /** Creates the driver with some essential properties set. */
        Driver(time_t startupDuration, time_t shutdownDuration, time_t tickInterval, size_t channelsNum) noexcept;

        /** Updates the device state, always called in the separate thread. Returns whether the device is active now. */
        virtual bool Tick(time_t time, time_t dt, const std::vector<ChannelState>& channels) noexcept = 0;

        /** Ensure that the device is active. Designed to be used with transform/and_then functions. */
        auto ShouldBeActive() const noexcept -> std::expected<void, ActionError>;

    private:
        void Mainloop(const std::stop_token& token) noexcept;
    };
}