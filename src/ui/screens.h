#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *main_cont_engine_rpm;
    lv_obj_t *main_sca_engine_rpm;
    lv_obj_t *main_img_engine_rpm;
    lv_obj_t *main_arc_engine_rpm;
    lv_obj_t *main_lbl_engine_rpm_val;
    lv_obj_t *main_lbl_engine_rpm_title;
    lv_obj_t *main_lbl_generator_voltage_val;
    lv_obj_t *main_lbl_generator_voltage_title;
    lv_obj_t *main_cont_gau_cyl_head_l_temp;
    lv_obj_t *main_sca_cyl_head_l_temp;
    lv_obj_t *main_arc_cyl_head_l_temp;
    lv_obj_t *main_lbl_cyl_head_l_temp_val;
    lv_obj_t *main_lbl_cyl_head_l_temp_title;
    lv_obj_t *main_cont_gau_cyl_head_r_temp;
    lv_obj_t *main_sca_cyl_head_r_temp;
    lv_obj_t *main_arc_cyl_head_r_temp;
    lv_obj_t *main_lbl_cyl_head_r_temp_val;
    lv_obj_t *main_lbl_cyl_head_r_temp_title;
    lv_obj_t *main_lbl_cyl_head_c_temp_title;
    lv_obj_t *main_lbl_cyl_head_c_temp_title_2;
    lv_obj_t *main_cont_exhaust_l_temp;
    lv_obj_t *main_sca_exhaust_l_temp;
    lv_obj_t *main_arc_exhaust_l_temp;
    lv_obj_t *main_lbl_exhaust_l_temp_val;
    lv_obj_t *main_cont_exhaust_r_temp;
    lv_obj_t *main_sca_exhaust_r_temp;
    lv_obj_t *main_arc_exhaust_r_temp;
    lv_obj_t *main_lbl_exhaust_r_temp_val;
    lv_obj_t *main_lbl_egt_c_temp_title;
    lv_obj_t *main_lbl_egt_c_temp_title_2;
    lv_obj_t *main_cont_fuel_pressure;
    lv_obj_t *main_sca_fuel_pressure;
    lv_obj_t *main_arc_fuel_pressure;
    lv_obj_t *main_lbl_fuel_pressure_val_pre;
    lv_obj_t *main_lbl_fuel_pressure_val_suf;
    lv_obj_t *main_lbl_fuel_pressure_point;
    lv_obj_t *main_lbl_fuel_pressure_title;
    lv_obj_t *main_cont_water_pressure;
    lv_obj_t *main_sca_water_pressure;
    lv_obj_t *main_arc_water_pressure;
    lv_obj_t *main_lbl_water_pressure_val_pre;
    lv_obj_t *main_lbl_water_pressure_val_suf;
    lv_obj_t *main_lbl_water_pressure_point;
    lv_obj_t *main_lbl_water_pressure_title;
    lv_obj_t *main_lbl_pressure_title;
    lv_obj_t *main_cont_fuel_level;
    lv_obj_t *main_sca_fuel_level;
    lv_obj_t *main_arc_fuel_level;
    lv_obj_t *main_lbl_fuel_level_val;
    lv_obj_t *main_lbl_fuel_level_title;
    lv_obj_t *main_but_dim;
    lv_obj_t *main_but_update;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/