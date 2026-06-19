cmake_minimum_required(VERSION 3.12)

add_definitions(-DMOCK_PICO_PI)

project(tests)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

find_package(Catch2 REQUIRED)

include_directories(.. ../include ../src)

add_executable(tests
        ../src/BleConnection.cpp
        ../src/BleConnectionTracker.cpp
        ../src/Base.cpp
        ../src/PacketBase.cpp
        ../src/Peer.cpp
        ../src/BinaryReader.cpp
        ../src/BinaryWriter.cpp
        ../src/Message.cpp
        pico_pi_mocks.cpp
        test_ble_smoke_detector.cpp
        test_circular_buffer.cpp
)

target_link_libraries(tests PRIVATE
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(tests)
