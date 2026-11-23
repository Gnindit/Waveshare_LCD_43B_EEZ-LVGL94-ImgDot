#include "ota_update.h"
#include "ota_service.h"
#include "ota_policy.h"
#include "ui/ui.h"

#include <cstdio>
#include <cstring>
#include <esp_app_desc.h>
#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <lvgl.h>

#include "app_config.h"

namespace {
lv_obj_t *overlay, *panel, *message, *available, *progressBar;
lv_obj_t *checkButton, *installButton, *wifiButton, *closeButton;
lv_obj_t *ssidInput, *passwordInput, *keyboard;
bool editingWifi = false;
bool confirming = false;
OtaPolicy::HoldGesture holdGesture;
uint32_t displayedRevision = UINT32_MAX;
int64_t healthySince = 0;
unsigned healthyTicks = 0;
bool pendingVerification = false;

lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, int width) {
    auto *object = lv_label_create(parent);
    lv_label_set_text(object, text);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_width(object, width);
    return object;
}

lv_obj_t *button(lv_obj_t *parent, const char *text, int x, int y, int width, lv_event_cb_t callback) {
    auto *object = lv_button_create(parent);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, 52);
    auto *caption = lv_label_create(object);
    lv_label_set_text(caption, text);
    lv_obj_center(caption);
    lv_obj_add_event_cb(object, callback, LV_EVENT_CLICKED, nullptr);
    return object;
}

void setEnabled(lv_obj_t *object, bool enabled) {
    if (enabled) lv_obj_remove_state(object, LV_STATE_DISABLED);
    else lv_obj_add_state(object, LV_STATE_DISABLED);
}

void showMenu();

void closeMenu(lv_event_t *) {
    if (OtaService::busy(OtaService::snapshot().phase)) return;
    // Deletion also frees Wi-Fi/password text and keyboard widgets.
    lv_obj_delete(overlay);
    overlay = panel = nullptr;
    editingWifi = false;
    confirming = false;
}

void check(lv_event_t *) { confirming = false; OtaService::requestCheck(); }
void install(lv_event_t *) {
    if (!confirming) {
        confirming = true;
        lv_label_set_text(message, "Stop the engine before updating. Keep power connected.\nInstall this release and restart the display?");
        lv_label_set_text(lv_obj_get_child(installButton, 0), "Confirm install");
        setEnabled(checkButton, false);
        setEnabled(wifiButton, false);
        return;
    }
    confirming = false;
    OtaService::requestInstall();
}

void focusInput(lv_event_t *event) {
    lv_keyboard_set_textarea(keyboard, static_cast<lv_obj_t *>(lv_event_get_target(event)));
}

void cancelWifi(lv_event_t *) { showMenu(); }
void saveWifi(lv_event_t *) {
    OtaService::Credentials credentials;
    const char *ssid = lv_textarea_get_text(ssidInput);
    const char *password = lv_textarea_get_text(passwordInput);
    const auto ssidLength = strlen(ssid), passwordLength = strlen(password);
    if (ssidLength == 0 || ssidLength > 32 || (passwordLength && (passwordLength < 8 || passwordLength > 63))) {
        lv_label_set_text(message, "SSID: 1-32 bytes. Password: 8-63 bytes, or empty for an open network.");
        return;
    }
    strlcpy(credentials.ssid, ssid, sizeof(credentials.ssid));
    strlcpy(credentials.password, password, sizeof(credentials.password));
    const bool saved = OtaService::saveCredentials(credentials);
    memset(&credentials, 0, sizeof(credentials));
    if (saved) showMenu();
    else lv_label_set_text(message, "Could not save Wi-Fi settings. Try again.");
}

void editWifi(lv_event_t *) {
    if (OtaService::busy(OtaService::snapshot().phase)) return;
    editingWifi = true;
    confirming = false;
    lv_obj_clean(panel);
    message = label(panel, "Same router as sensors: 2.4 GHz, fixed channel 1.", 8, 0, 710);
    label(panel, "Network", 8, 48, 105);
    ssidInput = lv_textarea_create(panel);
    lv_textarea_set_one_line(ssidInput, true);
    lv_textarea_set_max_length(ssidInput, 32);
    lv_obj_set_pos(ssidInput, 125, 37);
    lv_obj_set_size(ssidInput, 420, 48);
    label(panel, "Password", 8, 105, 115);
    passwordInput = lv_textarea_create(panel);
    lv_textarea_set_one_line(passwordInput, true);
    lv_textarea_set_max_length(passwordInput, 63);
    lv_textarea_set_password_mode(passwordInput, true);
    lv_textarea_set_password_show_time(passwordInput, 0);
    lv_obj_set_pos(passwordInput, 125, 94);
    lv_obj_set_size(passwordInput, 420, 48);
    OtaService::Credentials credentials;
    if (OtaService::loadCredentials(credentials)) {
        lv_textarea_set_text(ssidInput, credentials.ssid);
        lv_textarea_set_text(passwordInput, credentials.password);
    }
    memset(&credentials, 0, sizeof(credentials));
    button(panel, "Save", 560, 37, 145, saveWifi);
    button(panel, "Cancel", 560, 98, 145, cancelWifi);
    keyboard = lv_keyboard_create(panel);
    // Keyboards default to bottom-center alignment. Use top-left coordinates
    // here so y=160 positions the top edge instead of shifting it below the panel.
    lv_obj_set_align(keyboard, LV_ALIGN_TOP_LEFT);
    lv_obj_set_size(keyboard, 720, 240);
    lv_obj_set_pos(keyboard, 0, 160);
    lv_keyboard_set_textarea(keyboard, ssidInput);
    lv_obj_add_event_cb(ssidInput, focusInput, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(passwordInput, focusInput, LV_EVENT_FOCUSED, nullptr);
}

void showMenu() {
    if (!overlay) {
        overlay = lv_obj_create(lv_layer_top());
        lv_obj_set_size(overlay, 800, 480);
        lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
        lv_obj_set_style_bg_opa(overlay, LV_OPA_70, 0);
        lv_obj_set_style_border_width(overlay, 0, 0);
        lv_obj_set_style_pad_all(overlay, 0, 0);
        lv_obj_set_style_radius(overlay, 0, 0);
        lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
        panel = lv_obj_create(overlay);
        lv_obj_set_size(panel, 760, 440);
        lv_obj_center(panel);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x182331), 0);
        lv_obj_set_style_text_color(panel, lv_color_white(), 0);
        lv_obj_set_style_text_font(panel, &lv_font_montserrat_20, 0);
        lv_obj_set_style_pad_all(panel, 16, 0);
        lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_clean(panel);
    editingWifi = false;
    confirming = false;
    char versionText[100];
    snprintf(versionText, sizeof(versionText), "Software update\nInstalled: %s", esp_app_get_description()->version);
    label(panel, versionText, 8, 0, 700);
    available = label(panel, "", 8, 63, 700);
    message = label(panel, "", 8, 104, 700);
    progressBar = lv_bar_create(panel);
    lv_obj_set_pos(progressBar, 8, 240);
    lv_obj_set_size(progressBar, 695, 18);
    lv_bar_set_range(progressBar, 0, 100);
    checkButton = button(panel, "Check for updates", 8, 283, 285, check);
    installButton = button(panel, "Install and restart", 310, 283, 390, install);
    wifiButton = button(panel, "Wi-Fi setup", 8, 348, 285, editWifi);
    closeButton = button(panel, "Close", 510, 348, 190, closeMenu);
    displayedRevision = UINT32_MAX;
}

void onUpdateButton(lv_event_t *event) {
    switch (lv_event_get_code(event)) {
    case LV_EVENT_PRESSED:
        holdGesture.press(lv_tick_get());
        break;
    case LV_EVENT_PRESS_LOST:
        holdGesture.cancel();
        break;
    case LV_EVENT_RELEASED: {
        const bool open = holdGesture.release(lv_tick_get());
        // Open on release so the original held touch cannot click a popup action.
        if (open && !overlay) showMenu();
        break;
    }
    default: break;
    }
}

void verifyBoot() {
    if (!pendingVerification) return;
    ++healthyTicks;
    if (esp_timer_get_time() - healthySince < 15000000 || healthyTicks < 100) return;
    // Display/touch initialization and ESP-NOW initialization have succeeded, and
    // the UI loop has kept running. No sender/router is required for boot health.
    uint8_t channel;
    wifi_second_chan_t secondary;
    if (!objects.main_but_update || !lv_display_get_default() ||
        esp_wifi_get_channel(&channel, &secondary) != ESP_OK || channel != ESP_NOW_CHANNEL) return;
    const auto result = esp_ota_mark_app_valid_cancel_rollback();
    if (result == ESP_OK) {
        pendingVerification = false;
        ESP_LOGI("OTA", "Updated application passed startup checks and was confirmed.");
    } else {
        ESP_LOGE("OTA", "Cannot confirm updated application: %s", esp_err_to_name(result));
    }
}
} // namespace

namespace OtaUpdate {
void initializeNetwork() { OtaService::initializeNetwork(); }
void begin() {
    if (objects.main_but_update) {
        lv_obj_add_event_cb(objects.main_but_update, onUpdateButton, LV_EVENT_ALL, nullptr);
    }
    esp_ota_img_states_t state;
    pendingVerification = esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
                          state == ESP_OTA_IMG_PENDING_VERIFY;
    healthySince = esp_timer_get_time();
    ESP_LOGI("OTA", "Software %s; Wi-Fi connects only during update operations.", esp_app_get_description()->version);
}
void tick() {
    verifyBoot();
    if (!overlay || editingWifi) return;
    const auto state = OtaService::snapshot();
    if (state.revision == displayedRevision) return;
    displayedRevision = state.revision;
    confirming = false;
    lv_label_set_text(message, state.message);
    char versionText[96] = {};
    if (state.version[0]) snprintf(versionText, sizeof(versionText), "Release: %s", state.version);
    lv_label_set_text(available, versionText);
    lv_bar_set_value(progressBar, state.progress, LV_ANIM_OFF);
    const bool active = OtaService::busy(state.phase);
    setEnabled(checkButton, !active);
    setEnabled(wifiButton, !active);
    setEnabled(closeButton, !active);
    setEnabled(installButton, state.phase == OtaService::Phase::Available);
    lv_label_set_text(lv_obj_get_child(installButton, 0), "Install and restart");
}
} // namespace OtaUpdate
