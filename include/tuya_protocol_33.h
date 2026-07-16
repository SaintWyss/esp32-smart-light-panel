#pragma once

#include <Arduino.h>
#include <WiFiClient.h>

#include <cstdint>
#include <vector>

class TuyaProtocol33 {
public:
    bool begin(const String &ip, const String &deviceId, const String &localKey);
    bool query(bool &power, std::uint8_t &brightnessPercent, bool &colorMode, std::uint16_t &hue);
    bool setPower(bool on);
    bool setBrightness(std::uint8_t percent);
    bool setColorHue(std::uint16_t hue, std::uint8_t brightnessPercent);
    bool setWhite();

private:
    bool transact(std::uint32_t command, const String &json, bool addVersionHeader, String *response);
    bool readResponse(WiFiClient &client, String &cleartext);
    bool encrypt(const std::uint8_t *data, size_t length, std::vector<std::uint8_t> &output) const;
    bool decrypt(const std::uint8_t *data, size_t length, std::vector<std::uint8_t> &output) const;

    String ip_;
    String deviceId_;
    std::uint8_t key_[16]{};
    std::uint32_t sequence_{1};
};
