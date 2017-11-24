# 영상처리 성능 테스트
## 목적
- CPU에 내장된 GPU의 디코딩/인코딩/비디오 스케일링/비디오 오버레이 성능 파악
## 시나리오
- 3개의 1080p30f 파일을 디코딩 후 스케일링과 오버레이 작업을 거친 후 1개의 1080p30f 로 인코딩
## 테스트 방법
- FFmpeg v3.3 & v3.4 Command Line Tool을 사용하여 진행
- 디코더는 H.264 S/W 사용
    - H.264 H/W 디코더인 h264_vda를 사용하려 했으나 FFmpeg v3.4에서 미지원
    - FFmpeg v3.3에서는 h264_vda를 지원하였으나 첫 프레임 비디오가 깨지는 증상이 있음
- 인코더는 H.264 videotoolbox H/W 사용
- 비디오 스케일링과 오버레이는 S/W 사용
- 테스트 영상은 H.264 Profile High, 1080p30f, Bitrate: 3200k 사용, 음성 AAC 192Kbps 기준
## 테스트 환경
- MacBook Pro 2017 mid 13인치
    - CPU: i5-7360U 2.3GHz(Boost 3.6GHz)
    - GPU: Iris Pro 640 300Hz(Boost 1.0GHz) 48EU
## 측정 방법
- CPU: Intel Power Gadget, htop CPU 점유율
- GPU: Intel Power Gadget GPU Frequency(MacOS GPU 점유율 확인 기능 미제공)
- FFmpeg: 프로그램에서 제공하는 정보 중 인코딩 speed
## 테스트 내역
- FFmpeg v3.3
    - 하드웨어 가속기 지원 여부 확인
    ```sh
    $ ffmpeg -hwaccels
    ~
    Hardware acceleration methods:
    vda
    videotoolbox
    ```
    - H.264 디코더/인코더 지원 여부 확인
    ```sh
    $ ffmpeg -codecs | grep 264
    ~
    DEV.LS h264 H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10 (decoders: h264 h264_vda )
    (encoders: libx264 libx264rgb h264_videotoolbox )
    ```
- FFmpeg v3.4
    - 하드웨어 가속기 지원 여부 확인
    ```sh
    $ ffmpeg -hwaccels
    ~
    Hardware acceleration methods:
    videotoolbox
    ```
    - H.264 디코더/인코더 지원 여부 확인
    ```sh
    $ ffmpeg -codecs | grep 264
    ~
    DEV.LS h264 H.264 / AVC / MPEG-4 AVC / MPEG-4 part 10
    (encoders: libx264 libx264rgb h264_videotoolbox )
    ```
- 시나리오 테스트
    ```sh
    $ ffmpeg -re -i [input1.mkv] -i [input2.mkv] -i [input3.mkv] -filter_complex \
    "[1:v]scale=(iw/2)-20:-1[a]; \
    [2:v]scale=(iw/2)-20:-1[b]; \
    [0:v][a]overlay=10:(main_h/2)-(overlay_h/2):shortest=1[c]; \
    [c][b]overlay=main_w-overlay_w-10:(main_h/2)-(overlay_h/2)[video]; \
    [1:a][2:a]amerge,pan=stereo:c0<c0+c2:c1<c1+c3[audio]" \
    -map "[video]" -map "[audio]" -c:v h264_videotoolbox -b:v 3200k output.mkv
    ```
## 테스트 결과
- FFmpeg 인코딩 Speed 0.99x~1x 기준
- CPU 점유율: 최대 50%
- GPU Frequency: 최대 0.27GHz
## 결론
- 해당 사니리오 정도의 영상처리는 외장 GPU 도움없이 내장 GPU로도 영상처리 가능
- MacOS 아닌 다른 OS(Linux, Windows)에서는 H/W 디코더도 사용가능하면 GPU를 더 활용 가능
## 기타
- NVidia GTX 시리즈와는 다르게 H/W 인코더 인스턴스 개수 제한 없음
- FFmpeg 인코딩 Speed가 0.9x~1x 정도를 유지하며 H/W 인코더 최대 6개까지 동시 사용 가능
