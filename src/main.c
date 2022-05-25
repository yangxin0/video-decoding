#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#define MAX_ERROR 128

int total_frame = 0;
int total_key = 0;

void decode_video_file(const char *video_path, int keyframe);
int decode_packet(AVCodecContext *codec_ctx, AVPacket *packet, AVFrame *frame);
int open_video_codec(AVFormatContext *context, int keyframe,
                     AVCodecContext **codec_context, int *stream_index);
int open_video_file(const char *video_path, AVFormatContext **context);
int encode_jpeg(AVFrame *frame, const char *path);
int save_packet(AVPacket *packet, const char *path);


void decode_video_file(const char *video_path, int keyframe) 
{
    int rc = 0;
    AVFormatContext *context = NULL;
    int stream_index = -1;
    AVCodecContext *codec_context = NULL;
    AVFrame *frame = NULL;
    AVPacket *packet = NULL;
    if ((rc = open_video_file(video_path, &context)) < 0) {
        goto failed;
    }
    if ((rc = open_video_codec(context, keyframe, &codec_context, &stream_index)) < 0) {
        goto failed;
    }
    frame = av_frame_alloc();
    packet = av_packet_alloc();
    if (!frame || !packet) {
        rc = AVERROR(ENOMEM);
        goto failed;
    }
    // 循环的从demuxer中读取packet并送入decoder解码
    while ((rc = av_read_frame(context, packet)) >= 0) {
        if (packet->stream_index == stream_index) {
            rc = decode_packet(codec_context, packet, frame);
            if (rc == AVERROR(EAGAIN)) {
                continue;
            }
            if (rc < 0) {
                break;
            }
        }
    }
    // demuxer数据读取结束，需要把decoder剩余frame读取出来
    if (rc == AVERROR_EOF) {
        rc = decode_packet(codec_context, packet, frame);
    }
failed:
    if (context) avformat_close_input(&context);
    if (codec_context) avcodec_free_context(&codec_context);
    if (packet) av_packet_free(&packet);
    if (frame) av_frame_free(&frame);
    if (rc != AVERROR_EOF) {
        char error[MAX_ERROR];
        av_strerror(rc, error, MAX_ERROR);
        fprintf(stderr, "decode video error: %s\n", error);
        exit(-1);
    }
}


int decode_packet(AVCodecContext *codec_ctx, AVPacket *packet, AVFrame *frame)
{
    char path[128];
    int rc;
    // 把压缩的帧送入解码器
    if ((rc = avcodec_send_packet(codec_ctx, packet)) < 0) return rc;
    av_packet_unref(packet);
    rc = 0;
    while (rc >= 0) {
        // 从解码器中获取已经被解码的帧，由于存在B帧所以送一帧到解码器中可能不会有对应帧
        rc = avcodec_receive_frame(codec_ctx, frame);
        if (rc < 0) {
            return rc;
        }
        total_frame++;
        if (frame->key_frame) total_key++;
        sprintf(path, "extracted/frame_%d_%lld.jpg", total_frame, frame->pts);
        if ((rc =encode_jpeg(frame, path)) < 0) {
            printf("Encode frame failed, seq %d, %lld pts\n", total_frame, frame->pts);
        }
        printf("Frame: key %d, pts %lld, dts %lld\n", frame->key_frame, frame->pkt_dts, frame->pts);
        // unref frame directly
        av_frame_unref(frame);
    }
    return 0;
}


int open_video_codec(AVFormatContext *context, int keyframe,
                     AVCodecContext **codec_context, int *stream_index)
{
    int i, rc;
    AVStream *s;
    const AVCodec *c = NULL;
    AVDictionary *opts = NULL;
    for (i = 0; i < context->nb_streams; i++) {
        s = context->streams[i];
        // 找到第一条视频流
        if (s->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (keyframe) {
                // 丢掉非关键帧
                s->discard = AVDISCARD_NONKEY;
            }
            c = avcodec_find_decoder(s->codecpar->codec_id);
            if (c == NULL) return AVERROR_DECODER_NOT_FOUND;
            break;
        }
    }
    if (c == NULL) return AVERROR_STREAM_NOT_FOUND;
    // 视频解码是一个过程，同样适用AVCodecContext来表示解码状态
    *codec_context = avcodec_alloc_context3(c);
    if (*codec_context == NULL) return AVERROR(ENOMEM);
    if ((rc = avcodec_parameters_to_context(*codec_context, s->codecpar)) < 0) {
        goto  failed;
    }
    // 对于AVFrame结构体使用引用计数，减小内存压力
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((rc = avcodec_open2(*codec_context, c, &opts)) < 0) {
        goto failed;
    }
    *stream_index = i;
    return 0;
failed:
    avcodec_free_context(codec_context);
    *codec_context = NULL;
    if (opts) av_dict_free(&opts);
    return rc;
}


int open_video_file(const char *video_path, AVFormatContext **context)
{
    int rc;
    // 读取视频文件是一个过程，提供了AVFrormatContext表示读取中间状态
    if ((rc = avformat_open_input(context, video_path, NULL, NULL)) < 0) {
        goto failed;
    }
    // 获取视频文件中流信息（包含流的编码信息）
    if ((rc = avformat_find_stream_info(*context, NULL)) < 0) {
        goto failed;
    }
    return 0;
failed:
    avformat_free_context(*context);
    *context = NULL;
    return rc;
}


int encode_jpeg(AVFrame *frame, const char *path) 
{
    const AVCodec *jpeg_encoder = NULL;
    AVCodecContext *jpeg_ctx = NULL;
    AVPacket packet;
    int rc = 0;

    jpeg_encoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!jpeg_encoder) {
        return AVERROR_ENCODER_NOT_FOUND;
    }

    jpeg_ctx = avcodec_alloc_context3(jpeg_encoder);
    if (!jpeg_ctx) {
        return AVERROR_ENCODER_NOT_FOUND;
    }
    
    jpeg_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    // yuv420p is not supported on mac
    // jpeg_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    jpeg_ctx->height = frame->height;
    jpeg_ctx->width = frame->width;
    jpeg_ctx->time_base.num = 1;
    jpeg_ctx->time_base.den = 25;

    if ((rc = av_new_packet(&packet, 0)) < 0) {
        avcodec_free_context(&jpeg_ctx);
        return rc;
    }

    if ((rc = avcodec_open2(jpeg_ctx, jpeg_encoder, NULL)) < 0) {
        goto out;
    }

    if ((rc = avcodec_send_frame(jpeg_ctx, frame)) < 0) {
        goto out;
    }
    
    if ((rc = avcodec_receive_packet(jpeg_ctx, &packet)) < 0) {
        goto out;
    }

    if ((rc = save_packet(&packet, path)) < 0) {
        goto out;
    }

out:
    if (jpeg_ctx) avcodec_free_context(&jpeg_ctx);
    av_packet_unref(&packet);
    return rc;
}


int save_packet(AVPacket *packet, const char *path) 
{
    FILE *fout = fopen(path, "wb");
    if (!fout) {
        fprintf(stderr, "open %s failed\n", path);
        return AVERROR(errno);
    }

    fwrite(packet->data, 1, packet->size, fout);
    fclose(fout);

    return 0;
}


int main(int argc, char *argv[])
{
    int keyframe = 0;
    const char *video_path = "";
    // 提供一个keyframe选项，该选项表示只解码关键帧
    if (argc < 2) {
        fprintf(stderr, "video-frame-extractor video_path [keyframe]\n");
        exit(-1);
    }
    video_path = argv[1];
    if (argc > 2 && !strcmp(argv[2], "keyframe")) keyframe = 1;
    decode_video_file(video_path, keyframe);
    printf("total frame: %d, key frame: %d\n", total_frame, total_key);
    return 0;
}
