#include "device_config.h"

#include <Preferences.h>
#include <mbedtls/base64.h>

#include <vector>

namespace {
constexpr char kNamespace[] = "smartlight";

bool decodeBase64(const String &encoded, String &decoded)
{
    size_t outputLength = 0;
    const int probe = mbedtls_base64_decode(
        nullptr, 0, &outputLength,
        reinterpret_cast<const unsigned char *>(encoded.c_str()), encoded.length()
    );
    if (probe != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL || outputLength == 0) {
        return false;
    }

    std::vector<unsigned char> output(outputLength + 1, 0);
    if (mbedtls_base64_decode(
            output.data(), output.size(), &outputLength,
            reinterpret_cast<const unsigned char *>(encoded.c_str()), encoded.length()
        ) != 0) {
        return false;
    }
    decoded = String(reinterpret_cast<const char *>(output.data()), outputLength);
    return true;
}

bool save(const DeviceConfig &config)
{
    Preferences preferences;
    if (!preferences.begin(kNamespace, false)) {
        return false;
    }
    const bool ok = preferences.putString("ssid", config.wifiSsid) > 0
        && preferences.putString("wifi_pass", config.wifiPassword) > 0
        && preferences.putString("tuya_ip", config.tuyaIp) > 0
        && preferences.putString("tuya_id", config.tuyaDeviceId) > 0
        && preferences.putString("tuya_key", config.tuyaLocalKey) > 0;
    preferences.end();
    return ok;
}
}  // namespace

bool DeviceConfig::valid() const
{
    IPAddress address;
    return !wifiSsid.isEmpty()
        && address.fromString(tuyaIp)
        && tuyaDeviceId.length() >= 20
        && tuyaDeviceId.length() <= 24
        && tuyaLocalKey.length() == 16;
}

bool DeviceConfig::load(DeviceConfig &config)
{
    Preferences preferences;
    if (!preferences.begin(kNamespace, true)) {
        return false;
    }
    config.wifiSsid = preferences.getString("ssid", "");
    config.wifiPassword = preferences.getString("wifi_pass", "");
    config.tuyaIp = preferences.getString("tuya_ip", "");
    config.tuyaDeviceId = preferences.getString("tuya_id", "");
    config.tuyaLocalKey = preferences.getString("tuya_key", "");
    preferences.end();
    return config.valid();
}

bool DeviceConfig::pollSerialProvisioning()
{
    static String line;
    while (Serial.available() > 0) {
        const char character = static_cast<char>(Serial.read());
        if (character == '\r') {
            continue;
        }
        if (character != '\n') {
            if (line.length() < 512) {
                line += character;
            }
            continue;
        }

        String command = line;
        line = "";
        command.trim();
        if (command.startsWith("WIFI ")) {
            const int separator = command.indexOf(' ', 5);
            DeviceConfig config;
            if (separator < 0
                || !load(config)
                || !decodeBase64(command.substring(5, separator), config.wifiSsid)
                || !decodeBase64(command.substring(separator + 1), config.wifiPassword)
                || !config.valid()
                || !save(config)) {
                Serial.println("WIFI_CONFIG_ERROR");
                return false;
            }
            Serial.println("WIFI_CONFIG_OK");
            delay(200);
            ESP.restart();
        }
        if (!command.startsWith("CFG ")) {
            continue;
        }

        String fields[5];
        int field = 0;
        int start = 4;
        while (field < 5) {
            const int separator = command.indexOf(' ', start);
            if (separator < 0) {
                fields[field++] = command.substring(start);
                break;
            }
            fields[field++] = command.substring(start, separator);
            start = separator + 1;
        }
        if (field != 5) {
            Serial.println("PROVISIONING_ERROR formato");
            return false;
        }

        DeviceConfig config;
        if (!decodeBase64(fields[0], config.wifiSsid)
            || !decodeBase64(fields[1], config.wifiPassword)
            || !decodeBase64(fields[4], config.tuyaLocalKey)) {
            Serial.println("PROVISIONING_ERROR base64");
            return false;
        }
        config.tuyaIp = fields[2];
        config.tuyaDeviceId = fields[3];
        if (!config.valid() || !save(config)) {
            Serial.println("PROVISIONING_ERROR validacion");
            return false;
        }

        Serial.println("PROVISIONING_OK");
        delay(200);
        ESP.restart();
    }
    return false;
}
