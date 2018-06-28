#include <string.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
#include "include/ffmpeg/libavcodec/avcodec.h"

static AVCodecContext *pCodecCtx= NULL;
static AVPacket avpkt;
static FILE * video_file;
static unsigned char *outbuf=NULL;
static unsigned char *yuv420buf=NULL;
static AVFrame * yuv420pframe = NULL;
static int outsize=0;
static int mwidth = 352;
static int mheight = 288;
static int count = 0;
#define LOG_TAG "mwxw_ffmpeg_encoder.c"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

/**
   * yyyy yyyy
   * uv    uv
   * ->
   * yyyy yyyy
   * uu
   * vv
   * @param outputBuffer
   * @param inputBuffer
   * @param width
   * @param height
   * @return
   */
static void trans420spTo420p(unsigned char *outputBuffer, unsigned char *inputBuffer, int width, int height){
	    int i = 0;
	    int n = 0;
	    int j = 0;
	    int frameSize = width * height;
	    int startPos_Y = 0;
	    int startPos_U = frameSize;
	    int startPos_V = startPos_U + frameSize/4;

	    //--1---处理y分量
	    for (i = 0; i < width * height; i++) {
	      outputBuffer[j++] = inputBuffer[i * 2]; // Y
	    }
	    //--2---处理uv(uvuv...uvuv -> uu...uuvv..vv)
	    unsigned char tmp;
	    for (i=0;i<frameSize/2;i+=2) {
	    	//--取出u分量
	    	tmp = outputBuffer[frameSize+i];
	    	outputBuffer[startPos_U+i] = tmp;

	    	//--取出v分量
	    	tmp = outputBuffer[frameSize+i+1];
	    	outputBuffer[startPos_V+i] = tmp;
	    }

	  }

  /**
  yyyy yyyy
  uv    uv
  ->
  yyyy yyyy
  uu
  vv
  */
static void yuv420sp_to_yuv420p(unsigned char* yuv420sp, unsigned char* yuv420p, int width, int height)
  {
      int i, j;
      int y_size = width * height;

      unsigned char* y = yuv420sp;
      unsigned char* uv = yuv420sp + y_size;

      unsigned char* y_tmp = yuv420p;
      //unsigned char* u_tmp = yuv420p + y_size;
      //unsigned char* v_tmp = yuv420p + y_size * 5 / 4;
      unsigned char* v_tmp = yuv420p + y_size;
      unsigned char* u_tmp = yuv420p + y_size * 5 / 4;

      // y
      memcpy(y_tmp, y, y_size);

      // u
      for (j = 0, i = 0; j < y_size/2; j+=2, i++)
      {
          u_tmp[i] = uv[j];
          v_tmp[i] = uv[j+1];
      }
  }

  /**
  yyyy yyyy
  uu
  vv
  ->
  yyyy yyyy
  uv    uv
  */
static void yuv420p_to_yuv420sp(unsigned char* yuv420p, unsigned char* yuv420sp, int width, int height)
  {
      int i, j;
      int y_size = width * height;

      unsigned char* y = yuv420p;
      unsigned char* u = yuv420p + y_size;
      unsigned char* v = yuv420p + y_size * 5 / 4;

      unsigned char* y_tmp = yuv420sp;
      unsigned char* uv_tmp = yuv420sp + y_size;

      // y
      memcpy(y_tmp, y, y_size);

      // u
      for (j = 0, i = 0; j < y_size/2; j+=2, i++)
      {
      // 此处可调整U、V的位置，变成NV12或NV21
  #if 01
          uv_tmp[j] = u[i];
          uv_tmp[j+1] = v[i];
  #else
          uv_tmp[j] = v[i];
          uv_tmp[j+1] = u[i];
  #endif
      }
  }

//====================================================


int XwFfmpegEncoderInit(int width, int height,int bitrate,int framerate)
{
	LOGD("%s ffmpeg-wyf \n",__func__);
    AVCodec * pCodec=NULL;
    avcodec_register_all();
    pCodec=avcodec_find_encoder(AV_CODEC_ID_H264);  //AV_CODEC_ID_H264//AV_CODEC_ID_MPEG1VIDEO
    if(pCodec == NULL) {
        LOGD("%s ffmppeg-wyf ++++++++++++codec not found\n",__func__);
        return -1;
    }
    pCodecCtx=avcodec_alloc_context3(pCodec);
    if (pCodecCtx == NULL) {
    	LOGD("%s ffmpeg-wyf ++++++Could not allocate video codec context\n",__func__);
        return -1;
    }
    /* put sample parameters */
    mwidth = width;
    mheight = height;
    pCodecCtx->bit_rate = bitrate;
    /* resolution must be a multiple of two */
    pCodecCtx->width = width;
    pCodecCtx->height = height;
    /* frames per second */
    pCodecCtx->time_base= (AVRational){1,framerate};
    pCodecCtx->gop_size = 10; /* emit one intra frame every ten frames */
    pCodecCtx->max_b_frames=1;
    pCodecCtx->pix_fmt = PIX_FMT_YUV420P;
    //pCodecCtx->pix_fmt = PIX_FMT_NV21;
    /* open it */
    LOGD("%s ffmpeg width=%d height=%d bit_rate=%d\n",__func__,pCodecCtx->width,pCodecCtx->height,pCodecCtx->bit_rate);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
    	LOGD("%s ffmpeg-wyf +++++++Could not open codec\n",__func__);
        return -1;
    }
    outsize = mwidth * mheight*2;
    outbuf = malloc(outsize*sizeof(char));
    yuv420buf = malloc(outsize*sizeof(char));

    LOGD("%s ffmpeg-wyf  init success",__func__);

    //pFile = fopen("/sdcard/ffmpeg.h264","wb");

	return 1;
}


int XwFfmpegEncoderDestroy()
{
    avcodec_close(pCodecCtx);
    av_free(pCodecCtx);
    //av_freep(&yuv420pframe->data[0]);
    if(yuv420pframe){
      av_free(yuv420pframe);
      yuv420pframe = NULL;
    }
    free(outbuf);
    /*if(pFile){
    fclose(pFile);
    pFile = 0;
    }*/

}


int XwFfmpegEncodeYuvNv21( unsigned char yuvdata[], int yuvdata_size, unsigned char out[])
{
    int frameFinished=0,size=0;

    if(NULL!=yuv420pframe){
    	av_free(yuv420pframe);
    	yuv420pframe=NULL;
    }


    av_init_packet(&avpkt);

    avpkt.data = NULL;//h264data;
    avpkt.size = 0;

    //generate a I frame per 50 input
    if(count%50==0){
       avpkt.flags        |= AV_PKT_FLAG_KEY;
    }
    yuv420pframe=avcodec_alloc_frame();
    int y_size = pCodecCtx->width * pCodecCtx->height;

    uint8_t* picture_buf;
    int size1 = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t*)av_malloc(y_size);
    if (!picture_buf)
    {
        av_free(yuv420pframe);
     }

    avpicture_fill((AVPicture*)yuv420pframe, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);


    //--420sp->420p
    yuv420sp_to_yuv420p(yuvdata,outbuf,pCodecCtx->width,pCodecCtx->height);
    yuv420pframe->pts = count;
    yuv420pframe->data[0] = outbuf;  //Y Data
    yuv420pframe->data[1] = outbuf+ y_size;      // U
    yuv420pframe->data[2] = outbuf+ y_size*5/4;  // V

    size = avcodec_encode_video2(pCodecCtx, &avpkt, yuv420pframe, &frameFinished);

    count++;
    if (size < 0) {
    	LOGD("ffmpeg +++++Error encoding frame\n");

        return -1;
    }
    if(frameFinished){
    	memcpy(out,avpkt.data,avpkt.size);
        size = avpkt.size;
        //fwrite(h264data,1,size,pFile);
        //LOGD("ffmpeg encode success , size = %d %d %d %d %d %d",size,avpkt.data[0],avpkt.data[1],avpkt.data[2],avpkt.data[3],avpkt.data[4]);
    }else{
    	size = -1;
    	LOGD("ffmpeg encode frameFinished is false");
    }
    av_free_packet(&avpkt);
    av_free(yuv420pframe);
    yuv420pframe = NULL;

	return size;
}

