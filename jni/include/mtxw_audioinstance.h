
#ifndef __MTXW_AUDIO_INSTANCE_H__
#define __MTXW_AUDIO_INSTANCE_H__

#include "mtxw_instance.h"
#include "mtxw_rtp.h"
#include "mtxw_amrrtp.h"
#include "mtxw_g729rtp.h"
#include "mtxw_amrcodec.h"
#include "mtxw_g729codec.h"




class MTXW_AudioInstance:public MTXW_Instance
{
private:



    MTXW_ENUM_AUDIO_CODEC_TYPE enCodecType; //音频编码类型
    UINT32 sampleRate;
    UINT32 frameTime;

    UINT32 bitRate; //AMR编码的码率
    MTXW_ENUM_AMR_ALIGN_MODE enAlignMode;//amr payload的对齐模式
    MTXW_Rtp *pRtpObj;
    Amr_Codec *pAmrcodec;
    G729_Codec *pG729codec;

    //---receiver side param

    bool  bFirstRcvPackt;
    UINT16 usRcvWnd;  //接收窗大小
    UINT16 usLastRcvSeqNum;//上次接收的sequence number
    UINT32 ulRcvSSRC;//当前接收到的SSRC
    Mode GetMode();
	bool MTXW_StartSpeechEnhanceLib();

public:
    MTXW_AudioInstance(INT32 uInstanceId,INT32 callId);
    virtual ~MTXW_AudioInstance();



    INT32 SetAudioCodecType(MTXW_ENUM_AUDIO_CODEC_TYPE type);
    INT32 SetAudioSampleRate(UINT32 sampleRate);
    INT32 SetG729Param(UINT32 frameTime);
    INT32 SetAmrParam(UINT32 bitRate,
               UINT32 frameTime,
               MTXW_ENUM_AMR_ALIGN_MODE payloadFormat);



    //---重载父类函数---------------------
    INT32 Send(UINT8 *pPCMData,UINT32 uLength);

    INT32 Start();
    INT32 Stop();


    //codec编码，pInData-入参，内容为原始采样数据(如PCM)  pOutData-出参，内容为编码后的输出(如AMR)，调用者要负责buffer足够大
    INT32 Encode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData);

    //decode解码，pInData-入参，内容为编码后的数据(如AMR)  pOutData-出参，内容为解码后的输出(如PCM)，调用者要确保足够大的buffer
    INT32 Decode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData);

    INT32 Play(MTXW_DATA_BUFFER_STRU *pData);


    /*打包函数，输入是未打包的数据，输出是打包后的数据，函数调用者负责释放vecPackedData里的内存
       pPackedData->pData的数据的数据格式为: |-len0-|--pack0--|-len1-|--pack1--|-len2-|--pack2--|......
           lenX的长度为2字节，主机字节序

    */
    INT32 Pack(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_DATA_BUFFER_STRU* pPackedData);

    INT32 ReceiveRtpPacket(MTXW_DATA_BUFFER_STRU* pInputData);

    INT32 ReceiveAmrRtpPacket(MTXW_DATA_BUFFER_STRU* pInputData, UINT32 ulLength);

    INT32 ReceiveG729RtpPacket(MTXW_DATA_BUFFER_STRU* pInputData, UINT32 ulLength);
    UINT32 GetPlayFrameInterval(){  return  frameTime;}
    UINT16 GetRcvWnd() {  return usRcvWnd;}
    UINT16 GetLastRcvSeqNum(){ return usLastRcvSeqNum;}
    UINT32 GetRcvSSRC(){ return ulRcvSSRC; }

    void SetLastRcvSeqNum(UINT16 LastRSn){ this->usLastRcvSeqNum = LastRSn;}
    void SetRcvSSRC(UINT32 ssrc){ this->ulRcvSSRC = ssrc; }

    
};


#endif //__MTXW_AUDIO_INSTANCE_H__
