#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

class InputFormatContext {
    InputFormatContext(const std::string& filename) noexcept : filename_(filename) {
        std::cout << "Hello!!" << '\n';
    }

public:
    static std::shared_ptr<InputFormatContext> create(const std::string& filename) {
        std::shared_ptr<InputFormatContext> ifc(new InputFormatContext(filename));
        int rc = ifc->open(filename);
        if (0 > rc) {
            return nullptr;
        }
        return ifc;
    }

    int read(AVPacket *packet) {
        int rc = av_read_frame(fmt_ctx_, packet);
        if (0 > rc) {
            std::cout << "av_read_frame:"
                << av_make_error_string(errstr_, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            return rc;
        }

        std::cout << "av_read_frame packet"
                << " stream_index: " << packet->stream_index
                << " frame size: " << packet->size << std::endl;

        return rc;
    }

    AVFormatContext* data() {
        return fmt_ctx_;
    }

    virtual ~InputFormatContext() {
        std::cout << "ByeBye!" << '\n';
        avformat_close_input(&fmt_ctx_);
    }

private:
    int open(const std::string& filename) {
        int rc = 0;

        rc = avformat_open_input(&fmt_ctx_, filename.c_str(), nullptr, nullptr);
        if (0 > rc) {
            std::cout << "avformat_open_input:"
                << av_make_error_string(errstr_, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            return rc;
        }

        av_dump_format(fmt_ctx_, 0, filename.c_str(), 0);

        rc = avformat_find_stream_info(fmt_ctx_, nullptr);
        if (0 > rc) {
            std::cout << "avformat_find_stream_info:"
                << av_make_error_string(errstr_, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            return rc;
        }

        return rc;
    }

private:
    char errstr_[AV_ERROR_MAX_STRING_SIZE];
    AVFormatContext *fmt_ctx_{nullptr};
    std::string filename_;
};
