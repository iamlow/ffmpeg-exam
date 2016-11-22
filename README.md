# FFmpeg Example
## ffmpeg
## ffplay
## ffprobe
## libavformat
- CMake 빌드를 위한 기본적인 설정 파일 추가
 - cmake 실행 시에 FFmpeg 헤더 참조를 위해 FFMPEG_INCLUDE_DIR 설정 필요
 - cmake 실행 시에 FFmpeg 라이브러리 참조를 위해 FFMPEG_LIB_DIR 설정 필요
- CMake 사용법
 - $ mkdir build && cd build
 - $ cmake -D FFMPEG_INCLUDE_DIR="ffmpeg-3.2-win64-dev\include" -D FFMPEG_LIB_DIR="ffmpeg-3.2-win64-dev\lib" -G "Visual Studio 14 Win64" ../
 - cmake --build .
