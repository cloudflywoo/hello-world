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

    //����Ƶ������ʱ�����в�������ˢ��
    this->ulLastFrameTimeStamp = 0;
    this->lastFrameRcvTime = {0,0};
    this->jitter = 0;
    this->meanJitter = 0;
    this->alpha = 1.0/10;

    return true;
    
}

/*
*���յ��µ�ʱ���
*timeStamp: ���½��յ���RTP����ʱ���(RTP��ͷ) �������ֽ���
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

    //--1--Ԥ����-------------
    if(this->lastFrameRcvTime.tv_sec==0 && this->lastFrameRcvTime.tv_nsec==0  )
    {
        //��һ�����ݰ�
        this->ulLastFrameTimeStamp = timeStamp;
        clock_gettime(CLOCK_MONOTONIC, &this->lastFrameRcvTime);
        return -1;
    }

    //��������timeStamp��ͬ��˵������ͬһ֡�Ĳ�ͬ��Ƭ
    if(this->ulLastFrameTimeStamp == timeStamp)
    {
         return 0;
    }

    clock_gettime(CLOCK_MONOTONIC, &currRecvFrameTime); //��ȡ��ǰʱ��

    

    
    //---2----������֡��ǰһ֡��ʱ���(����ʱ���)
    if(currRecvFrameTime.tv_nsec > lastFrameRcvTime.tv_nsec)
    {
        RcvDelta = (currRecvFrameTime.tv_sec - lastFrameRcvTime.tv_sec)*1000000000 + (currRecvFrameTime.tv_nsec - lastFrameRcvTime.tv_nsec);//nano second
    }
    else
    {
        RcvDelta = (currRecvFrameTime.tv_sec - lastFrameRcvTime.tv_sec - 1)*1000000000 + (currRecvFrameTime.tv_nsec + 1000000000 - lastFrameRcvTime.tv_nsec);//nano second
    }   

    RcvDelta /= 1000000; //����->����
    

    //----3-----������֡���ݵ�ʱ�����
    if(timeStamp>=this->ulLastFrameTimeStamp)
    {
        StampDelta = timeStamp -this->ulLastFrameTimeStamp;
    }else
    {
        //ʱ�����ת�Ĵ���
        StampDelta = (timeStamp - 0)+(0xFFFFFFFF - this->ulLastFrameTimeStamp);
    }


    //---4---��������----------------
    this->ulLastFrameTimeStamp = timeStamp;
    this->lastFrameRcvTime = currRecvFrameTime;

    if(StampDelta>0xFFFFFF)
    {  //�����������֮��Ĳ�������˵���п�����������Ǵ���İ�
       //�˳����²�����jitter.
        return 0;
    }
    
    //---4.1-����Jitter (= (RcvDelta - StampDelta))-----
    this->jitter = (INT32)RcvDelta - (StampDelta*1000/this->ulFrequency);
    this->meanJitter = (INT32) ( this->meanJitter*(1-this->alpha) + this->jitter*this->alpha );


    return 0;


}

