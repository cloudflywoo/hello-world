#include <string.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <android/log.h>
//#include "../../include/ffmpeg/libavcodec/avcodec.h"
#include "avcodec.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

static struct AVFormatContext *pFormatCtx;
static int             i, videoStream;
static struct AVCodecContext  *pAVCodecCtx;
static struct AVCodec         *pAVCodec;
static struct AVPacket        mAVPacket;
static struct AVFrame         *pAVFrame;
static struct AVFrame         *pAVFrameYUV;
static struct SwsContext      *pImageConvertCtx ;

static int iWidth=0;
static int iHeight=0;


#define LOG_TAG "mtxw_ffmpeg_decoder.cpp"

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)


int XwFfmpegDecoderInit( int width, int height)
{
	iWidth=width;
	iHeight=height;

	LOGD("XwFfmpegDecoderInit() width=%d height=%d ",width,height);

	if(pAVCodecCtx!=NULL)
	{
		avcodec_close(pAVCodecCtx);
		pAVCodecCtx=NULL;
	}
	if(pAVFrame!=NULL)
	{
		av_free(pAVFrame);
		pAVFrame=NULL;
	}
	// Register all formats and codecs
    av_register_all();
	LOGD("init() avcodec register success");

	pAVCodec=avcodec_find_decoder(CODEC_ID_H264);
	if(pAVCodec==NULL){
		LOGD("init() failed to get H264 decoder");
		return -1;
	}


	LOGD("avcodec find decoder success");
	//init AVCodecContext
	pAVCodecCtx=avcodec_alloc_context3(pAVCodec);
	if(pAVCodecCtx==NULL){
		LOGD("init() failed to call avcodec_alloc_context3()");
		return -1;
	}

	LOGD("init() avcodec alloc context success");


	/* we do not send complete frames */
	if(pAVCodec->capabilities & CODEC_CAP_TRUNCATED)
        pAVCodecCtx->flags|= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */

	/* open it */
    if (avcodec_open2(pAVCodecCtx, pAVCodec,NULL) < 0)return -1;

	LOGD("Ffmpegh264decoder init() avcodec open success");

	av_init_packet(&mAVPacket);

	pAVFrame=avcodec_alloc_frame();
	if(pAVFrame==NULL){
		LOGD("init() failed to call avcodec_alloc_frame()");
		return -1;
	}

	LOGD("init() avcodec frame allocat success");

	LOGD(" XwFfmpegDecoderInit()  success  , init finished!");




	return 1;
}


int XwFfmpegDecoderDestroy()
{
	if(pAVCodecCtx!=NULL)
	{

		avcodec_close(pAVCodecCtx);
		pAVCodecCtx=NULL;
	}

	if(pAVFrame!=NULL)
	{

		av_free(pAVFrame);
		pAVFrame=NULL;
	}

	return 1;

}


int XwFfmpegDecodeH264Frame(unsigned char *inbuf, int inbuf_size, unsigned char *out)
{
	int i;
	int u_start,v_start;

	avcodec_get_frame_defaults(pAVFrame);
	//pAVFrame=avcodec_alloc_frame();
	mAVPacket.data=inbuf;
	mAVPacket.size=inbuf_size;
	int len =-1, got_picture=0;
	//LOGD("H264data: inbuf_size=%d %x %x %x %x ",inbuf_size,inbuf[0],inbuf[1],inbuf[2],inbuf[3]);
	len = avcodec_decode_video2(pAVCodecCtx, pAVFrame, &got_picture, &mAVPacket);
	//LOGD("Ffmpegh264decoder len=%d got_picture=%d width=%d height=%d inbuf_size=%d",len,got_picture,iWidth,iHeight,inbuf_size);
	if(len<0)
	{
		LOGD("Ffmpegh264decoder len=-1,decode error");
		return len;
	}

	if(got_picture>0)
	{
		//LOGD("Ffmpegh264decoder GOT PICTURE");
		/*pImageConvertCtx = sws_getContext (pAVCodecCtx->width,
						pAVCodecCtx->height, pAVCodecCtx->pix_fmt,
						pAVCodecCtx->width, pAVCodecCtx->height,
						PIX_FMT_RGB565LE, SWS_BICUBIC, NULL, NULL, NULL);
		sws_scale (pImageConvertCtx, pAVFrame->data, pAVFrame->linesize,0, pAVCodecCtx->height, pAVFrame->data, pAVFrame->linesize);

		*/
		//---拷贝y数据
		for(i=0;i<pAVCodecCtx->height;i++){
			memcpy(out+i*pAVCodecCtx->width,pAVFrame->data[0]+i*pAVFrame->linesize[0],pAVCodecCtx->width);
		}

		//--拷贝u数据
		u_start = pAVCodecCtx->width*pAVCodecCtx->height;
		for(i=0;i<pAVCodecCtx->height/2;i++){
			memcpy(out+u_start+i*pAVCodecCtx->width/2,pAVFrame->data[1]+i*pAVFrame->linesize[1],pAVCodecCtx->width/2);
		}

		//--拷贝v数据
		v_start = u_start + pAVCodecCtx->width*pAVCodecCtx->height/4;
		for(i=0;i<pAVCodecCtx->height/2;i++){
			memcpy(out+v_start+i*pAVCodecCtx->width/2,pAVFrame->data[2]+i*pAVFrame->linesize[2],pAVCodecCtx->width/2);
		}
		//LOGD("Ffmpegh264decoder pAVCodecCtx->width=%d pAVCodecCtx->height=%d",pAVCodecCtx->width,pAVCodecCtx->height);
		//LOGD("Ffmpegh264decoder pAVFrame->linesize[0]=%d pAVFrame->linesize[1]=%d pAVFrame->linesize[2]=%d",pAVFrame->linesize[0],pAVFrame->linesize[1],pAVFrame->linesize[2]);
		/*以下这种拷贝方式是有问题的，因为解码器解码出来的yuv数据可能会比iWidth要更大
		memcpy(Picture,pAVFrame->data[0],iWidth*iHeight);
		memcpy(Picture+iWidth*iHeight,pAVFrame->data[1],iWidth*iHeight/4);
		memcpy(Picture+iWidth*iHeight*5/4,pAVFrame->data[2],iWidth*iHeight/4);
		*/

	}
	else{
		LOGD("Ffmpegh264decoder GOT PICTURE fail");
	}

	return len;
}


//#ifdef __cplusplus
//}

//#endif
