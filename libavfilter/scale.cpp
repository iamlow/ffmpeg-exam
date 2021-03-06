#include <array>
#include <iostream>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
// #include <libavutil/opt.h>
}

static char errstr[AV_ERROR_MAX_STRING_SIZE];

static AVFormatContext* input_file_init(const std::string& filename) {
    AVFormatContext *fmt_ctx = nullptr;
    int rc = avformat_open_input(&fmt_ctx, filename.c_str(), NULL, NULL);
    if (0 > rc) {
        std::cout << "avformat_open_input:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return nullptr;
    }
    av_dump_format(fmt_ctx, 0, filename.c_str(), 0);

    rc = avformat_find_stream_info(fmt_ctx, NULL);
    if (0 > rc) {
        std::cout << "avformat_find_stream_info:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return nullptr;
    }

    return fmt_ctx;
}

static AVCodecContext* decoder_init(AVFormatContext *fmt_ctx) {
    int rc = 0;

    // Codec
    // XXX note: 'codec' has been explicitly marked deprecated here
    // AVCodec* inCodec1 = avcodec_find_decoder(inFmtCtx1->streams[0]->codec->codec_id);
    AVCodec* codec = avcodec_find_decoder(fmt_ctx->streams[0]->codecpar->codec_id);
    if (nullptr == codec) {
        std::cout << "avcodec_find_decoder:" << std::endl;
        return nullptr;
    }

    /* Allocate a codec context for the decoder */
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    if (nullptr == codec_ctx) {
        std::cout << "avcodec_alloc_context3:" << std::endl;
        return nullptr;
    }

    /* Copy codec parameters from input stream to output codec context */
    rc = avcodec_parameters_to_context(codec_ctx, fmt_ctx->streams[0]->codecpar);
    if (rc < 0) {
        std::cout << "avcodec_parameters_to_context:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return nullptr;
    }

    /* Init the decoders, with or without reference counting */
    // AVDictionary *opts = nullptr;
    // av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);

    rc = avcodec_open2(codec_ctx, codec, nullptr);
    if (rc < 0) {
        std::cout << "avcodec_open2:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return nullptr;
    }

    return codec_ctx;
}

static AVFilterContext* filter_init(AVFilterGraph **fg,
        const std::string& filter_name,
        const std::string& filter_alias,
        const std::string& args) {

    AVFilter *filter = avfilter_get_by_name(filter_name.data());

    AVFilterContext *filter_ctx = nullptr;
    int rc = avfilter_graph_create_filter(&filter_ctx,
            filter, filter_alias.data(), args.data(), NULL, *fg);
    if (rc < 0) {
        std::cout << "avfilter_graph_create_filter:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return nullptr;
    }

    return filter_ctx;
}

static AVCodecContext* encoder_init(AVCodecContext *dec_ctx) {
    int rc = 0;

    AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (nullptr == encoder) {
        std::cout << "avcodec_find_encoder:" << std::endl;
        return nullptr;
    }

    /* Allocate a codec context for the decoder */
    AVCodecContext *enc_ctx = avcodec_alloc_context3(encoder);
    if (nullptr == enc_ctx) {
        std::cout << "avcodec_alloc_context3:" << std::endl;
        return nullptr;
    }

    enc_ctx->bit_rate = dec_ctx->bit_rate;
    enc_ctx->width = dec_ctx->width >> 1;
    enc_ctx->height = dec_ctx->height >> 1;
    enc_ctx->time_base = dec_ctx->time_base;
    // ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
    enc_ctx->time_base       = (AVRational){ 1, 1000 };
    enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
    enc_ctx->pix_fmt = avcodec_default_get_format(enc_ctx, encoder->pix_fmts);

    rc = avcodec_open2(enc_ctx, encoder, nullptr);
    if ( rc < 0) {
        std::cout << "avcodec_open2:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return nullptr;
    }

    return enc_ctx;
}

static AVFormatContext* muxer_init(const std::string& filename, AVCodecContext *outCodecCtx) {
    int rc = 0;

    AVFormatContext *outFmtCtx = nullptr;
    avformat_alloc_output_context2(&outFmtCtx, NULL, NULL, filename.data());
    if (!outFmtCtx) {
        std::cout << "avformat_alloc_output_context2:" << std::endl;
        return nullptr;
    }

    AVStream *stream = avformat_new_stream(outFmtCtx, nullptr);
    if (!stream) {
        std::cout << "avformat_new_stream:" << std::endl;
        return nullptr;
    }

    /* copy the stream parameters to the muxer */
    rc = avcodec_parameters_from_context(stream->codecpar, outCodecCtx);
    if (rc < 0) {
        std::cout << "avcodec_parameters_from_context:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return nullptr;
    }

    if (outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        outFmtCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

    /** Open the output file to write to it. */
    rc = avio_open(&outFmtCtx->pb, filename.data(), AVIO_FLAG_WRITE);
    if (rc < 0) {
        std::cout << "avio_open:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return nullptr;
    }

    av_dump_format(outFmtCtx, 0, filename.data(), 1);

    rc = avformat_write_header(outFmtCtx, NULL);
    if (rc < 0) {
        std::cout << "avformat_write_header:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return nullptr;
    }

    return outFmtCtx;
}

/**
 * input file - decoder - scale filter - encoder - output file
 *
 */
int main(int argc, char const *argv[]) {
    if (3 != argc) {
        std::cout << "invalid arguement!" << std::endl;
        return -1;
    }

    const std::string input_file{argv[1]};
    const std::string output_file{argv[2]};

    std::cout << "input_file : " << input_file << '\n';
    std::cout << "output_file : " << output_file << '\n';

    av_log_set_level(AV_LOG_TRACE);
    av_register_all();
    avfilter_register_all();

    int rc = 0;

    AVFormatContext *input_fmt_ctx = input_file_init(input_file);
    AVCodecContext *dec_ctx = decoder_init(input_fmt_ctx);

    AVFilterGraph *fg = avfilter_graph_alloc();

    std::array<char, 512> args;

    // Create input filter
	// Create Buffer Source -> input filter
    AVStream *stream = input_fmt_ctx->streams[0];
	snprintf(args.data(), args.size(), "time_base=%d/%d:video_size=%dx%d:pix_fmt=%d:pixel_aspect=%d/%d"
		, stream->time_base.num, stream->time_base.den
		, dec_ctx->width, dec_ctx->height
		, dec_ctx->pix_fmt
		, dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);
    std::cout << "args = " << args.data() << '\n';
    AVFilterContext *buf_filter_ctx = filter_init(&fg, "buffer", "src", args.data());

    // Create rescaler filter to resize video resolution
	snprintf(args.data(), args.size(), "%d:%d", dec_ctx->width >> 1, dec_ctx->height >> 1);
    std::cout << "args = " << args.data() << '\n';
    AVFilterContext *scale_filter_ctx = filter_init(&fg, "scale", "scale", args.data());

    memset(args.data(), 0, args.size());

    // snprintf(args.data(), args.size(), "time_base=%d/%d:video_size=%dx%d:pix_fmt=%d:pixel_aspect=%d/%d"
	// 	, stream->time_base.num, stream->time_base.den
	// 	, dec_ctx->width >> 1, dec_ctx->height >> 1
	// 	, dec_ctx->pix_fmt
	// 	, dec_ctx->sample_aspect_ratio.num, dec_ctx->sample_aspect_ratio.den);

    std::cout << "args = " << args.data() << '\n';
    AVFilterContext *bufsink_filter_ctx = filter_init(&fg, "buffersink", "sink", args.data());

    /* Connect the filters; */
	rc = avfilter_link(buf_filter_ctx, 0, scale_filter_ctx, 0);
	if (rc >= 0)
        rc = avfilter_link(scale_filter_ctx, 0, bufsink_filter_ctx, 0);
    if (rc < 0) {
        std::cout << "avfilter_link:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    /* Configure the graph. */
    rc = avfilter_graph_config(fg, nullptr);
    if (rc < 0) {
        std::cout << "avfilter_graph_config:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    char* dump = avfilter_graph_dump(fg, nullptr);
    av_log(NULL, AV_LOG_ERROR, "Graph :\n%s\n", dump);

    AVCodecContext *enc_ctx = encoder_init(dec_ctx);
    AVFormatContext *out_fmt_ctx = muxer_init(output_file, enc_ctx);

    rc = avformat_write_header(out_fmt_ctx, NULL);
    if (rc < 0) {
        std::cout << "avformat_write_header:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    while (1) {

        // read input file
        AVPacket packet;
        int rc = av_read_frame(input_fmt_ctx, &packet);
        if (0 > rc) {
            std::cout << "av_read_frame:"
                << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            break;
        }

        std::cout << "input_fmt_ctx"
                << " stream_index: " << packet.stream_index
                << " frame size: " << packet.size << std::endl;

        // video only
        if (0 != packet.stream_index) {
            continue;
        }

        // decoder
        /* send the packet with the compressed data to the decoder */
        rc = avcodec_send_packet(dec_ctx, &packet);
        if (rc < 0) {
            std::cout << "avcodec_send_packet:"
                << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            break;
        }

        AVFrame *dec_frame = av_frame_alloc();
        while (rc >= 0) {
            rc = avcodec_receive_frame(dec_ctx, dec_frame);
            if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF) {
                std::cout << "avcodec_receive_frame:"
                    << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                    << ":" << rc << std::endl;
                break;
            }

            else if (rc < 0) {
                std::cout << "avcodec_receive_frame:"
                    << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                    << ":" << rc << std::endl;
                break;
            }
            std::cout << "dec res: " << dec_frame->width << "x" << dec_frame->height << '\n';
            break;
        }

        // check frame
        if (rc < 0) {
            continue;
        }

        // filter
        rc = av_buffersrc_write_frame(buf_filter_ctx, dec_frame);
        if (rc < 0) {
            std::cout << "av_buffersrc_write_frame:"
                << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            break;
        }

        AVFrame *filt_frame = av_frame_alloc();
        /* pull filtered audio from the filtergraph */
        while (1) {
            rc = av_buffersink_get_frame(bufsink_filter_ctx, filt_frame);
            if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF)
                continue;
            else if (rc < 0) {
                std::cout << "av_buffersink_get_frame:"
                    << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                    << ":" << rc << std::endl;
                break;
            }
            break;
        }

        std::cout << "filter res: " << filt_frame->width << "x" << filt_frame->height << '\n';

        {
            {
                AVPacket packet;
                av_init_packet(&packet);
                packet.data = nullptr;
                packet.size = 0;

                /* send the frame for encoding */
                rc = avcodec_send_frame(enc_ctx, filt_frame);
                if (rc < 0) {
                    std::cout << "avcodec_send_frame:"
                        << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                        << ":" << rc << std::endl;
                    break;
                }

                /* read all the available output packets (in general there may be any
                 * number of them */
                while (rc >= 0) {
                    rc = avcodec_receive_packet(enc_ctx, &packet);
                    if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF)
                        continue;
                    else if (rc < 0) {
                        std::cout << "avcodec_receive_packet:"
                            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                            << ":" << rc << std::endl;
                        break;
                    }
                    std::cout << "encoding packet = " << packet.size << std::endl;

                    if ((rc = av_write_frame(out_fmt_ctx, &packet)) < 0) {
                        std::cout << "av_write_frame(:"
                            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                            << ":" << rc << std::endl;
                        break;
                    }

                    av_packet_unref(&packet);
                }
            }

            av_frame_unref(filt_frame);
        }

        // av_frame_free(&dec_frame);
        av_frame_free(&filt_frame);

        av_frame_free(&dec_frame);
    }

    av_write_trailer(out_fmt_ctx);

    avcodec_close(enc_ctx);
    avcodec_free_context(&enc_ctx);
    avformat_close_input(&out_fmt_ctx);

    avfilter_free(bufsink_filter_ctx);
    avfilter_free(scale_filter_ctx);
    avfilter_free(buf_filter_ctx);
    avfilter_graph_free(&fg);

    avcodec_close(dec_ctx);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&input_fmt_ctx);
    return 0;
}
