#include "mtxw_jitter.h"


MTXW_Jitter::MTXW_Jitter()
{
    this->ulFrequency = -1;
    this->ulLastFrameTimeStamp = 0;
    this->lastFrameRcvTime = {0,0};
    this->jitter = 0;
    this->meanJitter = 0;
    this->alpha = 1.0/10;
    
}
MTXW_Jitter::~MTXW_Jitter()
{

}

bool MTXW_Jitter::setFrequency(UINT32 ulFrequency)
{
    this->ulFrequency = ulFrequency;

    //采样频率设置时，所有参数从新刷新
    this->ulLastFrameTimeStamp = 0;
    this->lastFrameRcvTime = {0,0};
    this->jitter = 0;
    this->meanJitter = 0;
    this->alpha = 1.0/10;

    return true;
    
}

/*
*接收到新的时间戳
*timeStamp: 最新接收到的RTP包的时间戳(RTP包头) ，主机字节序
*/
INT32 MTXW_Jitter::onRecv(UINT32 timeStamp)
{
    double RcvDelta;
    UINT32 StampDelta;
    struct timespec currRecvFrameTime;

    if(this->ulFrequency == -1)
    {
        return -1;
    }

    //--1--预处理-------------
    if(this->lastFrameRcvTime.tv_sec==0 && this->lastFrameRcvTime.tv_nsec==0  )
    {
        //第一个数据包
        this->ulLastFrameTimeStamp = timeStamp;
        clock_gettime(CLOCK_MONOTONIC, &this->lastFrameRcvTime);
        return -1;
    }

    //两个包的timeStamp相同，说明这是同一帧的不同分片
    if(this->ulLastFrameTimeStamp == timeStamp)
    {
         return 0;
    }

    clock_gettime(CLOCK_MONOTONIC, &currRecvFrameTime); //获取当前时间

    

    
    //---2----计算新帧和前一帧的时间差(本地时间差)
    if(currRecvFrameTime.tv_nsec > lastFrameRcvTime.tv_nsec)
    {
        RcvDelta = (currRecvFrameTime.tv_sec - lastFrameRcvTime.tv_sec)*1000000000 + (currRecvFrameTime.tv_nsec - lastFrameRcvTime.tv_nsec);//nano second
    }
    else
    {
        RcvDelta = (currRecvFrameTime.tv_sec - lastFrameRcvTime.tv_sec - 1)*1000000000 + (currRecvFrameTime.tv_nsec + 1000000000 - lastFrameRcvTime.tv_nsec);//nano second
    }   

    RcvDelta /= 1000000; //纳秒->毫秒
    

    //----3-----计算两帧数据的时间戳差
    if(timeStamp>=this->ulLastFrameTimeStamp)
    {
        StampDelta = timeStamp -this->ulLastFrameTimeStamp;
    }else
    {
        //时间戳翻转的处理
        StampDelta = (timeStamp - 0)+(0xFFFFFFFF - this->ulLastFrameTimeStamp);
    }


    //---4---更新数据----------------
    this->ulLastFrameTimeStamp = timeStamp;
    this->lastFrameRcvTime = currRecvFrameTime;

    if(StampDelta>0xFFFFFF)
    {  //如果两连续包之间的差距过大，则说明有可能乱序或者是错误的包
       //此场景下不更新jitter.
        return 0;
    }
    
    //---4.1-计算Jitter (= (RcvDelta - StampDelta))-----
    this->jitter = (INT32)RcvDelta - (StampDelta*1000/this->ulFrequency);
    this->meanJitter = (INT32) ( this->meanJitter*(1-this->alpha) + this->jitter*this->alpha );


    return 0;


}

