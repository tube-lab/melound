// Created by Tube Lab. Part of the meloun project.
#pragma once

#include "ActionError.h"
#include "Config.h"

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
     * @safety Fully exception and thread safe.
     *
     * Defines a universal API for managing different amplifier.
     * Takes care about internal states and the async mainloop.
     *
     * Provides 3 different kinds of APIs:
     * 1. Channels Open/Close/Opened -> Equivalent of table reservation system.
     * 2. Amplifier StartUp/ShutDown/Working -> Physically turns on/off the switch.
     * 3. Actions Enqueue/Skip/Clear/DurationLeft -> Manages the audio playback for the channel.
     *
     * Requirements for concrete implementations:
     * 1. When the channel with index=i is opened all the channels where index < i should be muted.
     * 2. When the channel is closed its playback queue should be immediately cleared.
     * 3. When the amplifier shuts down the playback queue for all channels should be cleared.
     * 4. All the actions MUST NOT be invocable when the device is inactive.
     * 5. The driver MUST NOT care about the runtime errors or the hardware disconnections.
     * 6. The driver MUST be completely exception and thread safe.
     * 7. The driver MUST start up/shut down as fast as possible when this is required urgently.
     */
    class Driver : public CustomConstructor
    {
        time_t StartupDuration_;
        time_t ShutdownDuration_;
        time_t UrgentStartupDuration_;
        time_t UrgentShutdownDuration_;
        time_t TickInterval_;
        size_t Channels_;

        std::vector<std::atomic<bool>> OpenedChannels_;

        bool Working_ {};
        bool DesiredWorking_ {};
        bool UrgentStateChange_ {};
        std::vector<std::promise<void>> ActivationListeners_;
        std::vector<std::promise<void>> DeactivationListeners_;
        mutable std::recursive_mutex DeviceStateLock_;

        std::jthread Mainloop_;

    public:
        /** Appends the track to the channel' queue, requires the device and channel is active. */
        auto Enqueue(uint channel, const audio::Track& track) -> std::expected<std::future<void>, ActionError>;

        /** Skips the first track in the channel' queue, requires the device to be active and the channel to be opened. */
        auto Skip(uint channel) noexcept -> std::expected<void, ActionError>;

        /** Clears the channel' queue, requires the device to be active and the channel to be opened. */
        auto Clear(uint channel) noexcept -> std::expected<void, ActionError>;

        /** Estimates how much playback time is left for the particular channel, requires the device to be active and the channel to be opened. */
        auto DurationLeft(uint channel) const noexcept -> std::expected<time_t, ActionError>;

        /** Requests the activation of the amplifier, so it can play sound. */
        auto StartUp(bool urgently) noexcept -> std::future<void>;

        /** Requests the deactivation of the amplifier, after completion the amplifier won't be able to play sound. */
        auto ShutDown(bool urgently) noexcept -> std::future<void>;

        /** Reserves the channel for playback, fails if the channel has already been opened. */
        auto Open(uint channel) noexcept -> bool;

        /** Releases the channel, fails if the channel isn't open. */
        auto Close(uint channel) noexcept -> bool;

        /** Returns whether the device is turned on. */
        auto Working() const noexcept -> bool;

        /** Returns whether the channel is opened. */
        auto Opened(uint channel) const noexcept -> bool;

        /** Returns the number of amplifier' channels. */
        auto Channels() const noexcept -> size_t;

        /** Returns how much time the device may take in order to start up in the worst case. */
        auto StartupDuration(bool urgently) const noexcept -> time_t;

        /** Returns how much time the device may take in order to shut down in the worst case. */
        auto ShutdownDuration(bool urgently) const noexcept -> time_t;

    protected:
        /** Creates the driver with some essential properties set. */
        Driver(const Config& config) noexcept;

        /** Appends the track to the channel' queue, invoked only if the device and channel are active. */
        virtual auto DoEnqueue(uint channel, const audio::Track& track) -> std::optional<std::future<void>> = 0;

        /** Skips the first track in the channel' queue, invoked only of the device to be active and the channel is opened. */
        virtual void DoSkip(uint channel) noexcept = 0;

        /** Clears the channel' queue, invoked only of the device to be active and the channel is opened. */
        virtual void DoClear(uint channel) noexcept = 0;

        /** Estimates how much playback time is left for the particular channel, invoked only of the device to be active and the channel is opened. */
        virtual auto DoDurationLeft(uint channel) const noexcept -> time_t = 0;

        /** Opens the channel, invoked synchronously. */
        virtual void DoOpen(uint channel) noexcept = 0;

        /** Closes the channel, invoked synchronously. */
        virtual void DoClose(uint channel) noexcept = 0;

        /** Activates the device, always called in the separate thread. Returns true when the device has been activated. */
        virtual bool DoActivation(time_t time, time_t elapsed, bool urgent) noexcept = 0;

        /** De-activates the device, always called in the separate thread. Returns true when the device is no longer active. */
        virtual bool DoDeactivation(time_t time, time_t elapsed, bool urgent) noexcept = 0;

    private:
        void Mainloop(const std::stop_token& token) noexcept;
        auto ActionWrapper(uint channel) const noexcept -> std::expected<void, ActionError>;
        static void FulfillListeners(std::vector<std::promise<void>>& listeners) noexcept;
    };
}