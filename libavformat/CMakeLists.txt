project(libavformat)

include_directories(${FFMPEG_INCLUDE_DIR})
add_definitions(-D__STDC_CONSTANT_MACROS)
link_directories(${FFMPEG_LIB_DIR})
add_executable(input input.cpp)
file(GLOB LIBAVFORMAT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*)
target_link_libraries(input avdevice avfilter avformat avcodec swresample swscale avutil postproc)
