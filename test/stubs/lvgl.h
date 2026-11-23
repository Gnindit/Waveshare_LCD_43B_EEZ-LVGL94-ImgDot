#pragma once
#include <cstdint>
#include <string>
using lv_color_t = uint32_t;
using lv_style_selector_t = uint32_t;
struct lv_obj_t {
    std::string text;
    uint32_t text_color = 0;
    uint32_t recolor = 0;
    int recolor_opa = 0;
    bool hidden = false;
    int text_writes = 0;
};
constexpr int LV_PART_MAIN = 0;
constexpr int LV_STATE_DEFAULT = 0;
constexpr int LV_OBJ_FLAG_HIDDEN = 1;
constexpr int LV_OPA_COVER = 255;
constexpr int LV_OPA_TRANSP = 0;
inline lv_color_t lv_color_hex(uint32_t c) { return c; }
inline void lv_label_set_text(lv_obj_t *obj, const char *s) { obj->text=s; ++obj->text_writes; }
inline void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t c, lv_style_selector_t) { obj->text_color=c; }
inline void lv_obj_set_style_image_recolor(lv_obj_t *obj, lv_color_t c, lv_style_selector_t) { obj->recolor=c; }
inline void lv_obj_set_style_image_recolor_opa(lv_obj_t *obj, int a, lv_style_selector_t) { obj->recolor_opa=a; }
inline void lv_obj_add_flag(lv_obj_t *obj, int) { obj->hidden=true; }
inline void lv_obj_remove_flag(lv_obj_t *obj, int) { obj->hidden=false; }
