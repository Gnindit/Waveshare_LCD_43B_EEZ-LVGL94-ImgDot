#pragma once

// Call on the LVGL/UI task only, after the generated widgets are created.
void updateEngineRpmIndicator(int rpm);
void clearEngineRpmIndicator();
