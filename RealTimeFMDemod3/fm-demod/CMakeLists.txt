add_library(fm-demod "fm-demod.cpp" "fm-demod.hpp")

target_sources(fm-demod
  PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/fm-demod.cpp
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/fm-demod.hpp
  )
target_include_directories(fm-demod
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
  )

find_path(READERWRITERQUEUE_INCLUDE_DIRS "readerwriterqueue/readerwriterqueue.h")

target_include_directories(fm-demod PRIVATE ${READERWRITERQUEUE_INCLUDE_DIRS})
