# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 1.5.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
include(${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
# ====================================================================================
cmake_minimum_required(VERSION 3.21)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico-projects C CXX ASM)

pico_sdk_init()

add_executable(joystick
   joystick.c
)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(joystick 0)
pico_enable_stdio_usb(joystick 1)

target_link_libraries(joystick pico_stdlib hardware_adc hardware_pwm)

pico_add_extra_outputs(joystick)