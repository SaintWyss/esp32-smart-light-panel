#pragma once

#include <lvgl.h>

#include "light_controller.h"

class SmartLightUi {
public:
    void begin(LightController &controller);
    void update();

private:
    static void onNavigate(lv_event_t *event);
    static void onPowerPressed(lv_event_t *event);
    static void onBrightnessChanged(lv_event_t *event);
    static void onBrightnessReleased(lv_event_t *event);
    static void onBrightnessPreset(lv_event_t *event);
    static void onColorPressed(lv_event_t *event);
    static void onTimerPressed(lv_event_t *event);
    static void onTimerCancel(lv_event_t *event);
    static void showPage(std::uint8_t page);
    static void renderBrightness(std::uint8_t value, bool updateSlider);
    static void renderTimer();
    static void refresh();

    static LightController *controller_;
    static lv_obj_t *pages_[3];
    static lv_obj_t *navButtons_[3];
    static lv_obj_t *powerButton_;
    static lv_obj_t *powerButtonLabel_;
    static lv_obj_t *stateLabel_;
    static lv_obj_t *brightnessLabel_;
    static lv_obj_t *brightnessSlider_;
    static lv_obj_t *connectionLabel_;
    static lv_obj_t *colorButtons_[7];
    static lv_obj_t *colorStatusLabel_;
    static lv_obj_t *timerStatusLabel_;
    static lv_obj_t *timerCancelButton_;
    static LightState renderedState_;
    static LightTimer renderedTimer_;
    static bool hasRenderedState_;
    static bool sliderActive_;
    static bool suppressSliderEvent_;
    static std::uint8_t activePage_;
    static char brightnessText_[8];
    static char timerText_[80];
    static char colorText_[32];
};
