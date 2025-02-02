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
char g_inputData[1*1024*1024] = {0};
char g_ouputData[4*1024*1024] = {0};

/***************************************************
 * name:read_raw_data
****************************************************/
int read_raw_data(char *path, char *data, int maxBuf)
{
    FILE *fp = NULL;
    int len = 0;
    if(NULL !=path)
    {
        fp = fopen(path, "r");
    }
  
    if (NULL != fp)
    {   
        memset(data, 0x0, maxBuf);
        len = fread(data, 1, maxBuf, fp);
        fclose(fp);
    }    

    return len;
}

/***************************************************
 * name:write_raw_data_to_file
****************************************************/
int write_raw_data_to_file(char *path, char *data, int dataLen)
{
    FILE *fp = NULL;
    int len = 0;
    int left = dataLen;
    int cnts = 0;

    if (NULL == data || 0 == dataLen)
    {
        printf("write_raw_data_to_file para error return\n");
        return 0;
    }
    if(NULL != path)
    {
        printf("write_raw_data_to_file path IS NULL\n");
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
            if (left >= 4096)
            {
                len += fwrite((void *)data+cnts*4096, 1, 4096, fp);
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

    //Step 1: init
    int ret = ffmpeg_init_h264_decoder();
    printf("Step 1 finished, ret = %d\n", ret);

    //Step 2: get YUV data
    dataLen = read_raw_data(H264_TEST_FILE, g_inputData, sizeof(g_inputData));
    if (0 == dataLen)
    {
        printf("Step 2 failed, Read YUV data error\n");
        return;
    }
    printf("Step 2 finished, readDatalen = %d\n", dataLen);

    //Step 3: decode
    ret =  ffmpeg_decode_h264(g_inputData, dataLen, g_paramBuf, NULL, yuvData);
    printf("Step 3 finished, decode ret = %d width = %d height = %d\n", ret, g_paramBuf[0], g_paramBuf[1]);


    //Put the data to the target file according to the format 420.
    memcpy(g_ouputData, (char *)(yuvData[0]), g_paramBuf[0]*g_paramBuf[1]);
    memcpy(g_ouputData + g_paramBuf[0]*g_paramBuf[1] , (char *)(yuvData[1]), (g_paramBuf[0]*g_paramBuf[1])/4);
    memcpy(g_ouputData + ((g_paramBuf[0]*g_paramBuf[1]))*5/4 , (char *)(yuvData[2]), (g_paramBuf[0]*g_paramBuf[1])/4);

    //Step 4: write yuv data to file
    ret = write_raw_data_to_file(TARGET_YUV_FILE, g_ouputData, g_paramBuf[0]*g_paramBuf[1]*3/2);

    printf("Step 4 write finish len = %d\n", ret);

    return 0;

}