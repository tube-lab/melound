// Created by Tube Lab. Part of the meloun project.
#include "hardware/amplifier/Driver.h"

#include <utility>
using namespace ml::amplifier;

auto Driver::Create(const Config& config) noexcept -> std::shared_ptr<Driver>
{
    auto sw = SwitchDriver::Create(config.PowerControlPort);
    if (!sw)
    {
        return nullptr;
    }

    auto mixer = audio::ChannelsMixer::Create(config.Channels, config.AudioDevice);
    if (!mixer)
    {
        return nullptr;
    }

    return std::shared_ptr<Driver> {
        new Driver { config, sw, mixer }
    };
}

void Driver::Pause() noexcept
{
    Mixer_->Pause();
}

void Driver::Resume() noexcept
{
    Mixer_->Resume();
}

auto Driver::Enqueue(uint channel, const audio::Track &audio) noexcept -> std::optional<std::future<void>>
{
    if (!Mixer_->Enabled(channel))
    {
        return std::nullopt;
    }

    return Mixer_->Enqueue(channel, audio);
}

void Driver::Clear(uint channel) noexcept
{
    Mixer_->Clear(channel);
}

void Driver::Skip(uint channel) noexcept
{
    Mixer_->Skip(channel);
}

auto Driver::Activate(uint channel, bool urgent) noexcept -> std::future<bool>
{
    std::lock_guard _ { CommLock_ };
    ControllerEvents_.push_back({
        .Channel = channel,
        .Enabled = true,
        .Urgent = urgent,
        .Listener = std::promise<bool>()
    });

    return ControllerEvents_.back().Listener.get_future();
}

void Driver::Deactivate(uint channel) noexcept
{
    std::lock_guard _ { CommLock_ };
    ControllerEvents_.push_back({
        .Channel = channel,
        .Enabled = false,
        .Urgent = false,
        .Listener = std::promise<bool>()
    });
}

void Driver::Pause(uint channel) noexcept
{
    return Mixer_->Pause(channel);
}

void Driver::Resume(uint channel) noexcept
{
    return Mixer_->Resume(channel);
}

void Driver::Mute(uint channel) noexcept
{
    return Mixer_->Mute(channel);
}

void Driver::Unmute(uint channel) noexcept
{
    return Mixer_->Unmute(channel);
}

auto Driver::Enabled(uint channel) const noexcept -> bool
{
    // The mixer channel state always represents its real state
    return Mixer_->Enabled(channel);
}

auto Driver::Paused(uint channel) const noexcept -> bool
{
    return Mixer_->Paused(channel);
}

auto Driver::Muted(uint channel) const noexcept -> bool
{
    return Mixer_->Muted(channel);
}

auto Driver::DurationLeft(uint channel) const noexcept -> time_t
{
    return Mixer_->DurationLeft(channel);
}

auto Driver::DurationLeft() const noexcept -> time_t
{
    return Mixer_->DurationLeft();
}

auto Driver::Channels() const noexcept -> size_t
{
    return Config_.Channels;
}

auto Driver::Working() const noexcept -> bool
{
    return Working_;
}

auto Driver::ActivationDuration() const noexcept -> time_t
{
    return Config_.WarmingDuration;
}

Driver::Driver(Config config, std::shared_ptr<SwitchDriver> sw, std::shared_ptr<audio::ChannelsMixer> mixer) noexcept
    : Config_(std::move(config)), Switch_(std::move(sw)), Mixer_(std::move(mixer))
{
    Controller_ = std::jthread {&Driver::ControllerLoop, this };
}

void Driver::ControllerLoop(const std::stop_token& token, Driver* self) noexcept
{
    using ChangeIt = std::list<ChannelStateChange>::iterator;

    while (!token.stop_requested())
    {
        const auto time = TimeNow();
        std::lock_guard _ { self->CommLock_ };

        std::vector<std::list<ChangeIt>> ups(self->Config_.Channels);
        std::vector<ChangeIt> completed;

        // Sort the requests and filter out requests that cancel out each other
        for (auto it = self->ControllerEvents_.begin(); it != self->ControllerEvents_.end(); ++it)
        {
            if (it->Enabled)
            {
                ups[it->Channel].push_back(it);
            }
            else
            {
                // The request has been cancelled
                while (!ups[it->Channel].empty())
                {
                    ups[it->Channel].front()->Listener.set_value(false);
                    completed.push_back(ups[it->Channel].front());
                    ups[it->Channel].pop_front();
                }

                // Immediately fulfill the channel disabling promise
                it->Listener.set_value(true);
                completed.push_back(it);
            }
        }

        // Revise the amplifier state in case of any changes
        bool turnOn = false;
        bool urgent = false;

        for (auto& row : ups)
        {
            turnOn |= !row.empty();
            urgent |= std::any_of(row.begin(), row.end(), [&](const auto& it){
                return it->Urgent;
            });
        }

        // Actually update the amplifier state
        if (turnOn)
        {
            if (MayTurnOn(time, self->LastWorkingTime_, self->WarmingStartTime_, urgent, self->Config_))
            {
                self->WarmingStartTime_ = 0;
                self->Switch_->Close();
                self->LastWorkingTime_ = time;
                self->Working_ = true;
            }
            else if (!self->WarmingStartTime_)
            {
                self->WarmingStartTime_ = time;
            }
        }
        else
        {
            self->Switch_->Close();
            self->Working_ = false;
            self->WarmingStartTime_ = 0;
        }

        // Close up-requests & update mixer states
        for (uint64_t i = 0; i < ups.size(); ++i)
        {
            if (ups.empty())
            {
                self->DisableMixerChannel(i);
            }
            else if (self->Working_ )
            {
                self->EnableMixerChannel(i);
                for (auto& it : ups[i])
                {
                    it->Listener.set_value(true);
                    completed.push_back(it);
                }
            }
        }

        // Remove the completed requests
        for (const auto& it : completed)
        {
            self->ControllerEvents_.erase(it);
        }

        std::this_thread::yield();
    }
}

auto Driver::MayTurnOn(time_t time, time_t workingMoment, time_t warmingStart, bool urgent, const Config& config) noexcept -> bool
{
    return urgent ||
           (warmingStart && time - warmingStart >= config.WarmingDuration) ||
           (workingMoment && time - workingMoment <= config.CoolingDuration);
}

void Driver::EnableMixerChannel(uint i) noexcept
{
    if (!Mixer_->Enabled(i))
    {
        Mixer_->Enable(i);
    }
}

void Driver::DisableMixerChannel(uint i) noexcept
{
    if (Mixer_->Enabled(i))
    {
        Mixer_->Clear(i);
        Mixer_->Pause(i);
        Mixer_->Unmute(i);
        Mixer_->Disable(i);
    }
}
