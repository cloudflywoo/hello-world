
#ifndef __MTXW_VIDEO_INSTANCE_H__
#define __MTXW_VIDEO_INSTANCE_H__

#include "mtxw_instance.h"
#include "mtxw_h264rtp.h"
#include "mtxw_comm.h"



extern INT32 mtxw_H264FragmentDataCompare(const MTXW_DATA_BUFFER_POINTER &,
                                                                                      const MTXW_DATA_BUFFER_POINTER &);

struct MTXW_H264_RCV_FRAME_LIST_STRU
{
    UINT32 ulTimStamp;  //时间戳，从RTP header获取
    UINT32 ulExpectSN;  //期望的SN
    UINT16 ulRotateAngle; //旋转角度
    UINT32 ulLengthOfFragmentData;//同一个H264帧的多个RTPPayLoadContent包长度之和,该长度可能会比实际包长要大，只用于申请内存用，并不能准确表示组装后的H264帧数据大小

    bool     bCompleted; //表示接收的数据是否已经完整

    /*
    *同一个帧里的多个RTP包，都存放在fragmentDataList里
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

    MTXW_ENUM_VIDEO_CODEC_TYPE enCodecType; //视频编码类型

    UINT32 frameRate;//帧率，取值10-30帧/秒

    bool bdiscardflag;//false:不丢弃烂帧 true:丢弃烂帧
    
    MTXW_ENUM_CONTINUE_DROP_FRAME_TYPE bContinueDropFrame;//0:停止丢帧,1:由于坏帧丢弃,继续丢帧直到遇到I帧置成，2:由于播放队列Buffer满了,继续丢帧直到遇到I帧置成
    UINT32 rtpRcvBufferSize;//rtp接收buffer大小
    UINT32 frameRcvBufferSize; //帧接收缓存大小
    UINT32 initPlayFactor; //初始播放因子，取值[0-100]，表示百分比

    MTXW_H264_RCV_BUFFER_STRU  h264RcvBuffer; //接收缓存队列
    UINT32 PopExpectSN;//当前timeStamp frameList的最后一个数据包的期望SN
    
    MTXW_Rtp *pRtpObj;
    
    bool bFirstRcvPackt;
    UINT32 ulRcvSSRC;
    UINT32 ulTimeStampWndStart; //接收窗的起点，小于这个窗的timeStamp的RTP包直接丢弃
    UINT32  ulTimeStampRcvWnd;  //接收窗大小

    UINT32 ulH264RcvBufferMax;  //接收缓存的最大值(也就是最多有几个timestamp节点)

    UINT32 ulRcvFrameWidth;//接收的视频的宽，从SPS里面获取，每收到SPS后更新
    UINT32 ulRcvFrameHeight;//接收的视频的高，从SPS里面获取
#ifndef WIN32
    struct timespec lastRecvFrameTime;
    struct timespec lastSendFrameTime;
#endif
    double smoothFactor;
    double delta;    //帧接收速率(统计值)  unit- nano second

    double sendDelta;//发帧速率(统计值) unit - nano second
    INT32  needDelaySendTime; //用于控制发送的帧率，使得实际发送的帧率和信令协商帧率一致,unit-ms


    UINT32 playYuvflag; //0-播放H264帧 1-表示播放YUV帧
    UINT8   mRcvSps[256]; //|-startcode-|-naluhdr-|----sps data----|
    UINT32 mRcvSpsLength;
    UINT8 mRcvPps[256];//|-startcode-|-naluhdr-|----pps data----|
    UINT32 mRcvPpsLength;

    bool  bRcvWndActive; //接收窗机制是否启用标志，true=启用,false=不启用;
                                    //启用: 不在接收窗范围内的RTP会被当做重复包丢弃
                                    //不启用:  不会丢弃RTP包，带来的一个副作用就是重复RTP也被播放
                                    
    
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
    * frameRate:帧率，正确的取值范围[10-30]
    * rtpRcvBufferSize: rtp接收缓存大小，单位为秒，合理取值范围[1-4]
    * frameRcvBufferSize: 帧接收缓存大小，单位为秒，合理取值范围[1-8]
    * initPlayFactor: 初始播放因子,合理取值范围[0-100]表示 0%-100%
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
