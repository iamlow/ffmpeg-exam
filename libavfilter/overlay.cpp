#include <array>
#include <iostream>
#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
// #include <libavformat/avformat.h>
// #include <libavfilter/buffersink.h>
// #include <libavfilter/buffersrc.h>
// #include <libavutil/opt.h>
}

#include "InputFormatContext.hpp"
// #include "OutputFormatContext.hpp"

int main(int argc, char const *argv[]) {

    av_log_set_level(AV_LOG_TRACE);
    av_register_all();
    // avfilter_register_all();

    auto ifc = InputFormatContext::create(argv[1]);
    // auto ofc = OutputFormatContext::create(argv[2], nullptr);

    while (1) {
        AVPacket pkt;
        int rc = ifc->read(&pkt);
        if (0 > rc)
            break;
        std::cout << "read: " << rc << ":" << pkt.size << '\n';
    }

    return 0;
}
