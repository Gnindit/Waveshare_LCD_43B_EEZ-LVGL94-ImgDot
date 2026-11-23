#include "rpm_indicator.h"

#include <cstdio>
#include <lvgl.h>

#include "app_config.h"
#include "ui_scale_helpers.h"
#include "ui/ui.h"

namespace {
int last_displayed_rpm_rounded_10 = -1;
uint32_t last_label_color = 0xFFFFFFFFu;

void setRpmLabelColor(uint32_t color)
{
    if (objects.main_lbl_engine_rpm_val != nullptr && color != last_label_color) {
        lv_obj_set_style_text_color(objects.main_lbl_engine_rpm_val,
                                   lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        last_label_color = color;
    }
}
} // namespace

void clearEngineRpmIndicator()
{
    // Invalidate the cache as well as the label: an identical RPM reading must
    // restore the number after a timeout or invalid sensor sample.
    last_displayed_rpm_rounded_10 = -1;
    if (objects.main_lbl_engine_rpm_val != nullptr) {
        lv_label_set_text(objects.main_lbl_engine_rpm_val, "----");
    }
    setRpmLabelColor(UI_HEX_STATE_DANGER);
    if (objects.main_img_engine_rpm != nullptr) {
        lv_obj_add_flag(objects.main_img_engine_rpm, LV_OBJ_FLAG_HIDDEN);
    }
    resetEngineRpmDot();
}

void updateEngineRpmIndicator(int rpm)
{
    const int rpm_clamped = rpm < 0 ? 0 : (rpm > RPM_UI_MAX_RPM ? RPM_UI_MAX_RPM : rpm);
    // Keep the numerical RPM readable above the graphical scale maximum.
    const int rounded = ((rpm + 5) / 10) * 10;
    if (objects.main_lbl_engine_rpm_val != nullptr && rounded != last_displayed_rpm_rounded_10) {
        char text[16];
        snprintf(text, sizeof(text), "%04d", rounded);
        lv_label_set_text(objects.main_lbl_engine_rpm_val, text);
        last_displayed_rpm_rounded_10 = rounded;
    }
    setRpmLabelColor(0xFFFFFF);

    // This positions the first fresh sample immediately and smooths subsequent
    // samples. It does not depend on the hidden legacy RPM arc being present.
    setEngineRpmDotTarget(rpm_clamped);
    if (objects.main_img_engine_rpm != nullptr) {
        const bool warning = rpm_clamped >= RPM_COLOR_CAUTION_MIN_RPM;
        const uint32_t color = rpm_clamped >= RPM_COLOR_WARN_MIN_RPM
                                   ? UI_HEX_STATE_DANGER : UI_HEX_STATE_CAUTION;
        lv_obj_set_style_image_recolor(objects.main_img_engine_rpm,
                                       lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
        // Preserve the original image colors during normal operation.
        lv_obj_set_style_image_recolor_opa(objects.main_img_engine_rpm,
                                           warning ? LV_OPA_COVER : LV_OPA_TRANSP,
                                           LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_remove_flag(objects.main_img_engine_rpm, LV_OBJ_FLAG_HIDDEN);
    }
}
