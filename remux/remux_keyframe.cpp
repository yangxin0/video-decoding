#include <cstdio>
#include "decoding/video_decoding.h"
#include "libavformat/avformat.h"

int remux_keyframe(const char *video_path, const char *output_path) 
{
    int rc;
    AVFormatContext *ifmt = nullptr, *ofmt = nullptr;
    AVStream *is = nullptr, *os = nullptr;
    AVPacket *pkt;
    int idx;

    rc = open_video_file(video_path, &ifmt);
    if (rc < 0) {
        fprintf(stderr, "remux: open %s failed\n", video_path);
        return 1;
    }

    av_dump_format(ifmt, 0, video_path, 0);

    rc = avformat_alloc_output_context2(&ofmt, NULL, NULL, output_path);
    if (rc < 0) {
        fprintf(stderr, "remux: alloc output format failed, code=%d\n", rc);
        goto out;
    }

    // find 1st video stream
    for (int i = 0; i < ifmt->nb_streams; i++) {
        is = ifmt->streams[i];
        if (is->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            idx = i;
            break;
        }
        is = nullptr;
    }

    if (is == nullptr) {
        fprintf(stderr, "remux: no video stream\n");
        return 1;
    }
    printf("video stream found: %d\n", idx);

    os = avformat_new_stream(ofmt, NULL);
    if (os == nullptr) {
        fprintf(stderr, "remux: alloc new stream failed\n");
        rc = AVERROR_UNKNOWN;
        goto out;
    }

    rc = avcodec_parameters_copy(os->codecpar, is->codecpar);
    if (rc < 0) {
        fprintf(stderr, "remux: copy codec params failed, code=%d\n", rc);
        goto out;
    }

    os->codecpar->codec_tag = 0;
    av_dump_format(ofmt, 0, output_path, 1);

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        rc = avio_open(&ofmt->pb, output_path, AVIO_FLAG_WRITE);
        if (rc < 0) {
            fprintf(stderr, "remux: cannot open output file\n");
            goto out;
        }
    }

    rc = avformat_write_header(ofmt, NULL);
    if (rc < 0) {
        fprintf(stderr, "remux: write header failed\n");
        goto out;
    }

    pkt = av_packet_alloc();
    if (pkt == nullptr) {
        fprintf(stderr, "remux: alloc packet failed\n");
        goto out;
    }
    while (1) {
        rc = av_read_frame(ifmt, pkt);
        if (rc < 0) {
            break;
        }
        if (pkt->stream_index != idx) {
            av_packet_unref(pkt);
            continue;
        }

        // skip non-keyframe
        if (!(pkt->flags & AV_PKT_FLAG_KEY)) {
            av_packet_unref(pkt);
            continue;
        }
        /* copy packet */
        pkt->pts = av_rescale_q_rnd(pkt->pts, is->time_base, os->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->dts = av_rescale_q_rnd(pkt->dts, is->time_base, os->time_base, AVRounding(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt->duration = av_rescale_q(pkt->duration, is->time_base, os->time_base);
        pkt->pos = -1; 

        rc = av_interleaved_write_frame(ofmt, pkt);
        if (rc < 0) {
            fprintf(stderr, "remux: write packet failed\n");
            break;
        }
        av_packet_unref(pkt);
    }

    av_write_trailer(ofmt);

out:
    avformat_close_input(&ifmt);
    if (ofmt && !(ofmt->flags & AVFMT_NOFILE)) {
        avio_closep(&ofmt->pb);
    }
    avformat_free_context(ofmt);
    if (rc < 0 && rc != AVERROR_EOF) {
        fprintf(stderr, "remux: %s\n", av_err2str(rc));
        return 1;
    }
    if (pkt) {
        av_packet_free(&pkt);
    }

    return 0;
}


int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "./remux_keyframe video_path output_path\n");
        return 1;
    }

    return remux_keyframe(argv[1], argv[2]);
}