cmake_minimum_required(VERSION 3.13)

if ("rp2350-arm-s" STREQUAL PICO_PLATFORM)
    set(PICO_BOARD pico2_w)  # needed for Pico2-W, otherwise #include "pico/cyw43_arch.h" won't be found
else ()
    set(PICO_BOARD pico_w)  # needed for Pico-W, otherwise #include "pico/cyw43_arch.h" won't be found
endif ()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(LIBDEFLATE_BUILD_SHARED_LIB OFF)
set(LIBDEFLATE_BUILD_GZIP OFF)

set(PICO_USE_STACK_GUARDS 1)

include(pico_sdk_import.cmake)

project(ble_smoke_detector C CXX ASM)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

add_definitions(-DPICO_ENTER_USB_BOOT_ON_EXIT)
add_definitions(-DCYW43_PIO_CLOCK_DIV_DYNAMIC=1)
add_definitions(-DPICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE=CYW43_COUNTRY_UK)

pico_sdk_init()

add_executable(ble_smoke_detector main.cpp
        src/BleConnection.cpp
        src/BleConnectionTracker.cpp
        src/Debugging.cpp
        src/Peer.cpp
        src/Base.cpp
        src/PacketBase.cpp
        src/Announce.cpp
        src/Message.cpp
        src/ProtocolProcessor.cpp
        src/BinaryReader.cpp
        src/BinaryWriter.cpp
)

include_directories(include src)

pico_enable_stdio_usb(ble_smoke_detector 1) # Use this when debugging from a uf2 usb dropped build
pico_enable_stdio_uart(ble_smoke_detector 1) # Use this when debugging through the debugger

target_link_libraries(ble_smoke_detector
        pico_stdlib
        pico_btstack_ble
        pico_btstack_cyw43
        pico_cyw43_arch_none
        hardware_gpio
        hardware_adc
        hardware_pwm
)

target_include_directories(ble_smoke_detector PRIVATE
        ${CMAKE_CURRENT_LIST_DIR} # For btstack config
)
pico_btstack_make_gatt_header(ble_smoke_detector PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble_smoke_detector.gatt")

pico_add_extra_outputs(ble_smoke_detector)
