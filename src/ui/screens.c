#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 800, 480);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // main_cont_engine_rpm
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_cont_engine_rpm = obj;
            lv_obj_set_pos(obj, 400, 0);
            lv_obj_set_size(obj, 395, 400);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_sca_engine_rpm
                    lv_obj_t *obj = lv_scale_create(parent_obj);
                    objects.main_sca_engine_rpm = obj;
                    lv_obj_set_pos(obj, 3, 5);
                    lv_obj_set_size(obj, 390, 390);
                    lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
                    lv_scale_set_range(obj, 0, 7000);
                    lv_scale_set_angle_range(obj, 270);
                    lv_scale_set_rotation(obj, 135);
                    lv_scale_set_total_tick_count(obj, 36);
                    lv_scale_set_major_tick_every(obj, 5);
                    lv_scale_set_label_show(obj, true);
                    static const char *label_texts[9] = {
                        "0",
                        "10",
                        "20",
                        "30",
                        "40",
                        "50",
                        "60",
                        "70",
                        NULL
                    };
                    lv_scale_set_text_src(obj, label_texts);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_pad_radial(obj, 15, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_MAIN);
                    lv_obj_set_style_length(obj, 8, LV_PART_ITEMS);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0x00e7ff), LV_PART_ITEMS);
                    lv_obj_set_style_length(obj, 25, LV_PART_INDICATOR);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_INDICATOR);
                }
                {
                    // main_img_engine_rpm
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    objects.main_img_engine_rpm = obj;
                    lv_obj_set_pos(obj, 54, 304);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_image_set_src(obj, &img_rpm_dot_24);
                }
                {
                    // main_arc_engine_rpm
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.main_arc_engine_rpm = obj;
                    lv_obj_set_pos(obj, 18, 20);
                    lv_obj_set_size(obj, 360, 360);
                    lv_arc_set_range(obj, 0, 7000);
                    lv_arc_set_value(obj, 2000);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_rounded(obj, false, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 12, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4040ff), LV_PART_KNOB | LV_STATE_DEFAULT);
                }
                {
                    // main_lbl_engine_rpm_val
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_engine_rpm_val = obj;
                    lv_obj_set_pos(obj, 126, 190);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_60_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "1234");
                }
                {
                    // main_lbl_engine_rpm_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_engine_rpm_title = obj;
                    lv_obj_set_pos(obj, 166, 255);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "RPMx100");
                }
                {
                    // main_lbl_generator_voltage_val
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_generator_voltage_val = obj;
                    lv_obj_set_pos(obj, 158, 120);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "14,4");
                }
                {
                    // main_lbl_generator_voltage_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_generator_voltage_title = obj;
                    lv_obj_set_pos(obj, 170, 160);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Voltage");
                }
            }
        }
        {
            // main_cont_gau_cyl_head_l_temp
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_cont_gau_cyl_head_l_temp = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_sca_cyl_head_l_temp
                    lv_obj_t *obj = lv_scale_create(parent_obj);
                    objects.main_sca_cyl_head_l_temp = obj;
                    lv_obj_set_pos(obj, 10, 10);
                    lv_obj_set_size(obj, 180, 180);
                    lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
                    lv_scale_set_range(obj, 50, 90);
                    lv_scale_set_angle_range(obj, 180);
                    lv_scale_set_rotation(obj, 180);
                    lv_scale_set_total_tick_count(obj, 2);
                    lv_scale_set_major_tick_every(obj, 1);
                    lv_scale_set_label_show(obj, true);
                    static const char *label_texts[3] = {
                        "50 C",
                        "90 C",
                        NULL
                    };
                    lv_scale_set_text_src(obj, label_texts);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_radial(obj, 6, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN);
                    lv_obj_set_style_length(obj, 10, LV_PART_INDICATOR);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR);
                }
                {
                    // main_arc_cyl_head_l_temp
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.main_arc_cyl_head_l_temp = obj;
                    lv_obj_set_pos(obj, 23, 23);
                    lv_obj_set_size(obj, 155, 155);
                    lv_arc_set_range(obj, 50, 90);
                    lv_arc_set_value(obj, 72);
                    lv_arc_set_bg_start_angle(obj, 180);
                    lv_arc_set_bg_end_angle(obj, 0);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4040ff), LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // main_lbl_cyl_head_l_temp_val
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_cyl_head_l_temp_val = obj;
                    lv_obj_set_pos(obj, 64, 45);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_60_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "72");
                }
                {
                    // main_lbl_cyl_head_l_temp_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_cyl_head_l_temp_title = obj;
                    lv_obj_set_pos(obj, 3, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "L");
                }
            }
        }
        {
            // main_cont_gau_cyl_head_r_temp
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_cont_gau_cyl_head_r_temp = obj;
            lv_obj_set_pos(obj, 200, 0);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_sca_cyl_head_r_temp
                    lv_obj_t *obj = lv_scale_create(parent_obj);
                    objects.main_sca_cyl_head_r_temp = obj;
                    lv_obj_set_pos(obj, 10, 10);
                    lv_obj_set_size(obj, 180, 180);
                    lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
                    lv_scale_set_range(obj, 50, 90);
                    lv_scale_set_angle_range(obj, 180);
                    lv_scale_set_rotation(obj, 180);
                    lv_scale_set_total_tick_count(obj, 2);
                    lv_scale_set_major_tick_every(obj, 1);
                    lv_scale_set_label_show(obj, true);
                    static const char *label_texts[3] = {
                        "50 C",
                        "90 C",
                        NULL
                    };
                    lv_scale_set_text_src(obj, label_texts);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_radial(obj, 6, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN);
                    lv_obj_set_style_length(obj, 10, LV_PART_INDICATOR);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR);
                }
                {
                    // main_arc_cyl_head_r_temp
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.main_arc_cyl_head_r_temp = obj;
                    lv_obj_set_pos(obj, 23, 23);
                    lv_obj_set_size(obj, 155, 155);
                    lv_arc_set_range(obj, 50, 90);
                    lv_arc_set_value(obj, 80);
                    lv_arc_set_bg_start_angle(obj, 180);
                    lv_arc_set_bg_end_angle(obj, 0);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4040ff), LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // main_lbl_cyl_head_r_temp_val
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_cyl_head_r_temp_val = obj;
                    lv_obj_set_pos(obj, 64, 45);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_60_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "80");
                }
                {
                    // main_lbl_cyl_head_r_temp_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_cyl_head_r_temp_title = obj;
                    lv_obj_set_pos(obj, 170, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "R");
                }
            }
        }
        {
            // main_lbl_cyl_head_c_temp_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.main_lbl_cyl_head_c_temp_title = obj;
            lv_obj_set_pos(obj, 170, 5);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "CHT");
        }
        {
            // main_lbl_cyl_head_c_temp_title_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.main_lbl_cyl_head_c_temp_title_2 = obj;
            lv_obj_set_pos(obj, 190, 40);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "C");
        }
        {
            // main_cont_exhaust_l_temp
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_cont_exhaust_l_temp = obj;
            lv_obj_set_pos(obj, 0, 150);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_sca_exhaust_l_temp
                    lv_obj_t *obj = lv_scale_create(parent_obj);
                    objects.main_sca_exhaust_l_temp = obj;
                    lv_obj_set_pos(obj, 10, 10);
                    lv_obj_set_size(obj, 180, 180);
                    lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
                    lv_scale_set_range(obj, 280, 750);
                    lv_scale_set_angle_range(obj, 180);
                    lv_scale_set_rotation(obj, 180);
                    lv_scale_set_total_tick_count(obj, 2);
                    lv_scale_set_major_tick_every(obj, 1);
                    lv_scale_set_label_show(obj, true);
                    static const char *label_texts[3] = {
                        "280 C",
                        "750 C",
                        NULL
                    };
                    lv_scale_set_text_src(obj, label_texts);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_radial(obj, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN);
                    lv_obj_set_style_length(obj, 10, LV_PART_INDICATOR);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR);
                }
                {
                    // main_arc_exhaust_l_temp
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.main_arc_exhaust_l_temp = obj;
                    lv_obj_set_pos(obj, 23, 23);
                    lv_obj_set_size(obj, 155, 155);
                    lv_arc_set_range(obj, 280, 750);
                    lv_arc_set_value(obj, 680);
                    lv_arc_set_bg_start_angle(obj, 180);
                    lv_arc_set_bg_end_angle(obj, 0);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4040ff), LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // main_lbl_exhaust_l_temp_val
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_exhaust_l_temp_val = obj;
                    lv_obj_set_pos(obj, 46, 45);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_60_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "680");
                }
            }
        }
        {
            // main_cont_exhaust_r_temp
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_cont_exhaust_r_temp = obj;
            lv_obj_set_pos(obj, 200, 150);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_sca_exhaust_r_temp
                    lv_obj_t *obj = lv_scale_create(parent_obj);
                    objects.main_sca_exhaust_r_temp = obj;
                    lv_obj_set_pos(obj, 10, 10);
                    lv_obj_set_size(obj, 180, 180);
                    lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
                    lv_scale_set_range(obj, 280, 750);
                    lv_scale_set_angle_range(obj, 180);
                    lv_scale_set_rotation(obj, 180);
                    lv_scale_set_total_tick_count(obj, 2);
                    lv_scale_set_major_tick_every(obj, 1);
                    lv_scale_set_label_show(obj, true);
                    static const char *label_texts[3] = {
                        "280 C",
                        "750 C",
                        NULL
                    };
                    lv_scale_set_text_src(obj, label_texts);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_radial(obj, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN);
                    lv_obj_set_style_length(obj, 10, LV_PART_INDICATOR);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR);
                }
                {
                    // main_arc_exhaust_r_temp
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.main_arc_exhaust_r_temp = obj;
                    lv_obj_set_pos(obj, 23, 23);
                    lv_obj_set_size(obj, 155, 155);
                    lv_arc_set_range(obj, 280, 750);
                    lv_arc_set_value(obj, 682);
                    lv_arc_set_bg_start_angle(obj, 180);
                    lv_arc_set_bg_end_angle(obj, 0);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4040ff), LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // main_lbl_exhaust_r_temp_val
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_exhaust_r_temp_val = obj;
                    lv_obj_set_pos(obj, 46, 45);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_60_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "682");
                }
            }
        }
        {
            // main_lbl_egt_c_temp_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.main_lbl_egt_c_temp_title = obj;
            lv_obj_set_pos(obj, 170, 155);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "EGT");
        }
        {
            // main_lbl_egt_c_temp_title_2
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.main_lbl_egt_c_temp_title_2 = obj;
            lv_obj_set_pos(obj, 190, 190);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "C");
        }
        {
            // main_cont_fuel_pressure
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_cont_fuel_pressure = obj;
            lv_obj_set_pos(obj, 0, 300);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_sca_fuel_pressure
                    lv_obj_t *obj = lv_scale_create(parent_obj);
                    objects.main_sca_fuel_pressure = obj;
                    lv_obj_set_pos(obj, 10, 10);
                    lv_obj_set_size(obj, 180, 180);
                    lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
                    lv_scale_set_range(obj, 0, 1);
                    lv_scale_set_angle_range(obj, 180);
                    lv_scale_set_rotation(obj, 180);
                    lv_scale_set_total_tick_count(obj, 2);
                    lv_scale_set_major_tick_every(obj, 1);
                    lv_scale_set_label_show(obj, true);
                    static const char *label_texts[3] = {
                        "0 bar",
                        "1 bar",
                        NULL
                    };
                    lv_scale_set_text_src(obj, label_texts);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_radial(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN);
                    lv_obj_set_style_length(obj, 10, LV_PART_INDICATOR);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR);
                }
                {
                    // main_arc_fuel_pressure
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.main_arc_fuel_pressure = obj;
                    lv_obj_set_pos(obj, 23, 23);
                    lv_obj_set_size(obj, 155, 155);
                    lv_arc_set_value(obj, 35);
                    lv_arc_set_bg_start_angle(obj, 180);
                    lv_arc_set_bg_end_angle(obj, 0);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4040ff), LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // main_lbl_fuel_pressure_val_pre
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_fuel_pressure_val_pre = obj;
                    lv_obj_set_pos(obj, 70, 62);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "0");
                }
                {
                    // main_lbl_fuel_pressure_val_suf
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_fuel_pressure_val_suf = obj;
                    lv_obj_set_pos(obj, 101, 62);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "35");
                }
                {
                    // main_lbl_fuel_pressure_point
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_fuel_pressure_point = obj;
                    lv_obj_set_pos(obj, 85, 62);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, ",");
                }
                {
                    // main_lbl_fuel_pressure_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_fuel_pressure_title = obj;
                    lv_obj_set_pos(obj, 3, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "F");
                }
            }
        }
        {
            // main_cont_water_pressure
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_cont_water_pressure = obj;
            lv_obj_set_pos(obj, 200, 300);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_sca_water_pressure
                    lv_obj_t *obj = lv_scale_create(parent_obj);
                    objects.main_sca_water_pressure = obj;
                    lv_obj_set_pos(obj, 10, 10);
                    lv_obj_set_size(obj, 180, 180);
                    lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
                    lv_scale_set_range(obj, 0, 3);
                    lv_scale_set_angle_range(obj, 180);
                    lv_scale_set_rotation(obj, 180);
                    lv_scale_set_total_tick_count(obj, 2);
                    lv_scale_set_major_tick_every(obj, 1);
                    lv_scale_set_label_show(obj, true);
                    static const char *label_texts[3] = {
                        "0 bar",
                        "3 bar",
                        NULL
                    };
                    lv_scale_set_text_src(obj, label_texts);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_radial(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN);
                    lv_obj_set_style_length(obj, 10, LV_PART_INDICATOR);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR);
                }
                {
                    // main_arc_water_pressure
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.main_arc_water_pressure = obj;
                    lv_obj_set_pos(obj, 23, 23);
                    lv_obj_set_size(obj, 155, 155);
                    lv_arc_set_range(obj, 0, 300);
                    lv_arc_set_value(obj, 220);
                    lv_arc_set_bg_start_angle(obj, 180);
                    lv_arc_set_bg_end_angle(obj, 0);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4040ff), LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // main_lbl_water_pressure_val_pre
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_water_pressure_val_pre = obj;
                    lv_obj_set_pos(obj, 70, 62);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "2");
                }
                {
                    // main_lbl_water_pressure_val_suf
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_water_pressure_val_suf = obj;
                    lv_obj_set_pos(obj, 101, 62);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "20");
                }
                {
                    // main_lbl_water_pressure_point
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_water_pressure_point = obj;
                    lv_obj_set_pos(obj, 85, 62);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, ",");
                }
                {
                    // main_lbl_water_pressure_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_water_pressure_title = obj;
                    lv_obj_set_pos(obj, 170, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "W");
                }
            }
        }
        {
            // main_lbl_pressure_title
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.main_lbl_pressure_title = obj;
            lv_obj_set_pos(obj, 170, 305);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "bar");
        }
        {
            // main_cont_fuel_level
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.main_cont_fuel_level = obj;
            lv_obj_set_pos(obj, 500, 360);
            lv_obj_set_size(obj, 200, 200);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // main_sca_fuel_level
                    lv_obj_t *obj = lv_scale_create(parent_obj);
                    objects.main_sca_fuel_level = obj;
                    lv_obj_set_pos(obj, 10, 10);
                    lv_obj_set_size(obj, 180, 180);
                    lv_scale_set_mode(obj, LV_SCALE_MODE_ROUND_INNER);
                    lv_scale_set_range(obj, 0, 100);
                    lv_scale_set_angle_range(obj, 180);
                    lv_scale_set_rotation(obj, 180);
                    lv_scale_set_total_tick_count(obj, 2);
                    lv_scale_set_major_tick_every(obj, 1);
                    lv_scale_set_label_show(obj, true);
                    static const char *label_texts[3] = {
                        "0%",
                        "100%   ",
                        NULL
                    };
                    lv_scale_set_text_src(obj, label_texts);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_ITEMS | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_radial(obj, 2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0x4040ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN);
                    lv_obj_set_style_length(obj, 10, LV_PART_INDICATOR);
                    lv_obj_set_style_line_color(obj, lv_color_hex(0x00e7ff), LV_PART_INDICATOR);
                }
                {
                    // main_arc_fuel_level
                    lv_obj_t *obj = lv_arc_create(parent_obj);
                    objects.main_arc_fuel_level = obj;
                    lv_obj_set_pos(obj, 23, 23);
                    lv_obj_set_size(obj, 155, 155);
                    lv_arc_set_value(obj, 65);
                    lv_arc_set_bg_start_angle(obj, 180);
                    lv_arc_set_bg_end_angle(obj, 0);
                    lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4040ff), LV_PART_KNOB | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_width(obj, 8, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                    lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
                }
                {
                    // main_lbl_fuel_level_val
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_fuel_level_val = obj;
                    lv_obj_set_pos(obj, 64, 45);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_clean_60_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "65");
                }
                {
                    // main_lbl_fuel_level_title
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.main_lbl_fuel_level_title = obj;
                    lv_obj_set_pos(obj, 3, 5);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0x00e7ff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_jet_brains_mono_regular_34_bpp4, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "F");
                }
            }
        }
        {
            // main_but_dim
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.main_but_dim = obj;
            lv_obj_set_pos(obj, 718, 428);
            lv_obj_set_size(obj, 82, 50);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Button");
                }
            }
        }
        {
            // main_but_update
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.main_but_update = obj;
            lv_obj_set_pos(obj, 718, 370);
            lv_obj_set_size(obj, 82, 50);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "Button");
                }
            }
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 1) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "JetBrainsMono-Regular_34_Bpp4", &ui_font_jet_brains_mono_regular_34_bpp4 },
    { "JetBrainsMono-Regular_60_Bpp4", &ui_font_jet_brains_mono_regular_60_bpp4 },
    { "JetBrainsMonoRegular-Clean_34_Bpp4", &ui_font_jet_brains_mono_regular_clean_34_bpp4 },
    { "JetBrainsMonoRegular-Clean_60_Bpp4", &ui_font_jet_brains_mono_regular_clean_60_bpp4 },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
}