
#include "mtxw_controlblock.h"
#include "mtxw_comm.h"
#include <map>
#include <errno.h>
#include <string.h>
#include "mtxw_osapi.h"

#if (defined WIN32 || WIN)
INT32 MTXW_PlayAudioPcmData(INT32 instId, const UINT8 *pData, UINT32 uLength)
{
    return 0;
}
#else
#include "com_mediatransxw_MediaTransXW.h"
#include <dlfcn.h>

#endif
UINT32 gMTXWLogSwitch = 3;

using namespace std;

//---静态成员变量初始化---------------------------------------------------------------
MTXW_map<INT32,MTXW_Instance*> MTXW_CB::mapInstance = MTXW_map<INT32,MTXW_Instance*>();
INT32 MTXW_CB::uInstanceId = MTXW_INSTANCE_MAX-1;

MTXW_map<INT32,pair<INT32,INT32>> MTXW_CB::mapAssociation = MTXW_map<INT32,pair<INT32,INT32>>();


INT32 MTXW_CB::CreateInstance(bool isAudio,INT32 callId=-1)
{
    //MTXW_FUNC_TRACE();

    MTXW_LOGA("file:%s line:%d function:%s isAudio:%d",__FILE__,__LINE__,__FUNCTION__,isAudio);

    MTXW_LOGA("MTXW_CB::CreateInstance. gMemCnt:%d",mtxwGetMemCnt());
    UINT8 ucIndex = 0;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();


	//对mapInstance上锁
    MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::CreateInstance  mapInstance");

	//---1---判断实例是否达到了最大----------------
    if(mapInstance.size()>=MTXW_INSTANCE_MAX)
    {
        MTXW_LOGE("\nCreateInstance: mapInstance.size()>=MTXW_INSTANCE_MAX!mapInstance.size():%d",mapInstance.size());
        return -1;
    }
		
	//---2----分配instance id--------------------------
    for(ucIndex = 0; ucIndex < MTXW_INSTANCE_MAX;ucIndex++)
    {
        uInstanceId = (uInstanceId+1)%MTXW_INSTANCE_MAX;
        iter = mapInstance.find(uInstanceId);
        if(iter == mapInstance.end())
        {
            break;
        }
    }

    if(ucIndex == MTXW_INSTANCE_MAX)
    {
        MTXW_LOGE("\n MTXW_CB::CreateInstance: uInstanceId is exhaust!");
        return (-1);
    }
		
    MTXW_LOGA("\n MTXW_CB::CreateInstance: uInstanceId:%d",uInstanceId);

	//---3-------创建实例------------------------------
    MTXW_Instance* pInst;

    if(isAudio)
    {
        pInst = new(std::nothrow) MTXW_AudioInstance(uInstanceId,callId);
    }
    else
    {
    	pInst = new (std::nothrow) MTXW_VideoInstance(uInstanceId,callId);
    }

    if(!pInst)
    {
        //内存分配失败
        MTXW_LOGE("MTXW_CB::CreateInstance: pInst is null!");
        return -1;
    }

    mapInstance.insert(pair<INT32,MTXW_Instance*>(uInstanceId,pInst));

    //---2018.4.2 建立音视频实例关联表
    map<INT32,pair<INT32,INT32>>::iterator  aIter = mapAssociation.find(callId);
    if(mapAssociation.end() == aIter){
        pair<INT32,INT32> value = (isAudio)?pair<INT32,INT32>(uInstanceId,-1):pair<INT32,INT32>(-1,uInstanceId);
        mapAssociation.insert(pair<INT32,pair<INT32,INT32>>(callId,value));
    }else{
         (isAudio) ? aIter->second.first=uInstanceId: aIter->second.second=uInstanceId;

         MTXW_LOGA("\n MTXW_CB::CreateInstance: call-id:%d audio-instId:%d video-instId:%d",callId,aIter->second.first,aIter->second.second);
    }
      
    return uInstanceId;

}

void MTXW_CB::DestroyInstance(INT32 instId)
{
    //MTXW_FUNC_TRACE();

    MTXW_LOGA("file:%s line:%d function:%s instId:%d",__FILE__,__LINE__,__FUNCTION__,instId);
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::DestroyInstance  mapInstance");  //对mapInstance上锁

        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\n MTXW_CB::DestroyInstance instanceId %d not found!",instId);
            return;
        }
        pInst = iter->second;
        

        //--删除关联映射表----
        map<INT32,pair<INT32,INT32>>::iterator  aIter = mapAssociation.find(pInst->GetCallId());
        if(mapAssociation.end() != aIter){
             pInst->isAudio() ? (aIter->second.first = -1):(aIter->second.second = -1);

             if((-1 == aIter->second.first) && (-1==aIter->second.second)){
                 mapAssociation.erase(aIter);
                 MTXW_LOGA("MTXW_CB::DestroyInstance: erase associate item(callId:%d)",pInst->GetCallId());
             }
        }else{
            MTXW_LOGE("MTXW_CB::DestroyInstance: can not find associate item(callId:%d)",pInst->GetCallId());
        }
        
        mapInstance.erase(iter);//从媒体实例列表删除

        MTXW_LOGA("MTXW_CB::DestroyInstance: mapAssociation.size()= %d, mapInstance.size()=%d",mapAssociation.size(),mapInstance.size());

        

    }//加锁块



    pInst->Stop();//停止进程

    delete pInst;//释放实例资源

    MTXW_LOGA("MTXW_CB::DestroyInstance. instId:%d gMemCnt:%d",instId,mtxwGetMemCnt());
		
}

INT32 MTXW_CB::Start(INT32 instId)
{
    //MTXW_FUNC_TRACE();

    MTXW_LOGA("file:%s line:%d function:%s instId:%d",__FILE__,__LINE__,__FUNCTION__,instId);

    MTXW_LOGA("MTXW_CB::Start. instId:%d gMemCnt:%d",instId,mtxwGetMemCnt());
    INT8 rslt = -1;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::Start mapInstance"); 

        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\nStart:instanceId %d not found!",instId);
            return -1;
        }
        pInst = iter->second;
        rslt = pInst->Start();

    }


    return rslt;	 
}
INT32 MTXW_CB::Stop(INT32 instId)
{
    //MTXW_FUNC_TRACE();
    MTXW_LOGA("file:%s line:%d  function:%s instId:%d",__FILE__,__LINE__,__FUNCTION__,instId);
    INT8 rslt = -1;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::Stop mapInstance"); 

        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\nStop:instanceId %d not found!",instId);
            return -1;
        }
        pInst = iter->second;
        rslt = pInst->Stop();

    }

    MTXW_LOGA("MTXW_CB::Stop. instId:%d  gMemCnt:%d",instId,mtxwGetMemCnt());
    return rslt;	  
}

INT32 MTXW_CB::SetLocalAddress(INT32 instId,std::string ip,UINT32 port)
{
    //MTXW_FUNC_TRACE();

    MTXW_LOGA("file:%s line:%d  function:%s instId:%d ip:%s port:%d",
                          __FILE__,__LINE__,__FUNCTION__,instId,ip.c_str(),port);
    INT8 rslt = -1;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SetLocalAddress mapInstance"); 

        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\nSetLocalAddress:instanceId %d not found!",instId);
            return -1;
        }
        pInst = iter->second;
        rslt = pInst->SetLocalAddress(ip,port);

    }


    return rslt;	  
}
INT32 MTXW_CB::SetRemoteAddress(INT32 instId,std::string ip,UINT32 port)
{
    //MTXW_FUNC_TRACE();

    MTXW_LOGA("file:%s line:%d  function:%s instId:%d ip:%s port:%d",
                          __FILE__,__LINE__,__FUNCTION__,instId,ip.c_str(),port);
    INT8 rslt = -1;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex()," MTXW_CB::SetRemoteAddress mapInstance");

        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\nSetRemoteAddress:instanceId %d not found!",instId);
            return -1;
        }
        pInst = iter->second;
        rslt = pInst->SetRemoteAddress(ip,port);

    }

    return rslt;	  
}
INT32 MTXW_CB::SetDirection(INT32 instId,MTXW_ENUM_DIRECTION direction)
{
    //MTXW_FUNC_TRACE();

    MTXW_LOGA("file:%s line:%d  function:%s instId:%d direction:%d",
                         __FILE__,__LINE__,__FUNCTION__,instId,direction);
    INT8 rslt = -1;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex()," MTXW_CB::SetDirection mapInstance");
        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\n SetDirection:instanceId %d not found!",instId);
            return -1;
        }
        pInst = iter->second;
        rslt = pInst->SetDirection(direction);

    }

    return rslt;	
}

INT32 MTXW_CB::SetInterface(INT32 instId,std::string strInterface)
{
    //MTXW_FUNC_TRACE();

    MTXW_LOGA("file:%s line:%d  function:%s instId:%d strInterface:%s",
                         __FILE__,__LINE__,__FUNCTION__,instId,strInterface.c_str());
    INT8 rslt = -1;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex()," MTXW_CB::SetInterface mapInstance");
        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\n SetInterface:instanceId %d not found!",instId);
            return -1;
        }
        pInst = iter->second;
        rslt = pInst->SetInterface( strInterface);
    }

    return rslt;	
}

INT32 MTXW_CB::SetFecParam(INT32 instId,
                      UINT32 FecSwitch,
                      UINT32 InputGDLI,UINT32 InputRank,UINT32 SimulationDropRate, UINT32 adaptiveSwitch)
{
     MTXW_LOGA("file:%s line:%d  function:%s instId:%d FecSwitch:%d InputGDLI:%d InputRank:%d SimulationDropRate:%d adaptiveSwitch=%d",
                            __FILE__,__LINE__,__FUNCTION__,instId,FecSwitch,InputGDLI,InputRank,SimulationDropRate,adaptiveSwitch);
    INT8 rslt = -1;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex()," MTXW_CB::SetFecParam mapInstance");
        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\n SetFecParam:instanceId %d not found!",instId);
            return -1;
        }
        pInst = iter->second;
        rslt = pInst->SetFecParam(FecSwitch,InputGDLI,InputRank,SimulationDropRate,adaptiveSwitch);
        return rslt;
        }
}

INT32 MTXW_CB::SetAudioCodecType(INT32 instId,MTXW_ENUM_AUDIO_CODEC_TYPE type)
{
       //MTXW_FUNC_TRACE();

       MTXW_LOGA("file:%s line:%d  function:%s instId:%d type:%d",
                             __FILE__,__LINE__,__FUNCTION__,instId,type);
	map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
	
	MTXW_AudioInstance *pAudioInst=0;
	
	
	MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SetAudioCodecType mapInstance");  //对mapInstance上锁
		
	iter = mapInstance.find(instId);
			
	if(mapInstance.end()==iter)
	{
		return -1;
	}
	pAudioInst = dynamic_cast<MTXW_AudioInstance *>(iter->second);
	
	if(pAudioInst)
	{
		return pAudioInst->SetAudioCodecType(type);
	}
	else
	{ //走到这里，说明instanceId对应是实例不是一个语音类型的instance实例		
		return -1;
	}
}
INT32 MTXW_CB::SetAudioSampleRate(INT32 instId,UINT32 sampleRate)
{
        //MTXW_FUNC_TRACE();

        MTXW_LOGA("file:%s line:%d  function:%s instId:%d sampleRate:%d",
                              __FILE__,__LINE__,__FUNCTION__,instId,sampleRate);
        map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();

        MTXW_AudioInstance *pAudioInst=0;


        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SetAudioSampleRate mapInstance");  

        iter = mapInstance.find(instId);
        	
        if(mapInstance.end()==iter)
        {
            return -1;
        }
        pAudioInst = dynamic_cast<MTXW_AudioInstance *>(iter->second);

        if(pAudioInst)
        {
            return pAudioInst->SetAudioSampleRate(sampleRate);
        }
        else
        { 
            return -1;
        }
}
INT32 MTXW_CB::SetG729Param(INT32 instId,UINT32 frameTime)
{
    //MTXW_FUNC_TRACE();
    MTXW_LOGA("file:%s line:%d  function:%s instId:%d frameTime:%d",
                        __FILE__,__LINE__,__FUNCTION__,instId,frameTime);
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();

    MTXW_AudioInstance *pAudioInst=0;


    MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SetG729Param mapInstance");  

    iter = mapInstance.find(instId);
    	
    if(mapInstance.end()==iter)
    {
        return -1;
    }
    pAudioInst = dynamic_cast<MTXW_AudioInstance *>(iter->second);

    if(pAudioInst)
    {
        return pAudioInst->SetG729Param(frameTime);
    }
    else
    { 
        return -1;
    }
        
    return 0;
}
INT32 MTXW_CB::SetAmrParam(INT32 instId,
                      UINT32 bitRate,
                      UINT32 frameTime,
                      UINT32 payloadFormat)
{
        //MTXW_FUNC_TRACE();
        MTXW_LOGA("file:%s line:%d  function:%s instId:%d bitRate:%d frameTime:%d payloadFormat:%d",
                            __FILE__,__LINE__,__FUNCTION__,instId,bitRate,frameTime,payloadFormat);
        map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();

        MTXW_AudioInstance *pAudioInst=0;


        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SetAudioSampleRate mapInstance");  

        iter = mapInstance.find(instId);
        	
        if(mapInstance.end()==iter)
        {
            return -1;
        }
        pAudioInst = dynamic_cast<MTXW_AudioInstance *>(iter->second);

        if(pAudioInst)
        {
            return pAudioInst->SetAmrParam(bitRate,frameTime,(MTXW_ENUM_AMR_ALIGN_MODE)payloadFormat);
        }
        else
        { 
            return -1;
        }
}
INT32 MTXW_CB::SetVideoCodecType(INT32 instId,MTXW_ENUM_VIDEO_CODEC_TYPE type)
{
    MTXW_LOGA("file:%s line:%d  function:%s instId:%d type:%d",
                       __FILE__,__LINE__,__FUNCTION__,instId,type);
                       
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();

    MTXW_VideoInstance *pVideoInst=0;


    MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SetVideoCodecType mapInstance");  

    iter = mapInstance.find(instId);
    	
    if(mapInstance.end()==iter)
    {
        
        return -1;
    }
    pVideoInst = dynamic_cast<MTXW_VideoInstance *>(iter->second);

    if(pVideoInst)
    {
        return pVideoInst->SetVideoCodecType(type);
    }
    else
    { 
        return -1;
    }
    
    return 0;
}
INT32 MTXW_CB::SetH264param(INT32 instId,UINT32 frameRate,UINT32 rtpRcvBufferSize, UINT32 frameRcvBufferSize, UINT32 initPlayFactor,UINT32 discardflag,UINT32 playYuvflag)
{
    MTXW_LOGA("file:%s line:%d  function:%s instId:%d frameRate:%d rtpRcvBufferSize:%d frameRcvBufferSize:%d initPlayFactor:%d discardflag:%d playYuvflag:%d",
                         __FILE__,__LINE__,__FUNCTION__,instId,frameRate,rtpRcvBufferSize,frameRcvBufferSize,initPlayFactor,discardflag,playYuvflag);
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();

    MTXW_VideoInstance *pVideoInst=0;


    MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SetH264param mapInstance");  

    iter = mapInstance.find(instId);
    	
    if(mapInstance.end()==iter)
    {
        
        return -1;
    }
    pVideoInst = dynamic_cast<MTXW_VideoInstance *>(iter->second);

    if(pVideoInst)
    {
        return pVideoInst->SetH264param(frameRate, rtpRcvBufferSize,frameRcvBufferSize,initPlayFactor,discardflag,playYuvflag);
    }
    else
    { 
        return -1;
    }
    
    return 0;
}

INT32 MTXW_CB::SendAudioData(INT32 instId,UINT8 *pData,UINT32 uLength)
{
       MTXW_FUNC_TRACE();
       map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
	
	MTXW_AudioInstance *pAudioInst=0;
	
	
	MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SendAudioData mapInstance");  //锟斤拷mapInstance锟斤拷锟斤拷
		
	iter = mapInstance.find(instId);
			
	if(mapInstance.end()==iter)
	{
		return -1;
	}
	pAudioInst = dynamic_cast<MTXW_AudioInstance *>(iter->second);
	
	if(pAudioInst)
	{
		return pAudioInst->Send(pData, uLength);
	}
	else
	{ 
		return -1;
	}
    return 0;
}
INT32 MTXW_CB::SendVideoData(INT32 instId,UINT8 *pData,UINT32 uLength,UINT32 rotatedAngle)
{
    MTXW_FUNC_TRACE();
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    INT32 rslt = 0;
    UINT32 uframeRate=0;
	
    MTXW_VideoInstance *pVideoInst=0;

    {
        	MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SendVideoData mapInstance");
        		
        	iter = mapInstance.find(instId);
        			
        	if(mapInstance.end()==iter)
        	{
        		return -1;
        	}
        	pVideoInst = dynamic_cast<MTXW_VideoInstance *>(iter->second);

        	if(!pVideoInst)
        	{
        	    return -1;
        	}

        	rslt = pVideoInst->Send(pData, uLength,rotatedAngle);
        	//uframeRate = pVideoInst->GetFrameRate();

    }

    //控制发视频包的速度，使之和信令协商的帧率一致

    MTXW_LOGV("SendVideoData sleep before %d thread:id%lu",uframeRate,pthread_self());
    //usleep((1000/uframeRate)*1000);// us = ms * 1000
    //if(rslt>0){usleep(rslt*1000);} // us = ms *1000,  rslt value is ms
    MTXW_LOGV("SendVideoData sleep after thread:id%lu",pthread_self());
    /*
    if(rslt == -2)
    {  
        MTXW_LOGI("the Setdirection is receive, but send the video frame, sleep 100ms ");
        //usleep(100*1000);
    }
    */
	
    return 0;


}
INT32 MTXW_CB::SetLogLevel(UINT32 level)
{
    gMTXWLogSwitch = level;  
    //Always print log level
    MTXW_LOGA("\n ENTER MTXW_CB::SetLogLevel gLogSwitch:%d",gMTXWLogSwitch);

    return 0;
}

int MTXW_CB::GetInstanceState(UINT32 instId)
{
    //MTXW_FUNC_TRACE();

    MTXW_LOGA("file:%s line:%d  function:%s instId:%d",
                          __FILE__,__LINE__,__FUNCTION__,instId);
    int rslt = -1;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex()," MTXW_CB::GetIsRunning mapInstance");

        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\n GetIsRunning:instanceId %d not found!",instId);
            return -1;
        }
        pInst = iter->second;
        if(pInst->IsRunning())
        {
            rslt = 1;
        }
        else
        {
            rslt = 0;
        }
    }

    return rslt;	  


}

std::string MTXW_CB::GetVersionName()
{
    MTXW_FUNC_TRACE();

    return std::string("VERSION:MTXW_")+mtxwGetBuildTime();
    
}

//该接口供播放线程调用
INT32 MTXW_CB::PlayAudioPcmData(INT32 instId, const UINT8 *pData, UINT32 uLength)
{
       MTXW_FUNC_TRACE();
       return MTXW_PlayAudioPcmData(instId,pData,uLength);
}

//该接口供播放线程调用
INT32 MTXW_CB::PlayVideoFrameData(INT32 instId, const UINT8 *pData, UINT32 uLength,UINT32 uRotatedAngle,UINT32 uWidth,UINT32 uHeight, UINT32 uSsrc)
{
    MTXW_FUNC_TRACE();
    return MTXW_PlayVedioH264Data(instId,pData,uLength,uRotatedAngle,uWidth,uHeight,uSsrc);
}

//该接口供播放线程调用
INT32 MTXW_CB::PlayVideoYUV(INT32 instId, const UINT8 *pData, UINT32 uLength,UINT32 uRotatedAngle,UINT32 uWidth,UINT32 uHeight, UINT32 uSsrc)
{
    MTXW_FUNC_TRACE();
    return MTXW_PlayVedioYUV(instId,pData,uLength,uRotatedAngle,uWidth,uHeight,uSsrc);
}

//该接口供播放线程调用
INT32 MTXW_CB::UpdateStatistic(INT32 instId, UINT32 statisticId, double statisticValue)
{
    MTXW_FUNC_TRACE();
    return MTXW_UpdateStatistic(instId, statisticId, statisticValue);
}

bool MTXW_CB::isAssoInstSupportFec(INT32 callId,INT32 instId)
{
    
    MTXW_LOGD("MTXW_CB::isAssoInstSupportFec: callId:%d instId=%d",callId,instId);
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::isAssoInstSupportFec  mapInstance");  //对mapInstance上锁

        iter = mapInstance.find(instId);

        map<INT32,pair<INT32,INT32>>::iterator  aIter = mapAssociation.find(callId);
        if(mapAssociation.end() == aIter || mapInstance.end()==iter){
            return true;
        }else
        {
            map<INT32, MTXW_Instance*>::iterator  tmpIter;
            iter->second->isAudio() ?  (tmpIter=mapInstance.find(aIter->second.second)):(tmpIter=mapInstance.find(aIter->second.first));
            
            if(tmpIter==mapInstance.end()){
               return true;
            }
            else
            {
                 //MTXW_LOGD("MTXW_CB::isAssoInstSupportFec: peer-InstId=%d: isPeerSupportFec=%s",tmpIter->first,tmpIter->second->isPeerSupportFec()?"true":"false");
                 return tmpIter->second->isPeerSupportFec();
            }
        }

     }
}

/*
*获取成功就返回true，否则返回false
*/
bool MTXW_CB::GetAssoInstCQI(INT32 callId,INT32 instId,CqiInforStru &assCqi)
{
    
    //MTXW_LOGE("MTXW_CB::GetAssoInstCQI: callId:%d instId=%d",callId,instId);
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::GetAssoInstCQI  mapInstance");  //对mapInstance上锁

        iter = mapInstance.find(instId);

        map<INT32,pair<INT32,INT32>>::iterator  aIter = mapAssociation.find(callId);
        if(mapAssociation.end() == aIter || mapInstance.end()==iter){
            return false;
        }else
        {
            map<INT32, MTXW_Instance*>::iterator  tmpIter;
            iter->second->isAudio() ?  (tmpIter=mapInstance.find(aIter->second.second)):(tmpIter=mapInstance.find(aIter->second.first));
            
            if(tmpIter==mapInstance.end()){
               return false;
            }
            else
            {
                 
                 return tmpIter->second->GetCQIByAssInst(assCqi);
            }
        }

     }
}

 void MTXW_CB::UpdAssoInstCQI(INT32 callId,INT32 instId,CqiInforStru assCqi)
 {
    
    //MTXW_LOGD("MTXW_CB::UpdAssoInstCQI: callId:%d instId=%d",callId,instId);
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::UpdAssoInstCQI  mapInstance");  //对mapInstance上锁

        iter = mapInstance.find(instId);

        map<INT32,pair<INT32,INT32>>::iterator  aIter = mapAssociation.find(callId);
        if(mapAssociation.end() == aIter || mapInstance.end()==iter){
            return ;
        }else
        {
            map<INT32, MTXW_Instance*>::iterator  tmpIter;
            iter->second->isAudio() ?  (tmpIter=mapInstance.find(aIter->second.second)):(tmpIter=mapInstance.find(aIter->second.first));
            
            if(tmpIter==mapInstance.end()){
               return ;
            }
            else
            {
                 
                 return tmpIter->second->RcvCQIFromAssInst(assCqi);
            }
        }

     }
}

INT32 MTXW_CB::SetRcvBuff(INT32 instId,UINT8 *pBuffer,UINT32 uLength)
{
    MTXW_LOGA("file:%s line:%d  function:%s instId:%d pBuffer:%x uLength:%d",
                          __FILE__,__LINE__,__FUNCTION__,instId,(UINT32)pBuffer,uLength);
    INT8 rslt = -1;
    map<INT32, MTXW_Instance*>::iterator  iter = mapInstance.end();
    MTXW_Instance* pInst = 0;

    {
        MTXW_lock   lock(mapInstance.getMutex(),"MTXW_CB::SetRcvBuff mapInstance"); 

        iter = mapInstance.find(instId);

        if(mapInstance.end()==iter)
        {
            MTXW_LOGE("\nSetRcvBuff:instanceId %d not found!",instId);
            return -1;
        }
        pInst = iter->second;
        rslt = pInst->SetPlayBuffer(pBuffer,uLength);

    }


    return rslt;
}

