// Created by Tube Lab. Part of the meloun project.
#include "hardware/speaker/Driver.h"
using namespace ml;

/*auto SpeakerDriver::Open(const std::string& sink) noexcept -> bool
{
    std::lock_guard _ {SinksStatsLock_ };
    {
        if (!Sinks_.contains(sink))
        {
            return false;
        }

        // Open the sink that will be opened for the 1 prolongation cycle
        SinksStats_[sink].Opened = true;
        SinksStats_[sink].ExpiresAt = TimeNow() + ProlongationDuration;
        return true;
    }
}

auto SpeakerDriver::Prolong(const std::string& sink) noexcept -> bool
{
    std::lock_guard _ { SinksStatsLock_ };
    {
        if (!Sinks_.contains(sink) || !SinksStats_[sink].Opened)
        {
            return false;
        }

        SinksStats_[sink].ExpiresAt = TimeNow() + ProlongationDuration;
        return true;
    }
}

auto SpeakerDriver::Grab(const std::string& sink, bool urgent) noexcept -> std::future<bool>
{
    return std::future<bool>();
}

void SpeakerDriver::ControlLoop(const std::stop_token& token) noexcept
{
    while (!token.stop_requested())
    {
        auto time = TimeNow();
        CloseExpiredSinks(time);
        ChoosePlayerByPriority();
    }
}

void SpeakerDriver::CloseExpiredSinks(time_t time) noexcept
{
    std::lock_guard _ { SinksStatsLock_ };
    {
        for (auto& [name, sink] : SinksStats_)
        {
            if (sink.ExpiresAt <= time)
            {
                sink.Opened = false;
                FindSinkInfo(name).Player->Clear();
            }
        }
    }
}

void SpeakerDriver::ChoosePlayerByPriority() noexcept
{
    std::lock_guard _ { SinksStatsLock_ };
    {
        // Find the opened player with the highest priority
        std::pair<std::string, Uint64> best;
        for (const auto& [name, stats] : SinksStats_)
        {
            if (!stats.Opened) continue;
            if (best.second && best.second >= FindSinkInfo(name).Priority) continue;

            best = { name, FindSinkInfo(name).Priority };
        }

        // Mute all the player except the topmost one
        for (const auto& [name, info] : Sinks_)
        {
            if (name == best.first) continue;
            info.Player->Mute();
        }
    }
}

auto SpeakerDriver::FindSinkInfo(const std::string& name) -> const SinkInfo&
{
    return Sinks_.find(name)->second;
}*/
