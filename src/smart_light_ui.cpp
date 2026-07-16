#include "smart_light_ui.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
lv_color_t color(std::uint32_t hex)
{
    return lv_color_hex(hex);
}

constexpr std::uint32_t kCanvas = 0xF4F3EF;
constexpr std::uint32_t kSurface = 0xFFFFFF;
constexpr std::uint32_t kText = 0x20201D;
constexpr std::uint32_t kMuted = 0x686761;
constexpr std::uint32_t kBorder = 0xD7D5CE;
constexpr std::uint32_t kWarm = 0xD8902F;
constexpr std::uint32_t kWarmPressed = 0xBF761D;
constexpr std::uint32_t kGreen = 0x346538;
constexpr std::uint32_t kRed = 0x9F2F2D;
constexpr std::uint16_t kColorHues[] = {0, 0, 30, 120, 180, 240, 285};
constexpr std::uint32_t kColorHex[] = {
    0xF4EFE4, 0xE53935, 0xF28C28, 0x43A047, 0x26A6A6, 0x356AE6, 0x8E44AD,
};
constexpr const char *kColorNames[] = {
    "BLANCO", "ROJO", "NARANJA", "VERDE", "CIAN", "AZUL", "VIOLETA",
};

lv_obj_t *makeLabel(
    lv_obj_t *parent, const char *text, const lv_font_t *font, std::uint32_t textColor
)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color(textColor), 0);
    return label;
}

void styleFlatButton(lv_obj_t *button)
{
    lv_obj_set_style_bg_color(button, color(kSurface), 0);
    lv_obj_set_style_bg_color(button, color(0xE6E4DD), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(button, color(kBorder), 0);
    lv_obj_set_style_border_width(button, 1, 0);
    lv_obj_set_style_radius(button, 0, 0);
    lv_obj_set_style_shadow_width(button, 0, 0);
}

lv_obj_t *makePage(lv_obj_t *screen)
{
    lv_obj_t *page = lv_obj_create(screen);
    lv_obj_set_size(page, 800, 372);
    lv_obj_set_pos(page, 0, 108);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 0, 0);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}
}  // namespace

LightController *SmartLightUi::controller_ = nullptr;
lv_obj_t *SmartLightUi::pages_[3]{};
lv_obj_t *SmartLightUi::navButtons_[3]{};
lv_obj_t *SmartLightUi::powerButton_ = nullptr;
lv_obj_t *SmartLightUi::powerButtonLabel_ = nullptr;
lv_obj_t *SmartLightUi::stateLabel_ = nullptr;
lv_obj_t *SmartLightUi::brightnessLabel_ = nullptr;
lv_obj_t *SmartLightUi::brightnessSlider_ = nullptr;
lv_obj_t *SmartLightUi::connectionLabel_ = nullptr;
lv_obj_t *SmartLightUi::colorButtons_[7]{};
lv_obj_t *SmartLightUi::colorStatusLabel_ = nullptr;
lv_obj_t *SmartLightUi::timerStatusLabel_ = nullptr;
lv_obj_t *SmartLightUi::timerCancelButton_ = nullptr;
LightState SmartLightUi::renderedState_{false, 0, false, "", false, 0};
LightTimer SmartLightUi::renderedTimer_{false, false, 0};
bool SmartLightUi::hasRenderedState_ = false;
bool SmartLightUi::sliderActive_ = false;
bool SmartLightUi::suppressSliderEvent_ = false;
std::uint8_t SmartLightUi::activePage_ = 255;
char SmartLightUi::brightnessText_[8]{};
char SmartLightUi::timerText_[80]{};
char SmartLightUi::colorText_[32]{};

void SmartLightUi::begin(LightController &controller)
{
    controller_ = &controller;
    hasRenderedState_ = false;
    sliderActive_ = false;
    suppressSliderEvent_ = false;

    lv_obj_t *screen = lv_scr_act();
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, color(kCanvas), 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = makeLabel(screen, "Luz del living", &lv_font_montserrat_20, kText);
    lv_obj_set_pos(title, 24, 15);
    connectionLabel_ = makeLabel(screen, "", &lv_font_montserrat_12, kGreen);
    lv_obj_align(connectionLabel_, LV_ALIGN_TOP_RIGHT, -24, 21);

    constexpr const char *navNames[] = {"LUZ", "COLOR", "TIMER"};
    for (int index = 0; index < 3; ++index) {
        navButtons_[index] = lv_btn_create(screen);
        lv_obj_set_size(navButtons_[index], 104, 44);
        lv_obj_set_pos(navButtons_[index], 24 + index * 112, 54);
        styleFlatButton(navButtons_[index]);
        lv_obj_add_event_cb(
            navButtons_[index], onNavigate, LV_EVENT_PRESSED,
            reinterpret_cast<void *>(static_cast<std::intptr_t>(index))
        );
        lv_obj_t *label = makeLabel(navButtons_[index], navNames[index], &lv_font_montserrat_14, kText);
        lv_obj_center(label);
    }

    pages_[0] = makePage(screen);
    stateLabel_ = makeLabel(pages_[0], "", &lv_font_montserrat_20, kText);
    lv_obj_set_pos(stateLabel_, 32, 20);
    powerButton_ = lv_btn_create(pages_[0]);
    lv_obj_set_size(powerButton_, 220, 120);
    lv_obj_set_pos(powerButton_, 32, 62);
    lv_obj_set_style_radius(powerButton_, 0, 0);
    lv_obj_set_style_shadow_width(powerButton_, 0, 0);
    lv_obj_add_event_cb(powerButton_, onPowerPressed, LV_EVENT_PRESSED, nullptr);
    powerButtonLabel_ = makeLabel(powerButton_, "", &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_center(powerButtonLabel_);

    lv_obj_t *brightnessTitle = makeLabel(pages_[0], "BRILLO", &lv_font_montserrat_12, kMuted);
    lv_obj_set_pos(brightnessTitle, 292, 20);
    brightnessLabel_ = makeLabel(pages_[0], "", &lv_font_montserrat_20, kText);
    lv_obj_set_pos(brightnessLabel_, 688, 16);
    brightnessSlider_ = lv_slider_create(pages_[0]);
    lv_obj_set_size(brightnessSlider_, 468, 48);
    lv_obj_set_pos(brightnessSlider_, 292, 66);
    lv_slider_set_range(brightnessSlider_, 0, 100);
    lv_obj_set_style_bg_color(brightnessSlider_, color(kBorder), LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightnessSlider_, color(kWarm), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightnessSlider_, color(kText), LV_PART_KNOB);
    lv_obj_set_style_pad_all(brightnessSlider_, 10, LV_PART_KNOB);
    lv_obj_add_event_cb(brightnessSlider_, onBrightnessChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(brightnessSlider_, onBrightnessReleased, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_event_cb(brightnessSlider_, onBrightnessReleased, LV_EVENT_PRESS_LOST, nullptr);
    constexpr int presets[] = {25, 50, 100};
    for (int index = 0; index < 3; ++index) {
        lv_obj_t *button = lv_btn_create(pages_[0]);
        lv_obj_set_size(button, 136, 58);
        lv_obj_set_pos(button, 292 + index * 156, 144);
        styleFlatButton(button);
        lv_obj_add_event_cb(
            button, onBrightnessPreset, LV_EVENT_PRESSED,
            reinterpret_cast<void *>(static_cast<std::intptr_t>(presets[index]))
        );
        char labelText[8];
        std::snprintf(labelText, sizeof(labelText), "%d %%", presets[index]);
        lv_obj_t *label = makeLabel(button, labelText, &lv_font_montserrat_14, kText);
        lv_obj_center(label);
    }
    makeLabel(pages_[0], "Respuesta directa por red local", &lv_font_montserrat_12, kMuted);
    lv_obj_set_pos(lv_obj_get_child(pages_[0], -1), 292, 232);

    pages_[1] = makePage(screen);
    lv_obj_t *colorTitle = makeLabel(pages_[1], "Elegí el tono", &lv_font_montserrat_16, kText);
    lv_obj_set_pos(colorTitle, 24, 18);
    for (int index = 0; index < 7; ++index) {
        colorButtons_[index] = lv_btn_create(pages_[1]);
        lv_obj_set_size(colorButtons_[index], 96, 80);
        lv_obj_set_pos(colorButtons_[index], 24 + index * 108, 70);
        lv_obj_set_style_bg_color(colorButtons_[index], color(kColorHex[index]), 0);
        lv_obj_set_style_bg_color(colorButtons_[index], color(kColorHex[index]), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(colorButtons_[index], color(kText), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(colorButtons_[index], 1, 0);
        lv_obj_set_style_border_width(colorButtons_[index], 4, LV_STATE_PRESSED);
        lv_obj_set_style_radius(colorButtons_[index], 0, 0);
        lv_obj_set_style_shadow_width(colorButtons_[index], 0, 0);
        lv_obj_add_event_cb(
            colorButtons_[index], onColorPressed, LV_EVENT_PRESSED,
            reinterpret_cast<void *>(static_cast<std::intptr_t>(index))
        );
        lv_obj_t *label = makeLabel(pages_[1], kColorNames[index], &lv_font_montserrat_12, kText);
        lv_obj_set_width(label, 96);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(label, 24 + index * 108, 160);
    }
    lv_obj_t *colorHint = makeLabel(
        pages_[1], "El brillo se mantiene al cambiar de color.", &lv_font_montserrat_12, kMuted
    );
    lv_obj_set_pos(colorHint, 24, 210);
    colorStatusLabel_ = makeLabel(pages_[1], "", &lv_font_montserrat_14, kText);
    lv_obj_set_pos(colorStatusLabel_, 24, 252);

    pages_[2] = makePage(screen);
    timerStatusLabel_ = makeLabel(pages_[2], "", &lv_font_montserrat_16, kText);
    lv_obj_set_pos(timerStatusLabel_, 24, 14);
    constexpr bool actions[] = {false, true};
    constexpr const char *actionLabels[] = {"APAGAR EN", "ENCENDER EN"};
    constexpr int minutes[] = {15, 30, 60};
    constexpr const char *minuteLabels[] = {"15 min", "30 min", "1 hora"};
    for (int row = 0; row < 2; ++row) {
        lv_obj_t *label = makeLabel(pages_[2], actionLabels[row], &lv_font_montserrat_12, kMuted);
        lv_obj_set_pos(label, 24, 76 + row * 92);
        for (int index = 0; index < 3; ++index) {
            lv_obj_t *button = lv_btn_create(pages_[2]);
            lv_obj_set_size(button, 142, 60);
            lv_obj_set_pos(button, 178 + index * 158, 58 + row * 92);
            styleFlatButton(button);
            const std::intptr_t encoded = (actions[row] ? 0x10000 : 0) | minutes[index];
            lv_obj_add_event_cb(
                button, onTimerPressed, LV_EVENT_PRESSED,
                reinterpret_cast<void *>(encoded)
            );
            lv_obj_t *buttonLabel = makeLabel(button, minuteLabels[index], &lv_font_montserrat_14, kText);
            lv_obj_center(buttonLabel);
        }
    }
    timerCancelButton_ = lv_btn_create(pages_[2]);
    lv_obj_set_size(timerCancelButton_, 190, 54);
    lv_obj_set_pos(timerCancelButton_, 178, 258);
    styleFlatButton(timerCancelButton_);
    lv_obj_add_event_cb(timerCancelButton_, onTimerCancel, LV_EVENT_PRESSED, nullptr);
    lv_obj_t *cancelLabel = makeLabel(timerCancelButton_, "CANCELAR TIMER", &lv_font_montserrat_14, kRed);
    lv_obj_center(cancelLabel);

    showPage(0);
    refresh();
}

void SmartLightUi::update()
{
    refresh();
}

void SmartLightUi::onNavigate(lv_event_t *event)
{
    showPage(static_cast<std::uint8_t>(reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event))));
}

void SmartLightUi::showPage(std::uint8_t page)
{
    page = page < 3 ? page : 0;
    if (page == activePage_) {
        return;
    }
    if (activePage_ < 3) {
        lv_obj_add_flag(pages_[activePage_], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_color(navButtons_[activePage_], color(kSurface), 0);
        lv_obj_set_style_text_color(lv_obj_get_child(navButtons_[activePage_], 0), color(kText), 0);
    } else {
        for (int index = 0; index < 3; ++index) {
            lv_obj_add_flag(pages_[index], LV_OBJ_FLAG_HIDDEN);
        }
    }
    activePage_ = page;
    lv_obj_clear_flag(pages_[activePage_], LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_bg_color(navButtons_[activePage_], color(kText), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(navButtons_[activePage_], 0), color(0xFFFFFF), 0);
    refresh();
}

void SmartLightUi::onPowerPressed(lv_event_t *)
{
    controller_->setPower(!controller_->state().power);
    refresh();
}

void SmartLightUi::onBrightnessChanged(lv_event_t *event)
{
    if (suppressSliderEvent_) {
        return;
    }
    sliderActive_ = true;
    renderBrightness(lv_slider_get_value(lv_event_get_target(event)), false);
}

void SmartLightUi::onBrightnessReleased(lv_event_t *event)
{
    const auto value = static_cast<std::uint8_t>(lv_slider_get_value(lv_event_get_target(event)));
    sliderActive_ = false;
    controller_->setBrightness(value);
    refresh();
}

void SmartLightUi::onBrightnessPreset(lv_event_t *event)
{
    const auto value = static_cast<std::uint8_t>(reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
    sliderActive_ = false;
    controller_->setBrightness(value);
    refresh();
}

void SmartLightUi::onColorPressed(lv_event_t *event)
{
    const int index = static_cast<int>(reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event)));
    if (index == 0) {
        controller_->setWhite();
    } else if (index > 0 && index < 7) {
        controller_->setColorHue(kColorHues[index]);
    }
    refresh();
}

void SmartLightUi::onTimerPressed(lv_event_t *event)
{
    const auto encoded = reinterpret_cast<std::intptr_t>(lv_event_get_user_data(event));
    controller_->setTimer((encoded & 0x10000) != 0, encoded & 0xFFFF);
    renderTimer();
}

void SmartLightUi::onTimerCancel(lv_event_t *)
{
    controller_->cancelTimer();
    renderTimer();
}

void SmartLightUi::renderBrightness(std::uint8_t value, bool updateSlider)
{
    std::snprintf(brightnessText_, sizeof(brightnessText_), "%u %%", value);
    lv_label_set_text_static(brightnessLabel_, brightnessText_);
    if (updateSlider && lv_slider_get_value(brightnessSlider_) != value) {
        suppressSliderEvent_ = true;
        lv_slider_set_value(brightnessSlider_, value, LV_ANIM_OFF);
        suppressSliderEvent_ = false;
    }
}

void SmartLightUi::renderTimer()
{
    const LightTimer timer = controller_->timer();
    if (!timer.active) {
        std::snprintf(timerText_, sizeof(timerText_), "Sin temporizador activo");
    } else {
        const std::uint32_t hours = timer.remainingSeconds / 3600;
        const std::uint32_t minutes = (timer.remainingSeconds % 3600 + 59) / 60;
        if (hours > 0) {
            std::snprintf(
                timerText_, sizeof(timerText_), "Quedan %lu h %02lu min | luego se %s",
                hours, minutes, timer.turnOn ? "ENCIENDE" : "APAGA"
            );
        } else {
            std::snprintf(
                timerText_, sizeof(timerText_), "Quedan %lu min | luego se %s",
                minutes, timer.turnOn ? "ENCIENDE" : "APAGA"
            );
        }
    }
    lv_label_set_text_static(timerStatusLabel_, timerText_);
    if (timer.active) {
        lv_obj_clear_flag(timerCancelButton_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(timerCancelButton_, LV_OBJ_FLAG_HIDDEN);
    }
    renderedTimer_ = timer;
}

void SmartLightUi::refresh()
{
    const LightState state = controller_->state();
    const bool firstRender = !hasRenderedState_;
    if (firstRender || state.power != renderedState_.power) {
        lv_label_set_text(stateLabel_, state.power ? "Encendida" : "Apagada");
        lv_label_set_text(powerButtonLabel_, state.power ? "APAGAR" : "ENCENDER");
        lv_obj_set_style_text_color(powerButtonLabel_, color(state.power ? kText : 0xFFFFFF), 0);
        lv_obj_set_style_bg_color(powerButton_, color(state.power ? kWarm : kText), 0);
        lv_obj_set_style_bg_color(powerButton_, color(state.power ? kWarmPressed : 0x3B3B37), LV_STATE_PRESSED);
    }
    if (!sliderActive_ && (firstRender || state.brightness != renderedState_.brightness)) {
        renderBrightness(state.brightness, true);
    }
    if (firstRender || state.connected != renderedState_.connected) {
        lv_label_set_text(connectionLabel_, state.connected ? "CONECTADO" : "SIN CONEXION");
        lv_obj_set_style_text_color(connectionLabel_, color(state.connected ? kGreen : kRed), 0);
    }
    if (firstRender || state.colorMode != renderedState_.colorMode || state.hue != renderedState_.hue) {
        int nearest = 0;
        if (state.colorMode) {
            int bestDistance = 361;
            for (int index = 1; index < 7; ++index) {
                const int linearDistance = std::abs(static_cast<int>(state.hue) - kColorHues[index]);
                const int distance = std::min(linearDistance, 360 - linearDistance);
                if (distance < bestDistance) {
                    bestDistance = distance;
                    nearest = index;
                }
            }
        }
        std::snprintf(colorText_, sizeof(colorText_), "Color actual: %s", kColorNames[nearest]);
        lv_label_set_text_static(colorStatusLabel_, colorText_);
    }
    const LightTimer currentTimer = controller_->timer();
    if (firstRender || activePage_ == 2
        || currentTimer.active != renderedTimer_.active
        || currentTimer.turnOn != renderedTimer_.turnOn) {
        renderTimer();
    }
    renderedState_ = state;
    hasRenderedState_ = true;
}
