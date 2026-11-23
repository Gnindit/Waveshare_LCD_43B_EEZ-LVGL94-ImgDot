set(SRCS_DIR "src")

set(CPP_SRCS
    ${SRCS_DIR}/ESP_IOExpander.cpp
    ${SRCS_DIR}/chip/CH422G.cpp
    ${SRCS_DIR}/chip/HT8574.cpp
    ${SRCS_DIR}/chip/TCA95xx_16bit.cpp
    ${SRCS_DIR}/chip/TCA95xx_8bit.cpp
    ${SRCS_DIR}/private/CheckResult.cpp
)

set(C_SRCS
    ${SRCS_DIR}/base/esp_io_expander.c
)

idf_component_register(
    SRCS
        ${C_SRCS}
        ${CPP_SRCS}
    INCLUDE_DIRS
        ${SRCS_DIR}
    REQUIRES
        driver
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
