#ifndef __MTXW_JITTER_H__
#define __MTXW_JITTER_H__

#include "mtxw.h"
#include <time.h>

class MTXW_Jitter
{
private:
    UINT32 ulFrequency;
    
    UINT32 ulLastFrameTimeStamp; //前一个RTP帧的时间戳
    struct timespec lastFrameRcvTime; //前一个RTP帧接收时间(接收端本地时间)
    
    INT32 jitter; //最近两个接受包的Jitter值，单位为ms
    INT32 meanJitter; //平均jitter值
    double alpha;


public:

    /*
    *
    */
    MTXW_Jitter();
    virtual ~MTXW_Jitter();

    bool setFrequency(UINT32 ulFrequency);

    /*
    *接收到新的时间戳
    *timeStamp: 最新接收到的RTP包的时间戳(RTP包头) ，主机字节序
    */
    INT32 onRecv(UINT32 timeStamp); 

    /*
    *获取最近两个数据包的抖动
    */
    INT32 getJitter(){return jitter;}

    /*
    *获取平滑后的最新抖动
    */
    INT32 getMeanJitter(){return meanJitter;}

    /*
    *
    */
    
	
};


#endif //__MTXW_JITTER_H__