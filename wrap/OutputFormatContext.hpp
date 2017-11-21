#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

class OutputFormatContext {
    OutputFormatContext(const std::string& filename) noexcept : filename_(filename) {
        std::cout << "Hello!!" << '\n';
    }

public:
    static std::shared_ptr<OutputFormatContext> create(
                const std::string& filename, const AVCodecContext *codec_ctx) {
        std::shared_ptr<OutputFormatContext> ifc(new OutputFormatContext(filename));
        int rc = ifc->open(filename, codec_ctx);
        if (0 > rc) {
            return nullptr;
        }
        return ifc;
    }

    int write(AVPacket *packet) {
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

    virtual ~OutputFormatContext() {
        std::cout << "ByeBye!" << '\n';
        avformat_close_input(&fmt_ctx_);
    }

private:
    int open(const std::string& filename, const AVCodecContext *codec_ctx) {
        int rc = 0;

        avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, filename.data());
        if (!fmt_ctx_) {
            std::cout << "avformat_alloc_output_context2:" << std::endl;
            return -1;
        }

        AVStream *stream = avformat_new_stream(fmt_ctx_, nullptr);
        if (!stream) {
            std::cout << "avformat_new_stream:" << std::endl;
            return -1;
        }

        /* copy the stream parameters to the muxer */
        rc = avcodec_parameters_from_context(stream->codecpar, codec_ctx);
        if (rc < 0) {
            std::cout << "avcodec_parameters_from_context:"
                << av_make_error_string(errstr_, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            return rc;
        }

        if (fmt_ctx_->oformat->flags & AVFMT_GLOBALHEADER)
            fmt_ctx_->flags |= CODEC_FLAG_GLOBAL_HEADER;

        /** Open the output file to write to it. */
        rc = avio_open(&fmt_ctx_->pb, filename.data(), AVIO_FLAG_WRITE);
        if (rc < 0) {
            std::cout << "avio_open:"
                << av_make_error_string(errstr_, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            return rc;
        }

        av_dump_format(fmt_ctx_, 0, filename.data(), 1);

        rc = avformat_write_header(fmt_ctx_, nullptr);
        if (rc < 0) {
            std::cout << "avformat_write_header:"
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
