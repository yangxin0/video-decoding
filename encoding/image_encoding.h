#ifndef _IMAGE_ENCODING_H_
#define _IMAGE_ENCODING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>

#ifdef __cplusplus
}
#endif

int write_packet(AVPacket *packet, const char *filename);
int encode_jpeg(AVFrame *frame, const char *filename); 

#endif // _IMAGE_ENCODING_H_