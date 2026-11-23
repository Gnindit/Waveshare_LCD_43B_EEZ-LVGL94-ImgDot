#include "ui_scale_helpers.h"
#include "gauge_smoothing_math.h"

#include <array>
#include <cmath>
#include <cstdio>

#include <esp_log.h>

#include "app_config.h"
#include "rpm_indicator.h"
#include "ui/ui.h"

static const char *TAG = "UI_SCALE_HELPERS";

static lv_style_t scale_style_caution;
static lv_style_t scale_style_nominal;
static lv_style_t scale_style_danger;
static bool scale_section_styles_ready = false;

struct PressureScaleSections {
    lv_obj_t *scale;
    lv_scale_section_t *low_section;
    lv_scale_section_t *normal_section;
    lv_scale_section_t *warn_section;
    lv_scale_section_t *danger_section;
    float normal_threshold;
    float warn_threshold;
    float value_max_for_scale;
};

static PressureScaleSections g_fuel_pressure_sections = {};
static PressureScaleSections g_water_pressure_sections = {};

static void ensureScaleSectionStyles()
{
    if (scale_section_styles_ready) {
        return;
    }

    lv_style_init(&scale_style_caution);
    lv_style_set_line_color(&scale_style_caution, lv_color_hex(UI_HEX_STATE_CAUTION));
    lv_style_set_line_width(&scale_style_caution, 4U);
    lv_style_set_arc_color(&scale_style_caution, lv_color_hex(UI_HEX_STATE_CAUTION));
    lv_style_set_arc_width(&scale_style_caution, 6U);
    lv_style_set_arc_opa(&scale_style_caution, 255U);

    lv_style_init(&scale_style_nominal);
    lv_style_set_line_color(&scale_style_nominal, lv_color_hex(UI_HEX_STATE_NOMINAL));
    lv_style_set_line_width(&scale_style_nominal, 4U);
    lv_style_set_arc_color(&scale_style_nominal, lv_color_hex(UI_HEX_STATE_NOMINAL));
    lv_style_set_arc_width(&scale_style_nominal, 6U);
    lv_style_set_arc_opa(&scale_style_nominal, 255U);

    lv_style_init(&scale_style_danger);
    lv_style_set_line_color(&scale_style_danger, lv_color_hex(UI_HEX_STATE_DANGER));
    lv_style_set_line_width(&scale_style_danger, 4U);
    lv_style_set_arc_color(&scale_style_danger, lv_color_hex(UI_HEX_STATE_DANGER));
    lv_style_set_arc_width(&scale_style_danger, 6U);
    lv_style_set_arc_opa(&scale_style_danger, 255U);

    scale_section_styles_ready = true;
}

static int32_t clampI32ToRange(int32_t value, int32_t min_value, int32_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static int32_t thresholdToScaleValue(float threshold, int32_t min_value, int32_t max_value)
{
    const int32_t rounded = static_cast<int32_t>(threshold + (threshold >= 0.0f ? 0.5f : -0.5f));
    return clampI32ToRange(rounded, min_value, max_value);
}

static int32_t mapValueToScaleRange(float value, lv_obj_t *scale, float value_max_for_scale)
{
    // Thresholds are defined in engineering units, but LVGL scales operate on
    // integer widget ranges that vary by gauge, so convert before applying bands.
    if (scale == NULL) {
        return static_cast<int32_t>(std::lround(value));
    }

    const int32_t min_value = lv_scale_get_range_min_value(scale);
    const int32_t max_value = lv_scale_get_range_max_value(scale);
    if (value_max_for_scale <= 0.0f) {
        return clampI32ToRange(static_cast<int32_t>(std::lround(value)), min_value, max_value);
    }

    const float span = static_cast<float>(max_value - min_value);
    const float normalized = value / value_max_for_scale;
    const int32_t mapped = static_cast<int32_t>(std::lround(min_value + normalized * span));
    return clampI32ToRange(mapped, min_value, max_value);
}

static int32_t pressureUiToLabelValue(int32_t value)
{
    return static_cast<int32_t>(std::lround(static_cast<float>(value) / static_cast<float>(PRESSURE_UI_VALUE_SCALE)));
}

struct RpmDotPos {
    int16_t x;
    int16_t y;
};

// Lazily built once (magic static), then reused: every call after the first is a plain array read, no trig.
static const std::array<RpmDotPos, RPM_DOT_LUT_COUNT> &rpmDotPositions()
{
    static const std::array<RpmDotPos, RPM_DOT_LUT_COUNT> positions = [] {
        constexpr float PI = 3.14159265358979323846f;
        std::array<RpmDotPos, RPM_DOT_LUT_COUNT> table{};
        for (int i = 0; i < RPM_DOT_LUT_COUNT; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(RPM_DOT_LUT_COUNT - 1);
            const float angle_rad = (RPM_DOT_START_ANGLE_DEG + t * RPM_DOT_SWEEP_ANGLE_DEG) * (PI / 180.0f);
            table[i].x = static_cast<int16_t>(std::lround(RPM_DOT_CENTER_X + RPM_DOT_RADIUS * std::cos(angle_rad))) - RPM_DOT_HALF_SIZE;
            table[i].y = static_cast<int16_t>(std::lround(RPM_DOT_CENTER_Y + RPM_DOT_RADIUS * std::sin(angle_rad))) - RPM_DOT_HALF_SIZE;
        }
        return table;
    }();
    return positions;
}

static void setEngineRpmDotPosition(int rpm)
{
    if (objects.main_img_engine_rpm == NULL) {
        return;
    }
    const int rpm_clamped = clampI32ToRange(rpm, 0, RPM_UI_MAX_RPM);
    const int index = rpm_clamped * (RPM_DOT_LUT_COUNT - 1) / RPM_UI_MAX_RPM;
    const RpmDotPos &pos = rpmDotPositions()[index];
    lv_obj_set_pos(objects.main_img_engine_rpm, pos.x, pos.y);
}

static float g_engine_rpm_target = 0.0f;
static float g_engine_rpm_displayed = 0.0f;
static bool g_engine_rpm_dot_initialized = false;
static lv_timer_t *g_engine_rpm_dot_anim_timer = NULL;

static void engineRpmDotAnimationTimerCb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    if (!g_engine_rpm_dot_initialized) {
        return;
    }
    if (std::fabs(g_engine_rpm_target - g_engine_rpm_displayed) < RPM_DOT_ANIM_SNAP_THRESHOLD_RPM) {
        g_engine_rpm_displayed = g_engine_rpm_target;
    } else {
        g_engine_rpm_displayed += (g_engine_rpm_target - g_engine_rpm_displayed) * RPM_DOT_ANIM_SMOOTHING;
    }
    setEngineRpmDotPosition(static_cast<int>(std::lround(g_engine_rpm_displayed)));
}

void initEngineRpmDotAnimation()
{
    if (g_engine_rpm_dot_anim_timer != NULL) {
        return;
    }
    clearEngineRpmIndicator();
    g_engine_rpm_dot_anim_timer = lv_timer_create(engineRpmDotAnimationTimerCb, RPM_DOT_ANIM_PERIOD_MS, NULL);
}

void setEngineRpmDotTarget(int rpm)
{
    g_engine_rpm_target = static_cast<float>(clampI32ToRange(rpm, 0, RPM_UI_MAX_RPM));
    if (!g_engine_rpm_dot_initialized) {
        // The first fresh reading after startup or signal loss must not animate
        // through old values while becoming visible again.
        g_engine_rpm_displayed = g_engine_rpm_target;
        setEngineRpmDotPosition(static_cast<int>(g_engine_rpm_target));
        g_engine_rpm_dot_initialized = true;
    }
}

void resetEngineRpmDot()
{
    g_engine_rpm_dot_initialized = false;
}

struct SmoothedArcEntry {
    lv_obj_t *arc = NULL;
    float target = 0.0f;
    float displayed = 0.0f;
    uint32_t transition_duration_ms = 0; // Zero preserves pressure smoothing.
    uint32_t transition_started_ms = 0;
    float transition_start = 0.0f;
    int32_t last_applied_value = INT32_MIN;
    bool initialized = false;
};

static SmoothedArcEntry g_smoothed_arcs[static_cast<size_t>(SmoothedArcId::Count)] = {};
static lv_timer_t *g_arc_smoothing_anim_timer = NULL;

static void arcSmoothingAnimationTimerCb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    for (SmoothedArcEntry &entry : g_smoothed_arcs) {
        if (entry.arc == NULL) {
            continue;
        }
        if (entry.transition_duration_ms != 0 && entry.displayed != entry.target) {
            entry.displayed = timedGaugeValue(entry.transition_start, entry.target,
                entry.transition_started_ms, lv_tick_get(), entry.transition_duration_ms);
        } else if (std::fabs(entry.target - entry.displayed) < ARC_SMOOTHING_ANIM_SNAP_THRESHOLD) {
            entry.displayed = entry.target;
        } else {
            entry.displayed += (entry.target - entry.displayed) * ARC_SMOOTHING_ANIM_FACTOR_FAST;
        }
        const int32_t rounded = static_cast<int32_t>(std::lround(entry.displayed));
        if (rounded != entry.last_applied_value) {
            entry.last_applied_value = rounded;
            lv_arc_set_value(entry.arc, rounded);
        }
    }
}

void initArcSmoothingAnimation()
{
    if (g_arc_smoothing_anim_timer != NULL) {
        return;
    }
    // Temperatures and fuel settle in one second between target updates.
    // Pressure retains its existing smoothing.
    g_smoothed_arcs[static_cast<size_t>(SmoothedArcId::CylHeadLTemp)].transition_duration_ms = SLOW_GAUGE_TRANSITION_MS;
    g_smoothed_arcs[static_cast<size_t>(SmoothedArcId::CylHeadRTemp)].transition_duration_ms = SLOW_GAUGE_TRANSITION_MS;
    g_smoothed_arcs[static_cast<size_t>(SmoothedArcId::ExhaustLTemp)].transition_duration_ms = SLOW_GAUGE_TRANSITION_MS;
    g_smoothed_arcs[static_cast<size_t>(SmoothedArcId::ExhaustRTemp)].transition_duration_ms = SLOW_GAUGE_TRANSITION_MS;
    g_smoothed_arcs[static_cast<size_t>(SmoothedArcId::FuelLevel)].transition_duration_ms = SLOW_GAUGE_TRANSITION_MS;
    g_arc_smoothing_anim_timer = lv_timer_create(arcSmoothingAnimationTimerCb, ARC_SMOOTHING_ANIM_PERIOD_MS, NULL);
}

void setSmoothedArcTarget(SmoothedArcId id, lv_obj_t *arc, int32_t value)
{
    SmoothedArcEntry &entry = g_smoothed_arcs[static_cast<size_t>(id)];
    entry.arc = arc;
    const float target = static_cast<float>(value);
    if (entry.transition_duration_ms != 0 && entry.target != target) {
        entry.transition_start = entry.displayed;
        entry.transition_started_ms = lv_tick_get();
    }
    entry.target = target;
    if (!entry.initialized) {
        // First reading (or first one after a stale reset): snap instead of
        // animating in from whatever the arc's default/old value was.
        entry.displayed = entry.target;
        entry.transition_start = entry.target;
        entry.initialized = true;
    }
}

void resetSmoothedArc(SmoothedArcId id)
{
    g_smoothed_arcs[static_cast<size_t>(id)].initialized = false;
}

struct ScaleLabelStorage {
    lv_obj_t *scale;
    const char *texts[3];
    char min_text[16];
    char max_text[16];
};

// Currently 7 scales use labels; bump this size if applyAllScaleLabels() grows past 10.
static ScaleLabelStorage g_scale_label_storage[10] = {};

static ScaleLabelStorage *getScaleLabelStorage(lv_obj_t *scale)
{
    // LVGL keeps pointers to the label text source, so each scale needs stable
    // backing storage instead of stack buffers shared across calls.
    if (scale == NULL) {
        return NULL;
    }

    ScaleLabelStorage *free_slot = NULL;
    for (ScaleLabelStorage &slot : g_scale_label_storage) {
        if (slot.scale == scale) {
            return &slot;
        }
        if (slot.scale == NULL && free_slot == NULL) {
            free_slot = &slot;
        }
    }

    if (free_slot != NULL) {
        free_slot->scale = scale;
        free_slot->texts[0] = free_slot->min_text;
        free_slot->texts[1] = free_slot->max_text;
        free_slot->texts[2] = NULL;
        return free_slot;
    }

    // Pool exhausted: labels for this scale will silently not be set unless we flag it here.
    ESP_LOGE(TAG, "Scale label storage pool exhausted (max %d), scale %p not tracked",
             static_cast<int>(sizeof(g_scale_label_storage) / sizeof(g_scale_label_storage[0])),
             static_cast<void *>(scale));
    return NULL;
}

static void setScaleLabelText(lv_obj_t *scale, const char *min_text, const char *max_text)
{
    if (scale == NULL) {
        return;
    }

    ScaleLabelStorage *storage = getScaleLabelStorage(scale);
    if (storage == NULL) {
        return;
    }

    snprintf(storage->min_text, sizeof(storage->min_text), "%s", min_text);
    snprintf(storage->max_text, sizeof(storage->max_text), "%s", max_text);
    lv_scale_set_label_show(scale, true);
    lv_scale_set_text_src(scale, storage->texts);
}

static void setScaleLabelsFromIntegerRange(lv_obj_t *scale, const char *suffix, bool pressure_scale)
{
    if (scale == NULL) {
        return;
    }

    const int32_t min_value = lv_scale_get_range_min_value(scale);
    const int32_t max_value = lv_scale_get_range_max_value(scale);
    char min_text[16];
    char max_text[16];

    if (pressure_scale) {
        snprintf(min_text, sizeof(min_text), "%ld%s", static_cast<long>(pressureUiToLabelValue(min_value)), suffix);
        snprintf(max_text, sizeof(max_text), "%ld%s", static_cast<long>(pressureUiToLabelValue(max_value)), suffix);
    } else {
        snprintf(min_text, sizeof(min_text), "%ld%s", static_cast<long>(min_value), suffix);
        snprintf(max_text, sizeof(max_text), "%ld%s", static_cast<long>(max_value), suffix);
    }

    setScaleLabelText(scale, min_text, max_text);
}

static void syncArcRangeFromScale(lv_obj_t *scale, lv_obj_t *arc)
{
    if (scale == NULL || arc == NULL) {
        return;
    }

    const int32_t min_value = lv_scale_get_range_min_value(scale);
    const int32_t max_value = lv_scale_get_range_max_value(scale);
    lv_arc_set_range(arc, min_value, max_value);
}

static void configureTemperatureScaleBands(lv_obj_t *scale,
                                          float cool_max_c,
                                          float normal_max_c,
                                          float warn_max_c,
                                          float value_scale)
{
    if (scale == NULL) {
        return;
    }

    ensureScaleSectionStyles();

    const int32_t min_value = lv_scale_get_range_min_value(scale);
    const int32_t max_value = lv_scale_get_range_max_value(scale);

    int32_t cool_end = thresholdToScaleValue(cool_max_c * value_scale, min_value, max_value);
    int32_t normal_end = thresholdToScaleValue(normal_max_c * value_scale, min_value, max_value);
    int32_t warn_end = thresholdToScaleValue(warn_max_c * value_scale, min_value, max_value);

    if (cool_end > normal_end) {
        cool_end = normal_end;
    }
    if (warn_end < normal_end) {
        warn_end = normal_end;
    }

    lv_scale_section_t *cool_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, cool_section, min_value, cool_end);
    lv_scale_set_section_style_main(scale, cool_section, &scale_style_caution);

    lv_scale_section_t *normal_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, normal_section, cool_end, normal_end);
    lv_scale_set_section_style_main(scale, normal_section, &scale_style_nominal);

    lv_scale_section_t *warn_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, warn_section, normal_end, warn_end);
    lv_scale_set_section_style_main(scale, warn_section, &scale_style_caution);

    lv_scale_section_t *danger_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, danger_section, warn_end, max_value);
    lv_scale_set_section_style_main(scale, danger_section, &scale_style_danger);
}

static void configureLowerIsWorseScaleBands(lv_obj_t *scale,
                                            float warning_threshold,
                                            float caution_threshold,
                                            float value_max_for_scale)
{
    if (scale == NULL) {
        return;
    }

    ensureScaleSectionStyles();

    const int32_t min_value = lv_scale_get_range_min_value(scale);
    const int32_t max_value = lv_scale_get_range_max_value(scale);

    int32_t warning_end = mapValueToScaleRange(warning_threshold, scale, value_max_for_scale);
    int32_t caution_end = mapValueToScaleRange(caution_threshold, scale, value_max_for_scale);

    if (warning_end > caution_end) {
        warning_end = caution_end;
    }

    lv_scale_section_t *danger_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, danger_section, min_value, warning_end);
    lv_scale_set_section_style_main(scale, danger_section, &scale_style_danger);

    lv_scale_section_t *caution_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, caution_section, warning_end, caution_end);
    lv_scale_set_section_style_main(scale, caution_section, &scale_style_caution);

    lv_scale_section_t *nominal_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, nominal_section, caution_end, max_value);
    lv_scale_set_section_style_main(scale, nominal_section, &scale_style_nominal);
}

static void updatePressureScaleSectionRanges(PressureScaleSections &sections, float low_threshold)
{
    if (sections.scale == NULL ||
        sections.low_section == NULL ||
        sections.normal_section == NULL ||
        sections.warn_section == NULL ||
        sections.danger_section == NULL) {
        return;
    }

    const int32_t min_value = lv_scale_get_range_min_value(sections.scale);
    const int32_t max_value = lv_scale_get_range_max_value(sections.scale);

    int32_t low_end = mapValueToScaleRange(low_threshold, sections.scale, sections.value_max_for_scale);
    int32_t normal_end = mapValueToScaleRange(sections.normal_threshold, sections.scale, sections.value_max_for_scale);
    int32_t warn_end = mapValueToScaleRange(sections.warn_threshold, sections.scale, sections.value_max_for_scale);

    if (warn_end > max_value) {
        warn_end = max_value;
    }
    if (normal_end > warn_end) {
        normal_end = warn_end;
    }
    if (low_end > normal_end) {
        low_end = normal_end;
    }

    lv_scale_set_section_range(sections.scale, sections.low_section, min_value, low_end);
    lv_scale_set_section_range(sections.scale, sections.normal_section, low_end, normal_end);
    lv_scale_set_section_range(sections.scale, sections.warn_section, normal_end, warn_end);
    lv_scale_set_section_range(sections.scale, sections.danger_section, warn_end, max_value);
    lv_obj_invalidate(sections.scale);
}

static void configureTrackedPressureScaleBands(PressureScaleSections &sections,
                                               lv_obj_t *scale,
                                               float low_threshold,
                                               float normal_threshold,
                                               float warn_threshold,
                                               float value_max_for_scale)
{
    if (scale == NULL) {
        return;
    }

    ensureScaleSectionStyles();

    if (sections.scale != scale ||
        sections.low_section == NULL ||
        sections.normal_section == NULL ||
        sections.warn_section == NULL ||
        sections.danger_section == NULL) {
        sections = {};
        sections.scale = scale;
        sections.low_section = lv_scale_add_section(scale);
        lv_scale_set_section_style_main(scale, sections.low_section, &scale_style_caution);
        sections.normal_section = lv_scale_add_section(scale);
        lv_scale_set_section_style_main(scale, sections.normal_section, &scale_style_nominal);
        sections.warn_section = lv_scale_add_section(scale);
        lv_scale_set_section_style_main(scale, sections.warn_section, &scale_style_caution);
        sections.danger_section = lv_scale_add_section(scale);
        lv_scale_set_section_style_main(scale, sections.danger_section, &scale_style_danger);
    }

    sections.normal_threshold = normal_threshold;
    sections.warn_threshold = warn_threshold;
    sections.value_max_for_scale = value_max_for_scale;
    updatePressureScaleSectionRanges(sections, low_threshold);
}

static void configureHigherIsWorseScaleBands(lv_obj_t *scale,
                                             float caution_threshold,
                                             float warning_threshold,
                                             float value_max_for_scale)
{
    if (scale == NULL) {
        return;
    }

    ensureScaleSectionStyles();

    const int32_t min_value = lv_scale_get_range_min_value(scale);
    const int32_t max_value = lv_scale_get_range_max_value(scale);

    int32_t caution_end = mapValueToScaleRange(caution_threshold, scale, value_max_for_scale);
    int32_t warning_end = mapValueToScaleRange(warning_threshold, scale, value_max_for_scale);

    if (warning_end < caution_end) {
        warning_end = caution_end;
    }

    lv_scale_section_t *nominal_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, nominal_section, min_value, caution_end);
    lv_scale_set_section_style_main(scale, nominal_section, &scale_style_nominal);

    lv_scale_section_t *caution_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, caution_section, caution_end, warning_end);
    lv_scale_set_section_style_main(scale, caution_section, &scale_style_caution);

    lv_scale_section_t *danger_section = lv_scale_add_section(scale);
    lv_scale_set_section_range(scale, danger_section, warning_end, max_value);
    lv_scale_set_section_style_main(scale, danger_section, &scale_style_danger);
}

void configureScaleBandsFromThresholds()
{
    // Project the application threshold constants onto each LVGL scale once so
    // arc colors and section bands share the same operating ranges.
    lv_scale_set_range(objects.main_sca_fuel_pressure, 0, static_cast<int32_t>(FUEL_PRESSURE_UI_MAX_BAR * PRESSURE_UI_VALUE_SCALE));
    lv_scale_set_range(objects.main_sca_water_pressure, 0, static_cast<int32_t>(WATER_PRESSURE_UI_MAX_BAR * PRESSURE_UI_VALUE_SCALE));

    configureTemperatureScaleBands(objects.main_sca_cyl_head_l_temp,
                                   CHT_COLOR_COOL_MAX_C,
                                   CHT_COLOR_NORMAL_MAX_C,
                                   CHT_COLOR_WARN_MAX_C,
                                   1.0f);
    configureTemperatureScaleBands(objects.main_sca_cyl_head_r_temp,
                                   CHT_COLOR_COOL_MAX_C,
                                   CHT_COLOR_NORMAL_MAX_C,
                                   CHT_COLOR_WARN_MAX_C,
                                   1.0f);
    configureTemperatureScaleBands(objects.main_sca_exhaust_l_temp,
                                   EGT_COLOR_COOL_MAX_C,
                                   EGT_COLOR_NORMAL_MAX_C,
                                   EGT_COLOR_WARN_MAX_C,
                                   1.0f);
    configureTemperatureScaleBands(objects.main_sca_exhaust_r_temp,
                                   EGT_COLOR_COOL_MAX_C,
                                   EGT_COLOR_NORMAL_MAX_C,
                                   EGT_COLOR_WARN_MAX_C,
                                   1.0f);

    configureTrackedPressureScaleBands(g_fuel_pressure_sections,
                                       objects.main_sca_fuel_pressure,
                                       FUEL_PRESSURE_COLOR_LOW_MAX_BAR,
                                       FUEL_PRESSURE_COLOR_NORMAL_MAX_BAR,
                                       FUEL_PRESSURE_COLOR_WARN_MAX_BAR,
                                       FUEL_PRESSURE_UI_MAX_BAR);

    configureTrackedPressureScaleBands(g_water_pressure_sections,
                                       objects.main_sca_water_pressure,
                                       WATER_PRESSURE_COLOR_LOW_MAX_BAR,
                                       WATER_PRESSURE_COLOR_NORMAL_MAX_BAR,
                                       WATER_PRESSURE_COLOR_WARN_MAX_BAR,
                                       WATER_PRESSURE_UI_MAX_BAR);

    configureLowerIsWorseScaleBands(objects.main_sca_fuel_level,
                                    static_cast<float>(FUEL_COLOR_WARN_MIN_PERCENT),
                                    static_cast<float>(FUEL_COLOR_NORMAL_MIN_PERCENT),
                                    100.0f);

    configureHigherIsWorseScaleBands(objects.main_sca_engine_rpm,
                                     static_cast<float>(RPM_COLOR_CAUTION_MIN_RPM),
                                     static_cast<float>(RPM_COLOR_WARN_MIN_RPM),
                                     static_cast<float>(RPM_UI_MAX_RPM));
}

void syncArcRangesFromScales()
{
    syncArcRangeFromScale(objects.main_sca_cyl_head_l_temp,
                          objects.main_arc_cyl_head_l_temp);
    syncArcRangeFromScale(objects.main_sca_cyl_head_r_temp,
                          objects.main_arc_cyl_head_r_temp);
    syncArcRangeFromScale(objects.main_sca_exhaust_l_temp,
                          objects.main_arc_exhaust_l_temp);
    syncArcRangeFromScale(objects.main_sca_exhaust_r_temp,
                          objects.main_arc_exhaust_r_temp);
    syncArcRangeFromScale(objects.main_sca_engine_rpm,
                          objects.main_arc_engine_rpm);
    syncArcRangeFromScale(objects.main_sca_fuel_pressure,
                          objects.main_arc_fuel_pressure);
    syncArcRangeFromScale(objects.main_sca_water_pressure,
                          objects.main_arc_water_pressure);
    syncArcRangeFromScale(objects.main_sca_fuel_level,
                          objects.main_arc_fuel_level);
}

void applyAllScaleLabels()
{
    setScaleLabelsFromIntegerRange(objects.main_sca_cyl_head_l_temp, " C", false);
    setScaleLabelsFromIntegerRange(objects.main_sca_cyl_head_r_temp, " C", false);
    setScaleLabelsFromIntegerRange(objects.main_sca_exhaust_l_temp, " C", false);
    setScaleLabelsFromIntegerRange(objects.main_sca_exhaust_r_temp, " C", false);
    setScaleLabelsFromIntegerRange(objects.main_sca_fuel_pressure, " bar", true);
    setScaleLabelsFromIntegerRange(objects.main_sca_water_pressure, " bar", true);
    setScaleLabelsFromIntegerRange(objects.main_sca_fuel_level, " %", false);
}

void refreshFuelPressureScaleBand(float fuel_low_threshold)
{
    configureTrackedPressureScaleBands(g_fuel_pressure_sections,
                                       objects.main_sca_fuel_pressure,
                                       fuel_low_threshold,
                                       FUEL_PRESSURE_COLOR_NORMAL_MAX_BAR,
                                       FUEL_PRESSURE_COLOR_WARN_MAX_BAR,
                                       FUEL_PRESSURE_UI_MAX_BAR);
}

void refreshWaterPressureScaleBand(float water_low_threshold)
{
    configureTrackedPressureScaleBands(g_water_pressure_sections,
                                       objects.main_sca_water_pressure,
                                       water_low_threshold,
                                       WATER_PRESSURE_COLOR_NORMAL_MAX_BAR,
                                       WATER_PRESSURE_COLOR_WARN_MAX_BAR,
                                       WATER_PRESSURE_UI_MAX_BAR);
}
