
#ifndef __MTXW_VIDEO_INSTANCE_H__
#define __MTXW_VIDEO_INSTANCE_H__

#include "mtxw_instance.h"
#include "mtxw_h264rtp.h"
#include "mtxw_comm.h"



extern INT32 mtxw_H264FragmentDataCompare(const MTXW_DATA_BUFFER_POINTER &,
                                                                                      const MTXW_DATA_BUFFER_POINTER &);

struct MTXW_H264_RCV_FRAME_LIST_STRU
{
    UINT32 ulTimStamp;  //ʱ�������RTP header��ȡ
    UINT32 ulExpectSN;  //������SN
    UINT16 ulRotateAngle; //��ת�Ƕ�
    UINT32 ulLengthOfFragmentData;//ͬһ��H264֡�Ķ��RTPPayLoadContent������֮��,�ó��ȿ��ܻ��ʵ�ʰ���Ҫ��ֻ���������ڴ��ã�������׼ȷ��ʾ��װ���H264֡���ݴ�С

    bool     bCompleted; //��ʾ���յ������Ƿ��Ѿ�����

    /*
    *ͬһ��֡��Ķ��RTP�����������fragmentDataList��
    */
    MTXW_sortlist<MTXW_DATA_BUFFER_POINTER> FragmentDataList; 

    MTXW_H264_RCV_FRAME_LIST_STRU()
    {
        FragmentDataList.setcomparefun(mtxw_H264FragmentDataCompare);

        ulTimStamp = 0;

        ulExpectSN = 0;
        
        ulRotateAngle = 0;
        
        ulLengthOfFragmentData = 0;

        bCompleted = false;
        
    }
    ~MTXW_H264_RCV_FRAME_LIST_STRU()
    {
        
    }
    
};

extern INT32 mtxw_H264FrameListCompare(const MTXW_H264_RCV_FRAME_LIST_STRU & E1,
                                                                                      const MTXW_H264_RCV_FRAME_LIST_STRU & E2);

struct MTXW_H264_RCV_BUFFER_STRU{

     MTXW_sortlist<MTXW_H264_RCV_FRAME_LIST_STRU> RcvFrameList;//(mtxw_H264FrameListCompare);

     MTXW_H264_RCV_BUFFER_STRU()
     {
         RcvFrameList.setcomparefun(mtxw_H264FrameListCompare);
     }

     ~MTXW_H264_RCV_BUFFER_STRU()
     {

     }
     
};


class MTXW_VideoInstance:public MTXW_Instance
{
private:

    MTXW_ENUM_VIDEO_CODEC_TYPE enCodecType; //��Ƶ��������

    UINT32 frameRate;//֡�ʣ�ȡֵ10-30֡/��

    bool bdiscardflag;//false:��������֡ true:������֡
    
    MTXW_ENUM_CONTINUE_DROP_FRAME_TYPE bContinueDropFrame;//0:ֹͣ��֡,1:���ڻ�֡����,������ֱ֡������I֡�óɣ�2:���ڲ��Ŷ���Buffer����,������ֱ֡������I֡�ó�
    UINT32 rtpRcvBufferSize;//rtp����buffer��С
    UINT32 frameRcvBufferSize; //֡���ջ����С
    UINT32 initPlayFactor; //��ʼ�������ӣ�ȡֵ[0-100]����ʾ�ٷֱ�

    MTXW_H264_RCV_BUFFER_STRU  h264RcvBuffer; //���ջ������
    UINT32 PopExpectSN;//��ǰtimeStamp frameList�����һ�����ݰ�������SN
    
    MTXW_Rtp *pRtpObj;
    
    bool bFirstRcvPackt;
    UINT32 ulRcvSSRC;
    UINT32 ulTimeStampWndStart; //���մ�����㣬С���������timeStamp��RTP��ֱ�Ӷ���
    UINT32  ulTimeStampRcvWnd;  //���մ���С

    UINT32 ulH264RcvBufferMax;  //���ջ�������ֵ(Ҳ��������м���timestamp�ڵ�)

    UINT32 ulRcvFrameWidth;//���յ���Ƶ�Ŀ���SPS�����ȡ��ÿ�յ�SPS�����
    UINT32 ulRcvFrameHeight;//���յ���Ƶ�ĸߣ���SPS�����ȡ
#ifndef WIN32
    struct timespec lastRecvFrameTime;
    struct timespec lastSendFrameTime;
#endif
    double smoothFactor;
    double delta;    //֡��������(ͳ��ֵ)  unit- nano second

    double sendDelta;//��֡����(ͳ��ֵ) unit - nano second
    INT32  needDelaySendTime; //���ڿ��Ʒ��͵�֡�ʣ�ʹ��ʵ�ʷ��͵�֡�ʺ�����Э��֡��һ��,unit-ms


    UINT32 playYuvflag; //0-����H264֡ 1-��ʾ����YUV֡
    UINT8   mRcvSps[256]; //|-startcode-|-naluhdr-|----sps data----|
    UINT32 mRcvSpsLength;
    UINT8 mRcvPps[256];//|-startcode-|-naluhdr-|----pps data----|
    UINT32 mRcvPpsLength;

    bool  bRcvWndActive; //���մ������Ƿ����ñ�־��true=����,false=������;
                                    //����: ���ڽ��մ���Χ�ڵ�RTP�ᱻ�����ظ�������
                                    //������:  ���ᶪ��RTP����������һ�������þ����ظ�RTPҲ������
                                    
    
public:
    MTXW_VideoInstance(INT32 uInstanceId,INT32 callId);
    virtual ~MTXW_VideoInstance();    
    INT32 ReceiveRtpPacket(MTXW_DATA_BUFFER_STRU* pInputData);
    INT32 ReceiveH264RtpPacket(MTXW_DATA_BUFFER_STRU* pInputData, UINT32 RtpHeadLength, MTXW_Rtp rcvRtpPact);
    INT32 Start();

    INT32 Pack(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_DATA_BUFFER_STRU* pPackedData);
	INT32 Stop();
    INT32 SetVideoCodecType(MTXW_ENUM_VIDEO_CODEC_TYPE type);
    /*
    * frameRate:֡�ʣ���ȷ��ȡֵ��Χ[10-30]
    * rtpRcvBufferSize: rtp���ջ����С����λΪ�룬����ȡֵ��Χ[1-4]
    * frameRcvBufferSize: ֡���ջ����С����λΪ�룬����ȡֵ��Χ[1-8]
    * initPlayFactor: ��ʼ��������,����ȡֵ��Χ[0-100]��ʾ 0%-100%
    */
    INT32 SetH264param(UINT32 frameRate,UINT32 rtpRcvBufferSize, UINT32 frameRcvBufferSize, UINT32 initPlayFactor,UINT32 discardflag,UINT32 playYuvflag);
    UINT32 GetInitPlayQueueSize(){return ulH264RcvBufferMax*initPlayFactor/100;}
    INT32 Send(UINT8 *pVideoData,UINT32 uLength,UINT32 rotatedAngle);
    UINT32 GetTimeStampWndStart(){ return ulTimeStampWndStart; }
    void SetTimeStampWndStart(UINT32 ts){ this->ulTimeStampWndStart = ts; }
    UINT32 GetTimeStampRcvWnd(){ return ulTimeStampRcvWnd;}
    void SetRcvSSRC(UINT32 ssrc){ this->ulRcvSSRC = ssrc; }
    UINT32 GetRcvSSRC(){ return ulRcvSSRC; }
    UINT32 GetH264RcvBufferMax(){ return ulH264RcvBufferMax; }
    bool IsCompletNaluOrStart1Fu(MTXW_DATA_BUFFER_STRU* pFragmentData);
    MTXW_ENUM_H264_FU_A_TYPE FragmentType(std::list<MTXW_DATA_BUFFER_STRU*>::iterator it);
    MTXW_ENUM_H264_FU_A_TYPE FragmentType(MTXW_DATA_BUFFER_STRU* pFragmentData);
    INT32 PutHeadFramListToPlayQueue();
    void ClearRcvBuffer();
    void ClearOneFrameList(std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator it);
    UINT32 GetPlayFrameInterval(){  return  (delta/1000000);}
    bool Getbdiscardflag(){    return bdiscardflag; }
    
    MTXW_ENUM_FRAME_TYPE IsIDRFrame(std::list<MTXW_DATA_BUFFER_STRU*>::iterator it);    
    MTXW_NALU_TYPE_ENUM GetNaluType(MTXW_H264_FRAGMENT_HEADER_STRU fragHdr);//2017.11.14 added
    INT32 UpdateFrameToPlayQueue();

    UINT32 GetFrameRate(){return frameRate;}
    void SetPopExpectSN(UINT32 ExpectSN){ this->PopExpectSN = ExpectSN;}   
    UINT32 GetPopExpectSN(){ return this->PopExpectSN;}

    INT32 Decode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData);
    INT32 Play(MTXW_DATA_BUFFER_STRU *pData);

};

#endif //__MTXW_VIDEO_INSTANCE_H__
