#include <cstddef>
#include <cstdio>
#include <cassert>
#include "video_decoding.h"


int video_decoder_new(const char *filename, VideoDecoder *decoder)
{
    int rc;

    if (filename == nullptr || decoder == nullptr) {
        return VID_ERR_PARAMS;
    }

    memset(decoder, 0, sizeof(VideoDecoder));

    rc = open_video_file(filename, &decoder->format);
    if (rc < 0) {
        return rc;
    }

    rc = find_video_stream(decoder->format, &decoder->stream, -1);
    if (rc < 0) {
        video_decoder_free(decoder);
        return VID_ERR_STREAMS;
    }
    decoder->stream_id = rc;

    rc = open_video_codec(decoder->format, decoder->stream, &decoder->codec);
    if (rc < 0) {
        video_decoder_free(decoder);
        return rc;
    }

    // runtime 
    decoder->packet = av_packet_alloc();
    if (decoder->packet == nullptr) {
        video_decoder_free(decoder);
        return VID_ERR_ALLOC;
    }

    decoder->frame = av_frame_alloc();
    if (decoder->frame == nullptr) {
        video_decoder_free(decoder);
        return VID_ERR_ALLOC;
    }

    return VID_OK;
}


int video_decoder_frame_left(VideoDecoder *decoder, AVFrame **frame)
{
    int rc;
    
    while ((rc = decode_packet(decoder->codec, NULL, decoder->frame)) < 0) {
        assert(rc != VID_ERR_EOF);
        // no more frame left
        if (rc == VID_ERR_AGAIN) {
            return VID_ERR_EOF;
        } else {
            return rc;
        }
    }

    *frame = decoder->frame;
    return VID_OK;
}


int video_decoder_frame_read(VideoDecoder *decoder, AVFrame **frame) 
{
    int rc, rc2;
    
    if (decoder == nullptr || frame == nullptr) {
        return VID_ERR_PARAMS;
    }

    if (decoder->eof) {
        return video_decoder_frame_left(decoder, frame);
    } else {
        while ((rc = av_read_frame(decoder->format, decoder->packet)) >= 0) {
            if (decoder->packet->stream_index != decoder->stream_id) {
                continue;
            }
            rc2 = decode_packet(decoder->codec, decoder->packet, decoder->frame);
            assert(rc2 != VID_ERR_EOF);
            if (rc2 == VID_OK) {
                *frame = decoder->frame;
                return VID_OK;
            } else if (rc2 == VID_ERR_AGAIN) {
                continue;
            } else {
                // error
                return rc2;
            }
        }

        if (rc == AVERROR_EOF) {
            decoder->eof = 1;
            return video_decoder_frame_left(decoder, frame);
        }

        return VID_ERR;
    }
}


void video_decoder_frame_free(AVFrame *frame)
{
    av_frame_unref(frame);
}


void video_decoder_free(VideoDecoder *decoder)
{
    assert(decoder != nullptr);
    if (decoder->format) {
      avformat_close_input(&decoder->format);
    }

    if (decoder->codec) {
      avcodec_free_context(&decoder->codec);
    }

    if (decoder->packet) {
        av_packet_free(&decoder->packet);
    }

    if (decoder->frame) {
        av_frame_free(&decoder->frame);
    }
}


int decode_packet(AVCodecContext *codec, AVPacket *packet, AVFrame *frame)
{
    int rc;

    if (packet) {
        if ((rc = avcodec_send_packet(codec, packet)) < 0) {
            return VID_ERR_CODEC;
        }
        av_packet_unref(packet);
    }
    
    rc = avcodec_receive_frame(codec, frame);
    if (rc < 0) {
        if (rc == AVERROR(EAGAIN)) {
            return VID_ERR_AGAIN;
        } else if (rc == AVERROR_EOF) {
            return VID_ERR_EOF;
        }
        return VID_ERR_CODEC;
    }

    return VID_OK;
}


int open_video_codec(AVFormatContext *format, AVStream *stream, AVCodecContext **codec)
{
    int rc;
    AVCodecContext *c;
    const AVCodec *C;
    AVDictionary *opts = nullptr;

    C = avcodec_find_decoder(stream->codecpar->codec_id);
    assert(C != nullptr);
    c = avcodec_alloc_context3(C);
    if (c == nullptr) {
        return VID_ERR_ALLOC;
    }
    rc = avcodec_parameters_to_context(c, stream->codecpar);
    if (rc < 0) {
        avcodec_free_context(&c);
        return VID_ERR_CODEC;
    }

    av_dict_set(&opts, "refcounted_frames", "1", 0);
    rc = avcodec_open2(c, C, &opts);
    if (rc < 0) {
        avcodec_free_context(&c);
        av_dict_free(&opts);
        return VID_ERR_CODEC;
    }

    *codec = c;
    return VID_OK;
}


int find_video_stream(AVFormatContext *format, AVStream **stream, int idx)
{
    AVStream *s = nullptr;

    // preferred stream id
    if (idx >= 0) {
        if (idx >= format->nb_streams) {
            return -1;
        }
        s = format->streams[idx];
        if (s->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            return -1;
        }
    } else {
        // iterate all streams
        for (int i = 0; i < format->nb_streams; i++) {
            s = format->streams[i];
            if (s->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                idx = i;
                break;
            }
            s = nullptr;
        }
    }

    if (s) {
        // make sure that codec exists
        const AVCodec *c = avcodec_find_decoder(s->codecpar->codec_id);
        if (c) {
            *stream = s;
            return idx;
        }
    }

    return -1;
}


int open_video_file(const char *filename, AVFormatContext **format)
{
    int rc;
    AVFormatContext *fmt = nullptr;

    if (filename == nullptr || format == nullptr) {
        return VID_ERR_PARAMS;
    }

    // open demux context
    if ((rc = avformat_open_input(&fmt, filename, NULL, NULL)) < 0) {
        return VID_ERR_FORMAT;
    }

    // read streams info
    if ((rc = avformat_find_stream_info(fmt, NULL)) < 0) {
        avformat_free_context(fmt);
        return VID_ERR_STREAMS;
    }

    *format = fmt;
    return VID_OK;
}


