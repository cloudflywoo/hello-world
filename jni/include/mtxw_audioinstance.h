
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



    MTXW_ENUM_AUDIO_CODEC_TYPE enCodecType; //��Ƶ��������
    UINT32 sampleRate;
    UINT32 frameTime;

    UINT32 bitRate; //AMR���������
    MTXW_ENUM_AMR_ALIGN_MODE enAlignMode;//amr payload�Ķ���ģʽ
    MTXW_Rtp *pRtpObj;
    Amr_Codec *pAmrcodec;
    G729_Codec *pG729codec;

    //---receiver side param

    bool  bFirstRcvPackt;
    UINT16 usRcvWnd;  //���մ���С
    UINT16 usLastRcvSeqNum;//�ϴν��յ�sequence number
    UINT32 ulRcvSSRC;//��ǰ���յ���SSRC
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



    //---���ظ��ຯ��---------------------
    INT32 Send(UINT8 *pPCMData,UINT32 uLength);

    INT32 Start();
    INT32 Stop();


    //codec���룬pInData-��Σ�����Ϊԭʼ��������(��PCM)  pOutData-���Σ�����Ϊ���������(��AMR)��������Ҫ����buffer�㹻��
    INT32 Encode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData);

    //decode���룬pInData-��Σ�����Ϊ����������(��AMR)  pOutData-���Σ�����Ϊ���������(��PCM)��������Ҫȷ���㹻���buffer
    INT32 Decode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData);

    INT32 Play(MTXW_DATA_BUFFER_STRU *pData);


    /*���������������δ��������ݣ�����Ǵ��������ݣ����������߸����ͷ�vecPackedData����ڴ�
       pPackedData->pData�����ݵ����ݸ�ʽΪ: |-len0-|--pack0--|-len1-|--pack1--|-len2-|--pack2--|......
           lenX�ĳ���Ϊ2�ֽڣ������ֽ���

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
