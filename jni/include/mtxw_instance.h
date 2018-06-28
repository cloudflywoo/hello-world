
#ifndef __MTXW_INSTANCE_H__
#define __MTXW_INSTANCE_H__

#include "mtxw_osapi.h"

#include "mtxw_rtp.h"
#include "mtxw.h"
#include "mtxw_comm.h"
#include "mtxw_jitter.h" 
#include "rtpfec.h"

#include <string>
#include <vector>

#if (defined WIN32 || WIN)
#define MTXW_SOCKADDR SOCKADDR
#else
#define MTXW_SOCKADDR sockaddr
#endif

class MTXW_Instance
{
private:

	INT32 uInstanceId;
	
        //---媒体参数
	bool  bAudio;//true = audio, false = video

	bool  bRuning;//控制各个线程的停止参数，bRuning=true表示线程继续跑，bRuning=false表示线程要终止

       INT32 callId;//2018.4.2
       
	//--传输参数
	MTXW_ENUM_DIRECTION enDirection; //传输方向
	std::string strLocalAddr;
	UINT16    uLocalPort;
	std::string strRemoteAddr;
	UINT16    uRemotePort;

	std::string strInterface; //指定的网卡
	
       UINT32 ulRcvBytes;//接收线程每收到一个数据包时，ulRcvBytes就增加这个数据包的长度
       UINT32 ulSndBytes;//发送线程每发送一个数据包时，ulSndBytes就增加这个数据包的长度
       UINT32 ulRcvPacketNum;//统计收包数
       UINT32 ulDropPacketNum;//统计丢包数，包含网络传输丢包和媒体库主动丢包数之和

       bool bFecSwitch; //FEC开关
       bool bPeerSupportFec; //对端是否支持FEC，默认认为支持，当检测到不支持时在赋值更新

       RtpFecEncoder *pRtpFecEncoder;
       UINT16 usEncodeRank;//设置冗余包阶数(取0-3)
       UINT16 usEncodeGDLI;//设置编码内数据包个数(0-8,1-16,2-32,3-64个)
       UINT16 usSimulationDropRate; //模拟丢包率测试
     

       

       RtpFecDecoder *pRtpFecDecoder;

       /*
       *自适应算法，接收端根据信道接收质量反馈CQI给发送端
       *发送端根据接收到的CQI自动调整FEC参数，以及音视频分辨率和编码参数
       */
       bool bAdaptive; //FEC自适应算法开关，

       CqiStru mRcvCQI; //对端反馈的CQI信息
       CqiStru mSndCQI; //本端产生的、需要发给对端的CQI信息

       
	

	//--数据队列
protected:
	MTXW_queue<MTXW_DATA_BUFFER_STRU*> mSendQueue; //发送队列-这个数据的读写要考虑线程同步,发送线程负责内存释放
	UINT32 ulSendQueueLengthMax; //发送队列的最大长度 
		
	MTXW_queue<MTXW_DATA_BUFFER_STRU*> mPlayQueue; //播放队列-这个数据的读写要考虑线程的同步，解包线程申请内存，播放线程负责释放内存
        UINT32 ulPlayQueueLengthMax; //发送队列的最大长度
        MTXW_VEDIO_COUNT_STRU VedioCount;

       MTXW_DATA_BUFFER_STRU* m_pEncodeBuffer;
	MTXW_DATA_BUFFER_STRU* m_pDecodeBuffer;
	MTXW_DATA_BUFFER_STRU* m_pSockRecvBuffer;

	MTXW_DATA_BUFFER_STRU* m_pFecDecodeBuffer;
	MTXW_DATA_BUFFER_STRU* m_pPackBuffer;

	MTXW_Jitter mJitter; //接收Jitter

       UINT32 ulSndFrameNum;//发送的帧数，发送线程每发送一帧数据时就++，用于统计发送帧率
       UINT32 ulRcvFrameNum;//接收的帧数，播放线程每播放一帧数据时就++，用于统计接收帧率
       UINT32 ulSndFrameRate;//实际统计出来的发送帧率
       UINT32 ulRcvFrameRate;//实际统计出来的接收帧率
       UINT32 ulPlayFrameNum;//调用上层回调接口进行播放的帧数

       struct{
           UINT8 *pBuffer; //缓存地址
           UINT32 ulSize; //缓存空间大小
       } mPlayBuffer; //播放缓存，由上层传入


private:

	//--socket 
#if (defined WIN32 || WIN)
	SOCKET mSock;
#else
	INT32 mSock;  
#endif
       MTXW_MUTEX_HANDLE mhSocketMutex;
	
	
	
	
	//--线程
	MTXW_THREAD_HANDLE sendThread;//发送线程
	MTXW_THREAD_HANDLE receiveThread;//接收线程
	MTXW_THREAD_HANDLE playThread;//播放线程
	MTXW_THREAD_HANDLE statisticThread;//统计线程


	//--日志
	UINT32 uLogSwitch;//设置日志级别，按照bit位对应一个开关，…b4b3b2b1,b1-严重级别，b2-一般级别，b3-提示级别 b4-函数轨迹打印

	static void* gSendThread(void* pvParam)
	{
	    MTXW_Instance *thiz = (MTXW_Instance*)pvParam;
	    thiz->SendThread();
	    return 0;
	}
	static void* gPlayThread(void* pvParam)
	{
		MTXW_Instance *thiz = (MTXW_Instance*)pvParam;
		thiz->PlayThread();
		return 0;
	}
	static void* gReceiveThread(void* pvParam)
	{
		MTXW_Instance *thiz = (MTXW_Instance*)pvParam;
		thiz->ReceiveThread();
		return 0;
	}
	static void* gStatisticThread(void* pvParam)
	{
        	MTXW_Instance *thiz = (MTXW_Instance*)pvParam;
        	thiz->StatisticThread();
        	return 0;
	}
	
	INT32 CreateSocket();
	INT32 CloseSocket();
	bool  IsMultiCastIp(std::string addr);

	INT32 CreateMultiCastSocket();

	void ClearEncodeBuffer();
	void ClearPackBuffer();
	void ClearSockRecvBuffer();

       INT32 SendTo(UINT8* pData,UINT16 usOnePackLen,UINT32 time, MTXW_SOCKADDR* pDestAddr,UINT16 usDstAddrLen);
	INT32 _SendTo(UINT8* pData,UINT16 usOnePackLen,UINT32 time, MTXW_SOCKADDR* pDestAddr,UINT16 usDstAddrLen);

	static void rtpDecodeOutput(void* pUserData,UINT8 *pData, UINT16 ulDataLen){
		MTXW_Instance *thiz = (MTXW_Instance*)pUserData;
		thiz->_rtpDecodeOutput(pData,ulDataLen);
		return;
	}
	void _rtpDecodeOutput(UINT8 *pData, UINT16 ulDataLen);
	static void rtpEncodeOutput(void* pUserData,bool bRFlag,UINT8 *pData, UINT16 ulDataLen)
	{
	       MTXW_Instance *thiz = (MTXW_Instance*)pUserData;
		thiz->_rtpEncodeOutput(bRFlag,pData,ulDataLen);
		return;
	}
	void _rtpEncodeOutput(bool bRFlag,UINT8 *pData, UINT16 ulDataLen);

       //接收到对端反馈回来的CQI信息
	static void RcvCQI(void* pUserData,CqiStru cqi){
	    MTXW_Instance *thiz = (MTXW_Instance*)pUserData;
		thiz->_RcvCQI(cqi);
		return;
	}
	void _RcvCQI(CqiStru cqi);

       //本端FEC解码器根据接收情况产生的CQI，这个CQI信息要发给对端
	static void UpdCQI_GDLI_RI(void* pUserData,UINT8 GDLI,UINT8 RI){
	    MTXW_Instance *thiz = (MTXW_Instance*)pUserData;
		thiz->_UpdCQI_GDLI_RI(GDLI,RI);
		return;
	}
	void _UpdCQI_GDLI_RI(UINT8 GDLI,UINT8 RI);
	void _UpdCQI_RFR(UINT8 RFR);

	void SetFeedbackCQI();

	
public:
	MTXW_Instance(bool isAudio,INT32 uInstanceId,INT32 callId);
	virtual ~MTXW_Instance();


	virtual INT32 Start();
	virtual INT32 Stop();
	INT32 SetLocalAddress(std::string ip,UINT16 port);
	INT32 SetRemoteAddress(std::string ip,UINT16 port);
	INT32 SetDirection(MTXW_ENUM_DIRECTION direction);
	MTXW_ENUM_DIRECTION GetDirection(){return this->enDirection;}

	INT32 SetInterface(std::string strInterface);
	INT32 SetFecParam(UINT16 FecSwitch,UINT16 InputGDLI,UINT16 InputRank,UINT16 SimulationDropRate,UINT16 adaptiveSwitch);

	bool IsRunning(){return bRuning;}

	
	bool isPeerSupportFec(){return bPeerSupportFec;}

	bool isAudio(){return bAudio;}

	INT32 GetCallId(){return callId;}

	//关联实例获取本实例的CQI信息去发送
	bool GetCQIByAssInst(CqiInforStru &cqiInfo);

	//关联实例收到本实例的反馈信息，通过这个函数传入更新
	void RcvCQIFromAssInst(CqiInforStru cqiInfo);

	INT32 SetPlayBuffer(UINT8 *pBuffer, INT32 Length);

protected:
	
	//codec编码，pInData-入参，内容为原始采样数据(如PCM)  pOutData-出参，内容为编码后的输出(如AMR)，调用者要负责buffer足够大
	virtual INT32 Encode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData);

	//decode解码，pInData-入参，内容为编码后的数据(如AMR)  pOutData-出参，内容为解码后的输出(如PCM)，调用者要确保足够大的buffer
	virtual INT32 Decode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData);



	/*打包函数，输入是未打包的数据，输出是打包后的数据，函数调用者负责释放vecPackedData里的内存
	   pPackedData->pData的数据的数据格式为: |-len0-|--pack0--|-len1-|--pack1--|-len2-|--pack2--|......
           lenX的长度为2字节，主机字节序

	*/
	virtual INT32 Pack(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_DATA_BUFFER_STRU* pPackedData);

	//收到RTP包的处理函数，解包后的数据输出到播放队列vecPlayQueue，供播放线程使用。
	virtual INT32 ReceiveRtpPacket(MTXW_DATA_BUFFER_STRU* pInputData);

        //获取播放的帧间隔，各个子类根据具体的情况去重载实现, 函数的返回值单位为毫秒
        virtual UINT32 GetPlayFrameInterval(){return 0;}

        //获取初始播放队列大小，也就是播放队列要满足多大时才开始播放，提升流畅度
        virtual UINT32 GetInitPlayQueueSize(){return 0;}

        //播放接口:对于语音pData为PCM数据，对于视频，pData为H264 Frame
        virtual INT32 Play(MTXW_DATA_BUFFER_STRU *pData){return 0;};

	//用于子类获取播放队列，子类Unpack后，将准备好的PCM数据或H264数据放入播放队列
	MTXW_queue<MTXW_DATA_BUFFER_STRU*>& GetPlayQueue(){return mPlayQueue;} 
       UINT32 GetSendQueueLengthMax(){return ulSendQueueLengthMax;}


	//用于子类获取发送队列，子类收到APK发送过来的数据包后，会将数据放入发送队列
	MTXW_queue<MTXW_DATA_BUFFER_STRU*>& GetSendQueue(){return mSendQueue;} 
	UINT32 GetPlayQueueLengthMax(){return ulPlayQueueLengthMax;}

	void SetPlayQueueLengthMax(UINT32 ulPlayQueueLengthMax){this->ulPlayQueueLengthMax = ulPlayQueueLengthMax;}
	
	MTXW_VEDIO_COUNT* GetVedioCount(){return &VedioCount;}
	
	int GetInstance(){return uInstanceId;}
	UINT32 GetDropPacketNum(){return ulDropPacketNum;}
	void SetDropPacketNum(UINT32 Num){this->ulDropPacketNum = Num;}

	void SendThread();
	void PlayThread();
	void ReceiveThread();
	void StatisticThread();
	
};


#endif  //__MTXW_INSTANCE_H__
