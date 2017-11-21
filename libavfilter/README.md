# FFmpeg Example
## FFmpeg Version
ffmpeg 3.2
## How to build
- Environment: macOS High Sierra
- FFmpeg 설치
  ```sh
  $ brew install ffmpeg --with-sdl2
  ```
- CMake 설치
  ```sh
  $ brew install cmake
  ```
 - Build
   ```sh
   $ mkdir build && cd build
   $ cmake --build .
   ```

## libavfiter
### amix(amix.cpp)
- 시나리오
  - libavformat 라이브러리를 사용하여 두 개의 음성 파일을 읽기
  - libavcodec 라이브러리를 사용하여 디코딩 및 인코딩
  - libavfilter 라이브러리를 사용하여 두 개의 음성을 믹싱
- ffmpeg Command line tool
  ```sh
  $ ffmpeg -I [INPUT_FILE1] -I [INPUT_FILE2] -filter_complex amix=inputs=2 [OUTPUT_FILE] -v trace
  ```
- 실행방법
  ```sh
  $ ./amix [INPUT_FILE1] [INPUT_FILE2] [OUTPUT_FILE]
  ```
- 참고
  - AVStream에 codec이 deprecated 되었음. codecpar을 사용해야 함
  - codecpar를 사용하려면 추가로 avcodec_parameters_XXX 함수들을 사용해야 함
  - 코덱 인코딩 디코딩을 위한 API 추가됨(https://www.ffmpeg.org/doxygen/3.1/group__lavc__encdec.html)

### scale(scale.cpp)
- 시나리오
  - libavformat 라이브러리를 사용하여 하나의 영상 파일을 읽기
  - libavcodec 라이브러리를 사용하여 디코딩 및 인코딩
  - libavfilter 라이브러리를 사용하여 영상 해상도 변경
- ffmpeg Command line tool
  ```sh
  $ ffmpeg -i [INPUT_FILE] -vf scale=500:200 [OUTPUT_FILE] -v trace
  ```
- 실행방법
  ```sh
  $ ./scale [INPUT_FILE] [OUTPUT_FILE]
  ```
