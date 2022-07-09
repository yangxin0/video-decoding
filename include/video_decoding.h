#ifndef VIDEO_DECODING_H
#define VIDEO_DECODING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#define MAX_ERROR 128

void decode_video_file(const char *video_path, int keyframe);
int decode_packet(AVCodecContext *codec_ctx, AVPacket *packet, AVFrame *frame);
int open_video_codec(AVFormatContext *context, int keyframe,
        AVCodecContext **codec_context, int *stream_index);
int open_video_file(const char *video_path, AVFormatContext **context);
int encode_jpeg(AVFrame *frame, const char *path);
int save_packet(AVPacket *packet, const char *path);


#ifdef __cplusplus
}
#endif

#endif // VIDEO_DECODING_H
