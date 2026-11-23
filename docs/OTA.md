# Screen updates from GitHub Releases

## First installation

Build and flash this OTA-capable version over USB once. A normal application OTA
cannot update the old bootloader to enable rollback. The partition layout is
unchanged: two 6.25 MiB OTA slots in `default_16MB.csv`.

Run from the directory containing `platformio.ini`:

```sh
pio run -e waveshare_lcd_4_3
pio run -e waveshare_lcd_4_3 -t upload
```

The upload installs the generated bootloader, partition table, OTA selection data,
and application. This does not request a full flash erase. Do not erase flash when
you want to retain saved Wi-Fi settings. No device was flashed during development.

## On the Waveshare screen

1. Hold **Update** for at least **3 seconds**, then release. A short tap does nothing.
2. Open **Wi-Fi setup** and enter the same router's network name and password used
   by the other projects. Tap each field to select it on the on-screen keyboard.
   Save the settings. Password entry is masked.
3. The router must have Internet access and use **2.4 GHz, fixed channel 1**.
   Wi-Fi and ESP-NOW share one radio. This firmware creates no access point.
4. Tap **Check for updates**. The display connects on channel 1, synchronizes its
   clock for TLS, and checks the latest published stable release's manifest.
5. If a newer version exists, stop the engine, tap **Install and restart**, then
   **Confirm install**. Keep power connected. Progress appears in the popup.
6. After validation, the board restarts. The new application confirms itself after
   15 seconds of successful display/ESP-NOW initialization and continuing UI work.
   If it resets before confirmation, the rollback-enabled bootloader can return
   to the previous image. An active sensor sender or Internet connection is not
   required to confirm boot health.

Wi-Fi credentials are stored together as a device-local NVS record. They survive
reboot and application updates. They are not compiled into binaries, printed in
logs, or stored in this repository. NVS is not encrypted on this configuration.
The display connects only during a check/install; afterward it disconnects and
returns the radio to ESP-NOW channel 1. Gauge processing stays on the main task;
downloads run in a temporary worker task that never calls LVGL.

The popup, Wi-Fi fields and keyboard are created only when opened and freed when
closed/replaced. No extra framebuffer is allocated. During an operation, menu
actions are disabled. Failed downloads leave the running image selected.

## Automatic GitHub build and release assets

`.github/workflows/firmware.yml` builds and tests pushes to `main` and
`feature/github-ota`, pull requests, and manually started runs. It uploads downloadable
workflow artifacts. Publishing a stable GitHub Release also builds the tagged code
and automatically attaches the firmware files to that release. You do not upload
`.bin` files manually or need a separate repository secret for the GitHub token.

Once the code is merged and pushed, create a new **published, non-prerelease** GitHub
Release targeting the reviewed commit. Use a new tag such as `v1.0.0`, then increment
it for subsequent updates (`v1.0.1`, `v1.1.0`, etc.). Wait for the **Firmware** workflow
to finish before checking from the display. GitHub's automatic ZIP/TAR source
archives are not firmware. The updater rejects a release that is the same version
or older, and stable versions must have three numeric parts with an optional `v`.

Release assets:

| Asset | Purpose |
| --- | --- |
| `firmware.bin` | Application image consumed by the screen updater |
| `firmware.manifest` | Board identity, exact release tag, image size and SHA-256 |
| `waveshare-43b-factory.bin` | Combined USB recovery image; never use for app OTA |
| `SHA256SUMS` | Checksums for the release files |
| `OTA.md` | These operator instructions |
| `LICENSES.zip` | Application, font and bundled component license notices |

The latest manifest is discovered at:

```text
https://github.com/Gnindit/Waveshare_LCD_43B_EEZ-LVGL94-ImgDot/releases/latest/download/firmware.manifest
```

The image is then fetched from `releases/download/<exact-tag>/firmware.bin`.
Publishing a new release during an update cannot switch the selected image to a
different tag. The app descriptor's version and project name must match the
manifest and this display; sensor-project images and merged flash images fail
validation before the OTA slot is erased. HTTPS validates certificates and permits
only the GitHub release/download hosts; SHA-256 verifies the complete image before
the boot slot is changed. Like the current unsigned release flow in the other
projects, this trusts the GitHub repository and HTTPS; the manifest is not signed
with an independent firmware signing key.

For a local reproduction of a release build (does not publish):

```sh
RELEASE_TAG=v1.0.0 pio run -e waveshare_lcd_4_3
RELEASE_TAG=v1.0.0 python scripts/package_release.py
```

The script checks the actual image descriptor and slot size. The combined recovery
image is intended to be written at offset **0x0** on this exact 16 MB ESP32-S3 board.
Prefer the PlatformIO USB upload command for recovery from a source checkout.

## Hardware validation before merging or publishing

Local build/test success does not prove display, radio or OTA behavior on hardware.
Test the development branch on the desktop display first:

- Short tap, a hold shorter than 3 seconds, and sliding off the button do not open
  the menu. A full hold followed by release does; the release cannot confirm install.
- Wi-Fi can be saved, survives restart, and works with the same channel-1 router.
  Wrong password, missing router, wrong channel, no DHCP and no Internet produce
  recoverable errors. The sensor displays continue receiving fresh ESP-NOW data.
- Check menu readability, keyboard focus, password masking and repeated open/close
  behavior. Compare internal/DMA heap and gauge responsiveness before/after HTTPS.
- Check a real stable release, absent/incomplete release assets, same/older version,
  wrong-board image, truncated image, checksum mismatch and interrupted download.
- Install a newer test version, watch reboot and startup confirmation, then verify
  sensor freshness. Reboot an unconfirmed update to exercise rollback and verify
  that USB recovery still works.

References: [ESP-IDF OTA and rollback](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-reference/system/ota.html),
[GitHub release workflow events](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#release).
