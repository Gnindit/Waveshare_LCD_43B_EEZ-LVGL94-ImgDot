"""Package the verified application, manifest and USB recovery image; never flash."""
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import zipfile

DEVICE = "waveshare-43b-imgdot"
PROJECT = "waveshare_lcd_43"
SLOT_SIZE = 0x640000


def image_version(image):
    if len(image) < 288 or len(image) > SLOT_SIZE:
        raise ValueError("Application does not fit the OTA slot")
    if image[0] != 0xE9 or struct.unpack_from("<H", image, 12)[0] != 9:
        raise ValueError("Not an ESP32-S3 application")
    if struct.unpack_from("<I", image, 32)[0] != 0xABCD5432:
        raise ValueError("Missing application descriptor (bootloader/merged image?)")
    version = image[48:80].split(b"\0", 1)[0].decode("ascii")
    project = image[80:112].split(b"\0", 1)[0].decode("ascii")
    if project != PROJECT:
        raise ValueError("Wrong application project")
    return version


def stable_version(version):
    return (len(version) <= 31 and
            re.fullmatch(r"v?(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version) and
            all(int(part) <= 65535 for part in version.lstrip("v").split(".")))


def manifest_text(image, version):
    if not stable_version(version) or image_version(image) != version:
        raise ValueError("Manifest version must match the stable application version")
    return (f"format=1\ndevice={DEVICE}\nversion={version}\nsize={len(image)}\n"
            f"sha256={hashlib.sha256(image).hexdigest()}\nimage=firmware.bin\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("dist"))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    build = root / ".pio/build/waveshare_lcd_4_3"
    image = (build / "firmware.bin").read_bytes()
    version = image_version(image)
    release_tag = os.environ.get("RELEASE_TAG", "")
    if release_tag and (not stable_version(release_tag) or version != release_tag):
        raise ValueError("RELEASE_TAG and firmware descriptor do not match")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    # Prevent an old manifest from leaking into a development artifact.
    (output / "firmware.manifest").unlink(missing_ok=True)
    (output / "firmware.bin").write_bytes(image)
    if stable_version(version):
        (output / "firmware.manifest").write_text(manifest_text(image, version))

    # PlatformIO's merge command respects the actual generated flash offsets/options.
    # Use explicit IDF images here; ota_data_initial.bin initializes OTA selection.
    core = Path(os.environ.get("PLATFORMIO_CORE_DIR", Path.home() / ".platformio"))
    esptool = core / "packages/tool-esptoolpy/esptool.py"
    subprocess.run([
        sys.executable, str(esptool), "--chip", "esp32s3", "merge_bin",
        "--output", str(output / "waveshare-43b-factory.bin"),
        "--flash_mode", "qio", "--flash_freq", "80m", "--flash_size", "16MB",
        "0x0", str(build / "bootloader.bin"),
        "0x8000", str(build / "partitions.bin"),
        "0xe000", str(build / "ota_data_initial.bin"),
        "0x10000", str(build / "firmware.bin"),
    ], check=True)
    shutil.copyfile(root / "docs/OTA.md", output / "OTA.md")
    with zipfile.ZipFile(output / "LICENSES.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        for path in (root / "LICENSE", root / "THIRD_PARTY.md", root / "licenses/JetBrainsMono-OFL.txt"):
            archive.write(path, path.relative_to(root))
        for component in (root / "components", root / "managed_components"):
            for path in sorted(component.rglob("*")):
                if path.is_file() and ("licen" in path.name.lower() or path.name == "COPYRIGHTS.md"):
                    archive.write(path, path.relative_to(root))
    names = ["firmware.bin", "waveshare-43b-factory.bin", "OTA.md", "LICENSES.zip"]
    if (output / "firmware.manifest").exists():
        names.append("firmware.manifest")
    (output / "SHA256SUMS").write_text("".join(
        f"{hashlib.sha256((output / name).read_bytes()).hexdigest()}  {name}\n" for name in names
    ))
    print(f"Packaged {version} in {output}")


if __name__ == "__main__":
    main()
