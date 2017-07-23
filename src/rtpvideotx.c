#include <rtpvideotx.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#ifdef __GNUC__
  #ifndef likely
    #define likely(x)       __builtin_expect((x),1)
  #endif
  #ifndef unlikely
    #define unlikely(x)     __builtin_expect((x),0)
  #endif
#else
  #define likely
  #define unlikely
#endif


#define RTP_HEADER_SIZE 12
#define RFC4175_HEADER_SIZE 2
#define RFC4175_SEGMENT_HEADER_SIZE 6

static struct { 
    /*RtpVideoTx_Format format;*/
    uint8_t           pixelCount; // per pgroup
    uint8_t           byteCount;  // per pgroup
} _RtpVideoTxFormatPixelGroupMap[] = {
        { /*RtpVideoTx_Format_YCbCr411_8bit,*/ 4, 6},
        { /*RtpVideoTx_Format_RGB_8bit,     */ 1, 3},
        { /*RtpVideoTx_Format_RGBA_8bit,    */ 1, 4},
        { /*RtpVideoTx_Format_BGR_8bit,     */ 1, 3},
        { /*RtpVideoTx_Format_BGRA_8bit,    */ 1, 4},
        { /*RtpVideoTx_Format_RGB_10bit,    */ 4, 1},
        { /*RtpVideoTx_Format_RGBA_10bit,   */ 1, 5},
        { /*RtpVideoTx_Format_BGR_10bit,    */ 4, 1},
        { /*RtpVideoTx_Format_BGRA_10bit,   */ 1, 5},
        { /*RtpVideoTx_Format_RGB_12bit,    */ 2, 9},
        { /*RtpVideoTx_Format_RGBA_12bit,   */ 1, 6},
        { /*RtpVideoTx_Format_BGR_12bit,    */ 2, 9},
        { /*RtpVideoTx_Format_BGRA_12bit,   */ 1, 6},
        { /*RtpVideoTx_Format_RGB_16bit,    */ 1, 6},
        { /*RtpVideoTx_Format_RGBA_16bit,   */ 1, 8},
        { /*RtpVideoTx_Format_BGR_16bit,    */ 1, 6},
        { /*RtpVideoTx_Format_BGR_16bit,   */ 1, 8},
        { /*RtpVideoTx_Format_RGBp_5bit,   */ 1, 3},
        { /*RtpVideoTx_Format_RGpB_5bit,   */ 1, 3},
        { /*RtpVideoTx_Format_RpGB_5bit,   */ 1, 3},
        { /*RtpVideoTx_Format_BGRp_5bit,   */ 1, 3},
        { /*RtpVideoTx_Format_BGpR_5bit,   */ 1, 3},
        { /*RtpVideoTx_Format_BpGR_5bit,   */ 1, 3}
  };

typedef struct _RtpVideoTx {
    RtpVideoTx_Format format;
    int socket;
    struct sockaddr_in destination;
    uint16_t mtu;
    uint8_t  header[RTP_HEADER_SIZE + RFC4175_HEADER_SIZE + 4 * RFC4175_SEGMENT_HEADER_SIZE];
    uint8_t* buffer;
    size_t   bufferSize;
    size_t   bufferOffset;

    uint32_t seqno;
    uint32_t timestamp;
} RtpVideoTx;

RtpVideoTx_t RtpVideoTx_new(int sock, const RtpVideoTx_Format format)
{
    uint32_t seed = time(0);
    RtpVideoTx* self = calloc(1, sizeof(RtpVideoTx));

    self->format = format;
    if (sock != -1)
        self->socket = sock;
    else
        self->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    self->destination.sin_family = AF_INET;

    self->mtu = 1500;
    srand(seed); //initialize rand number generator
    uint32_t ssrc = rand_r(&seed);
    self->header[0] = 0x80;
    self->header[1] = 0x7f;
    self->header[2] = (self->seqno>>8)&0xff;
    self->header[3] = (self->seqno>>0)&0xff;
    self->header[8] = (ssrc>>24)&0xff;
    self->header[9] = (ssrc>>16)&0xff;
    self->header[10] = (ssrc>>8)&0xff;
    self->header[11] = (ssrc>>0)&0xff;
    self->header[12] = (self->seqno>>24)&0xff;
    self->header[13] = (self->seqno>>16)&0xff;

    return (RtpVideoTx_t)self;
}

int RtpVideoTx_release( RtpVideoTx_t v )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;
    close(self->socket);
    if (self->buffer)
        free(self->buffer);
    free(self);
    
    return 0;
}

int RtpVideoTx_addDestination( RtpVideoTx_t v, const char* host, unsigned int port )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;
    if (unlikely(!host))
        return -1;
    if (inet_aton(host , &self->destination.sin_addr) == 0)
    {
        return -1;
    }
    self->destination.sin_port = htons(port);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;                 /* Internet address type */
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);  /* Set for any local IP */

    if(bind(self->socket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        if (errno != EINVAL)
            return -1;
    }

    return 0;
}

int RtpVideoTx_removeDestination( RtpVideoTx_t v, const char* host, unsigned int port )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;
    self->destination.sin_port = 0;
    return 0;
}

int RtpVideoTx_setMTU( RtpVideoTx_t v, const unsigned int mtu )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;
    self->mtu = mtu;

    return 0;
}

int RtpVideoTx_getVideoFormat( RtpVideoTx_t v, RtpVideoTx_Format* out_format )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;

    *out_format = self->format;

    return 0;
}

int RtpVideoTx_setPayloadFormat( RtpVideoTx_t v, const uint8_t payloadFormat )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;

    self->header[1] = payloadFormat&0x7f;

    return 0;
}

int RtpVideoTx_setSSRC( RtpVideoTx_t v, const uint32_t ssrc )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;

    self->header[8] = (ssrc>>24)&0xff;
    self->header[9] = (ssrc>>16)&0xff;
    self->header[10] = (ssrc>>8)&0xff;
    self->header[11] = (ssrc>>0)&0xff;

    return 0;
}


int RtpVideoTx_beginFrame( RtpVideoTx_t v, const uint32_t timestamp )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;

    self->timestamp = timestamp;
    RtpVideoTx_flush( v );

    return 0;
}

int RtpVideoTx_getLineBuffer( RtpVideoTx_t v, const unsigned int length, uint8_t** out_buffer )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;

    if (self->buffer == NULL)
    {
        unsigned int bufferSize = self->mtu*4;
        if (length > bufferSize)
            bufferSize = length;
        self->buffer = malloc(bufferSize);
        self->bufferSize = bufferSize;
    }
    else if (self->bufferSize - self->bufferOffset < length)
    {
        fprintf(stderr,"Resizing buffer to %d\n", length + self->bufferOffset);
        uint8_t* newBuffer = realloc(self->buffer, length + self->bufferOffset);
        if (newBuffer == NULL)
            return -1;
        self->buffer = newBuffer;
        self->bufferSize = length + self->bufferOffset;
    }
    *out_buffer = self->buffer;

    return 0;
}

int RtpVideoTx_addLine( RtpVideoTx_t v, unsigned int lineNo, unsigned int pixelOffset, uint32_t length, uint8_t* buffer, unsigned long flags )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;

    struct msghdr msg;
    struct iovec  iov[2];
    const unsigned int pgroupSize = _RtpVideoTxFormatPixelGroupMap[self->format].byteCount;
    const unsigned int pgroupPixelCount = _RtpVideoTxFormatPixelGroupMap[self->format].pixelCount;


    self->header[4] = (self->timestamp>>24)&0xff;
    self->header[5] = (self->timestamp>>16)&0xff;
    self->header[6] = (self->timestamp>>8)&0xff;
    self->header[7] = (self->timestamp>>0)&0xff;

    iov[0].iov_base = self->header;
    iov[0].iov_len = 20;
    msg.msg_name = &self->destination;
    msg.msg_namelen = sizeof(self->destination);
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    msg.msg_flags = 0;
    int maxPayloadSize = self->mtu - (RTP_HEADER_SIZE + RFC4175_HEADER_SIZE + RFC4175_SEGMENT_HEADER_SIZE);
    maxPayloadSize -= maxPayloadSize%pgroupSize;
    int bytesLeft = length;
    while(bytesLeft > 0)
    {
        int payloadSize = bytesLeft;
        if (payloadSize > maxPayloadSize)
        {
            payloadSize = maxPayloadSize;
        }
        self->header[2] = (self->seqno>>8)&0xff;
        self->header[3] = (self->seqno>>0)&0xff;
        self->header[12] = (self->seqno>>24)&0xff;
        self->header[13] = (self->seqno>>16)&0xff;
        self->header[14] = (payloadSize>>8)&0xff;
        self->header[15] = (payloadSize>>0)&0xff;
        self->header[16] = (lineNo>>8)&0xff;
        self->header[17] = (lineNo>>0)&0xff;
        self->header[18] = (pixelOffset>>8)&0xff;
        self->header[19] = (pixelOffset>>0)&0xff;

        bytesLeft -= payloadSize;
        if (bytesLeft <= 0)
        {
            if (flags&0x01 == 0x01) // End of frame/field
            {
                self->header[1] |= 0x80; // Set Marker bit
            }
        }

        iov[1].iov_base = buffer;
        iov[1].iov_len = payloadSize;
        ssize_t sentSize = sendmsg(self->socket,&msg,0);
        if (sentSize != iov[0].iov_len + iov[1].iov_len )
            return -1;

        ++self->seqno;
        pixelOffset += (payloadSize/pgroupSize)*pgroupPixelCount;
        buffer += payloadSize;
        self->bufferOffset += payloadSize;
    }
    if (flags&0x01 == 0x01) { // End of frame/field
         self->header[1] &= 0x7f; // Reset Marker bit
         self->bufferOffset = 0;
    }
    self->bufferOffset = 0;

    return 0;
}

int RtpVideoTx_flush( RtpVideoTx_t v )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    if (unlikely(!self))
        return -1;

    return 0;
}


