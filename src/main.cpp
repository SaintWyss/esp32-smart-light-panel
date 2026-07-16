#include <Arduino.h>
#include <esp_display_panel.hpp>

#include "lvgl_v8_port.h"
#include "device_config.h"
#include "smart_light_ui.h"
#include "tuya_light_controller.h"

using namespace esp_panel::board;
using namespace esp_panel::drivers;

namespace {
Board *board = nullptr;
TuyaLightController lightController;
SmartLightUi ui;
unsigned long lastUiUpdate = 0;
}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.println("Smart Light Panel: inicio");

    board = new Board();
    board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
    auto *lcd = board->getLCD();
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto *lcdBus = lcd->getBus();
    if (lcdBus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcdBus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif

    if (!board->begin()) {
        Serial.println("ERROR: no se pudo iniciar pantalla/tactil");
        return;
    }

    lvgl_port_init(board->getLCD(), board->getTouch());
    lightController.begin();

    lvgl_port_lock(-1);
    ui.begin(lightController);
    lvgl_port_unlock();

    Serial.println("Smart Light Panel: interfaz lista");
}

void loop()
{
    DeviceConfig::pollSerialProvisioning();
    if (millis() - lastUiUpdate >= 1000) {
        lastUiUpdate = millis();
        lvgl_port_lock(-1);
        ui.update();
        lvgl_port_unlock();
    }
    delay(20);
}
