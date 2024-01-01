// Created by Tube Lab. Part of the meloun project.
#include "hardware/amplifier/LampDriver.h"
using namespace ml::amplifier;

auto LampDriver::Create(const LampConfig& cfg) noexcept -> std::shared_ptr<LampDriver>
{
    return std::shared_ptr<LampDriver>();
}

LampDriver::~LampDriver() {

}

auto LampDriver::DoEnqueue(uint channel, const ml::audio::Track &track) -> std::optional<std::future<void>> {
    return std::optional<std::future<void>>();
}

void LampDriver::DoSkip(uint channel) noexcept {

}

void LampDriver::DoClear(uint channel) noexcept {

}

auto LampDriver::DoDurationLeft(uint channel) const noexcept -> time_t {
    return 0;
}

void LampDriver::DoOpen(uint channel) noexcept {

}

void LampDriver::DoClose(uint channel) noexcept {

}

bool LampDriver::DoActivation(time_t time, time_t elapsed, bool urgently) noexcept {
    return false;
}

bool LampDriver::DoDeactivation(time_t time, time_t elapsed, bool urgently) noexcept {
    return false;
}
