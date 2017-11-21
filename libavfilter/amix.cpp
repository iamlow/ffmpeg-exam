#include <array>
#include <iostream>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
}

static char errstr[AV_ERROR_MAX_STRING_SIZE];

static int check_args(int argc, char const *argv[]) {
    if (4 != argc) {
        std::cout << "invalid argument!\n";
        return -1;
    }

    std::cout << "input_file1: " << argv[1] << std::endl;
    std::cout << "input_file2: " << argv[2] << std::endl;
    std::cout << "output_file: " << argv[3] << std::endl;

    return 0;
}

static int refcount = 0;

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
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);

    rc = avcodec_open2(codec_ctx, codec, &opts);
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

int main(int argc, char const *argv[]) {
    if (check_args(argc, argv))
        return -1;

    const std::string input_file1 = argv[1];
    const std::string input_file2 = argv[2];
    const std::string output_file = argv[3];

    av_log_set_level(AV_LOG_TRACE);
    av_register_all();
    avfilter_register_all();

    int rc = 0;

    ////////////////////////////////////////////////////////////////////////////
    // input_file1
    //

    // Container
    AVFormatContext *inFmtCtx1 = input_file_init(input_file1);
    AVCodecContext *inCodecCtx1 = decoder_init(inFmtCtx1);

    ////////////////////////////////////////////////////////////////////////////
    // input_file2

    // Container
    AVFormatContext *inFmtCtx2 = input_file_init(input_file2);
    AVCodecContext *inCodecCtx2 = decoder_init(inFmtCtx2);

    ////////////////////////////////////////////////////////////////////////////
    // filter
    //
    std::array<char, 512> args;

    AVFilterGraph *fg = avfilter_graph_alloc();

    // abuffer src1
    if (!inCodecCtx1->channel_layout)
        inCodecCtx1->channel_layout = av_get_default_channel_layout(inCodecCtx1->channels);

    snprintf(args.data(), args.size(),
		 "sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
         inCodecCtx1->sample_rate,
         av_get_sample_fmt_name(inCodecCtx1->sample_fmt),
         inCodecCtx1->channel_layout);

    AVFilterContext *abufferCtx1 = filter_init(&fg, "abuffer", "src1", args.data());

    // abuffer src2
    if (!inCodecCtx2->channel_layout)
        inCodecCtx2->channel_layout = av_get_default_channel_layout(inCodecCtx2->channels);

    snprintf(args.data(), args.size(),
         "sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
         inCodecCtx2->sample_rate,
         av_get_sample_fmt_name(inCodecCtx1->sample_fmt),
         inCodecCtx2->channel_layout);

    AVFilterContext *abufferCtx2 = filter_init(&fg, "abuffer", "src2", args.data());

    // amix
    snprintf(args.data(), args.size(), "inputs=2");
    AVFilterContext *amixCtx = filter_init(&fg, "amix", "amix", args.data());

    // abuffersink
    /* Finally create the abuffersink filter;
     * it will be used to get the filtered data out of the graph. */
    AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    if (!abuffersink) {
        std::cout << "avfilter_get_by_name:" << std::endl;
        return AVERROR_FILTER_NOT_FOUND;
    }

    AVFilterContext *abuffersinkCtx = avfilter_graph_alloc_filter(fg, abuffersink, "sink");
    if (!abuffersinkCtx) {
        std::cout << "avfilter_graph_alloc_filter:" << rc << std::endl;
        return AVERROR(ENOMEM);
    }

    /* Same sample fmts as the output file. */
    rc = av_opt_set_int_list(abuffersinkCtx, "sample_fmts",
                              ((int[]){ AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE }),
                              AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

#define OUTPUT_CHANNELS 1
    // char ch_layout[64];
    // av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0, OUTPUT_CHANNELS);
    // rc = av_opt_set(abuffersinkCtx, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
    // if (rc < 0) {
    //     std::cout << "av_opt_set:"
    //         << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
    //         << ":" << rc << std::endl;
    //     return rc;
    // }

    rc = avfilter_init_str(abuffersinkCtx, NULL);
    if (rc < 0) {
        std::cout << "avfilter_init_str:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    /* Connect the filters; */
	rc = avfilter_link(abufferCtx1, 0, amixCtx, 0);
	if (rc >= 0)
        rc = avfilter_link(abufferCtx2, 0, amixCtx, 1);
	if (rc >= 0)
        rc = avfilter_link(amixCtx, 0, abuffersinkCtx, 0);
    if (rc < 0) {
        std::cout << "avfilter_link:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    /* Configure the graph. */
    rc = avfilter_graph_config(fg, NULL);
    if (rc < 0) {
        std::cout << "avfilter_graph_config:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    char* dump = avfilter_graph_dump(fg, NULL);
    av_log(NULL, AV_LOG_ERROR, "Graph :\n%s\n", dump);

    ////////////////////////////////////////////////////////////////////////////
    // Encoding
    // AVCodec* outCodec = avcodec_find_encoder(inFmtCtx1->streams[0]->codecpar->codec_id);
    AVCodec* outCodec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    if (nullptr == outCodec) {
        std::cout << "avcodec_find_encoder:" << std::endl;
        return -1;
    }

    /* Allocate a codec context for the decoder */
    AVCodecContext *outCodecCtx = avcodec_alloc_context3(outCodec);
    if (nullptr == outCodecCtx) {
        std::cout << "avcodec_alloc_context3:" << std::endl;
        return -1;
    }

    /* put sample parameters */
    outCodecCtx->bit_rate = 11025;

    /* select other audio parameters supported by the encoder */
    outCodecCtx->channels       = OUTPUT_CHANNELS;
    outCodecCtx->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
    outCodecCtx->sample_rate    = 11025;
    outCodecCtx->sample_fmt     = AV_SAMPLE_FMT_S16;

    ////////////////////////////////////////////////////////////////////////////
    // OutputFormat
    AVFormatContext *outFmtCtx = nullptr;
    avformat_alloc_output_context2(&outFmtCtx,
                                    NULL, NULL, output_file.data());
    if (!outFmtCtx) {
        std::cout << "avformat_alloc_output_context2:" << std::endl;
        return AVERROR_UNKNOWN;
    }

    /** Create a new audio stream in the output file container. */
    AVStream *stream = avformat_new_stream(outFmtCtx, nullptr);
    if (!stream) {
        std::cout << "avformat_new_stream:" << std::endl;
        return AVERROR(ENOMEM);
    }

    /* copy the stream parameters to the muxer */
    rc = avcodec_parameters_from_context(stream->codecpar, outCodecCtx);
    if (rc < 0) {
        std::cout << "avcodec_parameters_from_context:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    if (outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        outFmtCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

    /** Open the output file to write to it. */
    rc = avio_open(&outFmtCtx->pb, output_file.data(), AVIO_FLAG_WRITE);
    if (rc < 0) {
        std::cout << "avio_open:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    av_dump_format(outFmtCtx, 0, output_file.data(), 1);

    /** Open the encoder for the audio stream to use it later. */
    rc = avcodec_open2(outCodecCtx, outCodec, NULL);
    if (rc < 0) {
        std::cout << "avcodec_open2:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    rc = avformat_write_header(outFmtCtx, NULL);
    if (rc < 0) {
        std::cout << "avformat_write_header:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    for (;;) {
        // Input File 1
        {
            AVPacket packet;
            int rc = av_read_frame(inFmtCtx1, &packet);
            if (0 > rc) {
                std::cout << "av_read_frame:"
                    << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                    << ":" << rc << std::endl;
                break;
            }
            std::cout << "inFmtCtx1 read_frame size: " << packet.size << std::endl;

            /**
             * Decode the audio frame stored in the temporary packet.
             * The input audio stream decoder is used to do this.
             * If we are at the end of the file, pass an empty packet to the decoder
             * to flush it.
             */
            AVFrame *frame = av_frame_alloc();

            int i, ch;
            int data_size;
            /* send the packet with the compressed data to the decoder */
            rc = avcodec_send_packet(inCodecCtx1, &packet);
            if (rc < 0) {
                std::cout << "avcodec_send_packet:"
                    << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                    << ":" << rc << std::endl;
                break;
            }
            /* read all the output frames (in general there may be any number of them */
            while (rc >= 0) {
                rc = avcodec_receive_frame(inCodecCtx1, frame);
                if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF)
                    break;
                else if (rc < 0) {
                    std::cout << "avcodec_receive_frame:"
                        << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                        << ":" << rc << std::endl;
                    break;
                }

                data_size = av_get_bytes_per_sample(inCodecCtx1->sample_fmt);
                if (data_size < 0) {
                    std::cout << "av_get_bytes_per_sample:" << std::endl;
                    break;
                }

                std::cout << "decoding data size: " << frame->nb_samples
                        * av_get_bytes_per_sample((enum AVSampleFormat)frame->format) << std::endl;

                rc = av_buffersrc_write_frame(abufferCtx1, frame);
                if (rc < 0) {
                    std::cout << "av_buffersrc_write_frame:"
                        << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                        << ":" << rc << std::endl;
                    break;
                }
            }

            av_frame_free(&frame);
        }

        // Input File 2
        {
            AVPacket packet;
            int rc = av_read_frame(inFmtCtx2, &packet);
            if (0 > rc) {
                std::cout << "av_read_frame:"
                    << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                    << ":" << rc << std::endl;
                break;
            }
            std::cout << "inFmtCtx2 read_frame size: " << packet.size << std::endl;

            /**
             * Decode the audio frame stored in the temporary packet.
             * The input audio stream decoder is used to do this.
             * If we are at the end of the file, pass an empty packet to the decoder
             * to flush it.
             */
            AVFrame *frame = av_frame_alloc();

            int i, ch;
            int data_size;
            /* send the packet with the compressed data to the decoder */
            rc = avcodec_send_packet(inCodecCtx2, &packet);
            if (rc < 0) {
                std::cout << "avcodec_send_packet:"
                    << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                    << ":" << rc << std::endl;
                break;
            }
            /* read all the output frames (in general there may be any number of them */
            while (rc >= 0) {
                 rc = avcodec_receive_frame(inCodecCtx2, frame);
                 if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF)
                     break;
                 else if (rc < 0) {
                     std::cout << "avcodec_receive_frame:"
                         << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                         << ":" << rc << std::endl;
                     break;
                 }

                 data_size = av_get_bytes_per_sample(inCodecCtx2->sample_fmt);
                 if (data_size < 0) {
                    std::cout << "av_get_bytes_per_sample:" << std::endl;
                    break;
                 }

                 rc = av_buffersrc_write_frame(abufferCtx2, frame);
                 if (rc < 0) {
                     std::cout << "av_buffersrc_write_frame:"
                         << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                         << ":" << rc << std::endl;
                     break;
                 }

                 std::cout << "decoding data size: " << frame->pkt_size << std::endl;
            }

            av_frame_free(&frame);

            AVFrame *filt_frame = av_frame_alloc();

            /* pull filtered audio from the filtergraph */
            while (1) {
                rc = av_buffersink_get_frame(abuffersinkCtx, filt_frame);
                if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF){
                    std::cout << "av_buffersink_get_frame:"
                        << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                        << ":" << rc << std::endl;
                    break;
                }

                {
                    AVPacket packet;
                    av_init_packet(&packet);
                    packet.data = nullptr;
                    packet.size = 0;

                    /* send the frame for encoding */
                    rc = avcodec_send_frame(outCodecCtx, filt_frame);
                    if (rc < 0) {
                        std::cout << "avcodec_send_frame:"
                            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                            << ":" << rc << std::endl;
                        break;
                    }

                    /* read all the available output packets (in general there may be any
                     * number of them */
                    // while (ret >= 0) {
                    rc = avcodec_receive_packet(outCodecCtx, &packet);
                    if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF)
                        return -1;
                    else if (rc < 0) {
                        std::cout << "avcodec_receive_packet:"
                            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                            << ":" << rc << std::endl;
                        break;
                    }
                    std::cout << "encoding packet = " << packet.size << std::endl;

                    if ((rc = av_write_frame(outFmtCtx, &packet)) < 0) {
                        std::cout << "av_write_frame(:"
                            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                            << ":" << rc << std::endl;
                        break;
                    }

                    av_packet_unref(&packet);
                    // }
                }

                av_frame_unref(filt_frame);
            }

            av_frame_free(&filt_frame);
        }
    }

    avformat_write_header(outFmtCtx, nullptr);

    avcodec_close(outCodecCtx);
    avcodec_free_context(&outCodecCtx);
    avio_close(outFmtCtx->pb);
    avformat_free_context(outFmtCtx);

    avfilter_free(abufferCtx1);
    avfilter_free(abufferCtx2);
    avfilter_free(amixCtx);
    avfilter_free(abuffersinkCtx);
    avfilter_graph_free(&fg);

    avcodec_close(inCodecCtx1);
    avcodec_free_context(&inCodecCtx1);
    avformat_close_input(&inFmtCtx1);

    avcodec_close(inCodecCtx2);
    avcodec_free_context(&inCodecCtx2);
    avformat_close_input(&inFmtCtx2);

    return 0;
}
