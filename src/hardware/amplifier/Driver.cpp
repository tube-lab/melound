// Created by Tube Lab. Part of the meloun project.
#include "hardware/amplifier/Driver.h"
#include <iostream> // TODO: Debug

using namespace ml::amplifier;

auto Driver::Create(const Config& config) noexcept -> std::shared_ptr<Driver>
{
    // Initialize subsystems
    auto relay = relay::Driver::Create(config.PowerControlPort);
    if (!relay)
    {
        return nullptr;
    }

    auto mixer = audio::ChannelsMixer::Create(config.Channels, config.AudioDevice);
    if (!mixer)
    {
        return nullptr;
    }

    // Create the driver
    auto driver = std::make_shared<Driver>();
    driver->Config_ = config;
    driver->Relay_ = relay;
    driver->Mixer_ = mixer;
    driver->Channels_ = std::vector<ChannelInfo> (config.Channels);
    driver->Controller_ = std::jthread { [driver](const auto& t) { driver->ControllerLoop(t); } };

    return driver;
}

void Driver::Pause() noexcept
{
    Mixer_->Pause();
}

void Driver::Resume() noexcept
{
    Mixer_->Resume();
}

void Driver::Open(uint channel) noexcept
{
    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[channel].State == C_Closed)
        {
            Channels_[channel].State = C_Opened;
        }
    }
}

void Driver::Close(uint channel) noexcept
{
    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[channel].State != C_Closed)
        {
            Deactivate(channel);
            Channels_[channel].State = C_Closed;
        }
    }
}

auto Driver::Activate(uint channel, bool urgent) noexcept -> std::optional<std::future<bool>>
{
    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[channel].State == C_Opened)
        {
            Channels_[channel].State = C_PendingActivation;
            Channels_[channel].UrgentActivation = urgent;
            Channels_[channel].ActivationListeners.emplace_back();
            return Channels_[channel].ActivationListeners.back().get_future();
        }

        return std::nullopt;
    }
}

auto Driver::Deactivate(uint channel) noexcept -> bool
{
    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[channel].State == C_Active)
        {
            Channels_[channel].State = C_Opened;
            FulfillActivationListeners(channel, false); // Mark all the requests as cancelled
            return true;
        }

        return false;
    }
}

auto Driver::Enqueue(uint channel, const audio::Track &audio) noexcept -> std::expected<std::future<void>, EnqueueError>
{
    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[channel].State != C_Active)
        {
            return std::unexpected{EE_NotActive};
        }

        auto promise = Mixer_->Enqueue(channel, audio);
        if (!promise)
        {
            return std::unexpected{EE_BadTrack};
        }

        return std::move(*promise);
    }
}

auto Driver::Clear(uint channel) noexcept -> bool
{
    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[channel].State == C_Active)
        {
            Mixer_->Clear(channel);
            return true;
        }

        return false;
    }
}

auto Driver::Skip(uint channel) noexcept -> bool
{
    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[channel].State == C_Active)
        {
            Mixer_->Skip(channel);
            return true;
        }

        return false;
    }
}

auto Driver::DurationLeft(uint channel) const noexcept -> std::optional<time_t>
{
    std::lock_guard _ { StateLock_ };
    {
        if (Channels_[channel].State == C_Active)
        {
            return Mixer_->DurationLeft(channel);
        }

        return false;
    }
}

auto Driver::DurationLeft() const noexcept -> std::optional<time_t>
{
    std::lock_guard _ { StateLock_ };
    {
        for (uint64_t i = 0; i < Config_.Channels; ++i)
        {
            if (Channels_[i].State == C_Active)
            {
                return Mixer_->DurationLeft(i);
            }
        }

        return std::nullopt;
    }
}

auto Driver::State(uint channel) const noexcept -> ChannelState
{
    std::lock_guard _ { StateLock_ };
    return Channels_[channel].State;
}

auto Driver::Channels() const noexcept -> size_t
{
    return Config_.Channels;
}

auto Driver::Powered() const noexcept -> bool
{
    std::lock_guard _ { StateLock_ };
    return Powered_;
}

auto Driver::ActivationDuration() const noexcept -> time_t
{
    return Config_.WarmingDuration;
}

/* === Hardware management logic === */
void Driver::ControllerLoop(const std::stop_token& token) noexcept
{
    while (!token.stop_requested())
    {
        std::lock_guard _ { StateLock_ };
        const auto time = TimeNow();

        // Update the power relay state
        bool oldPoweredState = Powered_;
        Powered_ = false;

        for (uint64_t i = 0; i < Config_.Channels; ++i)
        {
            Powered_ |= Channels_[i].State == C_PendingActivation;
            Powered_ |= Channels_[i].State == C_Active;
        }

        if (Powered_)
        {
            EnablePowerRelay();
        }
        else
        {
            DisablePowerRelay();
        }

        // Update the mixer state
        for (uint64_t i = 0; i < Config_.Channels; ++i)
        {
            if (Channels_[i].State == C_Closed) DisableMixerChannel(i);
            else if (Channels_[i].State == C_Active) EnableMixerChannel(i);
            else PutMixerChannelOnHold(i);
        }

        // Update powered interval
        if (Powered_)
        {
            if (!oldPoweredState)
            {
                PreviousPoweredInterval_ = PoweredInterval_;
                PoweredInterval_ = { time, time };
            }
            else
            {
                PoweredInterval_.second = time;
            }
        }

        // If already warm - fulfill all the listeners
        for (uint64_t i = 0; i < Config_.Channels; ++i)
        {
            if (Channels_[i].State == C_PendingActivation && Warm(time, Channels_[i].UrgentActivation))
            {
                Channels_[i].State = C_Active;
                FulfillActivationListeners(i, true);
            }
        }

        std::this_thread::yield();
    }
}

auto Driver::Warm(time_t time, bool urgent) const noexcept -> bool
{
    return urgent ||
           (PoweredInterval_.second - PoweredInterval_.first >= Config_.WarmingDuration ) ||
           (time - PreviousPoweredInterval_.second <= Config_.CoolingDuration);
}

void Driver::FulfillActivationListeners(uint channel, bool result) noexcept
{
    std::lock_guard _ { StateLock_ };
    {
        for (auto& p : Channels_[channel].ActivationListeners)
        {
            p.set_value(result);
        }
        Channels_[channel].ActivationListeners.clear();
    }
}

/* === Subsystems management === */
void Driver::EnablePowerRelay() noexcept
{
    Relay_->Closed() ? void() : Relay_->Close();
}

void Driver::DisablePowerRelay() noexcept
{
    Relay_->Closed() ? Relay_->Open() : void();
}

void Driver::EnableMixerChannel(uint i) noexcept
{
    if (!Mixer_->Enabled(i))
    {
        Mixer_->Enable(i);
        Mixer_->Resume(i);
    }
}

void Driver::PutMixerChannelOnHold(uint i) noexcept
{
    if (Mixer_->Enabled(i))
    {
        Mixer_->Clear(i);
    }
}

void Driver::DisableMixerChannel(uint i) noexcept
{
    if (Mixer_->Enabled(i))
    {
        Mixer_->Clear(i);
        Mixer_->Disable(i);
    }
}