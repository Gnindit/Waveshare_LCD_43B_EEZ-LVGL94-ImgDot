#include "display_setup.h"

#include <assert.h>
#include <cmath>
#include <cstring>

#include <esp_timer.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <lvgl.h>

#include "app_config.h"
#include "lcd_config.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"
#include "ui/ui.h"
#include "ui_scale_helpers.h"

static esp_lcd_panel_handle_t lcd_handle = nullptr;
static SemaphoreHandle_t sem_frame_buf_complete;
static SemaphoreHandle_t sem_vsync;
static esp_lcd_touch_handle_t touch_handle = NULL;
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_master_dev_handle_t ch422g_mode_handle = NULL;
static i2c_master_dev_handle_t ch422g_output_handle = NULL;
static uint8_t ch422g_output_state = 0x2e;
static bool display_backlight_on = true;

// The Waveshare 4.3B routes LCD backlight enable through the CH422G expander.
// EXIO2 is the backlight bit in the cached output register value.
static constexpr uint32_t CH422G_MODE_ADDRESS = 0x24;
static constexpr uint32_t CH422G_OUTPUT_ADDRESS = 0x38;
static constexpr uint32_t CH422G_I2C_SPEED_HZ = 200 * 1000;
static constexpr uint8_t CH422G_OUTPUT_MODE_VALUE = 0x01;
static constexpr uint8_t LCD_BL_PIN_ON_CH422G = 2;
static constexpr gpio_num_t GT911_RESET_GPIO = GPIO_NUM_4;

static void writeCh422gOutputState()
{
    // The CH422G needs a mode-select write before updating the push-pull outputs.
    ESP_ERROR_CHECK(i2c_master_transmit(ch422g_mode_handle,
                                        &CH422G_OUTPUT_MODE_VALUE,
                                        sizeof(CH422G_OUTPUT_MODE_VALUE),
                                        portMAX_DELAY));
    ESP_ERROR_CHECK(i2c_master_transmit(ch422g_output_handle,
                                        &ch422g_output_state,
                                        sizeof(ch422g_output_state),
                                        portMAX_DELAY));
}

static void writeCh422gOutputState(uint8_t output_state)
{
    ch422g_output_state = output_state;
    writeCh422gOutputState();
}

static void setDisplayBacklight(bool enabled)
{
    const uint8_t backlight_mask = static_cast<uint8_t>(1U << LCD_BL_PIN_ON_CH422G);
    if (enabled) {
        ch422g_output_state |= backlight_mask;
    } else {
        ch422g_output_state &= static_cast<uint8_t>(~backlight_mask);
    }

    writeCh422gOutputState();
    display_backlight_on = enabled;
}

static void toggleDisplayBacklight()
{
    setDisplayBacklight(!display_backlight_on);
}

static void initializeCh422gBacklightControl()
{
    i2c_device_config_t device_config;
    memset(&device_config, 0, sizeof(device_config));
    device_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    device_config.scl_speed_hz = CH422G_I2C_SPEED_HZ;

    // The board exposes the CH422G functions as separate I2C targets for mode
    // selection and output register writes.
    device_config.device_address = CH422G_MODE_ADDRESS;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &device_config, &ch422g_mode_handle));

    device_config.device_address = CH422G_OUTPUT_ADDRESS;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &device_config, &ch422g_output_handle));

    setDisplayBacklight(true);
}

static void onDimButtonEvent(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        // main_but_dim acts as a simple backlight toggle for the panel.
        toggleDisplayBacklight();
    }
}

static bool lcdVsyncEvent(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    (void)panel;
    (void)event_data;
    (void)user_data;

    BaseType_t high_task_awoken = pdFALSE;
    xSemaphoreGiveFromISR(sem_vsync, &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

// Fires once per frame, exactly when the bounce-buffer refill switches its read
// source to cur_fb_index; only from here on is the other framebuffer safe to reuse.
static bool lcdFrameBufCompleteEvent(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    (void)panel;
    (void)event_data;
    (void)user_data;

    BaseType_t high_task_awoken = pdFALSE;
    xSemaphoreGiveFromISR(sem_frame_buf_complete, &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

static void lcdIncreaseLvglTick(void *arg)
{
    (void)arg;
    lv_tick_inc(2);
}

static void lvglFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lcd_handle;
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;

    // Non-final flushes of a DIRECT-mode refresh already rendered into the same
    // off-screen buffer as the upcoming final flush; only the final one needs to
    // request the physical framebuffer switch.
    if (!lv_display_flush_is_last(disp)) {
        lv_display_flush_ready(disp);
        return;
    }

    // Discard any stale signal from a previous cycle so the wait below only
    // observes the event caused by the draw_bitmap() call that follows.
    xSemaphoreTake(sem_frame_buf_complete, 0);

    // px_map is one of the panel's own framebuffers, so this only flips cur_fb_index;
    // no pixel data is CPU-copied here.
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);

    // The bounce-buffer refill switching to read from this buffer is the only
    // event that guarantees the other buffer is now safe to redraw into.
    xSemaphoreTake(sem_frame_buf_complete, portMAX_DELAY);

    lv_display_flush_ready(disp);
}

static void myInputRead(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    uint16_t x[5], y[5], s[5];
    uint8_t count;
    if (esp_lcd_touch_get_coordinates(touch_handle, x, y, s, &count, 5)) {
        data->point.x = x[0];
        data->point.y = y[0];
        data->state = LV_INDEV_STATE_PRESSED;
        return;
    }

    data->state = LV_INDEV_STATE_RELEASED;
}

static void i2cInitialize()
{
    i2c_master_bus_config_t i2c_mst_config;
    memset(&i2c_mst_config, 0, sizeof(i2c_mst_config));
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = static_cast<i2c_port_num_t>(I2C_MASTER_NUM);
    i2c_mst_config.scl_io_num = static_cast<gpio_num_t>(I2C_MASTER_SCL_IO);
    i2c_mst_config.sda_io_num = static_cast<gpio_num_t>(I2C_MASTER_SDA_IO);
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = 1;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));
}

static void resetTouchController()
{
    // This mirrors the board's original Waveshare bring-up sequence for GT911.
    // CH422G output values 0x2C and 0x2E pulse the shared reset lines while
    // keeping the rest of the panel control outputs in their working state.
    writeCh422gOutputState(0x2c);
    esp_rom_delay_us(100 * 1000);
    gpio_set_level(GT911_RESET_GPIO, 0);
    esp_rom_delay_us(100 * 1000);
    writeCh422gOutputState(0x2e);
    esp_rom_delay_us(200 * 1000);
}

static void touchInitialize()
{
    esp_lcd_panel_io_i2c_config_t tio_cfg;
    esp_lcd_panel_io_handle_t tio_handle;
    esp_lcd_touch_config_t tp_cfg;

    resetTouchController();

    memset(&tio_cfg, 0, sizeof(tio_cfg));
    tio_cfg.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS;
    tio_cfg.control_phase_bytes = 1;
    tio_cfg.dc_bit_offset = 0;
    tio_cfg.lcd_cmd_bits = 16;
    tio_cfg.lcd_param_bits = 0;
    tio_cfg.flags.disable_control_phase = 1;
    tio_cfg.flags.dc_low_on_data = 0;
    tio_cfg.on_color_trans_done = NULL;
    tio_cfg.scl_speed_hz = 200 * 1000;
    tio_cfg.user_ctx = NULL;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tio_cfg, &tio_handle));

    memset(&tp_cfg, 0, sizeof(tp_cfg));
    tp_cfg.x_max = LCD_HRES;
    tp_cfg.y_max = LCD_VRES;
    tp_cfg.rst_gpio_num = static_cast<gpio_num_t>(-1);
    tp_cfg.int_gpio_num = static_cast<gpio_num_t>(-1);
    tp_cfg.levels.reset = 0;
    tp_cfg.levels.interrupt = 0;
    tp_cfg.flags.swap_xy = 0;
    tp_cfg.flags.mirror_x = 0;
    tp_cfg.flags.mirror_y = 0;

    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tio_handle, &tp_cfg, &touch_handle));

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, myInputRead);
}

void initializeDisplaySystem()
{
    i2cInitialize();
    initializeCh422gBacklightControl();

    sem_frame_buf_complete = xSemaphoreCreateBinary();
    assert(sem_frame_buf_complete);
    sem_vsync = xSemaphoreCreateBinary();
    assert(sem_vsync);

    #if LCD_PIN_NUM_BCKL >= 0
    gpio_config_t bk_gpio_config;
    memset(&bk_gpio_config, 0, sizeof(gpio_config_t));
    bk_gpio_config.mode = GPIO_MODE_OUTPUT;
    bk_gpio_config.pin_bit_mask = 1ULL << LCD_PIN_NUM_BCKL;
    ESP_ERROR_CHECK(gpio_config((gpio_num_t)LCD_PIN_NUM_BCKL, &bk_gpio_config));
    gpio_set_level((gpio_num_t)LCD_PIN_NUM_BCKL, LCD_BCKL_OFF_LEVEL);
    #endif

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config;
    memset(&panel_config, 0, sizeof(esp_lcd_rgb_panel_config_t));

    panel_config.data_width = 16;
    panel_config.num_fbs = 2;
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_config.disp_gpio_num = (gpio_num_t)-1;
    panel_config.pclk_gpio_num = (gpio_num_t)LCD_PIN_NUM_CLK;
    panel_config.vsync_gpio_num = (gpio_num_t)LCD_PIN_NUM_VSYNC;
    panel_config.hsync_gpio_num = (gpio_num_t)LCD_PIN_NUM_HSYNC;
    panel_config.de_gpio_num = (gpio_num_t)LCD_PIN_NUM_DE;
    panel_config.data_gpio_nums[0] = (gpio_num_t)LCD_PIN_NUM_D00;
    panel_config.data_gpio_nums[1] = (gpio_num_t)LCD_PIN_NUM_D01;
    panel_config.data_gpio_nums[2] = (gpio_num_t)LCD_PIN_NUM_D02;
    panel_config.data_gpio_nums[3] = (gpio_num_t)LCD_PIN_NUM_D03;
    panel_config.data_gpio_nums[4] = (gpio_num_t)LCD_PIN_NUM_D04;
    panel_config.data_gpio_nums[5] = (gpio_num_t)LCD_PIN_NUM_D05;
    panel_config.data_gpio_nums[6] = (gpio_num_t)LCD_PIN_NUM_D06;
    panel_config.data_gpio_nums[7] = (gpio_num_t)LCD_PIN_NUM_D07;
    panel_config.data_gpio_nums[8] = (gpio_num_t)LCD_PIN_NUM_D08;
    panel_config.data_gpio_nums[9] = (gpio_num_t)LCD_PIN_NUM_D09;
    panel_config.data_gpio_nums[10] = (gpio_num_t)LCD_PIN_NUM_D10;
    panel_config.data_gpio_nums[11] = (gpio_num_t)LCD_PIN_NUM_D11;
    panel_config.data_gpio_nums[12] = (gpio_num_t)LCD_PIN_NUM_D12;
    panel_config.data_gpio_nums[13] = (gpio_num_t)LCD_PIN_NUM_D13;
    panel_config.data_gpio_nums[14] = (gpio_num_t)LCD_PIN_NUM_D14;
    panel_config.data_gpio_nums[15] = (gpio_num_t)LCD_PIN_NUM_D15;

    memset(&panel_config.timings, 0, sizeof(esp_lcd_rgb_timing_t));
    panel_config.timings.pclk_hz = LCD_PIXEL_CLOCK_HZ;
    panel_config.timings.h_res = LCD_HRES;
    panel_config.timings.v_res = LCD_VRES;
    panel_config.timings.hsync_back_porch = LCD_HSYNC_BACK_PORCH;
    panel_config.timings.hsync_front_porch = LCD_HSYNC_FRONT_PORCH;
    panel_config.timings.hsync_pulse_width = LCD_HSYNC_PULSE_WIDTH;
    panel_config.timings.vsync_back_porch = LCD_VSYNC_BACK_PORCH;
    panel_config.timings.vsync_front_porch = LCD_VSYNC_FRONT_PORCH;
    panel_config.timings.vsync_pulse_width = LCD_VSYNC_PULSE_WIDTH;
    panel_config.timings.flags.pclk_active_neg = true;
    panel_config.timings.flags.hsync_idle_low = false;
    panel_config.timings.flags.pclk_idle_high = LCD_CLK_IDLE_HIGH;
    panel_config.timings.flags.de_idle_high = LCD_DE_IDLE_HIGH;
    panel_config.timings.flags.vsync_idle_low = false;
    panel_config.flags.bb_invalidate_cache = true;
    panel_config.flags.disp_active_low = false;
    panel_config.flags.double_fb = true;
    panel_config.flags.no_fb = false;
    panel_config.flags.refresh_on_demand = false;
    panel_config.flags.fb_in_psram = true;
    panel_config.bounce_buffer_size_px = LCD_HRES * (LCD_VRES / LCD_DIVISOR);

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    lcd_handle = panel_handle;

    #if LCD_PIN_NUM_BCKL >= 0
    gpio_set_level((gpio_num_t)LCD_PIN_NUM_BCKL, LCD_BCKL_ON_LEVEL);
    #endif

    lv_init();
    lv_display_t *display = lv_display_create(LCD_HRES, LCD_VRES);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    // LVGL renders full frames straight into the RGB panel's own two framebuffers, so
    // esp_lcd_panel_draw_bitmap() can hand off via cur_fb_index instead of a CPU copy
    // into whichever buffer the DMA is currently scanning out (see lvglFlush()).
    void *buf1 = NULL;
    void *buf2 = NULL;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
    const size_t draw_buffer_sz = LCD_HRES * LCD_VRES * sizeof(uint16_t);

    assert(buf1);
    assert(buf2);
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(display, lvglFlush);

    esp_lcd_rgb_panel_event_callbacks_t cbs;
    memset(&cbs, 0, sizeof(esp_lcd_rgb_panel_event_callbacks_t));
    cbs.on_vsync = lcdVsyncEvent;
    cbs.on_frame_buf_complete = lcdFrameBufCompleteEvent;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, display));

    esp_timer_create_args_t lvgl_tick_timer_args;
    memset(&lvgl_tick_timer_args, 0, sizeof(esp_timer_create_args_t));
    lvgl_tick_timer_args.callback = &lcdIncreaseLvglTick;
    lvgl_tick_timer_args.name = "lvgl_tick";
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2 * 1000));

    ui_init();
    if (objects.main_but_dim != NULL) {
        lv_obj_add_event_cb(objects.main_but_dim, onDimButtonEvent, LV_EVENT_CLICKED, NULL);
    }
    configureScaleBandsFromThresholds();
    syncArcRangesFromScales();
    applyAllScaleLabels();
    initEngineRpmDotAnimation();
    initArcSmoothingAnimation();
    touchInitialize();
}

void readTouchInput()
{
    // Cap GT911 I2C polling well above human touch response so the main loop
    // isn't forced to do an I2C transaction on every tick.
    static constexpr int64_t TOUCH_POLL_INTERVAL_US = 50 * 1000;
    static int64_t last_poll_time_us = 0;

    if (touch_handle == NULL) {
        return;
    }

    const int64_t now_us = esp_timer_get_time();
    if (now_us - last_poll_time_us < TOUCH_POLL_INTERVAL_US) {
        return;
    }
    last_poll_time_us = now_us;

    esp_lcd_touch_read_data(touch_handle);
}
