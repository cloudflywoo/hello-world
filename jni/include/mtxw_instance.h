
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
	
        //---ý�����
	bool  bAudio;//true = audio, false = video

	bool  bRuning;//���Ƹ����̵߳�ֹͣ������bRuning=true��ʾ�̼߳����ܣ�bRuning=false��ʾ�߳�Ҫ��ֹ

       INT32 callId;//2018.4.2
       
	//--�������
	MTXW_ENUM_DIRECTION enDirection; //���䷽��
	std::string strLocalAddr;
	UINT16    uLocalPort;
	std::string strRemoteAddr;
	UINT16    uRemotePort;

	std::string strInterface; //ָ��������
	
       UINT32 ulRcvBytes;//�����߳�ÿ�յ�һ�����ݰ�ʱ��ulRcvBytes������������ݰ��ĳ���
       UINT32 ulSndBytes;//�����߳�ÿ����һ�����ݰ�ʱ��ulSndBytes������������ݰ��ĳ���
       UINT32 ulRcvPacketNum;//ͳ���հ���
       UINT32 ulDropPacketNum;//ͳ�ƶ��������������紫�䶪����ý�������������֮��

       bool bFecSwitch; //FEC����
       bool bPeerSupportFec; //�Զ��Ƿ�֧��FEC��Ĭ����Ϊ֧�֣�����⵽��֧��ʱ�ڸ�ֵ����

       RtpFecEncoder *pRtpFecEncoder;
       UINT16 usEncodeRank;//�������������(ȡ0-3)
       UINT16 usEncodeGDLI;//���ñ��������ݰ�����(0-8,1-16,2-32,3-64��)
       UINT16 usSimulationDropRate; //ģ�ⶪ���ʲ���
     

       

       RtpFecDecoder *pRtpFecDecoder;

       /*
       *����Ӧ�㷨�����ն˸����ŵ�������������CQI�����Ͷ�
       *���Ͷ˸��ݽ��յ���CQI�Զ�����FEC�������Լ�����Ƶ�ֱ��ʺͱ������
       */
       bool bAdaptive; //FEC����Ӧ�㷨���أ�

       CqiStru mRcvCQI; //�Զ˷�����CQI��Ϣ
       CqiStru mSndCQI; //���˲����ġ���Ҫ�����Զ˵�CQI��Ϣ

       
	

	//--���ݶ���
protected:
	MTXW_queue<MTXW_DATA_BUFFER_STRU*> mSendQueue; //���Ͷ���-������ݵĶ�дҪ�����߳�ͬ��,�����̸߳����ڴ��ͷ�
	UINT32 ulSendQueueLengthMax; //���Ͷ��е���󳤶� 
		
	MTXW_queue<MTXW_DATA_BUFFER_STRU*> mPlayQueue; //���Ŷ���-������ݵĶ�дҪ�����̵߳�ͬ��������߳������ڴ棬�����̸߳����ͷ��ڴ�
        UINT32 ulPlayQueueLengthMax; //���Ͷ��е���󳤶�
        MTXW_VEDIO_COUNT_STRU VedioCount;

       MTXW_DATA_BUFFER_STRU* m_pEncodeBuffer;
	MTXW_DATA_BUFFER_STRU* m_pDecodeBuffer;
	MTXW_DATA_BUFFER_STRU* m_pSockRecvBuffer;

	MTXW_DATA_BUFFER_STRU* m_pFecDecodeBuffer;
	MTXW_DATA_BUFFER_STRU* m_pPackBuffer;

	MTXW_Jitter mJitter; //����Jitter

       UINT32 ulSndFrameNum;//���͵�֡���������߳�ÿ����һ֡����ʱ��++������ͳ�Ʒ���֡��
       UINT32 ulRcvFrameNum;//���յ�֡���������߳�ÿ����һ֡����ʱ��++������ͳ�ƽ���֡��
       UINT32 ulSndFrameRate;//ʵ��ͳ�Ƴ����ķ���֡��
       UINT32 ulRcvFrameRate;//ʵ��ͳ�Ƴ����Ľ���֡��
       UINT32 ulPlayFrameNum;//�����ϲ�ص��ӿڽ��в��ŵ�֡��

       struct{
           UINT8 *pBuffer; //�����ַ
           UINT32 ulSize; //����ռ��С
       } mPlayBuffer; //���Ż��棬���ϲ㴫��


private:

	//--socket 
#if (defined WIN32 || WIN)
	SOCKET mSock;
#else
	INT32 mSock;  
#endif
       MTXW_MUTEX_HANDLE mhSocketMutex;
	
	
	
	
	//--�߳�
	MTXW_THREAD_HANDLE sendThread;//�����߳�
	MTXW_THREAD_HANDLE receiveThread;//�����߳�
	MTXW_THREAD_HANDLE playThread;//�����߳�
	MTXW_THREAD_HANDLE statisticThread;//ͳ���߳�


	//--��־
	UINT32 uLogSwitch;//������־���𣬰���bitλ��Ӧһ�����أ���b4b3b2b1,b1-���ؼ���b2-һ�㼶��b3-��ʾ���� b4-�����켣��ӡ

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

       //���յ��Զ˷���������CQI��Ϣ
	static void RcvCQI(void* pUserData,CqiStru cqi){
	    MTXW_Instance *thiz = (MTXW_Instance*)pUserData;
		thiz->_RcvCQI(cqi);
		return;
	}
	void _RcvCQI(CqiStru cqi);

       //����FEC���������ݽ������������CQI�����CQI��ϢҪ�����Զ�
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

	//����ʵ����ȡ��ʵ����CQI��Ϣȥ����
	bool GetCQIByAssInst(CqiInforStru &cqiInfo);

	//����ʵ���յ���ʵ���ķ�����Ϣ��ͨ����������������
	void RcvCQIFromAssInst(CqiInforStru cqiInfo);

	INT32 SetPlayBuffer(UINT8 *pBuffer, INT32 Length);

protected:
	
	//codec���룬pInData-��Σ�����Ϊԭʼ��������(��PCM)  pOutData-���Σ�����Ϊ���������(��AMR)��������Ҫ����buffer�㹻��
	virtual INT32 Encode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData);

	//decode���룬pInData-��Σ�����Ϊ����������(��AMR)  pOutData-���Σ�����Ϊ���������(��PCM)��������Ҫȷ���㹻���buffer
	virtual INT32 Decode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData);



	/*���������������δ��������ݣ�����Ǵ��������ݣ����������߸����ͷ�vecPackedData����ڴ�
	   pPackedData->pData�����ݵ����ݸ�ʽΪ: |-len0-|--pack0--|-len1-|--pack1--|-len2-|--pack2--|......
           lenX�ĳ���Ϊ2�ֽڣ������ֽ���

	*/
	virtual INT32 Pack(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_DATA_BUFFER_STRU* pPackedData);

	//�յ�RTP���Ĵ�������������������������Ŷ���vecPlayQueue���������߳�ʹ�á�
	virtual INT32 ReceiveRtpPacket(MTXW_DATA_BUFFER_STRU* pInputData);

        //��ȡ���ŵ�֡���������������ݾ�������ȥ����ʵ��, �����ķ���ֵ��λΪ����
        virtual UINT32 GetPlayFrameInterval(){return 0;}

        //��ȡ��ʼ���Ŷ��д�С��Ҳ���ǲ��Ŷ���Ҫ������ʱ�ſ�ʼ���ţ�����������
        virtual UINT32 GetInitPlayQueueSize(){return 0;}

        //���Žӿ�:��������pDataΪPCM���ݣ�������Ƶ��pDataΪH264 Frame
        virtual INT32 Play(MTXW_DATA_BUFFER_STRU *pData){return 0;};

	//���������ȡ���Ŷ��У�����Unpack�󣬽�׼���õ�PCM���ݻ�H264���ݷ��벥�Ŷ���
	MTXW_queue<MTXW_DATA_BUFFER_STRU*>& GetPlayQueue(){return mPlayQueue;} 
       UINT32 GetSendQueueLengthMax(){return ulSendQueueLengthMax;}


	//���������ȡ���Ͷ��У������յ�APK���͹��������ݰ��󣬻Ὣ���ݷ��뷢�Ͷ���
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
