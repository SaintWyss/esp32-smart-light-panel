#pragma once

#include <cstdint>

struct LightState {
    bool power;
    std::uint8_t brightness;
    bool connected;
    const char *provider;
    bool colorMode;
    std::uint16_t hue;
};

struct LightTimer {
    bool active;
    bool turnOn;
    std::uint32_t remainingSeconds;
};

class LightController {
public:
    virtual ~LightController() = default;

    virtual bool begin() = 0;
    virtual bool setPower(bool on) = 0;
    virtual bool setBrightness(std::uint8_t percent) = 0;
    virtual bool setColorHue(std::uint16_t hue) = 0;
    virtual bool setWhite() = 0;
    virtual bool setTimer(bool turnOn, std::uint16_t minutes) = 0;
    virtual void cancelTimer() = 0;
    virtual LightState state() const = 0;
    virtual LightTimer timer() const = 0;
};
