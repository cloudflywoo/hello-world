#ifndef __RTP_FEC_H__
#define __RTP_FEC_H__

#include <string.h>
#include <time.h>
#include <vector>


#ifdef WIN32
#include <windows.h>
#define MTXW_LOGE 
#define MTXW_LOGD 
//#define MTXW_LOGE printf
//#define MTXW_LOGD printf
#else
#include "mtxw_comm.h"

#endif


#define MAX_SN 0xFFFF
#define MAX_RANK 3
#define MAX_ENCODE_DATA_LEN 1500
#define RTP_HEADER_LEN 12
#define MAX_RD_BUFF_LEN (MAX_ENCODE_DATA_LEN+RTP_HEADER_LEN)
#define MAX_DECODE_BUFF_LEN (64)
#define INIT_BUFF_SIZE (64)
 

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned int UINT;
typedef int INT32;


//----�ڴ��������----------------//
void *fecMemAlloc(UINT usize);
void  fecMemFree(void* p);
INT32 fecGetMemCnt();
#define fecMemSet(p,c,size) memset((p),(c),(size))
#define fecMemCopy(dest,src,size)   memcpy((dest),(src),(size))
#define fecMemMove(dest,src,count)  memmove((dest),(src),(count))
#define fecMemCmp(buf1,buf2,count)  memcmp((buf1),(buf2),(count))
#define fecMemRealloc(ptr,size) realloc((ptr),(size))

struct TimeVal{
	long long tv_sec;//��
	long long tv_msec;//����
};
struct FecInforStru{

    UINT8 ucGDLI:2; //�����������ݰ�����ָʾ����Чȡֵ��Χ��0-3,  ��ӳ��ֵΪ2^(3+i), 0-8 1-16 2-32 3-64�������鳤��=GDL + ���������
    UINT8 ucRV:3; //����汾 0-0������  1-1������ 2-2������ 3-�������� other-����ֵ
    UINT8 ucRI:3;  //����ָʾ0-��ʾ���ڵĵ�һ������� 1-��ʾ���ڵĵڶ�������� 2-��ʾ���ڵĵ����������....
    UINT16 usGID;  //������id����ֵ����SN/�鳤��
};

struct CqiInforStru{
#define INVALID_CQI_VALUE 0xFF

    UINT8 GDLI;
    UINT8 RI;
    UINT8 RFR;

    CqiInforStru(){
        this->GDLI = INVALID_CQI_VALUE;
        this->RI = INVALID_CQI_VALUE;
        this->RFR = INVALID_CQI_VALUE;
    }
};
struct CqiStru{
    bool bAcqi; //true: acqi exist, false acqi does not exist
    CqiInforStru cqi;
    CqiInforStru acqi;
   
    CqiStru(){
		this->bAcqi = false;
    }

};

struct PiggyMsgStru{
	UINT8 fecBit:1;
	UINT8 cqiBit:1;
	UINT8 rsvBits:6;
	FecInforStru fecInfo;
	CqiStru cqi;

	PiggyMsgStru(){
		fecBit = 0;
		cqiBit = 0;
	}
};

struct FecBuffStru{

	UINT16 usOffset; //�����ڻ������ʼλ��
	UINT16 usBuffLen; //��������Ĵ�С
	UINT16 usDataLen; //���ݵĴ�С

	UINT8  pData[0];
};



typedef void(*RtpFecEncodeOutputCallBack)(void* pUserData,bool bRFlag,UINT8 *pData, UINT16 ulDataLen);

/**********************************************************
*RTP FEC ���룬Ҫ�������RTP���ݳ��Ȳ��ܴ���1500�ֽڣ�������
*Padding���������޷�FEC����ģ����Padding��FEC����ʱ��Ϊ
*���ݱ߽硣
**********************************************************/
class RtpFecEncoder
{
private:
	UINT16 usCurrRank;  //��ǰʹ�õ����������Ҳ����ÿ��������Я�������������
	UINT16 usCurrGroupDataLen;  //���������ݰ�������Ҳ����ÿ������������������ݰ�
	UINT16 usCurrGroupId; //��ǰ���ڱ������id
	UINT16 usSettingRank; //�û�����(�����)���û�����������û�����Rank����Ҫ��ǰ������������¸������鿪ʼʱ����Ч
	UINT16 usSettingGroupDataLen;//���������ݰ���������Ҫ��ǰ������������¸������鿪ʼʱ����Ч
	UINT8 *pRDBuffer; //��������, RDBuffer: Redundant Data Buffer
	UINT8 usSSRC[4]; //��ǰ�����RTP SSRC IDֵ
	UINT16 usMaxLen; //��������������ݰ�����

	CqiStru mCQI; //2018.4.16 �ŵ�����ָʾ�������ն˲����ģ���Ҫ���ͷ������Զ�

	void createRDBuffer(){pRDBuffer = (UINT8*)fecMemAlloc(MAX_RD_BUFF_LEN*MAX_RANK);}

	void deleteRDBuffer(){fecMemFree(pRDBuffer);pRDBuffer=0;}

	void initalRDBuffer();


	void *pUserData;
	RtpFecEncodeOutputCallBack outputCb;

	UINT16 getGroupDataLenByGDLI(UINT16 usGDLI);
	UINT8 getGDLI();
	void outputRedundantData();
	CqiStru getCQI(){return mCQI;}

public:
	/*
	*@param rank=0,1,2,3  
	*@param usGDLI=0,1,2,3  ��Ӧ���������ݸ���2^(3+usGDLI)
	*/
	RtpFecEncoder(RtpFecEncodeOutputCallBack rtpFecEncodeOutputCb, void* pUserData,UINT16 rank, UINT16 usGDLI);
	~RtpFecEncoder();
	UINT32 input(UINT8 *pRawRtpData, UINT16 len);

	/*
	*@param rank :���벻����MAX_RANK
	*/
	bool setRank(UINT8 rank){
		if(rank > MAX_RANK){return false;}
		this->usSettingRank = rank;
		//MTXW_LOGE("RtpFecEncoder setRank(): rank = %d ",rank);
		return true;
	}
	/*
	*@param usGDLI :ȡֵ��Χ{0,1,2,3}
	*/
	bool setGDLI(UINT8 usGDLI){
		if(usGDLI>3){return false;}
		this->usSettingGroupDataLen = getGroupDataLenByGDLI(usGDLI);
		//MTXW_LOGE("RtpFecEncoder setGDLI(): usGDLI = %d ",usGDLI);
		return true;
	}
	UINT16 getCurrGroupDataLen(){return usCurrGroupDataLen;}
	UINT16 getCurrRank(){return usCurrRank;}

	/*
	*����Ҫ�������Զ˵�CQI��Ϣ�����ϲ����
	*/
	void updateCQI(CqiStru &cqi){
	    this->mCQI = cqi;
       }

};

typedef void(*RtpFecDecodeOutputCallBack)(void* pUserData,UINT8 *pData, UINT16 ulDataLen);

typedef void(*RtpFecDecodeRcvCQICallBack)(void* pUserData,CqiStru cqi);//���յ��Զ˷���������CQI��Ϣ

typedef void(*RtpFecDecodeUpdCQICallBack)(void* pUserData,UINT8 GDLI,UINT8 RI);//���˸��ݽ������������CQI����

class CQIEvaluate{
private:
	UINT8 eRI;//evaluated RI
	UINT8 eGDLI;//evaluated GDLI

	UINT32 eCount;//evaluated up rank count����ʾ���׵Ĵ���

	TimeVal eChange; //
public:
	CQIEvaluate();
	virtual ~CQIEvaluate();
	void input(UINT8 lostCnt,UINT8 GDLI);
	bool getEvaluatedResult(UINT8 & RI,UINT8 &GDLI);
	void reset();
};

class RtpFecDecoder
{
private:

	void *pUserData;
	RtpFecDecodeOutputCallBack outputCb;

    //FecInforStru fecInfo;
	UINT8 bitGDLI:2;//��ǰ������������ݰ�����ָʾ
	UINT8 bitRV:3; //��ǰ�����������汾
	UINT8 bitRsv:3;//����λ
	bool fecValid;
    UINT16 usDeliveredSN; //��һ��Ҫ����ݽ����ݰ�
	UINT16 usRcvCnt;//��ǰ�������Ѿ��յ������ݰ�����
	UINT32 ulSSRC;

	std::vector<FecBuffStru*> vecDecodeBuffer;//�������ݻ��棬���ڽ���������������Ҫ��������Ļ������Բ���Ҫ�˻���
	std::vector<FecBuffStru*> vecAlpha;//���ڽ����������ݡ��Լ��ָ����ݵļ���

	RtpFecDecodeRcvCQICallBack    rcvCqiCb;
	RtpFecDecodeUpdCQICallBack   updtCqiCb;
	CQIEvaluate cqiEvaluater;
	
	void createBuffer(){ 
		for(unsigned int i=0;i<MAX_DECODE_BUFF_LEN;i++){
		    FecBuffStru* pBuff = (FecBuffStru*)fecMemAlloc(sizeof(FecBuffStru)+INIT_BUFF_SIZE);
			pBuff->usBuffLen = INIT_BUFF_SIZE;
			pBuff->usDataLen = 0;
			pBuff->usOffset = 0;
		    vecDecodeBuffer.push_back(pBuff);
		}
		for(unsigned int i=0;i<MAX_RANK;i++){
			FecBuffStru* pBuff = (FecBuffStru*)fecMemAlloc(sizeof(FecBuffStru)+INIT_BUFF_SIZE);
			pBuff->usBuffLen = INIT_BUFF_SIZE;
			pBuff->usDataLen = 0;
			pBuff->usOffset = 0;
			vecAlpha.push_back(pBuff);
		}
		
	}
	void resetBuffer(){
		for(unsigned int i=0;i<vecDecodeBuffer.size();i++){
			vecDecodeBuffer[i]->usDataLen = 0;
			vecDecodeBuffer[i]->usOffset = 0;
		}
		for(unsigned int i=0;i<vecAlpha.size();i++){
			vecAlpha[i]->usDataLen = 0;
			vecAlpha[i]->usOffset = 0;
		}
	}
    void releaseBuffer(){
        for(unsigned int i=0;i<vecDecodeBuffer.size();i++){
			fecMemFree(vecDecodeBuffer[i]);
		}
		vecDecodeBuffer.clear();
		for(unsigned int i=0;i<vecAlpha.size();i++){
			fecMemFree(vecAlpha[i]);
		}
		vecAlpha.clear();
	}

	UINT8 DecodePiggyMsg(UINT8* pMsg, UINT8 msgLen,PiggyMsgStru* pPiggyMsg);
	UINT16 getGroupDataLenByGDLI(UINT16 usGDLI);
	UINT16 getRtpHeaderLen(UINT8 *pRawRtpData,UINT16 len);
	
	bool write2buff(std::vector<FecBuffStru*> &vecBuff,UINT16 pos,UINT8 *pData,UINT16 len);
	void recoverData();

	void reset();

       //�ϱ��ϲ���յ���CQI��Ϣ
	void reportRecvCQI(CqiStru cqi){this->rcvCqiCb(this->pUserData,cqi);}
	//֪ͨ�ϲ����CQI��Ϣ
	void updateCQI(UINT8 GDLI,UINT8 RI){this->updtCqiCb(this->pUserData,GDLI,RI);}


public:
	RtpFecDecoder(RtpFecDecodeOutputCallBack rtpFecDecodeOutputCb, 
	                        RtpFecDecodeRcvCQICallBack rtpFecDecodeRcvCqiCb,
	                        RtpFecDecodeUpdCQICallBack rtpFecDecodeUpdCqiCb,
	                        void* pUserData);
	~RtpFecDecoder();
	UINT32 input(UINT8 *pRawRtpData, UINT16 len);
	void flush();

       //���ص�ǰ������������Ƿ����FEC��Ϣ
	bool getFecFlag(){return fecValid;}

};


#endif //__RTP_FEC_H__
