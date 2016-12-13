# FFmpeg Example
## ffmpeg
## ffplay
## ffprobe
## libavformat
- CMake 빌드를 위한 기본적인 설정 파일 추가
 - cmake 실행 시에 FFmpeg 헤더 참조를 위해 FFMPEG_INCLUDE_DIR 설정 필요
 - cmake 실행 시에 FFmpeg 라이브러리 참조를 위해 FFMPEG_LIB_DIR 설정 필요
- CMake 사용법(**FFMPEG_INCLUDE_DIR와 FFMPEG_LIB_DIR에 FFmpeg 경로설정필요!**)
 - $ mkdir build && cd build
 - $ cmake -D FFMPEG_INCLUDE_DIR="**C:\app\ffmpeg\ffmpeg-3.2-win64-dev\include**" -D FFMPEG_LIB_DIR="**C:\app\ffmpeg\ffmpeg-3.2-win64-dev\lib**" -G "Visual Studio 14 Win64" ../
 - cmake --build .
- 실행방법
  - $ ./input.exe [URL]
- 동작여부
  - 정상동작되는 경우에는 화면에 100번 루프를 돌면서 읽어온 데이터의 크기가 표시
