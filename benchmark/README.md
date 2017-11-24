# 영상처리 성능 테스트
## 목적
- CPU에 내장된 GPU의 디코딩/인코딩/비디오 스케일링/비디오 오버레이 성능 파악
## 시나리오
- 3개의 1080p30f 파일을 디코딩 후 스케일링과 오버레이 작업을 거친 후 1개의 비디오로 인코딩
## 테스트 방법
- FFmpeg v3.4 Command Line Tool을 사용하여 진행
- 디코더는 H.264 S/W 사용
    - H.264 H/W 디코더인 h264_vda를 사용하려 했으나 FFmpeg v3.4에서 미지원
    - FFmpeg v3.3에서는 h264_vda를 지원하였으나 첫 프레임 비디오가 깨지는 증상이 있음
- 인코더는 H.264 videotoolbox H/W 사용
- 비디오 스케일링과 오버레이는 S/W 사용
## 테스트 환경
- MacBook Pro 2017 mid 13인치
    - CPU: i5-7360U 2.3GHz(Boost 3.6GHz)
    - GPU: Iris Pro 640 300Hz(Boost 1.0GHz) 48EU
## 측정 방법
- CPU: Intel Power Gadget, htop CPU 점유율
- GPU: Intel Power Gadget GPU Frequency(GPU 점유율 확인 기능 미제공)
- FFmpeg: 프로그램에서 제공하는 정보 중 인코딩 speed
## 테스트 결과
- CPU 점유율: 최대 50%
- GPU Frequency: 최대 0.18GHz
