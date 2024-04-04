cmake_minimum_required(VERSION 3.8)

project("main")

find_package(rtlsdr CONFIG REQUIRED)
find_package(rtaudio CONFIG REQUIRED)
find_path(READERWRITERQUEUE_INCLUDE_DIRS "readerwriterqueue.h")

add_subdirectory(rtl-sdr-helpers)
add_subdirectory(fm-demod)

add_executable(main "main.cpp")

target_include_directories(main PRIVATE ${READERWRITERQUEUE_INCLUDE_DIRS})
target_link_libraries(main PRIVATE rtlsdr::rtlsdr)
target_link_libraries(main PRIVATE RtAudio::rtaudio)
target_link_libraries(main PRIVATE rtl-sdr-helpers)
target_link_libraries(main PRIVATE fm-demod)