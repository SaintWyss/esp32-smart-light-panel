#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "device_config.h"
#include "light_controller.h"
#include "tuya_protocol_33.h"

class TuyaLightController final : public LightController {
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
    static void workerEntry(void *context);
    void worker();
    bool ensureWiFi();
    void setConnected(bool connected);

    mutable portMUX_TYPE stateMux_ = portMUX_INITIALIZER_UNLOCKED;
    LightState state_{false, 50, false, "Tuya local", false, 0};
    DeviceConfig config_;
    TuyaProtocol33 protocol_;
    TaskHandle_t workerTask_{nullptr};
    bool pendingPower_{false};
    bool pendingBrightness_{false};
    bool pendingColor_{false};
    bool pendingWhite_{false};
    bool timerActive_{false};
    bool timerTurnOn_{false};
    unsigned long timerDeadlineMs_{0};
    bool wifiStarted_{false};
    bool wifiConnectedLogged_{false};
    unsigned long lastWifiAttemptMs_{0};
};
