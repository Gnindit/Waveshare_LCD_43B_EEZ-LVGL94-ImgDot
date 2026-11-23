# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

set(app_sources
	main.cpp
	ota_service.cpp
	ota_update.cpp
	display_setup.cpp
	espnow_rx.cpp
	rpm_indicator.cpp
	ui_scale_helpers.cpp
)

# ui/ is intentionally small, so auto-pick all generated and hand-authored C files.
file(GLOB ui_sources
	"${CMAKE_CURRENT_LIST_DIR}/ui/*.c"
)

list(APPEND app_sources ${ui_sources})

idf_component_register(SRCS ${app_sources})
