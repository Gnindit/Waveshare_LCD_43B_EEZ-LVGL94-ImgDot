# Public source and GitHub Release preparation

## Local audit (2026-11-23)

- Removed the generated LVGL source cache containing a previous Windows user's
  absolute paths. CMake now discovers local LVGL sources directly.
- Kept LVGL Windows drivers and portability headers: these are third-party source,
  not personal Windows leftovers. They are disabled by the ESP32 configuration.
- Moved the device-specific Linux USB ports into ignored `platformio.local.ini`.
  The example file is portable; normal builds can use automatic port detection.
- Ignored EEZ Studio UI state, local credential files, old sdkconfig backups, and
  release output. The already-deleted UI-state file should remain deleted.
- Retained `dependencies.lock`: its current registry dependencies are portable and
  pin the touch component and ESP-IDF versions. It has no local filesystem paths.
- Added the JetBrains Mono font license and third-party notices.
- Bundled the previously global N16R8 board definition in `boards/` so a fresh
  checkout does not depend on a custom board installed on the original machine.
- Pattern scans of the working source and all reachable history found no confirmed
  credentials. Credential-like strings in the EEZ file were embedded font data.
  This is a pattern-based review, not a guarantee that no secret exists.

## Fresh public history

At the owner's request, this cleanup replaces the local history with a single clean
initial commit. Its author/committer use the GitHub handle and GitHub noreply email.
This avoids publishing older personal email metadata, Windows paths, EEZ session
state, and the board USB identifier. The GitHub handle still identifies the account;
it is not anonymous.

The original Git directory and working-tree patches are preserved locally under
`.private-backups/` (ignored by Git). Keep this backup private and copy it to your
normal backup storage. It is not included in the public source. The original
uncommitted dashboard changes are included in the new initial source snapshot.

The configured remote URL is retained locally. Nothing has been pushed, no remote
history has been replaced, and no repository visibility or release has been changed.
An existing remote with old history will need an explicitly approved replacement
or a new repository before this clean history can be published. Old remote commits,
forks, and caches are not removed by resetting local history.

The application uses MIT; bundled third-party code retains its original licenses.
Confirm redistribution rights for dashboard artwork before publication. Before each
publication, inspect `git diff --cached` for unintended local files and credentials,
as well as any newly introduced historical commits.

## Build and prepare release files

From the directory containing `platformio.ini`:

```sh
pio run -e waveshare_lcd_4_3
mkdir -p dist
cp .pio/build/waveshare_lcd_4_3/firmware.bin dist/firmware.bin
(cd dist && sha256sum firmware.bin > firmware.bin.sha256)
```

Build from the reviewed release commit with a clean working tree before preparing
final release assets. Record its commit ID, release tag, build environment and
firmware version in the release notes. Test the resulting firmware on the display
before declaring the release usable. Include the applicable third-party notices
and license texts in the release distribution.

Attach `firmware.bin` and `firmware.bin.sha256` to a GitHub Release manually after
review. The future stable download URL, once a published latest release has that
asset, is:

```text
https://github.com/Gnindit/Waveshare_LCD_43B_EEZ-LVGL94-ImgDot/releases/latest/download/firmware.bin
```

This URL is documented by [GitHub's release linking guide](https://docs.github.com/en/repositories/releasing-projects-on-github/linking-to-releases).
A tag-specific asset URL should be used when an update manifest pins a version.
Do not feed bootloader, partition-table, or merged flash images into an application
OTA updater.

## Local verification

Both the project build and a separate source-only copy built successfully with
`pio run -e waveshare_lcd_4_3`. The copy excluded ignored LVGL demos/examples,
editor state, build output, and local USB settings. Existing host tests passed via
`bash test/run_host_tests.sh`. No firmware was uploaded; hardware behavior and
remote GitHub access were not tested.

## Remaining firmware work for OTA retrieval

The selected `default_16MB.csv` has `otadata` plus `ota_0` and `ota_1`, each 0x640000
bytes (6.25 MiB). Retain these offsets for deployed devices; the unused
`8MB_no_ota.csv` is an alternative layout and is unsuitable for this OTA design.
The cleanup build verified both slots in the generated `partitions.bin`.
The sdkconfig still displays its upstream single-app defaults; PlatformIO uses the
explicit CSV configured in `board_build.partitions` for the generated flash table.
Validate the generated binary again for each release.

The running application currently has no GitHub firmware retrieval/install path.
Before enabling it, implement and test:

- Wi-Fi provisioning that preserves the ESP-NOW channel and receiver behavior.
- HTTPS downloads with certificate validation, redirects, timeouts, and image-size
  limits. Never embed GitHub access tokens in a public firmware image.
- A version/board-specific signed update manifest or signed firmware verification;
  a checksum alone detects corruption but does not authenticate the publisher.
- Writing to the inactive application slot, then checking the new firmware on first
  boot with rollback enabled. Rollback is disabled in the current configuration.
- Recovery over USB and hardware tests for failed downloads, power interruption,
  invalid images, successful boot, and rollback.

See the [ESP-IDF 6.0.1 OTA guide](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-reference/system/ota.html)
for application slots and rollback behavior. Hosting an asset does not by itself
make the installed firmware OTA capable.
