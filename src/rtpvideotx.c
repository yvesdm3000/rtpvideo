#include <rtpvideotx.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

#define RTP_HEADER_SIZE 12
#define RFC4175_HEADER_SIZE 2
#define RFC4175_SEGMENT_HEADER_SIZE 6

static struct { 
    RtpVideoTx_Format format;
    uint8_t           pixelCount;
    uint8_t           byteCount;
} _RtpVideoTxFormatSizeMap[] = {
        { RtpVideoTx_Format_YCbCr411_8bit, 4, 6},
        { RtpVideoTx_Format_RGB_8bit,      1, 3},
        { RtpVideoTx_Format_RGBA_8bit,     1, 4},
        { RtpVideoTx_Format_BGR_8bit,      1, 3},
        { RtpVideoTx_Format_BGRA_8bit,     1, 4},
        { RtpVideoTx_Format_RGB_10bit,     4, 1},
        { RtpVideoTx_Format_RGBA_10bit,    1, 5},
        { RtpVideoTx_Format_BGR_10bit,     4, 1},
        { RtpVideoTx_Format_BGRA_10bit,    1, 5},
        { RtpVideoTx_Format_RGB_12bit,     2, 9},
        { RtpVideoTx_Format_RGBA_12bit,    1, 6},
        { RtpVideoTx_Format_BGR_12bit,     2, 9},
        { RtpVideoTx_Format_BGRA_12bit,    1, 6},
        { RtpVideoTx_Format_RGB_16bit,     1, 6},
        { RtpVideoTx_Format_RGBA_16bit,    1, 8},
        { RtpVideoTx_Format_BGR_16bit,     1, 6},
        { RtpVideoTx_Format_BGRA_16bit,    1, 8} };

typedef struct _RtpVideoTx {
    RtpVideoTx_Format format;
    int socket;
    struct sockaddr_in destination;
    uint16_t mtu;
    uint8_t  header[RTP_HEADER_SIZE + RFC4175_HEADER_SIZE + 4 * RFC4175_SEGMENT_HEADER_SIZE];
    uint8_t* buffer;
    size_t   bufferSize;

    uint32_t seqno;
    uint32_t timestamp;
    uint32_t ssrc;
} RtpVideoTx;

RtpVideoTx_t RtpVideoTx_new(const RtpVideoTx_Format format)
{
    uint32_t seed = time(0);
    RtpVideoTx* self = calloc(1, sizeof(RtpVideoTx));

    self->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    self->destination.sin_family = AF_INET;



    self->mtu = 1492;
    srand(seed); //initialize rand number generator
    self->ssrc = rand_r(&seed);
    self->header[0] = 0x80;
    self->header[1] = 0x7f;
    self->header[2] = (self->seqno>>8)&0xff;
    self->header[3] = (self->seqno>>0)&0xff;
    self->header[8] = (self->ssrc>>24)&0xff;
    self->header[9] = (self->ssrc>>16)&0xff;
    self->header[10] = (self->ssrc>>8)&0xff;
    self->header[11] = (self->ssrc>>0)&0xff;
    self->header[12] = (self->seqno>>24)&0xff;
    self->header[13] = (self->seqno>>16)&0xff;

    return (RtpVideoTx_t)self;
}

int RtpVideoTx_release( RtpVideoTx_t v )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    close(self->socket);
    if (self->buffer)
        free(self->buffer);
    free(self);
    
    return 0;
}

int RtpVideoTx_addDestination( RtpVideoTx_t v, const char* host, unsigned int port )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
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
        return -1;
    }

    return 0;
}

int RtpVideoTx_removeDestination( RtpVideoTx_t v, const char* host, unsigned int port )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    self->destination.sin_port = 0;
    return 0;
}

int RtpVideoTx_setMTU( RtpVideoTx_t v, const unsigned int mtu )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    self->mtu = mtu;
    return 0;
}

int RtpVideoTx_getLineBuffer( RtpVideoTx_t v, const unsigned int length, uint8_t** out_buffer )
{
    RtpVideoTx* self = (RtpVideoTx*)v;


    if (self->buffer == NULL)
    {
        unsigned int bufferSize = self->mtu;
        if (length > bufferSize)
            bufferSize = length;
        self->buffer = malloc(bufferSize);
        self->bufferSize = bufferSize;
    }
    else if (self->bufferSize < length)
    {
        uint8_t* newBuffer = realloc(self->buffer, length);
        if (newBuffer == NULL)
            return -1;
        self->buffer = newBuffer;
        self->bufferSize = length;
    }
    *out_buffer = self->buffer;
    return 0;
}

int RtpVideoTx_addLine( RtpVideoTx_t v, const unsigned int lineNo, const unsigned int pixelOffset, const uint32_t length, uint8_t* buffer, unsigned long flags )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    struct msghdr msg;
    struct iovec  iov[2];
    iov[0].iov_base = self->header;
    iov[0].iov_len = 18;
    memset(&msg,0,sizeof(struct msghdr));
    msg.msg_name = &self->destination;
    msg.msg_namelen = sizeof(self->destination);
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;
    const int maxPayloadSize = self->mtu - (RTP_HEADER_SIZE + RFC4175_HEADER_SIZE + RFC4175_SEGMENT_HEADER_SIZE);
    int bytesLeft = length;
    while(bytesLeft > 0)
    {
        int payloadSize = bytesLeft;
        if (payloadSize > maxPayloadSize)
        {
            payloadSize = maxPayloadSize;
        }
        bytesLeft -= payloadSize;
        ++self->seqno;
        self->header[2] = (self->seqno>>8)&0xff;
        self->header[3] = (self->seqno>>0)&0xff;
        self->header[12] = (self->seqno>>24)&0xff;
        self->header[13] = (self->seqno>>16)&0xff;
        self->header[14] = (payloadSize>>24)&0xff;
        self->header[15] = (payloadSize>>16)&0xff;
        self->header[16] = (payloadSize>>8)&0xff;
        self->header[17] = (payloadSize>>0)&0xff;
        self->header[14] = (lineNo>>24)&0x7f;
        self->header[15] = (lineNo>>16)&0xff;
        self->header[16] = (lineNo>>8)&0xff;
        self->header[17] = (lineNo>>0)&0xff;
        self->header[14] = (pixelOffset>>24)&0x7f;
        self->header[15] = (pixelOffset>>16)&0xff;
        self->header[16] = (pixelOffset>>8)&0xff;
        self->header[17] = (pixelOffset>>0)&0xff;

        iov[1].iov_base = self->buffer;
        iov[1].iov_len = length;
        sendmsg(self->socket,&msg,0);
    }

    return 0;
}

int RtpVideoTx_flush( RtpVideoTx_t v )
{
    RtpVideoTx* self = (RtpVideoTx*)v;
    return 0;
}


