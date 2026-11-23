Import("env")

import os

build_dir = env.subst("$BUILD_DIR")
bootloader_dir = os.path.join(build_dir, "bootloader", "ld")

env.Append(
    LIBPATH=[bootloader_dir],
    LINKFLAGS=[f"-L{bootloader_dir}"],
)