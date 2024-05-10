cmake_minimum_required(VERSION 3.12)

set(CMAKE_TOOLCHAIN_FILE "C:/Users/AbdiMPC/Desktop/ESE5190/pico/pico-sdk/cmake/preload/toolchains/pico_arm_gcc.cmake" CACHE STRING "Toolchain file")


include(pico_sdk_import.cmake)
include(edge-impulse-sdk/cmake/utils.cmake)


# Add the directory containing TinyUSB source files
# include_directories(C:/Users/AbdiMPC/Desktop/ESE5190/pico/pico-sdk/lib/tinyusb/src)

project(rp2040_arducam C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)


pico_sdk_init()


# Add TinyUSB source files explicitly if they are not added by the SDK

# Add the specific TinyUSB source files required for your project
# file(GLOB TINYUSB_SOURCES C:/Users/AbdiMPC/Desktop/ESE5190/pico/pico-sdk/lib/tinyusb/src/tusb.c
#                            C:/Users/AbdiMPC/Desktop/ESE5190/pico/pico-sdk/lib/tinyusb/src/class/cdc/cdc_device.c
#                            C:/Users/AbdiMPC/Desktop/ESE5190/pico/pico-sdk/lib/tinyusb/src/device/usbd.c)


add_executable(HHR
	arducam/arducam.c
	main.c
	lib/st7735.c
	lib/fonts.c
	lib/DEV_Config.c
	lib/draw.c
	lib/pio_i2c.c
	lib/lsm6ds3.c
	# ${TINYUSB_SOURCES}
)

pico_generate_pio_header((HHR ${CMAKE_CURRENT_LIST_DIR}/image.pio)
pico_generate_pio_header((HHR ${CMAKE_CURRENT_LIST_DIR}/lib/i2c.pio)

target_link_libraries((HHR
	pico_stdlib
	hardware_dma
	hardware_i2c
	hardware_pio
	hardware_pwm
	hardware_spi
	pico_multicore
	# tinyusb_device 
	# tinyusb_board
)

pico_enable_stdio_usb((HHR 1)
pico_enable_stdio_uart((HHR 0)

# Enable TinyUSB device and CDC class

# target_compile_definitions(AR_racing PRIVATE
#     TINYUSB_DEVICE_ENABLED=1
#     CFG_TUD_CDC=1
# )
# # Debug messages to verify paths
# message(STATUS "TinyUSB headers: ${TINYUSB_HEADERS}")
# message(STATUS "TinyUSB sources: ${TINYUSB_SOURCES}")

pico_add_extra_outputs(HHR)