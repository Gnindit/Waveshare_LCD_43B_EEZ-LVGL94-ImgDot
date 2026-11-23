#include "espnow_rx.h"
#include "ota_update.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

#include "app_config.h"
#include "ui_scale_helpers.h"
#include "rpm_indicator.h"
#include "ui/ui.h"

static portMUX_TYPE espnow_mux = portMUX_INITIALIZER_UNLOCKED;
static bool espnow_packet_ready = false;
static uint32_t last_rs485_packet_ms = 0;
static uint32_t last_rs485_temp_ui_update_ms = 0;
static uint8_t rs485_temp_update_phase = 0;
static uint32_t last_rs485_pressure_ui_update_ms = 0;
static uint32_t last_esp32_fuel_ui_update_ms = 0;
static uint32_t last_esp32_rpm_ui_update_ms = 0;
static uint32_t last_esp32_rpm_arc_ui_update_ms = 0;
static uint32_t last_esp32_packet_ms = 0;
static int last_esp32_engine_rpm = 0;
static bool last_rs485_water_pressure_valid = false;
static bool last_rs485_fuel_pressure_valid = false;
static float last_rs485_water_pressure_bar = 0.0f;
static float last_rs485_fuel_pressure_bar = 0.0f;
static EspNowBroadcastPacket espnow_last_packet = {};
static SenderType espnow_last_sender = SenderType::Unknown;
static const int32_t RS485_PACKET_TIMEOUT_MS = 3000;
static const int32_t ESP32_PACKET_TIMEOUT_MS = 3000;
static const uint32_t UI_HEX_TEXT_DEFAULT = 0xFFFFFF;

static void formatFixed2Parts(char *whole_buffer,
                              size_t whole_buffer_size,
                              char *fraction_buffer,
                              size_t fraction_buffer_size,
                              float value)
{
    const int32_t scaled = static_cast<int32_t>(value * 100.0f + (value >= 0.0f ? 0.5f : -0.5f));
    const int32_t whole = scaled / 100;
    const int32_t frac = scaled < 0 ? -scaled % 100 : scaled % 100;
    snprintf(whole_buffer, whole_buffer_size, "%ld", static_cast<long>(whole));
    snprintf(fraction_buffer, fraction_buffer_size, "%02ld", static_cast<long>(frac));
}

static void setLabelTextColor(lv_obj_t *label, uint32_t color_hex)
{
    if (label == NULL) {
        return;
    }

    lv_obj_set_style_text_color(label,
                                lv_color_hex(color_hex),
                                static_cast<lv_style_selector_t>(LV_PART_MAIN) |
                                    static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
}

static void setArcSignalVisible(lv_obj_t *arc, bool visible)
{
    if (arc == NULL) {
        return;
    }

    const lv_opa_t target_opa = visible ? LV_OPA_COVER : LV_OPA_TRANSP;
    lv_obj_set_style_opa(arc,
                         target_opa,
                         static_cast<lv_style_selector_t>(LV_PART_INDICATOR) |
                             static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
    lv_obj_set_style_opa(arc,
                         target_opa,
                         static_cast<lv_style_selector_t>(LV_PART_KNOB) |
                             static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
}

static void setSplitPressureLabels(lv_obj_t *whole_label, lv_obj_t *fraction_label, float value)
{
    if (whole_label == NULL || fraction_label == NULL) {
        return;
    }

    char whole_text[16];
    char fraction_text[16];
    formatFixed2Parts(whole_text, sizeof(whole_text), fraction_text, sizeof(fraction_text), value);
    lv_label_set_text(whole_label, whole_text);
    lv_label_set_text(fraction_label, fraction_text);
    setLabelTextColor(whole_label, UI_HEX_TEXT_DEFAULT);
    setLabelTextColor(fraction_label, UI_HEX_TEXT_DEFAULT);
}

static void setSplitPressurePlaceholders(lv_obj_t *whole_label, lv_obj_t *fraction_label)
{
    if (whole_label != NULL) {
        lv_label_set_text(whole_label, "-");
        setLabelTextColor(whole_label, UI_HEX_STATE_DANGER);
    }
    if (fraction_label != NULL) {
        lv_label_set_text(fraction_label, "--");
        setLabelTextColor(fraction_label, UI_HEX_STATE_DANGER);
    }
}

static int clampToArcRange(int value, lv_obj_t *arc)
{
    if (arc == NULL) {
        return value;
    }

    const int min_value = static_cast<int>(lv_arc_get_min_value(arc));
    const int max_value = static_cast<int>(lv_arc_get_max_value(arc));
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static bool hasValidSensorValue(const EspNowBroadcastPacket &packet, uint8_t index)
{
    return packet.sensorCount > index && packet.sensors[index].valid;
}

static bool isFiniteAndInRange(float value, float min_value, float max_value)
{
    return std::isfinite(value) && value >= min_value && value <= max_value;
}

static bool hasUsablePressureSensorValue(const EspNowBroadcastPacket &packet, uint8_t index)
{
    if (packet.sensorCount <= index) {
        return false;
    }

    const SensorSnapshot &sensor = packet.sensors[index];
    if (!sensor.valid) {
        return false;
    }

    return isFiniteAndInRange(sensor.engineeringValue,
                              RS485_PRESSURE_MIN_BAR,
                              RS485_PRESSURE_MAX_BAR);
}

static float clampPressureForUi(float pressure_value)
{
    return pressure_value < 0.0f ? 0.0f : pressure_value;
}

static bool hasUsableTemperatureSensorValue(const EspNowBroadcastPacket &packet, uint8_t index)
{
    if (packet.sensorCount <= index) {
        return false;
    }

    const SensorSnapshot &sensor = packet.sensors[index];
    const bool is_egt_channel = (index >= RS485_SENSOR_EXHAUST_LEFT_TEMP);
    const float min_c = is_egt_channel ? RS485_EGT_TEMP_MIN_C : RS485_CHT_TEMP_MIN_C;
    const float max_c = is_egt_channel ? RS485_EGT_TEMP_MAX_C : RS485_CHT_TEMP_MAX_C;

    if (!sensor.valid) {
        return false;
    }

    return isFiniteAndInRange(sensor.engineeringValue, min_c, max_c);
}

static bool hasUsableVoltageSensorValue(const EspNowBroadcastPacket &packet, uint8_t index)
{
    if (packet.sensorCount <= index) {
        return false;
    }

    const SensorSnapshot &sensor = packet.sensors[index];
    if (!sensor.valid) {
        return false;
    }

    return isFiniteAndInRange(sensor.engineeringValue,
                              ESP32_2_BUS_VOLTAGE_MIN_V,
                              ESP32_2_BUS_VOLTAGE_MAX_V);
}

static SenderType classifySender(const EspNowBroadcastPacket &packet)
{
    // The two senders share the same outer packet shape, so we use the sensor
    // metadata layout to distinguish RS485 snapshots from the compact ESP32_2 payload.
    if (packet.sensorCount == 0 || packet.sensorCount > MAX_SENSORS) {
        return SenderType::Unknown;
    }

    bool any_modbus_metadata = false;
    bool all_zero_slave_and_register = true;

    for (uint8_t i = 0; i < packet.sensorCount; ++i) {
        const SensorSnapshot &sensor = packet.sensors[i];
        if (sensor.slaveId > 0 || sensor.registerAddress != 0) {
            any_modbus_metadata = true;
            all_zero_slave_and_register = false;
            break;
        }
    }

    if (any_modbus_metadata) {
        return SenderType::RS485;
    }

    // ESP32_2 payloads currently send fuel/rpm/voltage/current (4 slots), and
    // older builds used 2 slots. Accept either shape as long as the snapshot
    // has no Modbus metadata.
    if (packet.sensorCount <= MAX_SENSORS_ESP32_2 && all_zero_slave_and_register) {
        return SenderType::ESP32_2;
    }

    return SenderType::Unknown;
}

template <typename PacketType>
static void copyPacketToCommon(const PacketType &source, EspNowBroadcastPacket &destination)
{
    destination = {};
    destination.magic = source.magic;
    destination.version = source.version;
    destination.sensorCount = source.sensorCount;
    destination.reserved = source.reserved;
    destination.uptimeMs = source.uptimeMs;

    const uint8_t copy_count = source.sensorCount < MAX_SENSORS ? source.sensorCount : MAX_SENSORS;
    for (uint8_t i = 0; i < copy_count; ++i) {
        destination.sensors[i] = source.sensors[i];
    }
}

static void setArcColorFromThresholds(lv_obj_t *arc,
                                      int value,
                                      int caution_threshold,
                                      int warn_threshold,
                                      bool higher_is_worse)
{
    if (arc == NULL) {
        return;
    }

    lv_color_t color = lv_color_hex(UI_HEX_STATE_NOMINAL);
    if (higher_is_worse) {
        if (value >= warn_threshold) {
            color = lv_color_hex(UI_HEX_STATE_DANGER);
        } else if (value >= caution_threshold) {
            color = lv_color_hex(UI_HEX_STATE_CAUTION);
        }
    } else {
        if (value <= warn_threshold) {
            color = lv_color_hex(UI_HEX_STATE_DANGER);
        } else if (value <= caution_threshold) {
            color = lv_color_hex(UI_HEX_STATE_CAUTION);
        }
    }

    lv_obj_set_style_arc_color(arc,
                               color,
                               static_cast<lv_style_selector_t>(LV_PART_INDICATOR) |
                                   static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
}

static void setPressureArcColorFromThresholds(lv_obj_t *arc,
                                              float value,
                                              float low_threshold,
                                              float normal_threshold,
                                              float warn_threshold)
{
    if (arc == NULL) {
        return;
    }

    lv_color_t color = lv_color_hex(UI_HEX_STATE_NOMINAL);
    if (value < low_threshold) {
        color = lv_color_hex(UI_HEX_STATE_CAUTION);
    } else if (value >= warn_threshold) {
        color = lv_color_hex(UI_HEX_STATE_DANGER);
    } else if (value >= normal_threshold) {
        color = lv_color_hex(UI_HEX_STATE_CAUTION);
    }

    lv_obj_set_style_arc_color(arc,
                               color,
                               static_cast<lv_style_selector_t>(LV_PART_INDICATOR) |
                                   static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
}

// Ramps linearly between the two thresholds as RPM crosses the given band,
// and unwinds the same way when RPM falls back down (no hysteresis).
static float rampThresholdForRpm(int rpm, int ramp_start_rpm, int ramp_end_rpm, float low_bar, float high_bar)
{
    if (rpm <= ramp_start_rpm) {
        return low_bar;
    }
    if (rpm >= ramp_end_rpm) {
        return high_bar;
    }

    const float ramp_span = static_cast<float>(ramp_end_rpm - ramp_start_rpm);
    const float t = static_cast<float>(rpm - ramp_start_rpm) / ramp_span;
    return low_bar + t * (high_bar - low_bar);
}

static float getFuelPressureLowThresholdForCurrentRpm()
{
    return rampThresholdForRpm(last_esp32_engine_rpm,
                               FUEL_PRESSURE_LOW_BAND_RAMP_START_RPM,
                               FUEL_PRESSURE_LOW_BAND_RAMP_END_RPM,
                               FUEL_PRESSURE_COLOR_LOW_MAX_BAR,
                               FUEL_PRESSURE_COLOR_LOW_MAX_WOT_BAR);
}

static float getWaterPressureLowThresholdForCurrentRpm()
{
    return rampThresholdForRpm(last_esp32_engine_rpm,
                               WATER_PRESSURE_LOW_BAND_RAMP_START_RPM,
                               WATER_PRESSURE_LOW_BAND_RAMP_END_RPM,
                               WATER_PRESSURE_COLOR_LOW_MAX_BAR,
                               WATER_PRESSURE_COLOR_LOW_MAX_WOT_BAR);
}

static void refreshPressureArcColorsFromLatestValues()
{
    if (last_rs485_water_pressure_valid && objects.main_arc_water_pressure != NULL) {
        setPressureArcColorFromThresholds(objects.main_arc_water_pressure,
                                          last_rs485_water_pressure_bar,
                                          getWaterPressureLowThresholdForCurrentRpm(),
                                          WATER_PRESSURE_COLOR_NORMAL_MAX_BAR,
                                          WATER_PRESSURE_COLOR_WARN_MAX_BAR);
    }

    if (last_rs485_fuel_pressure_valid && objects.main_arc_fuel_pressure != NULL) {
        setPressureArcColorFromThresholds(objects.main_arc_fuel_pressure,
                                          last_rs485_fuel_pressure_bar,
                                          getFuelPressureLowThresholdForCurrentRpm(),
                                          FUEL_PRESSURE_COLOR_NORMAL_MAX_BAR,
                                          FUEL_PRESSURE_COLOR_WARN_MAX_BAR);
    }
}

// Low-band thresholds change every RPM tick while ramping, so cache the
// last-applied value and skip the scale/arc refresh when it hasn't moved.
static void applyPressureLowThresholdIfChanged(float new_threshold,
                                               float &cached_threshold,
                                               void (*refresh_scale_band)(float))
{
    constexpr float kThresholdChangeEpsilon = 0.001f;
    if (std::fabs(new_threshold - cached_threshold) < kThresholdChangeEpsilon) {
        return;
    }
    cached_threshold = new_threshold;
    refresh_scale_band(new_threshold);
    refreshPressureArcColorsFromLatestValues();
}

static float g_last_applied_fuel_pressure_low_threshold = FUEL_PRESSURE_COLOR_LOW_MAX_BAR;
static float g_last_applied_water_pressure_low_threshold = WATER_PRESSURE_COLOR_LOW_MAX_BAR;

static void applyFuelPressureLowThresholdIfChanged()
{
    applyPressureLowThresholdIfChanged(getFuelPressureLowThresholdForCurrentRpm(),
                                       g_last_applied_fuel_pressure_low_threshold,
                                       refreshFuelPressureScaleBand);
}

static void applyWaterPressureLowThresholdIfChanged()
{
    applyPressureLowThresholdIfChanged(getWaterPressureLowThresholdForCurrentRpm(),
                                       g_last_applied_water_pressure_low_threshold,
                                       refreshWaterPressureScaleBand);
}

// Ny från google
static void onEspNowReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    (void)info;

    // RS485 sender now trims the payload to header + activeSensorCount entries (to
    // stay under ESP-NOW's 250-byte cap), so the length is variable rather than fixed.
    static const size_t kHeaderSize = offsetof(EspNowBroadcastPacket, sensors);
    if (len < static_cast<int>(kHeaderSize)) {
        return;
    }

    uint8_t sensorCount = 0;
    memcpy(&sensorCount, data + offsetof(EspNowBroadcastPacket, sensorCount), sizeof(sensorCount));

    const bool is_rs485_packet = sensorCount <= MAX_SENSORS &&
        len == static_cast<int>(kHeaderSize + sensorCount * sizeof(SensorSnapshot));
    const bool is_esp32_2_packet = !is_rs485_packet &&
        len == static_cast<int>(sizeof(EspNowBroadcastPacketEsp32_2));
    if (!is_rs485_packet && !is_esp32_2_packet) {
        return;
    }

    // Vi skapar bara den slutgiltiga strukturen direkt
    EspNowBroadcastPacket packet = {};

    if (is_rs485_packet) {
        memcpy(&packet, data, len);
    } else {
        // Genom att använda ett eget skop {} frigörs stacken för packet_esp32_2 
        // så fort kopieringen är klar.
        {
            EspNowBroadcastPacketEsp32_2 packet_esp32_2;
            memcpy(&packet_esp32_2, data, sizeof(packet_esp32_2));
            copyPacketToCommon(packet_esp32_2, packet);
        }
    }

    if (packet.magic != ESP_NOW_MAGIC || packet.version != ESP_NOW_VERSION) {
        return;
    }

    if (packet.sensorCount > MAX_SENSORS) {
        return;
    }

    // SÄKERT FÖR ISR: Byt till ISR-varianten av spinlåset
    portENTER_CRITICAL_ISR(&espnow_mux);
    espnow_last_packet = packet;
    espnow_last_sender = classifySender(packet);
    espnow_packet_ready = true;
    portEXIT_CRITICAL_ISR(&espnow_mux);
}


/* static void onEspNowReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    (void)info;

    const bool is_rs485_packet = len == static_cast<int>(sizeof(EspNowBroadcastPacket));
    const bool is_esp32_2_packet = len == static_cast<int>(sizeof(EspNowBroadcastPacketEsp32_2));
    if (!is_rs485_packet && !is_esp32_2_packet) {
        return;
    }

    EspNowBroadcastPacket packet = {};
    if (is_rs485_packet) {
        memcpy(&packet, data, sizeof(packet));
    } else {
        EspNowBroadcastPacketEsp32_2 packet_esp32_2 = {};
        memcpy(&packet_esp32_2, data, sizeof(packet_esp32_2));
        copyPacketToCommon(packet_esp32_2, packet);
    }

    if (packet.magic != ESP_NOW_MAGIC || packet.version != ESP_NOW_VERSION) {
        return;
    }

    if (packet.sensorCount > MAX_SENSORS) {
        return;
    }

    portENTER_CRITICAL(&espnow_mux);
    espnow_last_packet = packet;
    espnow_last_sender = classifySender(packet);
    espnow_packet_ready = true;
    portEXIT_CRITICAL(&espnow_mux);
}
 */

// Ny från google
void setupEspNowReceiver()
{
    // 1. Keep NVS (Required for Wi-Fi hardware calibration)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    // Prepare on-demand IP networking before starting the shared Wi-Fi radio.
    OtaUpdate::initializeNetwork();

    // 3. Force a slim buffer configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    
    // Reduce static/dynamic buffers to the absolute minimum needed for ESP-NOW
    cfg.static_rx_buf_num = 4;   // Default is often 10+
    cfg.dynamic_rx_buf_num = 6;  // Lowered from 16 to 6
    cfg.static_tx_buf_num = 2;
    cfg.dynamic_tx_buf_num = 4;  // Lowered from 8 to 4 (Since this is a receiver)
    cfg.cache_tx_buf_num = 1;

    // 4. Initialize and start the radio
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // Disable AMPDU (packet aggregation) to free up memory contexts
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE)); // Optional: keeps radio alert

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    
    // 5. Start ESP-NOW
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(onEspNowReceive));
}


/* void setupEspNowReceiver()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESP_NOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(onEspNowReceive));
} */

void updateUiFromEspNow()
{
    EspNowBroadcastPacket packet = {};
    SenderType sender_type = SenderType::Unknown;
    bool have_packet = false;

    portENTER_CRITICAL(&espnow_mux);
    if (espnow_packet_ready) {
        packet = espnow_last_packet;
        sender_type = espnow_last_sender;
        espnow_packet_ready = false;
        have_packet = true;
    }
    portEXIT_CRITICAL(&espnow_mux);

    const uint32_t now_ms = lv_tick_get();
    // Clear stale RS485 values so the UI does not keep showing the last valid
    // pressures and temperatures after the sender has gone silent.
    if (last_rs485_packet_ms > 0 && (now_ms - last_rs485_packet_ms) > RS485_PACKET_TIMEOUT_MS) {
        last_rs485_packet_ms = 0;
        last_rs485_water_pressure_valid = false;
        last_rs485_fuel_pressure_valid = false;
        setSplitPressurePlaceholders(objects.main_lbl_water_pressure_val_pre,
                                     objects.main_lbl_water_pressure_val_suf);
        setSplitPressurePlaceholders(objects.main_lbl_fuel_pressure_val_pre,
                                     objects.main_lbl_fuel_pressure_val_suf);
        setArcSignalVisible(objects.main_arc_water_pressure, false);
        setArcSignalVisible(objects.main_arc_fuel_pressure, false);
        resetSmoothedArc(SmoothedArcId::WaterPressure);
        resetSmoothedArc(SmoothedArcId::FuelPressure);
        if (objects.main_lbl_cyl_head_l_temp_val != NULL) {
            lv_label_set_text(objects.main_lbl_cyl_head_l_temp_val, "--");
            setLabelTextColor(objects.main_lbl_cyl_head_l_temp_val, UI_HEX_STATE_DANGER);
        }
        if (objects.main_lbl_cyl_head_r_temp_val != NULL) {
            lv_label_set_text(objects.main_lbl_cyl_head_r_temp_val, "--");
            setLabelTextColor(objects.main_lbl_cyl_head_r_temp_val, UI_HEX_STATE_DANGER);
        }
        if (objects.main_lbl_exhaust_l_temp_val != NULL) {
            lv_label_set_text(objects.main_lbl_exhaust_l_temp_val, "---");
            setLabelTextColor(objects.main_lbl_exhaust_l_temp_val, UI_HEX_STATE_DANGER);
        }
        if (objects.main_lbl_exhaust_r_temp_val != NULL) {
            lv_label_set_text(objects.main_lbl_exhaust_r_temp_val, "---");
            setLabelTextColor(objects.main_lbl_exhaust_r_temp_val, UI_HEX_STATE_DANGER);
        }
        setArcSignalVisible(objects.main_arc_cyl_head_l_temp, false);
        setArcSignalVisible(objects.main_arc_cyl_head_r_temp, false);
        setArcSignalVisible(objects.main_arc_exhaust_l_temp, false);
        setArcSignalVisible(objects.main_arc_exhaust_r_temp, false);
        resetSmoothedArc(SmoothedArcId::CylHeadLTemp);
        resetSmoothedArc(SmoothedArcId::CylHeadRTemp);
        resetSmoothedArc(SmoothedArcId::ExhaustLTemp);
        resetSmoothedArc(SmoothedArcId::ExhaustRTemp);
    }

    // Clear stale ESP32 values so the UI does not keep showing the last valid
    // RPM/fuel labels after the sender has gone silent.
    if (last_esp32_packet_ms > 0 && (now_ms - last_esp32_packet_ms) > ESP32_PACKET_TIMEOUT_MS) {
        last_esp32_packet_ms = 0;
        last_esp32_rpm_arc_ui_update_ms = 0;
        last_esp32_engine_rpm = 0;
        applyFuelPressureLowThresholdIfChanged();
        applyWaterPressureLowThresholdIfChanged();
        if (objects.main_lbl_fuel_level_val != NULL) {
            lv_label_set_text(objects.main_lbl_fuel_level_val, "--");
            setLabelTextColor(objects.main_lbl_fuel_level_val, UI_HEX_STATE_DANGER);
        }
        clearEngineRpmIndicator();
        if (objects.main_lbl_generator_voltage_val != NULL) {
            lv_label_set_text(objects.main_lbl_generator_voltage_val, "--,-");
            setLabelTextColor(objects.main_lbl_generator_voltage_val, UI_HEX_STATE_DANGER);
        }
        setArcSignalVisible(objects.main_arc_fuel_level, false);
        setArcSignalVisible(objects.main_arc_engine_rpm, false);
        resetSmoothedArc(SmoothedArcId::FuelLevel);
    }

    if (!have_packet) {
        return;
    }

    if (sender_type == SenderType::RS485) {
        last_rs485_packet_ms = now_ms;

        if (last_rs485_pressure_ui_update_ms == 0 ||
            (now_ms - last_rs485_pressure_ui_update_ms) >= RS485_PRESSURE_UI_UPDATE_INTERVAL_MS) {
            last_rs485_pressure_ui_update_ms = now_ms;

            if (hasUsablePressureSensorValue(packet, RS485_SENSOR_WATER_PRESSURE)) {
                const float pressure_value = clampPressureForUi(packet.sensors[RS485_SENSOR_WATER_PRESSURE].engineeringValue);
                last_rs485_water_pressure_bar = pressure_value;
                last_rs485_water_pressure_valid = true;
                setSplitPressureLabels(objects.main_lbl_water_pressure_val_pre,
                                       objects.main_lbl_water_pressure_val_suf,
                                       pressure_value);
                if (objects.main_arc_water_pressure != NULL) {
                    setArcSignalVisible(objects.main_arc_water_pressure, true);
                    const int water_pressure_ui_value = static_cast<int>(std::lround(pressure_value * 100.0f));
                    setSmoothedArcTarget(SmoothedArcId::WaterPressure, objects.main_arc_water_pressure, water_pressure_ui_value);
                    setPressureArcColorFromThresholds(objects.main_arc_water_pressure,
                                                       pressure_value,
                                                       getWaterPressureLowThresholdForCurrentRpm(),
                                                       WATER_PRESSURE_COLOR_NORMAL_MAX_BAR,
                                                       WATER_PRESSURE_COLOR_WARN_MAX_BAR);
                }
            }

            if (hasUsablePressureSensorValue(packet, RS485_SENSOR_FUEL_PRESSURE)) {
                const float pressure_value = clampPressureForUi(packet.sensors[RS485_SENSOR_FUEL_PRESSURE].engineeringValue);
                last_rs485_fuel_pressure_bar = pressure_value;
                last_rs485_fuel_pressure_valid = true;
                setSplitPressureLabels(objects.main_lbl_fuel_pressure_val_pre,
                                       objects.main_lbl_fuel_pressure_val_suf,
                                       pressure_value);
                if (objects.main_arc_fuel_pressure != NULL) {
                    setArcSignalVisible(objects.main_arc_fuel_pressure, true);
                    const int fuel_pressure_ui_value = static_cast<int>(std::lround(pressure_value * 100.0f));
                    setSmoothedArcTarget(SmoothedArcId::FuelPressure, objects.main_arc_fuel_pressure, fuel_pressure_ui_value);
                    setPressureArcColorFromThresholds(objects.main_arc_fuel_pressure,
                                                      pressure_value,
                                                      getFuelPressureLowThresholdForCurrentRpm(),
                                                      FUEL_PRESSURE_COLOR_NORMAL_MAX_BAR,
                                                      FUEL_PRESSURE_COLOR_WARN_MAX_BAR);
                }
            }
        }

        if (last_rs485_temp_ui_update_ms == 0 ||
            (now_ms - last_rs485_temp_ui_update_ms) >= (RS485_TEMP_UI_UPDATE_INTERVAL_MS / 2)) {
            last_rs485_temp_ui_update_ms = now_ms;

            // Alternate between CHT and EGT groups to spread the more expensive
            // LVGL text/arc updates across 500ms windows: each group updates every 1s.
            const bool update_cht_group = (rs485_temp_update_phase == 0);
            rs485_temp_update_phase ^= 1U;

            if (update_cht_group) {
                if (objects.main_lbl_cyl_head_l_temp_val != NULL) {
                    if (hasUsableTemperatureSensorValue(packet, RS485_SENSOR_CYLINDER_LEFT_TEMP)) {
                        const int temp = static_cast<int>(packet.sensors[RS485_SENSOR_CYLINDER_LEFT_TEMP].engineeringValue + 0.5f);
                        char text[16];
                        snprintf(text, sizeof(text), "%d", temp);
                        lv_label_set_text(objects.main_lbl_cyl_head_l_temp_val, text);
                        setLabelTextColor(objects.main_lbl_cyl_head_l_temp_val, UI_HEX_TEXT_DEFAULT);
                        if (objects.main_arc_cyl_head_l_temp != NULL) {
                            setArcSignalVisible(objects.main_arc_cyl_head_l_temp, true);
                            setSmoothedArcTarget(SmoothedArcId::CylHeadLTemp,
                                                objects.main_arc_cyl_head_l_temp,
                                                clampToArcRange(temp, objects.main_arc_cyl_head_l_temp));
                        }
                    }
                }

                if (objects.main_lbl_cyl_head_r_temp_val != NULL) {
                    if (hasUsableTemperatureSensorValue(packet, RS485_SENSOR_CYLINDER_RIGHT_TEMP)) {
                        const int temp = static_cast<int>(packet.sensors[RS485_SENSOR_CYLINDER_RIGHT_TEMP].engineeringValue + 0.5f);
                        char text[16];
                        snprintf(text, sizeof(text), "%d", temp);
                        lv_label_set_text(objects.main_lbl_cyl_head_r_temp_val, text);
                        setLabelTextColor(objects.main_lbl_cyl_head_r_temp_val, UI_HEX_TEXT_DEFAULT);
                        if (objects.main_arc_cyl_head_r_temp != NULL) {
                            setArcSignalVisible(objects.main_arc_cyl_head_r_temp, true);
                            setSmoothedArcTarget(SmoothedArcId::CylHeadRTemp,
                                                objects.main_arc_cyl_head_r_temp,
                                                clampToArcRange(temp, objects.main_arc_cyl_head_r_temp));
                        }
                    }
                }
            } else {
                if (objects.main_lbl_exhaust_l_temp_val != NULL) {
                    if (hasUsableTemperatureSensorValue(packet, RS485_SENSOR_EXHAUST_LEFT_TEMP)) {
                        const int temp = static_cast<int>(packet.sensors[RS485_SENSOR_EXHAUST_LEFT_TEMP].engineeringValue + 0.5f);
                        char text[16];
                        snprintf(text, sizeof(text), "%d", temp);
                        lv_label_set_text(objects.main_lbl_exhaust_l_temp_val, text);
                        setLabelTextColor(objects.main_lbl_exhaust_l_temp_val, UI_HEX_TEXT_DEFAULT);
                        if (objects.main_arc_exhaust_l_temp != NULL) {
                            setArcSignalVisible(objects.main_arc_exhaust_l_temp, true);
                            setSmoothedArcTarget(SmoothedArcId::ExhaustLTemp,
                                                objects.main_arc_exhaust_l_temp,
                                                clampToArcRange(temp, objects.main_arc_exhaust_l_temp));
                        }
                    }
                }

                if (objects.main_lbl_exhaust_r_temp_val != NULL) {
                    if (hasUsableTemperatureSensorValue(packet, RS485_SENSOR_EXHAUST_RIGHT_TEMP)) {
                        const int temp = static_cast<int>(packet.sensors[RS485_SENSOR_EXHAUST_RIGHT_TEMP].engineeringValue + 0.5f);
                        char text[16];
                        snprintf(text, sizeof(text), "%d", temp);
                        lv_label_set_text(objects.main_lbl_exhaust_r_temp_val, text);
                        setLabelTextColor(objects.main_lbl_exhaust_r_temp_val, UI_HEX_TEXT_DEFAULT);
                        if (objects.main_arc_exhaust_r_temp != NULL) {
                            setArcSignalVisible(objects.main_arc_exhaust_r_temp, true);
                            setSmoothedArcTarget(SmoothedArcId::ExhaustRTemp,
                                                objects.main_arc_exhaust_r_temp,
                                                clampToArcRange(temp, objects.main_arc_exhaust_r_temp));
                        }
                    }
                }
            }
        }
    } else if (sender_type == SenderType::ESP32_2) {
        last_esp32_packet_ms = now_ms;

        if (last_esp32_fuel_ui_update_ms == 0 ||
            (now_ms - last_esp32_fuel_ui_update_ms) >= ESP32_FUEL_UI_UPDATE_INTERVAL_MS) {
            last_esp32_fuel_ui_update_ms = now_ms;
            if (objects.main_lbl_fuel_level_val != NULL) {
                if (hasValidSensorValue(packet, ESP32_2_SENSOR_FUEL_PERCENT)) {
                    const int fuel = static_cast<int>(packet.sensors[ESP32_2_SENSOR_FUEL_PERCENT].engineeringValue + 0.5f);
                    char text[16];
                    snprintf(text, sizeof(text), "%02d", fuel);
                    lv_label_set_text(objects.main_lbl_fuel_level_val, text);
                    setLabelTextColor(objects.main_lbl_fuel_level_val, UI_HEX_TEXT_DEFAULT);
                    if (objects.main_arc_fuel_level != NULL) {
                        setArcSignalVisible(objects.main_arc_fuel_level, true);
                        const int fuel_clamped = fuel < 0 ? 0 : (fuel > 100 ? 100 : fuel);
                        setSmoothedArcTarget(SmoothedArcId::FuelLevel, objects.main_arc_fuel_level, fuel_clamped);
                        setArcColorFromThresholds(objects.main_arc_fuel_level,
                                                   fuel_clamped,
                                                   FUEL_COLOR_NORMAL_MIN_PERCENT,
                                                   FUEL_COLOR_WARN_MIN_PERCENT,
                                                   false);
                    }
                }
            }

            if (objects.main_lbl_generator_voltage_val != NULL) {
                if (hasUsableVoltageSensorValue(packet, ESP32_2_SENSOR_BUS_VOLTAGE)) {
                    char text[16];
                    const float bus_voltage = packet.sensors[ESP32_2_SENSOR_BUS_VOLTAGE].engineeringValue;
                    snprintf(text, sizeof(text), "%.1f", bus_voltage);
                    for (char *cursor = text; *cursor != '\0'; ++cursor) {
                        if (*cursor == '.') {
                            *cursor = ',';
                            break;
                        }
                    }
                    lv_label_set_text(objects.main_lbl_generator_voltage_val, text);
                    setLabelTextColor(objects.main_lbl_generator_voltage_val, UI_HEX_TEXT_DEFAULT);
                }
            }
        }

        if (last_esp32_rpm_ui_update_ms == 0 ||
            (now_ms - last_esp32_rpm_ui_update_ms) >= ESP32_RPM_UI_UPDATE_INTERVAL_MS) {
            last_esp32_rpm_ui_update_ms = now_ms;
            if (objects.main_lbl_engine_rpm_val != NULL) {
                if (hasValidSensorValue(packet, ESP32_2_SENSOR_ENGINE_RPM)) {
                    const int rpm = static_cast<int>(packet.sensors[ESP32_2_SENSOR_ENGINE_RPM].engineeringValue + 0.5f);
                    last_esp32_engine_rpm = rpm;
                    applyFuelPressureLowThresholdIfChanged();
                    applyWaterPressureLowThresholdIfChanged();
                    updateEngineRpmIndicator(rpm);

                    if ((last_esp32_rpm_arc_ui_update_ms == 0 ||
                         (now_ms - last_esp32_rpm_arc_ui_update_ms) >= ESP32_RPM_ARC_UI_UPDATE_INTERVAL_MS) &&
                        objects.main_arc_engine_rpm != NULL) {
                        last_esp32_rpm_arc_ui_update_ms = now_ms;
                        setArcSignalVisible(objects.main_arc_engine_rpm, true);
                        const int rpm_clamped = rpm < 0 ? 0 : (rpm > RPM_UI_MAX_RPM ? RPM_UI_MAX_RPM : rpm);
                        //lv_arc_set_range(objects.main_arc_engine_rpm, 0, RPM_UI_MAX_RPM);
                        lv_arc_set_value(objects.main_arc_engine_rpm, rpm_clamped);
                        setArcColorFromThresholds(objects.main_arc_engine_rpm,
                                                   rpm_clamped,
                                                   RPM_COLOR_CAUTION_MIN_RPM,
                                                   RPM_COLOR_WARN_MIN_RPM,
                                                   true);
                    }
                } else {
                    clearEngineRpmIndicator();
                    last_esp32_engine_rpm = 0;
                    applyFuelPressureLowThresholdIfChanged();
                    applyWaterPressureLowThresholdIfChanged();
                }
            }
        }
    }
}
