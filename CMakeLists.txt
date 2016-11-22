cmake_minimum_required(VERSION 3.0)

project(ffmpeg_examples)
include_directories(${FFMPEG_INCLUDE_DIR})
add_definitions(-D__STDC_CONSTANT_MACROS)
link_directories(${FFMPEG_LIB_DIR})
add_executable(libavformat libavformat.cpp)
file(GLOB LIBAVFORMAT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*)
target_link_libraries(libavformat avdevice avfilter avformat avcodec swresample swscale avutil postproc)
