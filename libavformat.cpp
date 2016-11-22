#include <stdio.h>

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
}

int main(void)
{
    av_register_all();
    return 0;
}
