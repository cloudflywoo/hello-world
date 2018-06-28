#ifndef __MTXW_JITTER_H__
#define __MTXW_JITTER_H__

#include "mtxw.h"
#include <time.h>

class MTXW_Jitter
{
private:
    UINT32 ulFrequency;
    
    UINT32 ulLastFrameTimeStamp; //ǰһ��RTP֡��ʱ���
    struct timespec lastFrameRcvTime; //ǰһ��RTP֡����ʱ��(���ն˱���ʱ��)
    
    INT32 jitter; //����������ܰ���Jitterֵ����λΪms
    INT32 meanJitter; //ƽ��jitterֵ
    double alpha;


public:

    /*
    *
    */
    MTXW_Jitter();
    virtual ~MTXW_Jitter();

    bool setFrequency(UINT32 ulFrequency);

    /*
    *���յ��µ�ʱ���
    *timeStamp: ���½��յ���RTP����ʱ���(RTP��ͷ) �������ֽ���
    */
    INT32 onRecv(UINT32 timeStamp); 

    /*
    *��ȡ����������ݰ��Ķ���
    */
    INT32 getJitter(){return jitter;}

    /*
    *��ȡƽ��������¶���
    */
    INT32 getMeanJitter(){return meanJitter;}

    /*
    *
    */
    
	
};


#endif //__MTXW_JITTER_H__