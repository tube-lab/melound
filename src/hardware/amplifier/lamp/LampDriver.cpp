// Created by Tube Lab. Part of the meloun project.
#include "hardware/amplifier/lamp/LampDriver.h"
using namespace ml::amplifier;

auto LampDriver::Create(const LampConfig& cfg) noexcept -> std::shared_ptr<LampDriver>
{
    auto relay = relay::Driver::Create(cfg.PowerPort);
    if (!relay)
    {
        return nullptr;
    }

    auto mixer = audio::ChannelsMixer::Create(cfg.Channels, cfg.AudioDevice);
    if (!mixer)
    {
        return nullptr;
    }

    auto driver = new LampDriver { Config {
        .StartupDuration = cfg.WarmingDuration,
        .UrgentStartupDuration = 0,
        .ShutdownDuration = 0,
        .UrgentShutdownDuration = 0,
        .TickInterval = 20,
        .Channels = cfg.Channels
    }};

    driver->PowerRelay_ = relay;
    driver->Mixer_ = mixer;
    driver->CoolingDuration_ = cfg.CoolingDuration;

    return std::shared_ptr<LampDriver>(driver);
}

auto LampDriver::DoEnqueue(uint channel, const ml::audio::Track& track) -> std::optional<std::future<void>>
{
    return Mixer_->Enqueue(channel, track);
}

void LampDriver::DoSkip(uint channel) noexcept
{
    Mixer_->Skip(channel);
}

void LampDriver::DoClear(uint channel) noexcept
{
    Mixer_->Clear(channel);
}

auto LampDriver::DoDurationLeft(uint channel) const noexcept -> time_t
{
    return Mixer_->DurationLeft(channel);
}

void LampDriver::DoOpen(uint channel) noexcept
{
    Mixer_->Enable(channel);
}

void LampDriver::DoClose(uint channel) noexcept
{
    Mixer_->Disable(channel);
    Mixer_->Clear(channel);
}

bool LampDriver::DoActivation(time_t time, time_t elapsed, bool urgently) noexcept
{
    PowerRelay_->Close();
    return elapsed >= StartupDuration(urgently) || (time - DeactivatedAt_ < CoolingDuration_);
}

bool LampDriver::DoDeactivation(time_t time, time_t elapsed, bool urgently) noexcept
{
    Mixer_->ClearAll();
    PowerRelay_->Open();
    DeactivatedAt_ = time;
    return true;
}
