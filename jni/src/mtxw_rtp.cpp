#include "mtxw_rtp.h"
#include "stdlib.h"
#include <time.h>  


MTXW_Rtp::MTXW_Rtp()
{
        
        this->bitsVersion = 2;
        this->bitsP = false;
        this->bitsX = false;
        this->bitsCC = 0;
        this->bitMaker = false;
        this->payloadType = 0;
        this->seq = 0;
        this->timeStamp = 0;
        srand((unsigned)time(NULL)+(unsigned int)this);
        this->SSRC = rand();
        this->headExtension.tag = 0;
        this->headExtension.usLength = 0;
        this->headExtension.ulOffset = 0;
	
}

MTXW_Rtp::~MTXW_Rtp()
{
	;
}
INT32 MTXW_Rtp::PackRtpHeader (UINT8 *pData, UINT32 timeStamp, bool maker)
{
	UINT16 temp = 0;
	UINT16 lengthOfRtp=0;
	UINT32 temp1 = 0;
	

	if(NULL == pData)
	{
		return -1;
	}
	
	pData[0] = (bitsVersion << 6) | (bitsP << 5) |(bitsX << 4) |bitsCC;
	lengthOfRtp++;
	
	if(maker)
	{
		pData[1] = 0x80;
	}
	else
	{
		pData[1] = 0;
	}
	
	pData[1] =pData[1] |payloadType;
	lengthOfRtp++;
	
	
	temp = mtxwHtons(seq);
	mtxwMemCopy(&pData[2], &temp, 2);
	seq++;
	lengthOfRtp += 2;

	temp1 = mtxwHtonl(timeStamp);
	mtxwMemCopy(&pData[4], &temp1, 4);
	lengthOfRtp += 4;

	temp1 = mtxwHtonl(SSRC);
	mtxwMemCopy(&pData[8], &temp1, 4);
	lengthOfRtp += 4;
	
	return lengthOfRtp;
}

INT32 MTXW_Rtp::UnpackRtpHeader(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_Rtp *pRtp)
{
	INT32 RtpHeaderLength = 0;
	
	if(NULLPTR == pInputData || NULLPTR == pRtp)
	{
	       MTXW_LOGE("MTXW_Rtp::UnpackRtpHeader pInputData or pRtp is null!");
		return -1;
	}
	if(pInputData->uSize< RTP_HEAD_LENGTH)
	{
	       MTXW_LOGE("MTXW_Rtp::UnpackRtpHeader pInputData size is too small!");
		return -1;
	}

	pRtp->bitsVersion = pInputData->pData[0] >> 6;
	pRtp->bitsP = (pInputData->pData[0] & 0x20) >>5;
	pRtp->bitsX = (pInputData->pData[0] &0x10)>>4;
	pRtp->bitsCC = pInputData->pData[0] ;

	pRtp->bitMaker = (pInputData->pData[1] & 0x80)>>7;
	pRtp->payloadType = pInputData->pData[1] &0x7F;
	pRtp->seq= mtxwNtohs(*((UINT16*)&pInputData->pData[2]));	
	pRtp->timeStamp = mtxwNtohl(*((UINT32*)&pInputData->pData[4]));
	pRtp->SSRC = mtxwNtohl(*((UINT32*)&pInputData->pData[8]));

	RtpHeaderLength += 12;

	if(pRtp->bitsCC > 0)
	{
		if(pInputData->uSize < RTP_HEAD_LENGTH+pRtp->bitsCC*sizeof(UINT32))
		{
		       MTXW_LOGE("MTXW_Rtp::UnpackRtpHeader pInputData->uSize is too small! pRtp->bitsCC:%d",pRtp->bitsCC);
			return -1;
		}

		for(int i = 0; i< pRtp->bitsCC; i++)
		{
		    pRtp->SetCsrcId(mtxwNtohl(*((UINT32 *)&pInputData->pData[RTP_HEAD_LENGTH+i*sizeof(UINT32)])));
		}
		RtpHeaderLength += pRtp->bitsCC*sizeof(UINT32);
	}

	if(pRtp->bitsX)
	{
	        // 4:代表扩展头中的tag和uslength的长度，如果包含扩展头，则至少4个字节
		if(pInputData->uSize < RTP_HEAD_LENGTH + pRtp->bitsCC*sizeof(UINT32) + 4)
		{
		       MTXW_LOGE("MTXW_Rtp::UnpackRtpHeader pInputData->uSize is too small! pRtp->bitsX:%d",pRtp->bitsX);
			return -1;
		}

		pRtp->headExtension.tag = mtxwNtohs((*(UINT16*)&pInputData->pData[RTP_HEAD_LENGTH+pRtp->bitsCC*4]));
		pRtp->headExtension.usLength = mtxwNtohs((*(UINT16*)&pInputData->pData[RTP_HEAD_LENGTH+2+pRtp->bitsCC*4]));
		
		pRtp->headExtension.ulOffset = RtpHeaderLength+4;

		//第一个4:代表tag和uslength的长度；pRtp->headExtension.usLength的单位是4个字节
		RtpHeaderLength = RtpHeaderLength + 4 + pRtp->headExtension.usLength*4;
	}

	if(pRtp->bitsP)
	{
	    MTXW_LOGV("rtp Head has padding, padding length = %d", pInputData->pData[pInputData->uSize]);
		int paddinglength = pInputData->pData[pInputData->uSize-1];
	    pInputData->uSize -= paddinglength;
	}

	return RtpHeaderLength;
	

}




