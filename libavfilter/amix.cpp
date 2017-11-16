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

static char *const get_error_text(const int error)
{
    static char error_buffer[255];
    av_strerror(error, error_buffer, sizeof(error_buffer));
    return error_buffer;
}

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

int main(int argc, char const *argv[]) {

    if (check_args(argc, argv))
        return -1;

    const std::string input_file1 = argv[1];
    const std::string input_file2 = argv[2];
    const std::string output_file = argv[3];

    av_log_set_level(AV_LOG_TRACE);
    av_register_all();
    avfilter_register_all();

    char errstr[AV_ERROR_MAX_STRING_SIZE];
    int rc = 0;

    int total_out_samples = 0;

    ////////////////////////////////////////////////////////////////////////////
    // input_file1
    //

    // Container
    AVFormatContext *inFmtCtx1 = NULL;
    rc = avformat_open_input(&inFmtCtx1, input_file1.c_str(), NULL, NULL);
    if (0 > rc) {
        std::cout << "avformat_open_input:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return 0;
    }
    av_dump_format(inFmtCtx1, 0, input_file1.c_str(), 0);

    rc = avformat_find_stream_info(inFmtCtx1, NULL);
    if (0 > rc) {
        std::cout << "avformat_find_stream_info:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    // Codec
    // XXX note: 'codec' has been explicitly marked deprecated here
    // AVCodec* inCodec1 = avcodec_find_decoder(inFmtCtx1->streams[0]->codec->codec_id);
    AVCodec* inCodec1 = avcodec_find_decoder(inFmtCtx1->streams[0]->codecpar->codec_id);
    if (nullptr == inCodec1) {
        std::cout << "avcodec_find_decoder:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return -1;
    }

    /* Allocate a codec context for the decoder */
    AVCodecContext *inCodecCtx1 = avcodec_alloc_context3(inCodec1);
    if (nullptr == inCodecCtx1) {
        std::cout << "avcodec_alloc_context3:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return -1;
    }

    /* Copy codec parameters from input stream to output codec context */
    rc = avcodec_parameters_to_context(inCodecCtx1, inFmtCtx1->streams[0]->codecpar);
    if (rc < 0) {
        std::cout << "avcodec_parameters_to_context:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return -1;

     // fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
     //         av_get_media_type_string(type));
     // return ret;
    }

    /* Init the decoders, with or without reference counting */
    {
        AVDictionary *opts = nullptr;
        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);

        rc = avcodec_open2(inCodecCtx1, inCodec1, &opts);
        if (rc < 0) {
            std::cout << "avcodec_open2:"
                << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            return -1;

         // fprintf(stderr, "Failed to open %s codec\n",
         //         av_get_media_type_string(type));
         // return ret;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // input_file2

    // Container
    AVFormatContext *inFmtCtx2 = NULL;
    rc = avformat_open_input(&inFmtCtx2, input_file2.c_str(), NULL, NULL);
    if (0 > rc) {
        std::cout << "avformat_open_input:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return 0;
    }

    av_dump_format(inFmtCtx2, 0, input_file2.c_str(), 0);

    rc = avformat_find_stream_info(inFmtCtx2, NULL);
    if (0 > rc) {
        std::cout << "avformat_find_stream_info:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return rc;
    }

    // Codec
    // XXX note: 'codec' has been explicitly marked deprecated here
    // AVCodec* inCodec1 = avcodec_find_decoder(inFmtCtx1->streams[0]->codec->codec_id);
    AVCodec* inCodec2 = avcodec_find_decoder(inFmtCtx2->streams[0]->codecpar->codec_id);
    if (nullptr == inCodec2) {
        std::cout << "avcodec_find_decoder:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return -1;
    }

    /* Allocate a codec context for the decoder */
    AVCodecContext *inCodecCtx2 = avcodec_alloc_context3(inCodec2);
    if (nullptr == inCodecCtx2) {
        std::cout << "avcodec_alloc_context3:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return -1;
    }

    /* Copy codec parameters from input stream to output codec context */
    rc = avcodec_parameters_to_context(inCodecCtx2, inFmtCtx2->streams[0]->codecpar);
    if (rc < 0) {
        std::cout << "avcodec_parameters_to_context:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return -1;
    }

    /* Init the decoders, with or without reference counting */
    {
        AVDictionary *opts = nullptr;
        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);

        rc = avcodec_open2(inCodecCtx2, inCodec2, &opts);
        if (rc < 0) {
            std::cout << "avcodec_open2:"
                << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
                << ":" << rc << std::endl;
            return -1;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // filter
    //
    AVFilterGraph *fg = avfilter_graph_alloc();
    AVFilter *abuffer1 = avfilter_get_by_name("abuffer");

    if (!inCodecCtx1->channel_layout)
        inCodecCtx1->channel_layout = av_get_default_channel_layout(inCodecCtx1->channels);

    std::array<char, 512> args;
    // char args[512];
    snprintf(args.data(), args.size(),
		 "sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
         inCodecCtx1->sample_rate,
         av_get_sample_fmt_name(inCodecCtx1->sample_fmt),
         inCodecCtx1->channel_layout);

    AVFilterContext *abufferCtx1 = nullptr;
    rc = avfilter_graph_create_filter(&abufferCtx1, abuffer1, "src1", args.data(), NULL, fg);
    if (rc < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
        return -1;
    }

    AVFilter *abuffer2 = avfilter_get_by_name("abuffer");

    if (!inCodecCtx2->channel_layout)
        inCodecCtx2->channel_layout = av_get_default_channel_layout(inCodecCtx2->channels);

    // std::array<char, 512> args2;
    // char args[512];
    snprintf(args.data(), args.size(),
         "sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
         inCodecCtx2->sample_rate,
         av_get_sample_fmt_name(inCodecCtx1->sample_fmt),
         inCodecCtx2->channel_layout);

    AVFilterContext *abufferCtx2 = nullptr;
    rc = avfilter_graph_create_filter(&abufferCtx2, abuffer2, "src2", args.data(), NULL, fg);
    if (rc < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
        return -1;
    }

    /****** amix ******* */
    /* Create mix filter. */
    AVFilter *amix = avfilter_get_by_name("amix");
    if (!amix) {
        av_log(NULL, AV_LOG_ERROR, "Could not find the amix filter.\n");
        return AVERROR_FILTER_NOT_FOUND;
    }

    snprintf(args.data(), args.size(), "inputs=2");

    AVFilterContext *amixCtx = nullptr;
    rc = avfilter_graph_create_filter(&amixCtx, amix, "amix", args.data(), NULL, fg);
    if (rc < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio amix filter\n");
        return -1;
    }

    /* Finally create the abuffersink filter;
     * it will be used to get the filtered data out of the graph. */
    AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    if (!abuffersink) {
        av_log(NULL, AV_LOG_ERROR, "Could not find the abuffersink filter.\n");
        return AVERROR_FILTER_NOT_FOUND;
    }

    AVFilterContext *abuffersinkCtx = avfilter_graph_alloc_filter(fg, abuffersink, "sink");
    if (!abuffersinkCtx) {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate the abuffersink instance.\n");
        return AVERROR(ENOMEM);
    }

    /* Same sample fmts as the output file. */
    rc = av_opt_set_int_list(abuffersinkCtx, "sample_fmts",
                              ((int[]){ AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE }),
                              AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

    // uint8_t ch_layout[64];
#define OUTPUT_CHANNELS 1
    char ch_layout[64];
    av_get_channel_layout_string(ch_layout, sizeof(ch_layout), 0, OUTPUT_CHANNELS);
    printf("ch_layout = %s\n", ch_layout);
    av_opt_set(abuffersinkCtx, "channel_layout", ch_layout, AV_OPT_SEARCH_CHILDREN);
    if (rc < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could set options to the abuffersink instance.\n");
        return rc;
    }

    rc = avfilter_init_str(abuffersinkCtx, NULL);
    if (rc < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not initialize the abuffersink instance.\n");
        return rc;
    }


    /* Connect the filters; */

	rc = avfilter_link(abufferCtx1, 0, amixCtx, 0);
	if (rc >= 0)
        rc = avfilter_link(abufferCtx2, 0, amixCtx, 1);
	if (rc >= 0)
        rc = avfilter_link(amixCtx, 0, abuffersinkCtx, 0);
    if (rc < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error connecting filters\n");
        return rc;
    }

    /* Configure the graph. */
    rc = avfilter_graph_config(fg, NULL);
    if (rc < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error while configuring graph : %s\n", get_error_text(rc));
        return rc;
    }

    char* dump = avfilter_graph_dump(fg, NULL);
    av_log(NULL, AV_LOG_ERROR, "Graph :\n%s\n", dump);

    // Encoding

    // OutputFormat

    AVFormatContext *outFmtCtx = nullptr;
    avformat_alloc_output_context2(&outFmtCtx,
                                    NULL, NULL, output_file.data());
    if (!outFmtCtx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }


    // /* Copy codec parameters from input stream to output codec context */
    // rc = avcodec_parameters_to_context(outCodecCtx, inFmtCtx1->streams[0]->codecpar);
    // if (rc < 0) {
    //     std::cout << "avcodec_parameters_to_context:"
    //         << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
    //         << ":" << rc << std::endl;
    //     return -1;
    //
    //  // fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
    //  //         av_get_media_type_string(type));
    //  // return ret;
    // }
    // outCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;

    // AVCodec* outCodec = avcodec_find_encoder(inFmtCtx1->streams[0]->codecpar->codec_id);
    AVCodec* outCodec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
    if (nullptr == outCodec) {
        std::cout << "avcodec_find_encoder:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return -1;
    }

    /* Allocate a codec context for the decoder */
    AVCodecContext *outCodecCtx = avcodec_alloc_context3(outCodec);
    if (nullptr == outCodecCtx) {
        std::cout << "avcodec_alloc_context3:"
            << av_make_error_string(errstr, AV_ERROR_MAX_STRING_SIZE, rc)
            << ":" << rc << std::endl;
        return -1;
    }

    /* put sample parameters */
    outCodecCtx->bit_rate = 11025;

    /* select other audio parameters supported by the encoder */
    outCodecCtx->channels       = OUTPUT_CHANNELS;
    outCodecCtx->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
    outCodecCtx->sample_rate    = 11025;
    outCodecCtx->sample_fmt     = AV_SAMPLE_FMT_S16;

    /** Create a new audio stream in the output file container. */
    AVStream *stream = avformat_new_stream(outFmtCtx, nullptr);
    if (!stream) {
        av_log(NULL, AV_LOG_ERROR, "Could not create new stream\n");
        rc = AVERROR(ENOMEM);
        return -1;
    }

    /* copy the stream parameters to the muxer */
    rc = avcodec_parameters_from_context(stream->codecpar, outCodecCtx);
    if (rc < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }
    
    // outCodecCtx = stream->codec;

    if (outFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        outFmtCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

    /** Open the output file to write to it. */
    if ((rc = avio_open(&outFmtCtx->pb, output_file.data(), AVIO_FLAG_WRITE)) < 0) {
        // av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s' (error '%s')\n",
        //        filename, get_error_text(rc));
        std::cout << "avio_open is failed\n";
        return rc;
    }

    av_dump_format(outFmtCtx, 0, output_file.data(), 1);



    /** Open the encoder for the audio stream to use it later. */
    if ((rc = avcodec_open2(outCodecCtx, outCodec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Could not open output codec (error '%s')\n",
               get_error_text(rc));
        return -1;
    }

    rc = avformat_write_header(outFmtCtx, NULL);
    if (rc < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return -1;
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
            int ret, data_size;
            /* send the packet with the compressed data to the decoder */
            ret = avcodec_send_packet(inCodecCtx1, &packet);
            if (ret < 0) {
                fprintf(stderr, "Error submitting the packet to the decoder\n");
                exit(1);
            }
            /* read all the output frames (in general there may be any number of them */
            while (ret >= 0) {
                ret = avcodec_receive_frame(inCodecCtx1, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    exit(1);
                }
                data_size = av_get_bytes_per_sample(inCodecCtx1->sample_fmt);
                if (data_size < 0) {
                    /* This should not occur, checking just for paranoia */
                    fprintf(stderr, "Failed to calculate data size\n");
                    exit(1);
                }

                std::cout << "decoding data size: " << frame->nb_samples
                        * av_get_bytes_per_sample((enum AVSampleFormat)frame->format) << std::endl;

                rc = av_buffersrc_write_frame(abufferCtx1, frame);
                if (rc < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error writing EOF null frame for input abufferCtx1\n");
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
            int ret, data_size;
            /* send the packet with the compressed data to the decoder */
            ret = avcodec_send_packet(inCodecCtx2, &packet);
            if (ret < 0) {
                fprintf(stderr, "Error submitting the packet to the decoder\n");
                exit(1);
            }
            /* read all the output frames (in general there may be any number of them */
            while (ret >= 0) {
                 ret = avcodec_receive_frame(inCodecCtx2, frame);
                 if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                     break;
                 else if (ret < 0) {
                     fprintf(stderr, "Error during decoding\n");
                     exit(1);
                 }
                 data_size = av_get_bytes_per_sample(inCodecCtx2->sample_fmt);
                 if (data_size < 0) {
                     /* This should not occur, checking just for paranoia */
                     fprintf(stderr, "Failed to calculate data size\n");
                     exit(1);
                 }

                 rc = av_buffersrc_write_frame(abufferCtx2, frame);
                 if (rc < 0) {
                     av_log(NULL, AV_LOG_ERROR, "Error writing EOF null frame for input %d\n", i);
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
                    av_log(NULL, AV_LOG_INFO, "Need to read input \n");
                    break;
                }

                // av_log(NULL, AV_LOG_INFO, "remove %d samples from sink (%d Hz, time=%f, ttime=%f)\n",
                //        filt_frame->nb_samples, output_codec_context->sample_rate,
                //        (double)filt_frame->nb_samples / output_codec_context->sample_rate,
                //        (double)(total_out_samples += filt_frame->nb_samples) / output_codec_context->sample_rate);
                std::cout << "filt_frame->nb_samples : " << filt_frame->nb_samples << std::endl;
                // //av_log(NULL, AV_LOG_INFO, "Data read from graph\n");
                // ret = encode_audio_frame(filt_frame, output_format_context, output_codec_context, &data_present);
                // if (ret < 0)
                //     goto end;

                {
                    AVPacket packet;
                    av_init_packet(&packet);
                    packet.data = nullptr;
                    packet.size = 0;

                    /* send the frame for encoding */
                    int ret = avcodec_send_frame(outCodecCtx, filt_frame);
                    if (ret < 0) {
                        fprintf(stderr, "Error sending the frame to the encoder\n");
                        exit(1);
                    }

                    /* read all the available output packets (in general there may be any
                     * number of them */
                    // while (ret >= 0) {
                    ret = avcodec_receive_packet(outCodecCtx, &packet);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        return -1;
                    else if (ret < 0) {
                        fprintf(stderr, "Error encoding audio frame\n");
                        exit(1);
                    }
                    std::cout << "encoding packet = " << packet.size << std::endl;

                    if ((rc = av_write_frame(outFmtCtx, &packet)) < 0) {
                        // av_log(NULL, AV_LOG_ERROR, "Could not write frame (error '%s')\n",
                        //        get_error_text(error));
                        // av_free_packet(&output_packet);
                        return rc;
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
    avfilter_graph_free(&fg);

    avcodec_close(inCodecCtx1);
    avcodec_free_context(&inCodecCtx1);
    avformat_close_input(&inFmtCtx1);

    avcodec_close(inCodecCtx2);
    avcodec_free_context(&inCodecCtx2);
    avformat_close_input(&inFmtCtx2);

    return 0;
}
