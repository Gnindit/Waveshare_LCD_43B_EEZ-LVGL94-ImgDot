"""Apply the missing ESP-IDF 6.0.1 HPM conditional without changing timings.

The upstream flash-driver table references DC-only functions even when DC support
is disabled. Keep the table entry under the same guard as those functions.
The operation is idempotent and only changes this specific upstream source line.
"""
from pathlib import Path


def fix_source(source):
    lines = source.splitlines(keepends=True)
    for index, line in enumerate(lines):
        if '{ "dummy sr3-bit3_4",' not in line:
            continue
        if index and lines[index - 1].strip() == "#if CONFIG_SPI_FLASH_HPM_DC_ON":
            return source
        newline = "\r\n" if line.endswith("\r\n") else "\n"
        lines[index:index + 1] = ["#if CONFIG_SPI_FLASH_HPM_DC_ON" + newline,
                                  line, "#endif //CONFIG_SPI_FLASH_HPM_DC_ON" + newline]
        return "".join(lines)
    return source  # A newer fixed SDK may no longer contain this entry.


if "Import" in globals():
    Import("env")
    framework = env.PioPlatform().get_package_dir("framework-espidf")
    if framework:
        path = Path(framework) / "components/spi_flash/spi_flash_hpm_enable.c"
        if path.exists():
            original = path.read_bytes().decode()
            fixed = fix_source(original)
            if fixed != original:
                path.write_bytes(fixed.encode())
                print("Applied ESP-IDF HPM DC table guard compatibility fix")
