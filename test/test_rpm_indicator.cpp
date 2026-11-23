#include <cassert>
#include <iostream>
#include "app_config.h"
#include "rpm_indicator.h"
#include "ui/ui.h"

objects_t objects = {};
static int dot_target = -1;
static int dot_resets = 0;
void setEngineRpmDotTarget(int rpm) { dot_target = rpm; }
void resetEngineRpmDot() { ++dot_resets; }

int main()
{
    lv_obj_t label, dot;
    objects.main_lbl_engine_rpm_val = &label;
    objects.main_img_engine_rpm = &dot;
    // The legacy RPM arc is deliberately absent.
    clearEngineRpmIndicator();
    assert(dot.hidden && label.text == "----");
    assert(label.text_color == UI_HEX_STATE_DANGER && dot_resets == 1);

    updateEngineRpmIndicator(2342);
    assert(!dot.hidden && label.text == "2340" && dot_target == 2342);
    assert(dot.recolor_opa == LV_OPA_TRANSP && label.text_color == 0xFFFFFF);
    const int writes = label.text_writes;
    updateEngineRpmIndicator(2343);
    assert(label.text_writes == writes && dot_target == 2343);

    // Timeout followed by the same rounded RPM must restore the label.
    clearEngineRpmIndicator();
    assert(dot.hidden && label.text == "----");
    updateEngineRpmIndicator(2342);
    assert(!dot.hidden && label.text == "2340" && label.text_color == 0xFFFFFF);

    // Invalid sensor followed by the same valid value uses the same clear path.
    clearEngineRpmIndicator();
    clearEngineRpmIndicator();
    updateEngineRpmIndicator(2342);
    assert(label.text == "2340" && !dot.hidden && dot_resets == 4);

    updateEngineRpmIndicator(5599);
    assert(dot.recolor_opa == LV_OPA_TRANSP); // raw value governs warnings
    updateEngineRpmIndicator(5600);
    assert(dot.recolor_opa == LV_OPA_COVER && dot.recolor == UI_HEX_STATE_CAUTION);
    updateEngineRpmIndicator(5999);
    assert(dot.recolor == UI_HEX_STATE_CAUTION);
    updateEngineRpmIndicator(6000);
    assert(dot.recolor == UI_HEX_STATE_DANGER);
    updateEngineRpmIndicator(5600);
    assert(dot.recolor == UI_HEX_STATE_CAUTION);
    updateEngineRpmIndicator(5599);
    assert(dot.recolor_opa == LV_OPA_TRANSP); // restore original image colors

    updateEngineRpmIndicator(9000);
    assert(dot_target == 7000 && label.text == "9000");
    updateEngineRpmIndicator(0);
    assert(dot_target == 0 && label.text == "0000" && !dot.hidden);

    objects.main_img_engine_rpm = nullptr;
    objects.main_lbl_engine_rpm_val = nullptr;
    clearEngineRpmIndicator();
    updateEngineRpmIndicator(1000); // missing widgets must be harmless
    std::cout << "RPM indicator regression checks passed\n";
}
