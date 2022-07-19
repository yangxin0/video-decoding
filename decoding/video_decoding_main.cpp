#include "video_decoding.h"

int main(int argc, char *argv[])
{
    int rc;
    AVFrame *frame;
    
    if (argc < 2) {
        fprintf(stderr, "./video-decoding video_path \n");
        return 1;
    }

    VideoDecoder decoder;
    rc = video_decoder_new(argv[1], &decoder);
    if (rc < 0) {
        fprintf(stderr, "video_decoding: create decoder failed, code=%d\n", rc);
        return 1;
    }

    int seq = 0;
    while (1) {
        rc = video_decoder_frame_read(&decoder, &frame);
        if (rc == VID_OK) {
            printf("video_decoding: seq %d, pts %lld, dts %lld\n", seq++, frame->pts, frame->pkt_dts);
            video_decoder_frame_free(frame);
        } else if (rc == VID_ERR_EOF) {
            break;
        } else {
            fprintf(stderr, "video_decoding: receive frame failed, code=%d\n", rc);
        }
    }

    video_decoder_free(&decoder);

    return 0;
}
