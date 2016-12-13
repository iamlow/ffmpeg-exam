# FFmpeg Example
## FFmpeg Version
ffmpeg 3.2
## How to build
- CMake 빌드를 위한 기본적인 설정 파일 추가
 - cmake 실행 시에 FFmpeg 헤더 참조를 위해 FFMPEG_INCLUDE_DIR 설정 필요
 - cmake 실행 시에 FFmpeg 라이브러리 참조를 위해 FFMPEG_LIB_DIR 설정 필요
- CMake 사용법(**FFMPEG_INCLUDE_DIR와 FFMPEG_LIB_DIR에 FFmpeg 경로설정필요!**)
 - $ mkdir build && cd build
 - $ cmake -D FFMPEG_INCLUDE_DIR="**C:\app\ffmpeg\ffmpeg-3.2-win64-dev\include**" -D FFMPEG_LIB_DIR="**C:\app\ffmpeg\ffmpeg-3.2-win64-dev\lib**" -G "Visual Studio 14 Win64" ../
 - cmake --build .
## ffmpeg, ffplay, ffprobe
### PC에 카메라와 마이크에서 스트림을 캡처하여 송신
 - ffplay는 mpegts로 수신받은 영상과 음성 데이터를 재생

   ```sh
   $ ffplay -i "udp://127.0.0.1:5000"
   ```
   
 - ffmpeg으로 PC 카메라와 마이크에서 데이터를 가져와서 전송
   - 내 PC에 있는 장치 정보 얻기

     ```sh
     $ ffmpeg -list_devices true -f dshow -i dummy
     [dshow @ 000000000065f580] DirectShow video devices (some may be both video and audio devices)
     [dshow @ 000000000065f580]  "Integrated Camera"
     [dshow @ 000000000065f580]     Alternative name "@device_pnp_\\?\usb"
     [dshow @ 000000000065f580] DirectShow audio devices
     [dshow @ 000000000065f580]  "마이크 배열(Realtek High Definition Audio)"
     [dshow @ 000000000065f580]     Alternative name "@device_cm_"
     dummy: Immediate exit requested
     ```
      
    - 카메라 장치 이름은 "Integrated Camera"이며 오디오 장치 이름은 "마이크 배열(Realtek High Definition Audio)" 이다. 위 이름이 한글이라 ffmpeg에서 인식을 못하면 Alternative name을 사용하면 된다.
  - 내 PC에 있는 카메라와 마이크 장치로부터 미디어를 캡처하여 mpegts로 전송
     
    ```sh
    $ ffmpeg -f dshow -i video="Integrated Camera":audio="@device_cm_" -f mpegts "udp://127.0.0.1:5000"
    ```

## libavformat
### input(libavformat/input.cpp)
- libavformat 라이브러리를 사용하여 네트워크 카메라로 부터 영상 및 음성을 수신하고 데이터의 크기를 화면에 출력하는 예제 프로그램
- 실행방법
 - $ ./input.exe [URL]
- 동작여부
 - 정상동작되는 경우에는 화면에 100번 루프를 돌면서 읽어온 데이터의 크기가 표시
