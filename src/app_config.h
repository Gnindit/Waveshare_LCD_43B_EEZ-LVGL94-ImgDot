#pragma once

#include <stdint.h>

// ESP-NOW protocol and packet sizing.
inline constexpr uint8_t ESP_NOW_CHANNEL = 1;
inline constexpr uint32_t ESP_NOW_MAGIC = 0x574E5053;
inline constexpr uint8_t ESP_NOW_VERSION = 1;
inline constexpr uint8_t MAX_SENSORS = 24;
inline constexpr uint8_t MAX_SENSORS_ESP32_2 = 16;

// Shared packet payload used by both senders.
struct __attribute__((packed)) SensorSnapshot {
	uint8_t active;
	uint8_t valid;
	uint8_t hasCurrentMa;
	uint8_t valueMode;
	uint8_t slaveId;
	uint16_t registerAddress;
	uint16_t rawValue;
	float engineeringValue;
};

struct __attribute__((packed)) EspNowBroadcastPacket {
	uint32_t magic;
	uint8_t version;
	uint8_t sensorCount;
	uint16_t reserved;
	uint32_t uptimeMs;
	SensorSnapshot sensors[MAX_SENSORS];
};

struct __attribute__((packed)) EspNowBroadcastPacketEsp32_2 {
	uint32_t magic;
	uint8_t version;
	uint8_t sensorCount;
	uint16_t reserved;
	uint32_t uptimeMs;
	SensorSnapshot sensors[MAX_SENSORS_ESP32_2];
};

// Sender classification used in receiver/UI logic.
enum class SenderType : uint8_t {
	Unknown = 0,
	RS485 = 1,
	ESP32_2 = 2,
};

// I2C bus configuration for GT911 touch controller.
inline constexpr int I2C_MASTER_SCL_IO = 9;
inline constexpr int I2C_MASTER_SDA_IO = 8;
inline constexpr int I2C_MASTER_NUM = 0;
inline constexpr int I2C_MASTER_FREQ_HZ = 400000;
inline constexpr int I2C_MASTER_TX_BUF_DISABLE = 0;
inline constexpr int I2C_MASTER_RX_BUF_DISABLE = 0;
inline constexpr int I2C_MASTER_TIMEOUT_MS = 1000;

// Sensor indexes per sender packet format.
inline constexpr uint8_t RS485_SENSOR_WATER_PRESSURE = 0; // Input 1
inline constexpr uint8_t RS485_SENSOR_FUEL_PRESSURE = 1; // Input 2
// Spare 4-20 mA input 3
// Spare 4-20 mA input 4
inline constexpr uint8_t RS485_SENSOR_CYLINDER_LEFT_TEMP = 4; // Input 1
inline constexpr uint8_t RS485_SENSOR_CYLINDER_RIGHT_TEMP = 5; // Input 2
// Spare PT100 input 3
// Spare PT100 input 4
// Spare PT100 input 5
// Spare PT100 input 6
// Spare PT100 input 7
// Spare PT100 input 8

inline constexpr uint8_t RS485_SENSOR_EXHAUST_LEFT_TEMP = 12; // Input 1
inline constexpr uint8_t RS485_SENSOR_EXHAUST_RIGHT_TEMP = 13; // Input 2
// Spare K-type input 3
// Spare K-type input 4

// From ESP32_2 sender, the sensor indexes are:
inline constexpr uint8_t ESP32_2_SENSOR_FUEL_PERCENT = 0;
inline constexpr uint8_t ESP32_2_SENSOR_ENGINE_RPM = 1;
inline constexpr uint8_t ESP32_2_SENSOR_BUS_VOLTAGE = 2;
inline constexpr uint8_t ESP32_2_SENSOR_CURRENT_AMPS = 3;

// Plausible alternator/generator bus-voltage window for display updates.
inline constexpr float ESP32_2_BUS_VOLTAGE_MIN_V = 0.0f;
inline constexpr float ESP32_2_BUS_VOLTAGE_MAX_V = 40.0f;

// Physical ranges used for input validation.
inline constexpr float RS485_PRESSURE_MIN_BAR = -2.0f;
inline constexpr float RS485_PRESSURE_MAX_BAR = 8.0f;
inline constexpr float RS485_CHT_TEMP_MIN_C = -30.0f;
inline constexpr float RS485_CHT_TEMP_MAX_C = 200.0f;
inline constexpr float RS485_EGT_TEMP_MIN_C = -30.0f;
inline constexpr float RS485_EGT_TEMP_MAX_C = 1000.0f;

// Temperature band thresholds.
inline constexpr float CHT_COLOR_COOL_MAX_C = 61.0f;
inline constexpr float CHT_COLOR_NORMAL_MAX_C = 79.0f;
inline constexpr float CHT_COLOR_WARN_MAX_C = 82.0f;
inline constexpr float EGT_COLOR_COOL_MAX_C = 315.0f;
inline constexpr float EGT_COLOR_NORMAL_MAX_C = 676.0f;
inline constexpr float EGT_COLOR_WARN_MAX_C = 704.0f;

// Pressure band thresholds.
inline constexpr float FUEL_PRESSURE_COLOR_LOW_MAX_BAR = 0.1f;
inline constexpr float FUEL_PRESSURE_COLOR_LOW_MAX_WOT_BAR = 0.53f;
inline constexpr float FUEL_PRESSURE_COLOR_NORMAL_MAX_BAR = 0.8f;
inline constexpr float FUEL_PRESSURE_COLOR_WARN_MAX_BAR = 0.9f;
inline constexpr float WATER_PRESSURE_COLOR_LOW_MAX_BAR = 0.1f;
inline constexpr float WATER_PRESSURE_COLOR_LOW_MAX_WOT_BAR = 0.83f;
inline constexpr float WATER_PRESSURE_COLOR_NORMAL_MAX_BAR = 2.0f;
inline constexpr float WATER_PRESSURE_COLOR_WARN_MAX_BAR = 2.5f;

// UI gauge ranges/scales.
inline constexpr float FUEL_PRESSURE_UI_MAX_BAR = 1.0f;
inline constexpr float WATER_PRESSURE_UI_MAX_BAR = 3.0f;
inline constexpr int32_t PRESSURE_UI_VALUE_SCALE = 100;

// RPM and fuel color thresholds.
inline constexpr int RPM_COLOR_CAUTION_MIN_RPM = 5600;
inline constexpr int RPM_COLOR_WARN_MIN_RPM = 6000;
inline constexpr int RPM_UI_MAX_RPM = 7000;

// Fuel pressure's low-band (yellow) threshold ramps linearly between these RPM
// bounds instead of jumping, and unwinds the same way as RPM drops back down.
inline constexpr int FUEL_PRESSURE_LOW_BAND_RAMP_START_RPM = 2000;
inline constexpr int FUEL_PRESSURE_LOW_BAND_RAMP_END_RPM = 4000;

// Water pressure's low-band (yellow) threshold ramps the same way, with its
// own RPM bounds so it can be tuned independently of the fuel pressure ramp.
inline constexpr int WATER_PRESSURE_LOW_BAND_RAMP_START_RPM = 2000;
inline constexpr int WATER_PRESSURE_LOW_BAND_RAMP_END_RPM = 4000;

inline constexpr int FUEL_COLOR_WARN_MIN_PERCENT = 10;
inline constexpr int FUEL_COLOR_NORMAL_MIN_PERCENT = 20;

// UI target update intervals (ms). Temperatures use alternating half-interval
// windows so each CHT/EGT group updates every second.
// Fuel level and generator voltage share the same one-second update gate.
inline constexpr int32_t RS485_TEMP_UI_UPDATE_INTERVAL_MS = 1000;
inline constexpr int32_t RS485_PRESSURE_UI_UPDATE_INTERVAL_MS = 250;
inline constexpr int32_t ESP32_FUEL_UI_UPDATE_INTERVAL_MS = 1000;
inline constexpr int32_t ESP32_RPM_UI_UPDATE_INTERVAL_MS = 100;
inline constexpr int32_t ESP32_RPM_ARC_UI_UPDATE_INTERVAL_MS = 100;

// Shared UI palette (hex RGB) used by gauges/bars and temp band rendering.
// Common color reference (hex RGB):
// black 0x000000, white 0xFFFFFF, gray 0x808080
// red 0xFF0000, green 0x00FF00, blue 0x0000FF
// cyan 0x00FFFF, yellow 0xFFFF00, magenta 0xFF00FF
// orange 0xFFA500, orange-alt 0xFF8800, dark orange 0xFF8C00
// green (from figma) 0x009425
inline constexpr uint32_t UI_HEX_STATE_NOMINAL = 0x4040FF;
inline constexpr uint32_t UI_HEX_STATE_CAUTION = 0xFFFF00;
inline constexpr uint32_t UI_HEX_STATE_DANGER = 0xFF0000;


