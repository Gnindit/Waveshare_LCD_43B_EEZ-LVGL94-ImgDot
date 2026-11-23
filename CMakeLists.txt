cmake_minimum_required(VERSION 3.16.0)

# Force the bootloader subproject linker to locate bootloader.ld in its build output.
set(BOOTLOADER_EXTRA_LINKER_SCRIPTS "${CMAKE_BINARY_DIR}/bootloader/ld/bootloader.ld")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(waveshare_lcd_43)
