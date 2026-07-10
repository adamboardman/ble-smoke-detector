cmake_minimum_required(VERSION 3.12)

add_definitions(-DMOCK_PICO_PI)

project(tests)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

find_package(Catch2 REQUIRED)

include_directories(. ../src/BLEPacket)

add_executable(tests
        ../src/BLEPacket/BleConnection.cpp
        ../src/BLEPacket/BleConnectionTracker.cpp
        ../src/BLEPacket/Base.cpp
        ../src/BLEPacket/Peer.cpp
        ../src/BLEPacket/BinaryReader.cpp
        ../src/BLEPacket/BinaryWriter.cpp
        ../src/BLEPacket/Message.cpp
        ../src/BLEPacket/Announce.cpp
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
