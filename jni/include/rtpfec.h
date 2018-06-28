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


//----内存操作函数----------------//
void *fecMemAlloc(UINT usize);
void  fecMemFree(void* p);
INT32 fecGetMemCnt();
#define fecMemSet(p,c,size) memset((p),(c),(size))
#define fecMemCopy(dest,src,size)   memcpy((dest),(src),(size))
#define fecMemMove(dest,src,count)  memmove((dest),(src),(count))
#define fecMemCmp(buf1,buf2,count)  memcmp((buf1),(buf2),(count))
#define fecMemRealloc(ptr,size) realloc((ptr),(size))

struct TimeVal{
	long long tv_sec;//秒
	long long tv_msec;//毫秒
};
struct FecInforStru{

    UINT8 ucGDLI:2; //编码组内数据包个数指示，有效取值范围：0-3,  其映射值为2^(3+i), 0-8 1-16 2-32 3-64；编码组长度=GDL + 冗余包数量
    UINT8 ucRV:3; //冗余版本 0-0阶冗余  1-1阶冗余 2-2阶冗余 3-三阶冗余 other-保留值
    UINT8 ucRI:3;  //冗余指示0-表示组内的第一个冗余包 1-表示组内的第二个冗余包 2-表示组内的第三个冗余包....
    UINT16 usGID;  //编码组id，其值等于SN/组长度
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

	UINT16 usOffset; //数据在缓存的起始位置
	UINT16 usBuffLen; //整个缓存的大小
	UINT16 usDataLen; //数据的大小

	UINT8  pData[0];
};



typedef void(*RtpFecEncodeOutputCallBack)(void* pUserData,bool bRFlag,UINT8 *pData, UINT16 ulDataLen);

/**********************************************************
*RTP FEC 编码，要求输入的RTP数据长度不能大于1500字节，而且有
*Padding，否则是无法FEC解码的，这个Padding在FEC解码时作为
*数据边界。
**********************************************************/
class RtpFecEncoder
{
private:
	UINT16 usCurrRank;  //当前使用的冗余阶数，也即是每个编码组携带的冗余包数量
	UINT16 usCurrGroupDataLen;  //编码组数据包数量，也就是每个编码组包含几个数据包
	UINT16 usCurrGroupId; //当前正在编码的组id
	UINT16 usSettingRank; //用户设置(或更新)的用户冗余阶数，用户更新Rank后，需要当前编码组结束，下个编码组开始时才生效
	UINT16 usSettingGroupDataLen;//编码组数据包个数，需要当前编码组结束，下个编码组开始时才生效
	UINT8 *pRDBuffer; //冗余数据, RDBuffer: Redundant Data Buffer
	UINT8 usSSRC[4]; //当前处理的RTP SSRC ID值
	UINT16 usMaxLen; //编码组内最长的数据包长度

	CqiStru mCQI; //2018.4.16 信道质量指示，本接收端产生的，需要发送反馈给对端

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
	*@param usGDLI=0,1,2,3  对应编码组数据个数2^(3+usGDLI)
	*/
	RtpFecEncoder(RtpFecEncodeOutputCallBack rtpFecEncodeOutputCb, void* pUserData,UINT16 rank, UINT16 usGDLI);
	~RtpFecEncoder();
	UINT32 input(UINT8 *pRawRtpData, UINT16 len);

	/*
	*@param rank :必须不大于MAX_RANK
	*/
	bool setRank(UINT8 rank){
		if(rank > MAX_RANK){return false;}
		this->usSettingRank = rank;
		//MTXW_LOGE("RtpFecEncoder setRank(): rank = %d ",rank);
		return true;
	}
	/*
	*@param usGDLI :取值范围{0,1,2,3}
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
	*更新要反馈给对端的CQI信息，由上层调用
	*/
	void updateCQI(CqiStru &cqi){
	    this->mCQI = cqi;
       }

};

typedef void(*RtpFecDecodeOutputCallBack)(void* pUserData,UINT8 *pData, UINT16 ulDataLen);

typedef void(*RtpFecDecodeRcvCQICallBack)(void* pUserData,CqiStru cqi);//接收到对端反馈回来的CQI信息

typedef void(*RtpFecDecodeUpdCQICallBack)(void* pUserData,UINT8 GDLI,UINT8 RI);//本端根据接收情况产生的CQI参数

class CQIEvaluate{
private:
	UINT8 eRI;//evaluated RI
	UINT8 eGDLI;//evaluated GDLI

	UINT32 eCount;//evaluated up rank count，表示升阶的次数

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
	UINT8 bitGDLI:2;//当前解码组的组数据包数量指示
	UINT8 bitRV:3; //当前解码组的冗余版本
	UINT8 bitRsv:3;//保留位
	bool fecValid;
    UINT16 usDeliveredSN; //下一个要解码递交数据包
	UINT16 usRcvCnt;//当前解码组已经收到的数据包数量
	UINT32 ulSSRC;

	std::vector<FecBuffStru*> vecDecodeBuffer;//接收数据缓存，用于解码和排序，如果不需要按序输出的话，可以不需要此缓存
	std::vector<FecBuffStru*> vecAlpha;//用于接收冗余数据、以及恢复数据的计算

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

       //上报上层接收到的CQI信息
	void reportRecvCQI(CqiStru cqi){this->rcvCqiCb(this->pUserData,cqi);}
	//通知上层更新CQI信息
	void updateCQI(UINT8 GDLI,UINT8 RI){this->updtCqiCb(this->pUserData,GDLI,RI);}


public:
	RtpFecDecoder(RtpFecDecodeOutputCallBack rtpFecDecodeOutputCb, 
	                        RtpFecDecodeRcvCQICallBack rtpFecDecodeRcvCqiCb,
	                        RtpFecDecodeUpdCQICallBack rtpFecDecodeUpdCqiCb,
	                        void* pUserData);
	~RtpFecDecoder();
	UINT32 input(UINT8 *pRawRtpData, UINT16 len);
	void flush();

       //返回当前解码的数据流是否带了FEC信息
	bool getFecFlag(){return fecValid;}

};


#endif //__RTP_FEC_H__
