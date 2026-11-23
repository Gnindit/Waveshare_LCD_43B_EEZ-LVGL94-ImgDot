"""Validate release identity/manifest creation without flashing or a network."""
import importlib.util
from pathlib import Path
import struct
import runpy
import tempfile
import unittest
from unittest import mock

spec = importlib.util.spec_from_file_location(
    "package_release", Path(__file__).resolve().parents[1] / "scripts/package_release.py")
package = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package)


def application(version="v1.0.0", project="waveshare_lcd_43"):
    image = bytearray(1024)
    image[0] = 0xE9
    struct.pack_into("<H", image, 12, 9)
    struct.pack_into("<I", image, 32, 0xABCD5432)
    image[48:48 + len(version)] = version.encode()
    image[80:80 + len(project)] = project.encode()
    return bytes(image)


class ReleaseTests(unittest.TestCase):
    def test_manifest_matches_descriptor(self):
        text = package.manifest_text(application(), "v1.0.0")
        self.assertIn("device=waveshare-43b-imgdot\n", text)
        self.assertIn("size=1024\n", text)
        self.assertIn("image=firmware.bin\n", text)
        with self.assertRaises(ValueError):
            package.manifest_text(application(), "v1.0.1")

    def test_wrong_image_is_rejected(self):
        for image in [b"", application(project="esp32_sensor"),
                      bytes(1024), application()[:200]]:
            with self.assertRaises(ValueError):
                package.image_version(image)

    def test_release_versions(self):
        for version in ["v1.0.0", "2.3.4", "v65535.0.0"]:
            self.assertTrue(package.stable_version(version))
        for version in ["../x", "v1.0.0-rc1", "v01.0.0", "v65536.0.0", "v1.0"]:
            self.assertFalse(package.stable_version(version))

    def test_changing_tag_invalidates_cmake_model(self):
        class Board:
            def __init__(self):
                self.args = "-DANOTHER_OPTION=1"
            def get(self, key, default):
                return self.args
            def update(self, key, value):
                self.args = value

        class Environment:
            def __init__(self, build):
                self.build = build
                self.board = Board()
            def subst(self, key):
                return str(self.build)
            def BoardConfig(self):
                return self.board

        script = Path(__file__).resolve().parents[1] / "scripts/build_info.py"
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            cache = build / "CMakeCache.txt"
            ninja = build / "build.ninja"
            cache.write_text("PROJECT_VER:UNINITIALIZED=v1.0.0\n")
            ninja.write_text("existing build")
            with mock.patch.dict("os.environ", {"RELEASE_TAG": "v1.0.0"}):
                env = Environment(build)
                runpy.run_path(str(script), init_globals={"env": env, "Import": lambda *_: None})
                self.assertTrue(ninja.exists())
                self.assertIn("-DANOTHER_OPTION=1", env.board.args)
            with mock.patch.dict("os.environ", {"RELEASE_TAG": "v1.0.1"}):
                env = Environment(build)
                runpy.run_path(str(script), init_globals={"env": env, "Import": lambda *_: None})
                self.assertFalse(ninja.exists())
                self.assertIn("-DPROJECT_VER=v1.0.1", env.board.args)


if __name__ == "__main__":
    unittest.main()
