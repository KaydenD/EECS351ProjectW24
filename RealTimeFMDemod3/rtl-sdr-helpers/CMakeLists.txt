find_package(rtlsdr CONFIG REQUIRED)

add_library(rtl-sdr-helpers rtl-sdr-helpers.cpp rtl-sdr-helpers.hpp)

target_sources(rtl-sdr-helpers
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/rtl-sdr-helpers.cpp
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/rtl-sdr-helpers.hpp
  )
target_include_directories(rtl-sdr-helpers
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
  )

target_link_libraries(rtl-sdr-helpers PRIVATE rtlsdr::rtlsdr)