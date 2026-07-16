#pragma once

#include "light_controller.h"

class MockLightController final : public LightController {
public:
    bool begin() override;
    bool setPower(bool on) override;
    bool setBrightness(std::uint8_t percent) override;
    bool setColorHue(std::uint16_t hue) override;
    bool setWhite() override;
    bool setTimer(bool turnOn, std::uint16_t minutes) override;
    void cancelTimer() override;
    LightState state() const override;
    LightTimer timer() const override;

private:
    LightState state_{false, 50, true, "Simulacion local", false, 0};
    LightTimer timer_{false, false, 0};
};
