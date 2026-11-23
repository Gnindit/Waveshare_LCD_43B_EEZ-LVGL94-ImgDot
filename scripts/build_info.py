"""Set the public app descriptor version. Never embed credentials."""
import os
from pathlib import Path
import re
import subprocess

Import("env")

version = os.environ.get("RELEASE_TAG", "")
if version:
    if not re.fullmatch(r"v?(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)", version):
        raise ValueError("RELEASE_TAG must be a stable version such as v1.0.0")
    if len(version) > 31 or any(int(x) > 65535 for x in version.lstrip("v").split(".")):
        raise ValueError("Release version is too long or out of range")
else:
    result = subprocess.run(["git", "describe", "--always", "--dirty"],
                            cwd=env.subst("$PROJECT_DIR"), capture_output=True, text=True)
    revision = result.stdout.strip() if result.returncode == 0 else "local"
    version = ("dev-" + re.sub(r"[^A-Za-z0-9._-]", "_", revision))[:31]
board = env.BoardConfig()
board.update("build.cmake_extra_args", board.get("build.cmake_extra_args", "") + " -DPROJECT_VER=" + version)

# PlatformIO does not invalidate its CMake model when only an extra argument
# changes. Reconfigure on version changes instead of silently reusing an old app
# descriptor when switching between development and release builds.
build = Path(env.subst("$BUILD_DIR"))
cache = build / "CMakeCache.txt"
if cache.exists():
    match = re.search(r"^PROJECT_VER:[^=]+=(.*)$", cache.read_text(), re.M)
    if not match or match.group(1) != version:
        (build / "build.ninja").unlink(missing_ok=True)
