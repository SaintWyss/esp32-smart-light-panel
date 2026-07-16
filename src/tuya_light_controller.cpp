#include "tuya_light_controller.h"

#include <WiFi.h>

#include <algorithm>

bool TuyaLightController::begin()
{
    if (!DeviceConfig::load(config_)) {
        state_.provider = "Falta configurar";
        Serial.println("PROVISIONING_READY");
        return false;
    }
    if (!protocol_.begin(config_.tuyaIp, config_.tuyaDeviceId, config_.tuyaLocalKey)) {
        state_.provider = "Configuracion invalida";
        return false;
    }
    return xTaskCreatePinnedToCore(
        workerEntry, "tuya_worker", 8192, this, 2, &workerTask_, 0
    ) == pdPASS;
}

bool TuyaLightController::setPower(bool on)
{
    portENTER_CRITICAL(&stateMux_);
    state_.power = on;
    pendingPower_ = true;
    portEXIT_CRITICAL(&stateMux_);
    if (workerTask_) {
        xTaskNotifyGive(workerTask_);
    }
    return workerTask_ != nullptr;
}

bool TuyaLightController::setBrightness(std::uint8_t percent)
{
    percent = std::min<std::uint8_t>(percent, 100);
    portENTER_CRITICAL(&stateMux_);
    state_.brightness = percent;
    state_.power = percent > 0;
    pendingBrightness_ = true;
    portEXIT_CRITICAL(&stateMux_);
    if (workerTask_) {
        xTaskNotifyGive(workerTask_);
    }
    return workerTask_ != nullptr;
}

bool TuyaLightController::setColorHue(std::uint16_t hue)
{
    portENTER_CRITICAL(&stateMux_);
    if (state_.brightness == 0) {
        state_.brightness = 50;
    }
    state_.hue = hue % 360;
    state_.colorMode = true;
    state_.power = true;
    pendingColor_ = true;
    pendingWhite_ = false;
    portEXIT_CRITICAL(&stateMux_);
    if (workerTask_) {
        xTaskNotifyGive(workerTask_);
    }
    return workerTask_ != nullptr;
}

bool TuyaLightController::setWhite()
{
    portENTER_CRITICAL(&stateMux_);
    if (state_.brightness == 0) {
        state_.brightness = 50;
    }
    state_.colorMode = false;
    state_.power = true;
    pendingWhite_ = true;
    pendingColor_ = false;
    portEXIT_CRITICAL(&stateMux_);
    if (workerTask_) {
        xTaskNotifyGive(workerTask_);
    }
    return workerTask_ != nullptr;
}

bool TuyaLightController::setTimer(bool turnOn, std::uint16_t minutes)
{
    minutes = std::clamp<std::uint16_t>(minutes, 1, 1440);
    portENTER_CRITICAL(&stateMux_);
    timerActive_ = true;
    timerTurnOn_ = turnOn;
    timerDeadlineMs_ = millis() + static_cast<unsigned long>(minutes) * 60000UL;
    portEXIT_CRITICAL(&stateMux_);
    if (workerTask_) {
        xTaskNotifyGive(workerTask_);
    }
    Serial.printf("TIMER_SET action=%s minutes=%u\n", turnOn ? "on" : "off", minutes);
    return workerTask_ != nullptr;
}

void TuyaLightController::cancelTimer()
{
    portENTER_CRITICAL(&stateMux_);
    timerActive_ = false;
    portEXIT_CRITICAL(&stateMux_);
    Serial.println("TIMER_CANCELLED");
}

LightState TuyaLightController::state() const
{
    portENTER_CRITICAL(&stateMux_);
    const LightState snapshot = state_;
    portEXIT_CRITICAL(&stateMux_);
    return snapshot;
}

LightTimer TuyaLightController::timer() const
{
    portENTER_CRITICAL(&stateMux_);
    const bool active = timerActive_;
    const bool turnOn = timerTurnOn_;
    const unsigned long deadline = timerDeadlineMs_;
    portEXIT_CRITICAL(&stateMux_);
    if (!active) {
        return {false, turnOn, 0};
    }
    const std::int32_t remainingMs = static_cast<std::int32_t>(deadline - millis());
    return {
        remainingMs > 0,
        turnOn,
        remainingMs > 0 ? static_cast<std::uint32_t>((remainingMs + 999) / 1000) : 0,
    };
}

void TuyaLightController::workerEntry(void *context)
{
    static_cast<TuyaLightController *>(context)->worker();
}

bool TuyaLightController::ensureWiFi()
{
    if (WiFi.status() == WL_CONNECTED) {
        if (!wifiConnectedLogged_) {
            Serial.printf("WIFI_CONNECTED ip=%s\n", WiFi.localIP().toString().c_str());
            wifiConnectedLogged_ = true;
        }
        return true;
    }

    wifiConnectedLogged_ = false;
    const unsigned long now = millis();
    const bool retryDue = wifiStarted_ && (now - lastWifiAttemptMs_ >= 30000);
    if (!wifiStarted_ || retryDue) {
        if (wifiStarted_) {
            WiFi.disconnect(false, false);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        lastWifiAttemptMs_ = millis();
        wifiStarted_ = true;
        Serial.println(retryDue ? "WIFI_RETRY" : "WIFI_CONNECTING");
        WiFi.begin(config_.wifiSsid.c_str(), config_.wifiPassword.c_str());
    }

    for (int attempt = 0; attempt < 40 && WiFi.status() != WL_CONNECTED; ++attempt) {
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WIFI_CONNECTED ip=%s\n", WiFi.localIP().toString().c_str());
        wifiConnectedLogged_ = true;
        return true;
    }
    Serial.printf("WIFI_WAIT status=%d\n", static_cast<int>(WiFi.status()));
    return false;
}

void TuyaLightController::setConnected(bool connected)
{
    portENTER_CRITICAL(&stateMux_);
    state_.connected = connected;
    portEXIT_CRITICAL(&stateMux_);
}

void TuyaLightController::worker()
{
    unsigned long nextQueryMs = 0;
    while (true) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
        if (!ensureWiFi()) {
            setConnected(false);
            continue;
        }

        enum class Work { None, Power, Brightness, Color, White, Query };
        Work work = Work::None;
        bool sendPower = false;
        bool sendBrightness = false;
        bool sendColor = false;
        bool sendWhite = false;
        LightState desired;
        portENTER_CRITICAL(&stateMux_);
        if (timerActive_ && static_cast<std::int32_t>(millis() - timerDeadlineMs_) >= 0) {
            timerActive_ = false;
            state_.power = timerTurnOn_;
            pendingPower_ = true;
        }
        sendPower = pendingPower_;
        sendBrightness = pendingBrightness_;
        sendColor = pendingColor_;
        sendWhite = pendingWhite_;
        desired = state_;
        if (sendColor) {
            pendingColor_ = false;
            work = Work::Color;
        } else if (sendWhite) {
            pendingWhite_ = false;
            work = Work::White;
        } else if (sendBrightness) {
            pendingBrightness_ = false;
            work = Work::Brightness;
        } else if (sendPower) {
            pendingPower_ = false;
            work = Work::Power;
        }
        portEXIT_CRITICAL(&stateMux_);

        if (work == Work::None
            && static_cast<std::int32_t>(millis() - nextQueryMs) >= 0) {
            work = Work::Query;
        }
        if (work == Work::None) {
            continue;
        }

        bool ok = true;
        const unsigned long transactionStartedMs = millis();
        if (work == Work::Color) {
            ok = protocol_.setColorHue(desired.hue, desired.brightness);
        } else if (work == Work::White) {
            ok = protocol_.setWhite();
        } else if (work == Work::Brightness) {
            if (desired.brightness == 0) {
                ok = protocol_.setPower(false);
            } else {
                ok = desired.colorMode
                    ? protocol_.setColorHue(desired.hue, desired.brightness)
                    : protocol_.setBrightness(desired.brightness);
            }
        } else if (work == Work::Power) {
            ok = protocol_.setPower(desired.power);
        } else {
            bool power = false;
            std::uint8_t brightness = 0;
            bool colorMode = false;
            std::uint16_t hue = 0;
            ok = protocol_.query(power, brightness, colorMode, hue);
            if (ok) {
                portENTER_CRITICAL(&stateMux_);
                state_.power = power;
                state_.brightness = brightness;
                state_.colorMode = colorMode;
                state_.hue = hue;
                portEXIT_CRITICAL(&stateMux_);
            }
        }
        nextQueryMs = millis() + 15000;

        if (!ok) {
            portENTER_CRITICAL(&stateMux_);
            pendingPower_ = pendingPower_ || work == Work::Power;
            pendingBrightness_ = pendingBrightness_ || work == Work::Brightness;
            pendingColor_ = pendingColor_ || work == Work::Color;
            pendingWhite_ = pendingWhite_ || work == Work::White;
            portEXIT_CRITICAL(&stateMux_);
        }
        const char *kind = work == Work::Power ? "power"
            : work == Work::Brightness ? "brightness"
            : work == Work::Color ? "color"
            : work == Work::White ? "white"
            : "query";
        Serial.printf(
            "TUYA_TX kind=%s ok=%d elapsed_ms=%lu\n",
            kind, ok ? 1 : 0, millis() - transactionStartedMs
        );
        if (ok) {
            const LightState logged = state();
            Serial.printf(
                "TUYA_LOCAL_OK power=%s brightness=%u%% mode=%s hue=%u\n",
                logged.power ? "on" : "off",
                logged.brightness,
                logged.colorMode ? "color" : "white",
                logged.hue
            );
        } else {
            Serial.println("TUYA_LOCAL_RETRY");
        }
        setConnected(ok);
    }
}
