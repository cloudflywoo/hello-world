
#ifndef __MTXW_CONTROL_BLOCK_H__
#define __MTXW_CONTROL_BLOCK_H__


#include "mtxw_comm.h"
#include "mtxw_instance.h"
#include "mtxw_audioinstance.h"
#include "mtxw_videoinstance.h"
#include "rtpfec.h"

#include <map>
#include <string>

class MTXW_CB
{
private:
	static MTXW_map<INT32,MTXW_Instance*> mapInstance;
	static INT32 uInstanceId ;

        //2018.4.2 增加语音和视频的关联表
	static MTXW_map<INT32,std::pair<INT32,INT32>> mapAssociation;//音视频实例关联表，key为call-id, value为pair<audioInstId, VideoinstId>

private:
    //构造函数私有化
	MTXW_CB(){}

public:
	static INT32 CreateInstance(bool isAudio,INT32 callId);
	static void DestroyInstance(INT32 instanceId);

	static INT32 Start(INT32 instanceId);
	static INT32 Stop(INT32 instanceId);

	static INT32 SetLocalAddress(INT32 instanceId,std::string ip,UINT32 port);
	static INT32 SetRemoteAddress(INT32 instanceId,std::string ip,UINT32 port);
	static INT32 SetDirection(INT32 instanceId,MTXW_ENUM_DIRECTION direction);
	static INT32 SetInterface(INT32 instId,std::string strInterface);
	static INT32 SetFecParam(INT32 instanceId, UINT32 FecSwitch, UINT32 InputGDLI,UINT32 InputRank,UINT32 SimulationDropRate,UINT32 adaptiveSwitch);


	static INT32 SetAudioCodecType(INT32 instanceId,MTXW_ENUM_AUDIO_CODEC_TYPE type);
	static INT32 SetAudioSampleRate(INT32 instanceId,UINT32 sampleRate);
	static INT32 SetG729Param(INT32 intanceId,UINT32 frameRate);
	static INT32 SetAmrParam(INT32 instanceId,
	                  UINT32 bitRate,
	                  UINT32 frameTime,
	                  UINT32 payloadFormat);
					  
					  
	static INT32 SetVideoCodecType(INT32 instanceId,MTXW_ENUM_VIDEO_CODEC_TYPE type);
	static INT32 SetH264param(INT32 instId,UINT32 frameRate,UINT32 rtpRcvBufferSize, UINT32 frameRcvBufferSize, UINT32 initPlayFactor,UINT32 discardflag,UINT32 playYuvflag);
	static INT32 SendAudioData(INT32 instanceId,UINT8 *pData,UINT32 uLength);
	static INT32 SendVideoData(INT32 instanceId,UINT8 *pData,UINT32 uLength,UINT32 rotatedAngle);
	static INT32 SetLogLevel(UINT32 level);

	static int GetInstanceState(UINT32 instId);

	static std::string GetVersionName();

	static INT32 PlayAudioPcmData(INT32 instanceId, const UINT8 *pData, UINT32 uLength); //该接口供播放线程调用
	static INT32 PlayVideoFrameData(INT32 instId, const UINT8 *pData, UINT32 uLength,UINT32 uRotatedAngle,UINT32 uWidth,UINT32 uHeight,UINT32 uSsrc);//该接口供播放线程调用
       static INT32 PlayVideoYUV(INT32 instId, const UINT8 *pData, UINT32 uLength,UINT32 uRotatedAngle,UINT32 uWidth,UINT32 uHeight,UINT32 uSsrc);//该接口供播放线程调用
	static INT32 UpdateStatistic(INT32 instId, UINT32 statisticId, double statisticValue);

	//2018.4.2 获取关联实例是否支持FEC
	static bool isAssoInstSupportFec(INT32 callId,INT32 instId);
       //2018.4.26 获取关联实例的CQI
	static bool GetAssoInstCQI(INT32 callId,INT32 instId,CqiInforStru &assCqi);
	//2018.4.26 更新关联实例的CQI
	static void UpdAssoInstCQI(INT32 callId,INT32 instId,CqiInforStru assCqi);

	 static INT32 SetRcvBuff(INT32 instId,UINT8 *pBuffer,UINT32 uLength);
	
	 
};

#endif //__MTXW_CONTROL_BLOCK_H__
