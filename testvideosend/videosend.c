#include <stdio.h>
#include <rtpvideotx.h>
#include <unistd.h>
#include <string.h>

int gKeepRunning = 1;

int main(int argc, char* argv[])
{
    RtpVideoTx_t video = RtpVideoTx_new(-1, RtpVideoTx_Format_RGB_8bit);
 
    uint32_t timestamp = 0;
    uint32_t timestampStep = 90000/25;    
 
    int rows = 576;
    int cols = 720;

    int row;

    int croptop = 0;
    int cropleft = 0; 
    int cropbottom = 0;
    int cropright = 0;

    if (argc < 3)
    {
        fprintf(stderr,"Usage: %s remotehost remoteport croptop cropleft cropbottom cropright\n", argv[0]);
        return -1;
    }
    if (argc >= 4)
    {
        croptop = atoi(argv[3]);
    }
    if (argc >= 5)
    {
        cropleft = atoi(argv[4]);
    }
    if (argc >= 6)
    {
        cropbottom = atoi(argv[5]);
    }
    if (argc >= 7)
    {
        cropright = atoi(argv[6]);
    }
    
    RtpVideoTx_addDestination(video, argv[1], atoi(argv[2]));
    uint8_t c = 127;
    int8_t dc = 1;
    RtpVideoTx_setSSRC(video,0);
    int lineBytes = (cols - cropleft - cropright)*3;

    while (gKeepRunning)
    {
        uint8_t* buffer;
        RtpVideoTx_beginFrame(video, timestamp);
        timestamp += timestampStep;
        for ( row=croptop; row<rows - cropbottom;++row)
        {
            int ret = RtpVideoTx_getLineBuffer(video, lineBytes,&buffer);
            memset(buffer,c,cols*3);
            unsigned long opt = 0;
            if (row == rows-1)
                opt = 0x01;
            ret = RtpVideoTx_addLine(video, row, cropleft, lineBytes, buffer, opt);
            if (ret < 0)
                fprintf(stderr,"Failed to add line on row %d\n",row);
//            if (row%(rows/4) == 0)
//                usleep(10000);
            if (c == 0)
               dc = 1;
            else if (c == 255)
               dc = -1;
            c+=dc;
        }
        usleep(40000);
    }
    return 0;
}
