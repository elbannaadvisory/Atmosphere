cmake_minimum_required(VERSION 3.16)
project(atmosphere_usb_loader_prototype LANGUAGES CXX)

add_library(usb_loader_block_device INTERFACE)
target_include_directories(usb_loader_block_device INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(usb_loader_block_device INTERFACE cxx_std_17)

include(CTest)
if(BUILD_TESTING)
    add_executable(usb_loader_block_device_tests tests/block_device_tests.cpp)
    target_link_libraries(usb_loader_block_device_tests PRIVATE usb_loader_block_device)
    add_test(NAME usb_loader_block_device_tests COMMAND usb_loader_block_device_tests)

    add_executable(usb_loader_scsi_disk_tests tests/scsi_disk_tests.cpp)
    target_link_libraries(usb_loader_scsi_disk_tests PRIVATE usb_loader_block_device)
    add_test(NAME usb_loader_scsi_disk_tests COMMAND usb_loader_scsi_disk_tests)
endif()
