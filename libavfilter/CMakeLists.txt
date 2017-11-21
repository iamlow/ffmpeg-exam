project(libavformat)

set(CMAKE_CXX_STANDARD 14)

include_directories(${FFMPEG_INCLUDE_DIR})
include_directories(../wrap)
add_definitions(-D__STDC_CONSTANT_MACROS)
link_directories(${FFMPEG_LIB_DIR})

add_executable(amix amix.cpp)
# file(GLOB LIBAVFORMAT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*)
target_link_libraries(amix avfilter avformat avcodec avutil)

add_executable(scale scale.cpp)
target_link_libraries(scale avfilter avformat avcodec avutil)

add_executable(overlay overlay.cpp)
target_link_libraries(overlay avfilter avformat avcodec avutil)
