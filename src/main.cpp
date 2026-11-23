#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_timer.h>
#include <esp_idf_version.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_now.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_wifi.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <nvs_flash.h>
#include <string.h>
#include <cmath>
#include "lcd_config.h"
#include <lvgl.h>
#include "ui/ui.h"
#include "espnow_rx.h"
#include "ota_update.h"
#include "display_setup.h"
#include "ui_scale_helpers.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_gt911.h"

#include <esp_err.h>
#include <esp_log.h>

// For printMemoryStats
#include "esp_heap_caps.h"



// For printMemoryStats
static const char *TAG = "MEMORY_MONITOR";

// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;


// --- Diagnostics and app entry ---
// This section keeps the boot sequence readable: initialize hardware first,
// then bring up the UI and start listening for sensor updates.
void printMemoryStats(void) {
    ESP_LOGI(TAG, "=== ESP32-S3 MEMORY REPORT ===");

    // 1. Total Internal SRAM
    size_t free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t total_sram = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    size_t alloc_sram = total_sram - free_sram;

    ESP_LOGI(TAG, "SRAM: Total: %d KB | Allocated: %d KB | Free: %d KB", 
             total_sram / 1024, alloc_sram / 1024, free_sram / 1024);

    // 2. External PSRAM
    if (heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0) {
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        size_t alloc_psram = total_psram - free_psram;

        ESP_LOGI(TAG, "PSRAM: Total: %d KB | Allocated: %d KB | Free: %d KB", 
                 total_psram / 1024, alloc_psram / 1024, free_psram / 1024);
    } else {
        ESP_LOGW(TAG, "PSRAM: Not available or not enabled.");
    }

    // 3. Largest Free Block
    uint32_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "Largest Contiguous Free Block (SRAM): %d bytes", largest_block);

    size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    size_t total_dma = heap_caps_get_total_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    size_t largest_dma = heap_caps_get_largest_free_block(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "DMA SRAM: Total: %d KB | Free: %d KB | Largest: %d bytes",
    total_dma / 1024, free_dma / 1024, largest_dma);
}

extern "C" void app_main() {
    // Delay a bit to allow the serial monitor to connect before printing logs
    vTaskDelay(pdMS_TO_TICKS(300)); 

    printf("ESP-IDF version: %d.%d.%d\n",ESP_IDF_VERSION_MAJOR,ESP_IDF_VERSION_MINOR,ESP_IDF_VERSION_PATCH);
    printMemoryStats();

    initializeDisplaySystem();
    printf("DISPLAY OK\n");
    printMemoryStats(); 

    setupEspNowReceiver();
    printf("ESPNOW OK\n");
    OtaUpdate::begin();
    printMemoryStats(); 

/*     while(1) {
        readTouchInput();
        _lock_acquire(&lvgl_api_lock);
        lv_timer_handler();
        ui_tick();
        updateUiFromEspNow();
        OtaUpdate::tick();
        _lock_release(&lvgl_api_lock);

        vTaskDelay(5);
    } */
    while(1) {
        readTouchInput();
        
        _lock_acquire(&lvgl_api_lock);
        
        // 1. Capture the exact time until the next timer/refresh is needed
        uint32_t time_until_next = lv_timer_handler();
        
        ui_tick();
        updateUiFromEspNow();
        OtaUpdate::tick();
        
        _lock_release(&lvgl_api_lock);

        // 2. Bound the delay safely. 
        // Don't sleep more than 25ms (to keep ESP-NOW responsive)
        // Don't sleep less than 1ms (to prevent watchdog triggers)
        if(time_until_next == 0) {
            time_until_next = 1; // Run immediately next tick if a timer is ready
        } else if(time_until_next > 25) {
            time_until_next = 25; // Cap at 25ms so ESP-NOW doesn't lag
        }

        vTaskDelay(pdMS_TO_TICKS(time_until_next));
    }
}