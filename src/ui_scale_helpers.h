#pragma once

#include <lvgl.h>

void configureScaleBandsFromThresholds();
void syncArcRangesFromScales();
void applyAllScaleLabels();
void refreshFuelPressureScaleBand(float fuel_low_threshold);
void refreshWaterPressureScaleBand(float water_low_threshold);

// Geometry of the main_img_engine_rpm dot's travel circle, matching
// main_sca_engine_rpm's rotation(135)/angle_range(270) from screens.c.
inline constexpr int RPM_DOT_LUT_COUNT = 200;
inline constexpr int RPM_DOT_START_ANGLE_DEG = 135;
inline constexpr int RPM_DOT_SWEEP_ANGLE_DEG = 270;
inline constexpr int RPM_DOT_CENTER_X = 198;
inline constexpr int RPM_DOT_CENTER_Y = 200;
inline constexpr int RPM_DOT_RADIUS = 175; // tune this after visual testing on the real display
inline constexpr int RPM_DOT_HALF_SIZE = 12; // half of img_rpm_dot's 24x24 size

// Smoothing of the dot's on-screen travel toward the latest received RPM.
inline constexpr uint32_t RPM_DOT_ANIM_PERIOD_MS = 20;
inline constexpr float RPM_DOT_ANIM_SMOOTHING = 0.15f;
inline constexpr float RPM_DOT_ANIM_SNAP_THRESHOLD_RPM = 1.0f;

void initEngineRpmDotAnimation();
void setEngineRpmDotTarget(int rpm);
void resetEngineRpmDot();

// Shared smoothing for Arc widgets that are otherwise set directly from raw
// sensor reads (pressures, temperatures, fuel level). Mirrors the RPM dot's
// target/displayed exponential-smoothing approach but with one timer for all
// of them instead of one timer per arc.
enum class SmoothedArcId : uint8_t {
    WaterPressure,
    FuelPressure,
    CylHeadLTemp,
    CylHeadRTemp,
    ExhaustLTemp,
    ExhaustRTemp,
    FuelLevel,
    Count
};

inline constexpr uint32_t ARC_SMOOTHING_ANIM_PERIOD_MS = 20;
// Pressures update every 250 ms, so a brisker factor keeps up without lagging.
inline constexpr float ARC_SMOOTHING_ANIM_FACTOR_FAST = 0.15f;
// Temperature/fuel arcs reach a new target in one second, then remain still.
inline constexpr uint32_t SLOW_GAUGE_TRANSITION_MS = 1000;
inline constexpr float ARC_SMOOTHING_ANIM_SNAP_THRESHOLD = 0.5f;

void initArcSmoothingAnimation();
void setSmoothedArcTarget(SmoothedArcId id, lv_obj_t *arc, int32_t value);
void resetSmoothedArc(SmoothedArcId id);
