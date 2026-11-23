#pragma once

namespace OtaUpdate {
// Before Wi-Fi starts; the existing ESP-NOW setup remains the sole radio owner.
void initializeNetwork();
// After display and ESP-NOW startup, called only from the LVGL/main task.
void begin();
void tick();
} // namespace OtaUpdate
