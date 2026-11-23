# Architecture

This project drives a Waveshare ESP32-S3 4.3 inch touch display using ESP-IDF, LVGL, GT911 touch, and ESP-NOW.

The generated UI currently contains one screen (`SCREEN_ID_MAIN`) with:

- RPM arc and label
- Fuel level bar and label
- Water pressure and fuel pressure labels
- Left and right CHT arcs and labels
- Left and right EGT arcs and labels

At a high level, the firmware does four core jobs, now split across dedicated source files:

1. Bring up hardware and LVGL (I2C, LCD, touch, backlight) — [src/display_setup.cpp](../src/display_setup.cpp)
2. Receive and validate ESP-NOW packets, classify sender type (`RS485` vs `ESP32_2`), and apply values to UI widgets with update-rate limiting — [src/espnow_rx.cpp](../src/espnow_rx.cpp)
3. Configure gauge scale color bands and drive the animated RPM dot — [src/ui_scale_helpers.cpp](../src/ui_scale_helpers.cpp)
4. Boot sequencing and memory diagnostics — [src/main.cpp](../src/main.cpp)

Shared packet structs, sensor indexes, thresholds, and UI update intervals live in [src/app_config.h](../src/app_config.h).

## Startup Flow

`app_main()` runs this initialization order:

1. `initializeDisplaySystem()` — I2C init, CH422G backlight control, LCD/LVGL bring-up, `ui_init()`, gauge scale band setup, RPM dot animation init, and `touchInitialize()`
2. `setupEspNowReceiver()`

After startup, the main loop repeatedly:

1. Polls GT911 touch data via `readTouchInput()` (rate-limited to a 50 ms interval)
2. Locks LVGL
3. Runs `lv_timer_handler()` and `ui_tick()`
4. Runs `updateUiFromEspNow()`
5. Unlocks LVGL and delays briefly

The RPM dot animation runs on its own `lv_timer_t` (`RPM_DOT_ANIM_PERIOD_MS`), independent of the main loop cadence.

## Main Building Blocks

### Packet model and sender classification

Packet structs:

- `SensorSnapshot`
- `EspNowBroadcastPacket` (RS485 sender sized)
- `EspNowBroadcastPacketEsp32_2` (ESP32_2 sender sized)

The receiver accepts either packet size, normalizes both into `EspNowBroadcastPacket`, and stores the latest packet in shared state.

`classifySender()` uses packet content to infer source:

- Any non-zero `slaveId` or `registerAddress` => `RS485`
- Exactly two sensors with zero Modbus metadata => `ESP32_2`
- Otherwise => `Unknown`

### Receive path

`onEspNowReceive()` performs these checks before accepting data:

1. Packet size matches one supported struct
2. `magic` matches `ESP_NOW_MAGIC`
3. `version` matches `ESP_NOW_VERSION`
4. `sensorCount` is within `MAX_SENSORS`

When valid, it copies packet + sender type into shared state under a critical section and sets `espnow_packet_ready = true`.

### UI apply path

`updateUiFromEspNow()` runs from the LVGL thread and:

1. Copies latest packet out of shared state
2. Handles RS485 sender timeout fallback
3. Applies values by sender type
4. Respects per-group update intervals to reduce redraw pressure

## Validation And Missing-Data Behavior

The code filters sensor values before rendering:

- Pressure range: -2.0 to 8.0 bar
- CHT range: -30.0 to 200.0 C
- EGT range: -30.0 to 1000.0 C

Helpers:

- `hasUsablePressureSensorValue()`
- `hasUsableTemperatureSensorValue()`
- `isFiniteAndInRange()`

If values are invalid or stale, UI labels switch to placeholders such as `--.-- bar`, `--`, `---`, `--%`, or `----`.

RS485 timeout is enforced by:

- `RS485_PACKET_TIMEOUT_MS = 3000`

If no RS485 packet arrives within 3 seconds, RS485-backed labels are cleared to placeholder text.

## Gauge Thresholds And Colors

Shared palette constants (`UI_HEX_STATE_*` in [src/app_config.h](../src/app_config.h)):

- Nominal: `0x4040FF`
- Caution: `0xFFFF00`
- Danger: `0xFF0000`

### Temperature scales

Thresholds are configured in deg C:

- CHT: cool 61.0, normal 79.0, warn 82.0
- EGT: cool 315.0, normal 676.0, warn 704.0

`configureScaleBandsFromThresholds()` (in [src/ui_scale_helpers.cpp](../src/ui_scale_helpers.cpp)) configures color bands for each temperature and pressure scale.

For each temperature scale:

1. Reads the LVGL scale min/max
2. Converts threshold C values to scale values with `thresholdToScaleValue()`
3. Clamps boundaries to preserve ordering
4. Builds four colored sections: cool, normal, warn, danger

Arc ranges are synced to scale ranges by `syncArcRangesFromScales()`.

### Pressure scales (RPM-dependent low band)

Fuel and water pressure each have a low-band (yellow/caution) threshold that ramps linearly with engine RPM instead of using a fixed cutoff, so idle-normal low pressure doesn't falsely flag as low at high RPM:

- Fuel pressure: ramps between `2000` and `4000` RPM, from `0.1` bar (idle) to `0.53` bar (WOT)
- Water pressure: ramps between `2000` and `4000` RPM, from `0.1` bar (idle) to `0.83` bar (WOT)
- Fuel pressure normal/warn cutoffs: `0.8` / `0.9` bar
- Water pressure normal/warn cutoffs: `2.0` / `2.5` bar

`rampThresholdForRpm()` computes the current low threshold from the latest ESP32_2 RPM reading. The cached threshold is only reapplied to the scale band and arc color (`refreshFuelPressureScaleBand()` / `refreshWaterPressureScaleBand()`) when it changes by more than a small epsilon, to avoid redrawing every tick.

### Fuel and RPM

Fuel level color:

- `< 10%` => danger (`FUEL_COLOR_WARN_MIN_PERCENT`)
- `< 20%` => caution (`FUEL_COLOR_NORMAL_MIN_PERCENT`)
- `>= 20%` => nominal

RPM dot warning color (`updateEngineRpmIndicator()` in `src/rpm_indicator.cpp`):

- `>= 6000` => danger (`RPM_COLOR_WARN_MIN_RPM`)
- `>= 5600` => caution (`RPM_COLOR_CAUTION_MIN_RPM`)
- `< 5600` => original image colors

RPM gauge UI max is `RPM_UI_MAX_RPM = 7000`.

### Animated RPM dot

A small image (`main_img_engine_rpm`) travels along the RPM arc's circular path to give a secondary, glanceable RPM indicator:

- A 200-entry lookup table (`rpmDotPositions()`) precomputes `x, y` positions around the arc (center `198, 200`, radius `175`, sweeping 270° from a 135° start angle), avoiding runtime trig.
- `setEngineRpmDotTarget()` sets the target RPM; a dedicated `lv_timer_t` (`RPM_DOT_ANIM_PERIOD_MS = 20 ms`) exponentially smooths the displayed RPM toward that target (`RPM_DOT_ANIM_SMOOTHING = 0.15`) and snaps once within `1` RPM to avoid endless tiny moves.

The dot starts hidden until the first valid RPM reading. If the sender is silent
for more than 3000 ms, or its RPM sensor is invalid, `clearEngineRpmIndicator()`
hides the dot, shows a red `----` label, and resets the label cache and animation.
When valid RPM returns, even at the same value as before signal loss, the label
is restored and the dot snaps to the first fresh reading before smoothing later
updates. The dot does not depend on the hidden legacy RPM arc.

Host regression checks for recovery, warning thresholds, zero RPM, and label
caching can be run with `bash test/run_host_tests.sh`. These use a small widget
stub; actual dot appearance and panel behavior still need a hardware check.

## Update-Rate Limiting

To reduce redraw load, value groups update on separate intervals (`*_UI_UPDATE_INTERVAL_MS` in [src/app_config.h](../src/app_config.h)):

- RS485 pressure labels/arcs: `250 ms`
- RS485 temperature updates: `1000 ms`
- ESP32_2 fuel: `2000 ms`
- ESP32_2 RPM text: `100 ms`
- ESP32_2 RPM arc: `100 ms`

Temperature updates are phase-split (`rs485_temp_update_phase`) so CHT and EGT groups alternate rather than updating all four arcs in one cycle.

## Display, LVGL, and Touch Integration

### Display and LVGL

`initializeDisplaySystem()` (in [src/display_setup.cpp](../src/display_setup.cpp)) handles:

1. I2C bus init and CH422G backlight expander setup (`initializeCh422gBacklightControl()`)
2. RGB panel and timing configuration
3. LVGL init and display creation, using the RGB panel's own two framebuffers directly (no CPU copy) in `LV_DISPLAY_RENDER_MODE_DIRECT`
4. Flush callback registration (`lvglFlush()`)
5. VSYNC and frame-buffer-complete callback wiring (`lcdVsyncEvent()`, `lcdFrameBufCompleteEvent()`)
6. LVGL tick timer setup (`lcdIncreaseLvglTick()`)
7. UI creation via `ui_init()`, followed by `configureScaleBandsFromThresholds()`, `syncArcRangesFromScales()`, `applyAllScaleLabels()`, and `initEngineRpmDotAnimation()`
8. `touchInitialize()`

`lvglFlush()` uses a binary semaphore (`sem_frame_buf_complete`) to wait for the panel's bounce-buffer refill to switch onto the just-flushed buffer before returning, so the next draw doesn't race the DMA scan-out.

### Backlight control

The board routes LCD backlight enable through the CH422G I/O expander (EXIO2 bit). `main_but_dim` is wired to `onDimButtonEvent()`, which calls `toggleDisplayBacklight()` to flip the backlight on/off via `writeCh422gOutputState()`.

### Touch

`touchInitialize()` creates GT911 over I2C and registers `myInputRead()` as the LVGL indev callback. GT911 reset is bit-banged through the CH422G output register plus a dedicated reset GPIO (`resetTouchController()`).

`readTouchInput()` polls `esp_lcd_touch_read_data()` at most every 50 ms so the main loop doesn't do an I2C transaction on every tick.

`myInputRead()` currently reports pointer pressed/released and coordinates only. It does not implement custom swipe navigation logic.

## ESP-NOW Setup

`setupEspNowReceiver()` uses a lightweight STA-only Wi-Fi setup:

1. Initialize NVS
2. Initialize netif + event loop
3. Create STA netif
4. Init Wi-Fi, set STA mode, start Wi-Fi
5. Lock channel to `ESP_NOW_CHANNEL`
6. Init ESP-NOW and register receive callback

## Practical Mental Model

Think about the architecture as two synchronized paths:

- Receive path: `onEspNowReceive()` validates and stores latest packet
- Render path: main loop + `updateUiFromEspNow()` safely applies data to LVGL widgets

That separation keeps radio callback work short and pushes all UI mutations onto the LVGL thread.