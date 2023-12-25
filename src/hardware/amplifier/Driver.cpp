// Created by Tube Lab. Part of the meloun project.
#include "hardware/amplifier/Driver.h"
#include <iostream> // TODO: Debug
using namespace ml::amplifier;

auto Driver::Create(const Config& config) noexcept -> std::shared_ptr<Driver>
{
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

    return std::shared_ptr<Driver> {
        new Driver {config, relay, mixer }
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

auto Driver::Activate(uint channel, bool urgent) noexcept -> std::future<void>
{
    std::lock_guard _ { CommLock_ };
    ControllerEvents_[channel].push_back({
        .Enabled = true,
        .Urgent = urgent,
        .Listener = std::promise<void>()
    });

    return ControllerEvents_[channel].back().Listener.get_future();
}

void Driver::Deactivate(uint channel) noexcept
{
    std::lock_guard _ { CommLock_ };
    ControllerEvents_[channel].push_back({
        .Enabled = false,
        .Urgent = false,
        .Listener = std::promise<void>()
    });
}

auto Driver::Enqueue(uint channel, const audio::Track &audio) noexcept -> std::expected<std::future<void>, EnqueueError>
{
    if (!Mixer_->Enabled(channel))
    {
        return std::unexpected { EE_Closed };
    }

    auto promise = Mixer_->Enqueue(channel, audio);
    if (!promise)
    {
        return std::unexpected { EE_BadTrack };
    }

    return std::move(*promise);
}

auto Driver::Clear(uint channel) noexcept -> bool
{
    return DoIfChannelEnabled(channel, [&]() { Mixer_->Clear(channel); });
}

auto Driver::Skip(uint channel) noexcept -> bool
{
    return DoIfChannelEnabled(channel, [&]() { Mixer_->Skip(channel); });
}

auto Driver::Pause(uint channel) noexcept -> bool
{
    return DoIfChannelEnabled(channel, [&]() { Mixer_->Mute(channel); });
}

auto Driver::Resume(uint channel) noexcept -> bool
{
    return DoIfChannelEnabled(channel, [&]() { Mixer_->Resume(channel); });
}

auto Driver::Mute(uint channel) noexcept -> bool
{
    return DoIfChannelEnabled(channel, [&]() { Mixer_->Mute(channel); });
}

auto Driver::Unmute(uint channel) noexcept -> bool
{
    return DoIfChannelEnabled(channel, [&]() { Mixer_->Unmute(channel); });
}

auto Driver::Paused(uint channel) const noexcept -> std::optional<bool>
{
    return Mixer_->Enabled(channel) ? std::optional { Mixer_->Paused(channel) } : std::nullopt;
}

auto Driver::Muted(uint channel) const noexcept -> std::optional<bool>
{
    return Mixer_->Enabled(channel) ? std::optional { Mixer_->Muted(channel) } : std::nullopt;
}

auto Driver::DurationLeft(uint channel) const noexcept -> std::optional<time_t>
{
    return Mixer_->Enabled(channel) ? std::optional { Mixer_->DurationLeft(channel) } : std::nullopt;
}

auto Driver::DurationLeft() const noexcept -> std::optional<time_t>
{
    return Mixer_->CountEnabled() ? std::optional { Mixer_->DurationLeft() } : std::nullopt;
}

auto Driver::Active(uint channel) const noexcept -> bool
{
    std::lock_guard _ { StateLock_ };
    return ActiveChannels_[channel];
}

auto Driver::CountActive() const noexcept -> size_t
{
    std::lock_guard _ { StateLock_ };
    return std::count(ActiveChannels_.begin(), ActiveChannels_.end(), true);
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

Driver::Driver(Config config, std::shared_ptr<relay::Driver> sw, std::shared_ptr<audio::ChannelsMixer> mixer) noexcept
    : Config_(std::move(config)), Relay_(std::move(sw)), Mixer_(std::move(mixer))
{
    ControllerEvents_ = EventsList (config.Channels);
    ActiveChannels_ = std::vector<bool> (config.Channels);
    Controller_ = std::jthread {&Driver::ControllerLoop, this };
}

void Driver::ControllerLoop(const std::stop_token& token, Driver* self) noexcept
{
    time_t ptime = TimeNow();
    while (!token.stop_requested())
    {
        std::this_thread::yield();

        const auto time = TimeNow();
        const auto dt = time - ptime;
        ptime = time;

        std::lock_guard _ { self->CommLock_ };
        std::lock_guard __ { self->StateLock_ };

        auto& events = self->ControllerEvents_;

        // Reduce events that cancel each other
        for (auto& part : events)
        {
            auto it = std::find_if(part.rbegin(), part.rend(), [&](auto& it)
            {
                return !it.Enabled;
            });

            if (it != part.rend())
            {
                // Element on which "it" points is excluded
                FulfillEvents(part.begin(), std::next(it).base(), part);
            }
        }

        // Drop all the deactivation events ( it may only be the first element in each sub-list )
        for (auto& part : events)
        {
            if (!part.empty() && !part.front().Enabled)
            {
                part.front().Listener.set_value();
                part.pop_front();
            }
        }

        // Generate a final requirement for the amplifier controller based on the remaining events + the current state
        std::vector<bool> active = self->ActiveChannels_;
        bool turnOn = self->Working_;
        bool urgent = false;

        for (uint64_t i = 0; i < events.size(); ++i)
        {
            for (auto& event : events[i])
            {
                active[i] = active[i] | event.Enabled;
                turnOn |= event.Enabled;
                urgent |= event.Urgent;
            }
        }

        // Refine requirements into commands for the hardware controllers
        bool powered = false;
        bool playing = false;

        if (turnOn)
        {
            powered = true;
            playing = Warmed (
                time, self->LastWorkingMoment_, self->PoweringDuration_,
                urgent, self->Working_, self->Config_
            );
        }

        // Update the power relay state
        if (powered)
        {
            self->Relay_->Close();
        }
        else
        {
            self->Relay_->Open();
        }

        // Update the channel states
        for (size_t i = 0; i < active.size(); ++i)
        {
            if (active[i] && playing)
            {
                self->EnableMixerChannel(i);
            }
            else
            {
                self->DisableMixerChannel(i);
            }
        }

        // Update the state
        self->Working_ = powered && playing;
        self->LastWorkingMoment_ = (powered && playing) ? (time_t)time : (time_t)self->LastWorkingMoment_;
        self->PoweringDuration_ = powered ? (self->PoweringDuration_ + dt) : 0;

        for (uint i = 0; i < self->Config_.Channels; ++i)
        {
            self->ActiveChannels_[i] = active[i] && (powered && playing);
        }

        // Close the activation events
        if (self->Working_)
        {
            for (size_t i = 0; i < events.size(); ++i)
            {
                FulfillEvents(events[i].begin(), events[i].end(), events[i]);
            }
        }
    }
}

auto Driver::Warmed(time_t time, time_t workingMoment, time_t poweringDuration, bool urgent, bool working, const Config& config) noexcept -> bool
{
    return urgent ||
           working ||
           poweringDuration >= config.WarmingDuration ||
           (time - workingMoment <= config.CoolingDuration);
}

void Driver::FulfillEvents(ChannelEventsList::iterator begin, ChannelEventsList::iterator end, ChannelEventsList& list) noexcept
{
    for (auto k = begin; k != end; ++k)
    {
        k->Listener.set_value();
    }

    list.erase(begin, end);
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

auto Driver::DoIfChannelEnabled(uint i, const std::function<void()>& f) noexcept -> bool
{
    Mixer_->Enabled(i) ? f() : void();
    return Mixer_->Enabled(i);
}
