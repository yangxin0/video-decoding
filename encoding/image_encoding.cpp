#include "image_encoding.h"


int write_packet(AVPacket *packet, const char *filename) 
{
    FILE *fout = fopen(filename, "wb");
    if (!fout) {
        fprintf(stderr, "open %s failed\n", filename);
        return AVERROR(errno);
    }

    fwrite(packet->data, 1, packet->size, fout);
    fclose(fout);

    return 0;
}


int encode_jpeg(AVFrame *frame, const char *filename) 
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
    
#ifdef __APPLE__
    jpeg_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
#else
    // yuv420p is not supported on mac
    jpeg_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
#endif
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

    if ((rc = write_packet(&packet, filename)) < 0) {
        goto out;
    }

out:
    if (jpeg_ctx) avcodec_free_context(&jpeg_ctx);
    av_packet_unref(&packet);
    return rc;
}
