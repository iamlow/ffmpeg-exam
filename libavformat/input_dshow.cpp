#include <iostream>
#include <string>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}

int main(int argc, char *argv[])
{
    if (2 != argc) {
        std::cout << "invalid argument!\n";
        return 0;
    }

    const std::string uri = argv[1];
    std::cout << "uri: " << uri << std::endl;

    avdevice_register_all();
    av_register_all();

    char errstr[AV_ERROR_MAX_STRING_SIZE];
    int rc = 0;

    AVInputFormat *ifmt = av_find_input_format("dshow");
    if (NULL == ifmt) {
        std::cout << "av_find_input_format:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << std::endl;
        return 0;
    }

    AVFormatContext *fmtCtx = NULL;
    rc = avformat_open_input(&fmtCtx, uri.c_str(), ifmt, NULL);
    if (0 > rc) {
        std::cout << "avformat_open_input:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return 0;
    }

    rc = avformat_find_stream_info(fmtCtx, NULL);
    if (0 > rc) {
        std::cout << "avformat_find_stream_info:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    for (int i = 0; i < 100; ++i) {
        AVPacket packet;
        int rc = av_read_frame(fmtCtx, &packet);
        if (0 > rc) {
            std::cout << "av_read_frame:"
                << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            break;
        }
        std::cout << "read_frame size: " << packet.size << std::endl;
    }

    avformat_close_input(&fmtCtx);

    return 0;
}
