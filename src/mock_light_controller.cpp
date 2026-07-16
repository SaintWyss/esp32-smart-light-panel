#include "mock_light_controller.h"

#include <algorithm>

bool MockLightController::begin()
{
    return true;
}

bool MockLightController::setPower(bool on)
{
    state_.power = on;
    return true;
}

bool MockLightController::setBrightness(std::uint8_t percent)
{
    state_.brightness = std::min<std::uint8_t>(percent, 100);
    if (state_.brightness > 0) {
        state_.power = true;
    }
    return true;
}

bool MockLightController::setColorHue(std::uint16_t hue)
{
    state_.hue = hue % 360;
    state_.colorMode = true;
    state_.power = true;
    return true;
}

bool MockLightController::setWhite()
{
    state_.colorMode = false;
    state_.power = true;
    return true;
}

bool MockLightController::setTimer(bool turnOn, std::uint16_t minutes)
{
    timer_ = {minutes > 0, turnOn, static_cast<std::uint32_t>(minutes) * 60};
    return minutes > 0;
}

void MockLightController::cancelTimer()
{
    timer_.active = false;
    timer_.remainingSeconds = 0;
}

LightState MockLightController::state() const
{
    return state_;
}

LightTimer MockLightController::timer() const
{
    return timer_;
}
