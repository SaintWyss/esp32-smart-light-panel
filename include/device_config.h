#pragma once

#include <Arduino.h>

struct DeviceConfig {
    String wifiSsid;
    String wifiPassword;
    String tuyaIp;
    String tuyaDeviceId;
    String tuyaLocalKey;

    bool valid() const;
    static bool load(DeviceConfig &config);
    static bool pollSerialProvisioning();
};
