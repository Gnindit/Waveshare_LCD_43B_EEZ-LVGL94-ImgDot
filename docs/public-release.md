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

## Automatic firmware releases

The OTA branch adds `.github/workflows/firmware.yml`. It builds/tests the source
and attaches firmware, the board/version/checksum manifest, a USB recovery image,
and notices automatically when a stable GitHub Release is published. No manual
binary upload is required. See [OTA.md](OTA.md) for release tags, asset names,
first USB installation, Wi-Fi setup and hardware test steps.

The generated partition binary was checked against `default_16MB.csv`: `otadata`
plus two application slots of 0x640000 bytes, with unchanged offsets. The updater
writes the inactive slot and enables bootloader rollback. Credentials stay in NVS.

## Verification boundary

The public-source cleanup was built and tested locally. The OTA branch must also
pass host policy/packaging tests, the firmware build, a tagged-version packaging
check, and the screen/radio/rollback hardware tests listed in [OTA.md](OTA.md).
A successful local build does not verify GitHub Actions or a hardware installation.
