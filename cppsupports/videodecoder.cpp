#include <stdio.h>

#include "videodecoder.h"

VideoCodec::VideoCodec()
{
    std::cout << "init VideoCodec "<< std::endl;
    pAVCodecCtxDecoder = NULL;
    pAVCodecDecoder = NULL;
    pAVFrameDecoder = NULL;
    pImageConvertCtxDecoder = NULL;
    pFrameYUVDecoder = NULL;
}

VideoCodec::~VideoCodec()
{
    ;   
}

/**************************************************
 * name:ffmpeg_init_video_decoder
 * 
****************************************************/
int VideoCodec::ffmpeg_init_video_decoder(AVCodecParameters *codecParameters)
{
    std::cout << "ffmpeg_init_video_decoder start "<< std::endl;
    if (nullptr == codecParameters) 
    {
        std::cout << "ffmpeg_init_video_decoder codecParameters is NULL."<< std::endl;
        return -1;
    }
    
    //20230721 use release the all point
    ffmpeg_release_video_decoder();

    avcodec_register_all();
    //20230721 find the h264 docode to  codec_id decode
    pAVCodecDecoder = avcodec_find_decoder(codecParameters->codec_id);
    if (nullptr == pAVCodecDecoder) 
    {
        std::cout << "ffmpeg_init_video_decoder pAVCodecDecoder is null:" << std::endl;
        return -2;
    }
    pAVCodecCtxDecoder = avcodec_alloc_context3(pAVCodecDecoder);
    if (nullptr == pAVCodecCtxDecoder) 
    {
        std::cout << "ffmpeg_init_video_decoder pAVCodecCtxDecoder is null." << std::endl;
        ffmpeg_release_video_decoder();
        return -3;
    }

    if (avcodec_parameters_to_context(pAVCodecCtxDecoder, codecParameters) < 0) 
    {
        std::cout << "ffmpeg_init_video_decoder Failed to copy avcodec parameters to codec context." << std::endl;
        ffmpeg_release_video_decoder();
        return -3;
    }
 
    
    if (avcodec_open2(pAVCodecCtxDecoder, pAVCodecDecoder, NULL) < 0)
    {
        std::cout << "ffmpeg_init_video_decoder Failed to open h264 decoder" << std::endl;
        ffmpeg_release_video_decoder();
        return -4;
    }
    
    av_init_packet(&mAVPacketDecoder);
    
    pAVFrameDecoder = av_frame_alloc();
    pFrameYUVDecoder = av_frame_alloc();

    return 0;
}
/*************************************
 * name: ffmpeg_init_h264_decoder
 * description：
 * *************************************/
int VideoCodec::ffmpeg_init_h264_decoder()
{
    std::cout << "ffmpeg_init_h264_decoder start "<< std::endl;
    //20230721 add This registers all available decoders and encoders
    avcodec_register_all();
   
    //20230721 add to find a specific decoder or encoder
    AVCodec *pAVCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (nullptr == pAVCodec)
    {
        std::cout << "ffmpeg_init_h264_decoder pAVCodec is null"<< std::endl;
        return -1;
    }
    //20230721 is a function in FFmpeg that creates an AVCodecContext 
    //(decoder context) that represents the decoder's parameters and state information
    AVCodecContext *pAVCodecCtx = avcodec_alloc_context3(pAVCodec);
    if (nullptr == pAVCodecCtx) 
    {
        std::cout << "ffmpeg_init_h264_decoder pAVCodecCtx is null!"<< std::endl;
        return -2;
    }
    //is a function in FFmpeg that creates AVCodecParameters (Encoder parameters).
    AVCodecParameters *codecParameters = avcodec_parameters_alloc();

    //20230721 Used to copy parameter information from the encoder
    //or decoder context (AVCodecContext) into the AVCodecParameters structure
    //copy pAVCodecCtx -> codecParameters
    if (avcodec_parameters_from_context(codecParameters, pAVCodecCtx) < 0) 
    {
        std::cout << "Failed to copy avcodec parameters from codec context."<< std::endl;
        avcodec_parameters_free(&codecParameters);
        avcodec_free_context(&pAVCodecCtx);
        return -3;
    }
    
    int ret = ffmpeg_init_video_decoder(codecParameters);
    std::cout << "ffmpeg_init_h264_decoder ret:"<< ret << std::endl;
    avcodec_parameters_free(&codecParameters);
    avcodec_free_context(&pAVCodecCtx);

    
    return ret;
}
 /*************************************
 * name: ffmpeg_release_video_decoder
 * description：
 * *************************************/
int VideoCodec::ffmpeg_release_video_decoder() 
{
    std::cout << "ffmpeg_release_video_decoder strat "<< std::endl;

    if (pAVCodecCtxDecoder != NULL) 
    {
        avcodec_free_context(&pAVCodecCtxDecoder);
        pAVCodecCtxDecoder = NULL;
        std::cout << "ffmpeg_release_video_decoder pAVCodecCtxDecoder is null "<< std::endl;
    }


    if (pAVFrameDecoder != NULL) 
    {
        av_packet_unref(&mAVPacketDecoder);
        av_free(pAVFrameDecoder);
        pAVFrameDecoder = NULL;
        std::cout << "ffmpeg_release_video_decoder pAVFrameDecoder is null "<< std::endl;
    }


    if (pFrameYUVDecoder) 
    {
        av_frame_unref(pFrameYUVDecoder);
        av_free(pFrameYUVDecoder);
        pFrameYUVDecoder = NULL;
        std::cout << "ffmpeg_release_video_decoder pFrameYUVDecoder is null "<< std::endl;
    }
    
    if (pImageConvertCtxDecoder) 
    {
        sws_freeContext(pImageConvertCtxDecoder);
        pImageConvertCtxDecoder = NULL;
        std::cout << "ffmpeg_release_video_decoder pImageConvertCtxDecoder is null "<< std::endl;
    }
    std::cout << "ffmpeg_release_video_decoder end "<< std::endl;
        
    return 0;
}
/**************************************************
name: ffmpeg_decode_h264
description: get the output yuv buffer
**************************************************/
int VideoCodec::ffmpeg_decode_h264(unsigned char *inbuf, int inbufSize, 
                                   int *framePara, unsigned char **outRGBBuf, unsigned char *outYUVBuf)
{
    if (!pAVCodecCtxDecoder || !pAVFrameDecoder || !inbuf || inbufSize<=0 || !framePara || (!outRGBBuf && !outYUVBuf)) 
    {
        return -1;
    }
    av_frame_unref(pAVFrameDecoder);
    av_frame_unref(pFrameYUVDecoder);
    
    framePara[0] = 0;
    framePara[1] = 0;
    mAVPacketDecoder.data = inbuf;
    mAVPacketDecoder.size = inbufSize;
    //avcodec_send_packet is a function in FFmpeg that sends compressed audio 
    //or video data (Packet) to a codec for decoding.
    int ret = avcodec_send_packet(pAVCodecCtxDecoder, &mAVPacketDecoder);
    if (0  == ret) 
    {
        
        //vcodec_receive_frame is a function in FFmpeg that gets decoded audio or video frame data from a codec.
        ret = avcodec_receive_frame(pAVCodecCtxDecoder, pAVFrameDecoder);
        if (0 == ret) 
        {
            framePara[0] = pAVFrameDecoder->width;
            framePara[1] = pAVFrameDecoder->height;
            
            if (nullptr != outYUVBuf) 
            {
                //*outYUVBuf = (unsigned char *)pAVFrameDecoder->data;
                //20230721 note add for get the outyuv buffer
                memcpy(outYUVBuf, pAVFrameDecoder->data, sizeof(pAVFrameDecoder->data));
                /*
                In summary, framePara[0], framePara[1], and framePara[2] in the code save the row 
                sizes of the Y, U, and V components 
                in the AVFrame structure, respectively, so that the pixel data is properly
                accessed when processing video frames.
                */
                framePara[2] = pAVFrameDecoder->linesize[0];
                framePara[3] = pAVFrameDecoder->linesize[1];
                framePara[4] = pAVFrameDecoder->linesize[2];
            } 
            else if (outRGBBuf) 
            {
                //pFrameYUVDecoder->data[0] = outRGBBuf;
                pFrameYUVDecoder->data[1] = NULL;
                pFrameYUVDecoder->data[2] = NULL;
                pFrameYUVDecoder->data[3] = NULL;
                int linesize[4] = { pAVCodecCtxDecoder->width * 3, pAVCodecCtxDecoder->height * 3, 0, 0 };
                pImageConvertCtxDecoder = sws_getContext(pAVCodecCtxDecoder->width, pAVCodecCtxDecoder->height, 
                                                            AV_PIX_FMT_YUV420P, 
                                                            pAVCodecCtxDecoder->width, 
                                                            pAVCodecCtxDecoder->height, 
                                                            AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, 
                                                            NULL, NULL, NULL);
                sws_scale(pImageConvertCtxDecoder, (const uint8_t* const *) pAVFrameDecoder->data, pAVFrameDecoder->linesize, 0, pAVCodecCtxDecoder->height, pFrameYUVDecoder->data, linesize);
                sws_freeContext(pImageConvertCtxDecoder);
            }
            return 1;
        } 
        else if (ret == AVERROR(EAGAIN)) 
        {
            return 0;
        } 
        else 
        {
            return -1;
        }
    }
    
    return 0;
}