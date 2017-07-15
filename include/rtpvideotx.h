#ifndef __RTPVIDEOTX_H__
#define __RTPVIDEOTX_H__

#include <stdint.h>

typedef void* RtpVideoTx_t;

typedef enum{
    RtpVideoTx_Format_YCbCr411_8bit,      // Pixelgroup: 4 pixels in 6  bytes
    RtpVideoTx_Format_RGB_8bit,           // Pixelgroup: 1 pixel  in 3  bytes
    RtpVideoTx_Format_RGBA_8bit,          // Pixelgroup: 1 pixel  in 4  bytes
    RtpVideoTx_Format_BGR_8bit,           // Pixelgroup: 1 pixel  in 3  bytes
    RtpVideoTx_Format_BGRA_8bit,          // Pixelgroup: 1 pixel  in 4  bytes
    RtpVideoTx_Format_RGB_10bit,          // Pixelgroup: 4 pixels in 15 bytes
    RtpVideoTx_Format_RGBA_10bit,         // Pixelgroup: 1 pixel  in 5  bytes
    RtpVideoTx_Format_BGR_10bit,          // Pixelgroup: 4 pixels in 15 bytes
    RtpVideoTx_Format_BGRA_10bit,         // Pixelgroup: 1 pixel  in 5  bytes
    RtpVideoTx_Format_RGB_12bit,          // Pixelgroup: 2 pixels in 9  bytes
    RtpVideoTx_Format_RGBA_12bit,         // Pixelgroup: 1 pixel  in 6  bytes
    RtpVideoTx_Format_BGR_12bit,          // Pixelgroup: 2 pixels in 9  bytes
    RtpVideoTx_Format_BGRA_12bit,         // Pixelgroup: 1 pixel  in 6  bytes
    RtpVideoTx_Format_RGB_16bit,          // Pixelgroup: 1 pixel  in 6  bytes
    RtpVideoTx_Format_RGBA_16bit,         // Pixelgroup: 1 pixel  in 8  bytes
    RtpVideoTx_Format_BGR_16bit,          // Pixelgroup: 1 pixel  in 6  bytes
    RtpVideoTx_Format_BGRA_16bit,         // Pixelgroup: 1 pixel  in 8  bytes
}RtpVideoTx_Format;

RtpVideoTx_t RtpVideoTx_new(const RtpVideoTx_Format format);
int RtpVideoTx_release( RtpVideoTx_t v );

int RtpVideoTx_addDestination( RtpVideoTx_t v, const char* host, unsigned int port );
int RtpVideoTx_removeDestination( RtpVideoTx_t v, const char* host, unsigned int port );
int RtpVideoTx_setMTU( RtpVideoTx_t v, const unsigned int mtu );
int RtpVideoTx_getLineBuffer( RtpVideoTx_t v, const unsigned int length, uint8_t** out_buffer );
int RtpVideoTx_addLine( RtpVideoTx_t v, const unsigned int lineNo, const unsigned int pixelOffset, const uint32_t length, uint8_t* buffer, unsigned long flags );
int RtpVideoTx_flush( RtpVideoTx_t v );

#endif // __RTPVIDEO_H__
