cmake_minimum_required(VERSION 3.12)

# Add the custom boards directory
set(PICO_BOARD_HEADER_DIRS ${CMAKE_SOURCE_DIR}/boards)

# Set custom board
set(PICO_BOARD stepad)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import_optional.cmake)

project(stepad_firmware_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

# include(example_auto_set_url.cmake)
# # Add blink example
# add_subdirectory(blink)

# # Add hello world example
# add_subdirectory(hello_world) 

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

include_directories(inc)
set(SRC_DIR src)

add_executable(stepad_firmware
  ${SRC_DIR}/main.c
  ${SRC_DIR}/switch.c
  ${SRC_DIR}/lekker_switch.c
  ${SRC_DIR}/digital_switch.c
  ${SRC_DIR}/led_matrix.c
  ${SRC_DIR}/led_effects.c
  ${SRC_DIR}/settings.c
  ${SRC_DIR}/console.c
  ${SRC_DIR}/usb_descriptors.c
  ${SRC_DIR}/usb_hid.c
  ${SRC_DIR}/keymap.c
  ${SRC_DIR}/interface_hid.c
)

# Enable stdio prints over UART
pico_enable_stdio_uart(stepad_firmware 1)

# Enable outputs of bin, hex and map files 
pico_add_extra_outputs(stepad_firmware)

target_link_libraries(stepad_firmware 
  pico_stdlib
  pico_multicore
  pico_unique_id
  hardware_adc 
  hardware_dma
  hardware_pwm
  hardware_interp
  hardware_flash
  tinyusb_device
  tinyusb_board
)
