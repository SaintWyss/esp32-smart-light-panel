#include "tuya_protocol_33.h"

#include <mbedtls/aes.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace {
constexpr std::uint16_t kTuyaPort = 6668;
constexpr std::uint32_t kPrefix = 0x000055AA;
constexpr std::uint32_t kSuffix = 0x0000AA55;
constexpr std::uint32_t kControl = 0x07;
constexpr std::uint32_t kQuery = 0x0A;
constexpr size_t kBlockSize = 16;

void appendU32(std::vector<std::uint8_t> &output, std::uint32_t value)
{
    output.push_back((value >> 24) & 0xFF);
    output.push_back((value >> 16) & 0xFF);
    output.push_back((value >> 8) & 0xFF);
    output.push_back(value & 0xFF);
}

std::uint32_t readU32(const std::uint8_t *data)
{
    return (std::uint32_t(data[0]) << 24)
        | (std::uint32_t(data[1]) << 16)
        | (std::uint32_t(data[2]) << 8)
        | std::uint32_t(data[3]);
}

std::uint32_t crc32(const std::uint8_t *data, size_t length)
{
    std::uint32_t crc = 0xFFFFFFFF;
    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}

bool readExact(WiFiClient &client, std::uint8_t *data, size_t length)
{
    size_t received = 0;
    const unsigned long started = millis();
    while (received < length && millis() - started < 4000) {
        const int available = client.available();
        if (available <= 0) {
            delay(3);
            continue;
        }
        const int count = client.read(data + received, length - received);
        if (count > 0) {
            received += static_cast<size_t>(count);
        }
    }
    return received == length;
}

long commandTimestamp()
{
    const std::time_t now = std::time(nullptr);
    return now > 1700000000 ? static_cast<long>(now) : static_cast<long>(millis() / 1000);
}
}  // namespace

bool TuyaProtocol33::begin(const String &ip, const String &deviceId, const String &localKey)
{
    IPAddress address;
    if (!address.fromString(ip) || deviceId.isEmpty() || localKey.length() != sizeof(key_)) {
        return false;
    }
    ip_ = ip;
    deviceId_ = deviceId;
    std::memcpy(key_, localKey.c_str(), sizeof(key_));
    return true;
}

bool TuyaProtocol33::encrypt(
    const std::uint8_t *data, size_t length, std::vector<std::uint8_t> &output
) const
{
    const std::uint8_t padding = kBlockSize - (length % kBlockSize);
    std::vector<std::uint8_t> input(data, data + length);
    input.insert(input.end(), padding, padding);
    output.assign(input.size(), 0);

    mbedtls_aes_context context;
    mbedtls_aes_init(&context);
    int result = mbedtls_aes_setkey_enc(&context, key_, 128);
    for (size_t offset = 0; result == 0 && offset < input.size(); offset += kBlockSize) {
        result = mbedtls_aes_crypt_ecb(
            &context, MBEDTLS_AES_ENCRYPT, input.data() + offset, output.data() + offset
        );
    }
    mbedtls_aes_free(&context);
    return result == 0;
}

bool TuyaProtocol33::decrypt(
    const std::uint8_t *data, size_t length, std::vector<std::uint8_t> &output
) const
{
    if (length == 0 || length % kBlockSize != 0) {
        return false;
    }
    output.assign(length, 0);
    mbedtls_aes_context context;
    mbedtls_aes_init(&context);
    int result = mbedtls_aes_setkey_dec(&context, key_, 128);
    for (size_t offset = 0; result == 0 && offset < length; offset += kBlockSize) {
        result = mbedtls_aes_crypt_ecb(
            &context, MBEDTLS_AES_DECRYPT, data + offset, output.data() + offset
        );
    }
    mbedtls_aes_free(&context);
    if (result != 0 || output.empty()) {
        return false;
    }
    const std::uint8_t padding = output.back();
    if (padding == 0 || padding > kBlockSize || padding > output.size()) {
        return false;
    }
    for (size_t index = output.size() - padding; index < output.size(); ++index) {
        if (output[index] != padding) {
            return false;
        }
    }
    output.resize(output.size() - padding);
    return true;
}

bool TuyaProtocol33::transact(
    std::uint32_t command, const String &json, bool addVersionHeader, String *response
)
{
    WiFiClient client;
    if (!client.connect(ip_.c_str(), kTuyaPort, 3000)) {
        return false;
    }
    client.setTimeout(4000);

    std::vector<std::uint8_t> encrypted;
    if (!encrypt(
            reinterpret_cast<const std::uint8_t *>(json.c_str()), json.length(), encrypted
        )) {
        return false;
    }

    std::vector<std::uint8_t> payload;
    if (addVersionHeader) {
        static constexpr std::uint8_t versionHeader[15] = {
            '3', '.', '3', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };
        payload.insert(payload.end(), std::begin(versionHeader), std::end(versionHeader));
    }
    payload.insert(payload.end(), encrypted.begin(), encrypted.end());

    std::vector<std::uint8_t> packet;
    appendU32(packet, kPrefix);
    appendU32(packet, sequence_++);
    appendU32(packet, command);
    appendU32(packet, payload.size() + 8);
    packet.insert(packet.end(), payload.begin(), payload.end());
    appendU32(packet, crc32(packet.data(), packet.size()));
    appendU32(packet, kSuffix);

    if (client.write(packet.data(), packet.size()) != packet.size()) {
        return false;
    }
    String cleartext;
    const bool ok = readResponse(client, cleartext);
    client.stop();
    if (ok && response) {
        *response = cleartext;
    }
    return ok;
}

bool TuyaProtocol33::readResponse(WiFiClient &client, String &cleartext)
{
    std::uint8_t header[16]{};
    if (!readExact(client, header, sizeof(header)) || readU32(header) != kPrefix) {
        return false;
    }
    const std::uint32_t length = readU32(header + 12);
    if (length < 8 || length > 2048) {
        return false;
    }

    std::vector<std::uint8_t> rest(length);
    if (!readExact(client, rest.data(), rest.size())
        || readU32(rest.data() + rest.size() - 4) != kSuffix) {
        return false;
    }

    std::vector<std::uint8_t> checked(header, header + sizeof(header));
    checked.insert(checked.end(), rest.begin(), rest.end() - 8);
    if (readU32(rest.data() + rest.size() - 8) != crc32(checked.data(), checked.size())) {
        return false;
    }

    size_t payloadOffset = 4;  // Tuya responses start with a return code.
    size_t payloadLength = rest.size() - 8 - payloadOffset;
    if (payloadLength == 0) {
        cleartext = "{}";
        return true;
    }
    if (payloadLength >= 15 && rest[payloadOffset] == '3' && rest[payloadOffset + 1] == '.') {
        payloadOffset += 15;
        payloadLength -= 15;
    }

    std::vector<std::uint8_t> decrypted;
    if (!decrypt(rest.data() + payloadOffset, payloadLength, decrypted)) {
        return false;
    }
    cleartext = String(reinterpret_cast<const char *>(decrypted.data()), decrypted.size());
    return true;
}

bool TuyaProtocol33::query(
    bool &power, std::uint8_t &brightnessPercent, bool &colorMode, std::uint16_t &hue
)
{
    char payload[256]{};
    std::snprintf(
        payload, sizeof(payload),
        "{\"gwId\":\"%s\",\"devId\":\"%s\",\"uid\":\"%s\",\"t\":\"%ld\"}",
        deviceId_.c_str(), deviceId_.c_str(), deviceId_.c_str(), commandTimestamp()
    );
    String response;
    if (!transact(kQuery, payload, false, &response)) {
        return false;
    }

    const int powerIndex = response.indexOf("\"20\":");
    const int brightnessIndex = response.indexOf("\"22\":");
    if (powerIndex < 0 || brightnessIndex < 0) {
        return false;
    }
    power = response.substring(powerIndex + 5).startsWith("true");
    const int rawBrightness = response.substring(brightnessIndex + 5).toInt();
    brightnessPercent = static_cast<std::uint8_t>(
        std::clamp((rawBrightness - 10) * 100 / 990, 0, 100)
    );

    const int modeIndex = response.indexOf("\"21\":\"");
    colorMode = modeIndex >= 0 && response.substring(modeIndex + 6).startsWith("colour");
    hue = 0;
    if (colorMode) {
        const int colorIndex = response.indexOf("\"24\":\"");
        if (colorIndex >= 0) {
            const String hsv = response.substring(colorIndex + 6, colorIndex + 18);
            if (hsv.length() == 12) {
                hue = static_cast<std::uint16_t>(std::strtoul(hsv.substring(0, 4).c_str(), nullptr, 16));
                const int colorValue = std::strtoul(hsv.substring(8, 12).c_str(), nullptr, 16);
                brightnessPercent = static_cast<std::uint8_t>(std::clamp(colorValue / 10, 0, 100));
            }
        }
    }
    return true;
}

bool TuyaProtocol33::setPower(bool on)
{
    char payload[256]{};
    std::snprintf(
        payload, sizeof(payload),
        "{\"devId\":\"%s\",\"uid\":\"%s\",\"t\":\"%ld\",\"dps\":{\"20\":%s}}",
        deviceId_.c_str(), deviceId_.c_str(), commandTimestamp(), on ? "true" : "false"
    );
    return transact(kControl, payload, true, nullptr);
}

bool TuyaProtocol33::setBrightness(std::uint8_t percent)
{
    percent = std::min<std::uint8_t>(percent, 100);
    const int rawBrightness = percent == 0 ? 10 : 10 + percent * 990 / 100;
    char payload[288]{};
    std::snprintf(
        payload, sizeof(payload),
        "{\"devId\":\"%s\",\"uid\":\"%s\",\"t\":\"%ld\",\"dps\":{\"20\":%s,\"22\":%d}}",
        deviceId_.c_str(), deviceId_.c_str(), commandTimestamp(),
        percent > 0 ? "true" : "false", rawBrightness
    );
    return transact(kControl, payload, true, nullptr);
}

bool TuyaProtocol33::setColorHue(std::uint16_t hue, std::uint8_t brightnessPercent)
{
    hue %= 360;
    brightnessPercent = std::clamp<std::uint8_t>(brightnessPercent, 1, 100);
    const int value = brightnessPercent * 10;
    char colorData[13]{};
    std::snprintf(colorData, sizeof(colorData), "%04x%04x%04x", hue, 1000, value);
    char payload[320]{};
    std::snprintf(
        payload, sizeof(payload),
        "{\"devId\":\"%s\",\"uid\":\"%s\",\"t\":\"%ld\",\"dps\":{\"20\":true,\"21\":\"colour\",\"24\":\"%s\"}}",
        deviceId_.c_str(), deviceId_.c_str(), commandTimestamp(), colorData
    );
    return transact(kControl, payload, true, nullptr);
}

bool TuyaProtocol33::setWhite()
{
    char payload[288]{};
    std::snprintf(
        payload, sizeof(payload),
        "{\"devId\":\"%s\",\"uid\":\"%s\",\"t\":\"%ld\",\"dps\":{\"20\":true,\"21\":\"white\"}}",
        deviceId_.c_str(), deviceId_.c_str(), commandTimestamp()
    );
    return transact(kControl, payload, true, nullptr);
}
