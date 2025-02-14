# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(ProjectArc C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(ProjectArc ProjectArc.c )

pico_set_program_name(ProjectArc "ProjectArc")
pico_set_program_version(ProjectArc "0.1")

# Modify the below lines to enable/disable output over UART/USB

pico_enable_stdio_usb(ProjectArc 1)
pico_enable_stdio_uart(ProjectArc 1)
target_compile_definitions(ProjectArc PRIVATE
    LWIP_HTTPD=1
    LWIP_HTTP_CLIENT=1
)

# Add the standard library to the build
target_link_libraries(ProjectArc pico_stdlib cyw43-driver lwip) 

include_directories(${PICO_SDK_PATH}/src/rp2_common/cyw43_arch)
link_directories(${PICO_SDK_PATH}/lib)


# Add the standard include files to the build
target_include_directories(ProjectArc PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(ProjectArc 
        hardware_spi
        hardware_i2c
        pico_cyw43_arch_lwip_threadsafe_background
        
        )

pico_add_extra_outputs(ProjectArc)

