#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "videodecoder.h"

/* H264 source data,  I frame */
#define  H264_TEST_FILE  "./Iframe4test.h264"

/* Target yuv data, yuv 420 */
#define  TARGET_YUV_FILE "./target420.yuv"

int g_paramBuf[8] = {0};
unsigned char g_inputData[1*1024*1024] = {0};
unsigned char g_ouputData[4*1024*1024] = {0};

/*****************************************************
 * name: read_raw_data
 * ***************************************************/
int read_raw_data(char *path, unsigned char *data, int maxBuf)
{
    //20230720 add for judge start
    if(nullptr == path)
    {
        return 0;
    }
    //20230720 add for judge end
    FILE *fp = NULL;
    int len = 0;

    fp = fopen(path, "r");

    if (NULL != fp)
    {   
        memset(data, 0x0, maxBuf);
        len = fread(data, 1, maxBuf, fp);
        fclose(fp);
    }    

    return len;
}

/*****************************************************
 * name: write_raw_data_to_file
 * ***************************************************/
int write_raw_data_to_file(char *path, unsigned char *data, int dataLen)
{
    //20230720 add for judge start
    if(nullptr == path)
    {
        return 0;
    }
    //20230720 add for judge end
    FILE *fp = NULL;
    int len = 0;
    int left = dataLen;
    int cnts = 0;

    if (NULL == data || 0 == dataLen)
    {
        std::cout << "write_raw_data_to_file data  is null "<< std::endl;
        return 0;
    }

    if (access(path, F_OK) == 0)
    {
        remove(path);
    }

    fp = fopen(path, "ab+");

    if (NULL != fp)
    {   
        while(left)
        {
            //write the file
            if (left >= 4096)
            {
                len += fwrite((void *)data + cnts*4096, 1, 4096, fp);
                cnts++;
                left -= 4096;
                sync();
            }            
            else
            {
                len += fwrite(data+cnts*4096, 1, left, fp);
                break;
            }
        }
        
        fclose(fp);
    }    

    return len;
}


int main()
{
    int dataLen = 0;
    int index = 150;
    unsigned char *yuvData[10] = {NULL};
    char *rgbData = NULL;

    std::cout << "main start "<< std::endl;

    VideoCodec *VideoCodecObj = new VideoCodec();

    //Step 1: 20230721 init the h264 decode
    int ret = VideoCodecObj->ffmpeg_init_h264_decoder();
    std::cout <<"main ret:"<<  ret << std::endl;

    //Step 2: 20230721 get YUV data buffer
    dataLen = read_raw_data(H264_TEST_FILE, g_inputData, sizeof(g_inputData));
    if (0 == dataLen)
    {
        std::cout <<" main dataLen is 0"<< std::endl;
        return 1;
    }

    //Step 3: decode
    ret =  VideoCodecObj->ffmpeg_decode_h264(g_inputData, dataLen, g_paramBuf, NULL, (unsigned char *)yuvData);
    std::cout <<"Step 3 finished, decode ret" << ret << "width" << g_paramBuf[0] << "height" <<  g_paramBuf[1] << std::endl;


    //20230721 note,Put the data to the target file according to the format 420.
    memcpy(g_ouputData, (char *)(yuvData[0]), g_paramBuf[0]*g_paramBuf[1]);
    memcpy(g_ouputData + g_paramBuf[0]*g_paramBuf[1] , (char *)(yuvData[1]), (g_paramBuf[0]*g_paramBuf[1])/4);
    memcpy(g_ouputData + ((g_paramBuf[0]*g_paramBuf[1]))*5/4 , (char *)(yuvData[2]), (g_paramBuf[0]*g_paramBuf[1])/4);
    /*
    memcpy(g_outputData, (char *)(yuvData[0]), g_paramBuf[0]*g_paramBuf[1])：
    这行代码将 Y 分量数据（yuvData[0]）复制到 g_outputData 缓冲区。g_paramBuf[0]*g_paramBuf[1] 表示 Y 分量的数据大小，即视频帧的像素数。

    memcpy(g_outputData + g_paramBuf[0]*g_paramBuf[1], (char *)(yuvData[1]), (g_paramBuf[0]*g_paramBuf[1])/4)：
    这行代码将 U 分量数据（yuvData[1]）复制到 g_outputData 缓冲区中的 U 分量数据位置。U 分量的数据大小是 Y 分量的 1/4，因为 YUV420 格式中 U 和 V 分量的分辨率是 Y 分量的 1/2。

    memcpy(g_outputData + ((g_paramBuf[0]*g_paramBuf[1]))*5/4, (char *)(yuvData[2]), (g_paramBuf[0]*g_paramBuf[1])/4)：
    这行代码将 V 分量数据（yuvData[2]）复制到 g_outputData 缓冲区中的 V 分量数据位置。V 分量的数据大小也是 Y 分量的 1/4。
    */
    //Step 4: write yuv data to file
    ret = write_raw_data_to_file(TARGET_YUV_FILE, g_ouputData, g_paramBuf[0]*g_paramBuf[1]*3/2);

    VideoCodecObj->ffmpeg_release_video_decoder();

    std::cout <<"Step 4 write finish ret:" << ret << std::endl;

    return 0;

}