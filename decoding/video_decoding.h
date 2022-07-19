#ifndef _VIDEO_DECODING_H
#define _VIDEO_DECODING_H

#ifdef __cplusplus
extern "C" {
#endif

// c library
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/packet.h>
#include <libavutil/frame.h>

#ifdef __cplusplus
}
#endif


#define MAX_ERROR 128

#define VID_OK 0
#define VID_ERR -1
#define VID_ERR_PARAMS -2
#define VID_ERR_FORMAT -3
#define VID_ERR_STREAMS -4
#define VID_ERR_ALLOC -5
#define VID_ERR_CODEC -6
#define VID_ERR_AGAIN -7
#define VID_ERR_EOF -8

typedef struct {
    AVFormatContext *format;
    AVStream *stream;
    int stream_id;
    AVCodecContext *codec;
    AVPacket *packet;
    AVFrame *frame;
    int eof;
} VideoDecoder;


// API
int video_decoder_new(const char *filename, VideoDecoder *decoder);
int video_decoder_frame_read(VideoDecoder *decoder, AVFrame **frame);
void video_decoder_frame_free(AVFrame *frame);
void video_decoder_free(VideoDecoder *decoder);

// demux
int open_video_file(const char *filename, AVFormatContext **context);

// stream
int find_video_stream(AVFormatContext *format, AVStream **stream, int idx);

// codec
int open_video_codec(AVFormatContext *format, AVStream *stream, AVCodecContext **codec);

// decode packet
int decode_packet(AVCodecContext *codec_ctx, AVPacket *packet, AVFrame *frame);

#endif // _VIDEO_DECODING_H
