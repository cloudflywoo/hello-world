
#include "mtxw_videoinstance.h"
#include "mtxw_sequenceparameterset.h"
#include "mtxw_controlblock.h"

extern "C" {
extern int XwFfmpegDecoderInit( int width, int height);
extern int XwFfmpegDecoderDestroy();
extern int XwFfmpegDecodeH264Frame(const unsigned char *inbuf, int inbuf_size, unsigned char *out);
}

INT32 mtxw_H264FragmentDataCompare(const MTXW_DATA_BUFFER_POINTER & E1,
                                                                            const MTXW_DATA_BUFFER_POINTER & E2)
{
    const UINT16 MAX = 0xFFFF;
    UINT16 a,b;
    
    a = E1->header.h264_fragment_header.SN;
    b = E2->header.h264_fragment_header.SN;
    
    if(a==b)return 0;
    if(a>b)
    {
    	UINT16 gap12 = a-b;
    	UINT16 gap21 = (MAX-a)+(b-0+1);
    	return (gap12<=gap21) ? 1 : -1;
    	
    }
    else //b>a
    {
    	UINT16 gap12 = b-a;
    	UINT16 gap21 = (MAX-b)+(a-0+1);
    	return (gap12<=gap21) ? -1 : 1;
    }
    
}
INT32 mtxw_H264FrameListCompare(const MTXW_H264_RCV_FRAME_LIST_STRU & E1,
                                                                    const MTXW_H264_RCV_FRAME_LIST_STRU & E2)
{

    const UINT32 MAX = 0xFFFFFFFF;
    UINT32 a,b;
    
    a = E1.ulTimStamp;
    b = E2.ulTimStamp;
    
    if(a==b)return 0;
    if(a>b)
    {
    	UINT32 gap12 = a-b;
    	UINT32 gap21 = (MAX-a)+(b-0+1);
    	return (gap12<=gap21) ? 1 : -1;
    	
    }
    else //b>a
    {
    	UINT32 gap12 = b-a;
    	UINT32 gap21 = (MAX-b)+(a-0+1);
    	return (gap12<=gap21) ? -1 : 1;
    }

    
}



MTXW_VideoInstance::MTXW_VideoInstance(INT32 uInstanceId,INT32 callId):MTXW_Instance(false,uInstanceId,callId)
{
    MTXW_FUNC_TRACE();
    this->enCodecType = MTXW_VIDEO_H264;
    this->pRtpObj = 0;
    this->ulTimeStampRcvWnd = 0xfffffff;//
    this->ulRcvSSRC = 0;
    this->bFirstRcvPackt = false;   
    this->bdiscardflag = true;
    this->bContinueDropFrame = MTXW_STOP_DROP_FRAME;

    this->ulH264RcvBufferMax = 50;
    this->ulRcvFrameWidth = 0;
    this->ulRcvFrameHeight = 0;
#ifndef WIN32
    this->lastRecvFrameTime = {0,0};
    this->lastSendFrameTime = {0,0};
#endif
    this->smoothFactor = 1.0/50.0;    

    this->frameRate = 25;
    this->PopExpectSN = -1;

    this->playYuvflag = 0;
    this->bRcvWndActive = false;//2017.11.21 ��ʱ����Ϊfalse����ʱ��ܳ���̨RTP�����������
    
}
MTXW_VideoInstance::~MTXW_VideoInstance()
{
    MTXW_FUNC_TRACE();

    
    if(pRtpObj)
    {
        delete pRtpObj;
        pRtpObj = 0;
    }
}
INT32 MTXW_VideoInstance::SetVideoCodecType(MTXW_ENUM_VIDEO_CODEC_TYPE type)
{
        MTXW_FUNC_TRACE();
        this->enCodecType = type;
	return 0;
}
INT32 MTXW_VideoInstance::SetH264param(UINT32 frameRate,UINT32 rtpRcvBufferSize, UINT32 frameRcvBufferSize, UINT32 initPlayFactor,UINT32 discardflag,UINT32 playYuvflag)
{
    MTXW_FUNC_TRACE();

    this->frameRate = frameRate;
    this->rtpRcvBufferSize = rtpRcvBufferSize;
    this->frameRcvBufferSize = frameRcvBufferSize;
    this->playYuvflag = playYuvflag;

    if(0 == discardflag)
    {
        this->bdiscardflag = false;
    }
    else
    {
        this->bdiscardflag = true;
    }

    //2017-10-20 ����ֱ������벥��YUV�ĳ����£�������֡����ƵЧ������
    if(playYuvflag)
    {
        this->bdiscardflag = true;
    }

    this->initPlayFactor = (initPlayFactor <= 100) ? initPlayFactor : 0;

    this->ulH264RcvBufferMax = this->rtpRcvBufferSize * this->frameRate+3;//����ʱ��*֡��

    return 0;
}
INT32 MTXW_VideoInstance::Send(UINT8 *pVideoData,UINT32 uLength,UINT32 rotatedAngle)
{

    MTXW_FUNC_TRACE();
    
    MTXW_LOGD("MTXW_VideoInstance::Send length:%d",uLength);

    //MTXW_queue<MTXW_DATA_BUFFER_STRU*>& sendQueue = GetSendQueue();
    if(!this->IsRunning())
    {
        MTXW_LOGE("MTXW_VideoInstance::Send sendthread is not running");
        return -1;
    }
    
    if(pVideoData==NULLPTR)
    {
        MTXW_LOGE("MTXW_VideoInstance::Send pVideoData is Null");
        return -1;
    }

    if(MTXW_DIRECTION_RECEIVE_ONLY == this->GetDirection())
    {
        MTXW_LOGE("MTXW_VideoInstance::Send() Receive only, forbidden sending");
        return -2;
    }
   
    //--�ֶԷ��Ͷ�����������֤�����߳�ͬ��------------
    MTXW_DATA_BUFFER_STRU* pBuffer = (MTXW_DATA_BUFFER_STRU*)mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+uLength);

    if(pBuffer)
    {
        //���ݽ��뷢�Ͷ���
        pBuffer->uSize = uLength;
        pBuffer->header.h264_data_header.uRotatedAngle = rotatedAngle;
        
        mtxwMemCopy(pBuffer->pData, pVideoData, uLength);

        {
            MTXW_lock   lock(mSendQueue.getMutex(),"MTXW_VideoInstance::Send");
            mSendQueue.push(pBuffer);
            mSendQueue.signal();
        }
            //***********************************����֡���ͳ���㷨**************************************************
        struct timespec currSendFrameTime = {0,0};

        double newDelta;

        clock_gettime(CLOCK_MONOTONIC, &currSendFrameTime); //��ȡ��ǰʱ��
        //������֡��ǰһ֡��ʱ���
        if(currSendFrameTime.tv_nsec > lastSendFrameTime.tv_nsec)
        {
            newDelta = (currSendFrameTime.tv_sec - lastSendFrameTime.tv_sec)*1000000000 + (currSendFrameTime.tv_nsec - lastSendFrameTime.tv_nsec);//nano second
        }
        else
        {
            newDelta = (currSendFrameTime.tv_sec - lastSendFrameTime.tv_sec - 1)*1000000000 + (currSendFrameTime.tv_nsec + 1000000000 - lastSendFrameTime.tv_nsec);//nano second
        }           
        
        if((lastSendFrameTime.tv_sec == 0 && lastSendFrameTime.tv_nsec == 0) || newDelta > 1000000000)
        {
            //��������detal����
        }
        else
        {
             sendDelta = sendDelta * (1.0-smoothFactor) + smoothFactor * newDelta;
        }

        lastSendFrameTime = currSendFrameTime;

        if(sendDelta < 1000000000/frameRate)
        { 
            this->needDelaySendTime ++;
        }
        else if(sendDelta > 1000000000/frameRate) 
        {
            if(this->needDelaySendTime>0){this->needDelaySendTime --;}
        }
        //******************************************����֡���ͳ���㷨********************************************
        
        return this->needDelaySendTime;
    }
    else
    {
        //�����ڴ�ʧ��
        return -1;
    }
	return 0;
}


INT32 MTXW_VideoInstance::Start()
{
    MTXW_FUNC_TRACE();

    if(IsRunning())
    {
        MTXW_LOGE("\n MTXW_VideoInstance is already started!");
        return MTXW_Instance::Start();
    }

    this->bFirstRcvPackt = false;
    this->delta = 1000000000/frameRate;
    this->sendDelta = (1000000000/frameRate)/2;
    this->needDelaySendTime = (1000/frameRate)*(3/2);
    this->mRcvSpsLength = 0;
    this->mRcvPpsLength = 0;
    SetPlayQueueLengthMax(frameRcvBufferSize*this->frameRate);
    SetPopExpectSN(-1);

    this->mJitter.setFrequency(90000);
    
    switch(this->enCodecType)
    {
        case MTXW_VIDEO_H264:
        {
            this->pRtpObj = new (std::nothrow) MTXW_H264Rtp();
            if(!this->pRtpObj)
            {
                //����ʵ��ʧ��
                MTXW_LOGE("MTXW_VideoInstance::Start. Create h264 Rtp obj fail!");
                return -1;
            }

            MTXW_H264Rtp *ph264Rtp = dynamic_cast<MTXW_H264Rtp *>(this->pRtpObj);
            if(ph264Rtp)
            {
                ph264Rtp->SetH264FrameRate(this->frameRate);	   
            }	
            else
            {
                MTXW_LOGE("MTXW_VideoInstance::Start ph264Rtp is null!");
                return -1;
            }

            if(this->playYuvflag)
            {
                XwFfmpegDecoderInit(0,0);
            }
            
            
            break;
        }
        
        default:
        {
            //invalid type
            MTXW_LOGE("MTXW_VideoInstance::Start. Create h264 Rtp obj fail!");
            return -1;
        } 

    }

     //--���һ��Ҫ���ø����Start���������߳�
     return  MTXW_Instance::Start();

     
}

INT32 MTXW_VideoInstance::Stop()
{
    MTXW_FUNC_TRACE();

      //--���ø����Stopֹͣ�����߳�
    MTXW_Instance::Stop();    
    
    if(this->pRtpObj)
    {
        delete this->pRtpObj;
        this->pRtpObj = 0;
    }

    ClearRcvBuffer();

    if(this->playYuvflag)
    {
        XwFfmpegDecoderDestroy();
    }

     return  0;
}



/*���������������δ��������ݣ�����Ǵ��������ݣ����������߸����ͷ�vecPackedData����ڴ�
   pPackedData->pData�����ݵ����ݸ�ʽΪ: |-len0-|--pack0--|-len1-|--pack1--|-len2-|--pack2--|......
       lenX�ĳ���Ϊ2�ֽڣ������ֽ���

*/
INT32 MTXW_VideoInstance::Pack(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_DATA_BUFFER_STRU* pPackedData)
{
        MTXW_FUNC_TRACE();

	if(MTXW_VIDEO_H264 == this->enCodecType)
	{
	       
		MTXW_H264Rtp *ph264Rtp = dynamic_cast<MTXW_H264Rtp *>(this->pRtpObj);
		if(ph264Rtp)
		{
		   return ph264Rtp->Pack_H264_Payload(pInputData,pPackedData);		   
		}	
		else
		{
		    MTXW_LOGE("MTXW_VideoInstance::Pack ph264Rtp is null!");
		    return -1;
		}
	}
	else
	{
	       MTXW_LOGE("MTXW_VideoInstance::Pack this->enCodecType is Unknow Value!");
		return -1;
	}
}
/*
MTXW_H264_RCV_FRAME_LIST_STRU* MTXW_VideoInstance::GetTimeStampNodeInFrameList(UINT32 timeStamp)
{
    std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator it=this->h264RcvBuffer.pRcvFrameList.begin();
    std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator it_end=this->h264RcvBuffer.pRcvFrameList.end();
    for(;it!=it_end;++it)
    {
        if(it->ulTimStamp == timeStamp)
        {
            return it;
        }
    }

    return 

}
*/



bool MTXW_VideoInstance::IsCompletNaluOrStart1Fu(MTXW_DATA_BUFFER_STRU* pFragmentData)
{
    if( MTXW_NALU_TYPE_28_FU_A == pFragmentData->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsType)
    {
        if(pFragmentData->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_header.bitS== true)

        {
            return true;
        }
        else
        {
            return false;
        }
        
    }
    else if(pFragmentData->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsType >= MTXW_NALU_TYPE_1_NON_IDR 
            && pFragmentData->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsType <= MTXW_NALU_TYPE_12_FILLER_DATA)
    {

        return true;
    }

    return false;
}

MTXW_ENUM_H264_FU_A_TYPE MTXW_VideoInstance::FragmentType(MTXW_DATA_BUFFER_STRU* pFragmentData)
{
    MTXW_H264_RTP_PAYLOAD_HEADER_UNION Head = pFragmentData->header.h264_fragment_header.rtpPayloadHeader;
    if(Head.fu_A.fu_indicator.bitsType >= MTXW_NALU_TYPE_1_NON_IDR 
        && Head.fu_A.fu_indicator.bitsType <= MTXW_NALU_TYPE_12_FILLER_DATA)
    {
        return MTXW_COMPLETE_NALU;
    }
    else if(Head.fu_A.fu_indicator.bitsType == MTXW_NALU_TYPE_28_FU_A)
    {
        if(Head.fu_A.fu_header.bitS == true)
        {
            return MTXW_FU_A_START;
        }
        else if(Head.fu_A.fu_header.bitE == true)
        {
            return MTXW_FU_A_END;
        }
        else if((false == Head.fu_A.fu_header.bitS )&& (false == Head.fu_A.fu_header.bitE) )
        {
            return MTXW_FU_A_NO_START_END;
        }
    }

    return MTXW_FU_A_MAX;

}
MTXW_ENUM_H264_FU_A_TYPE MTXW_VideoInstance::FragmentType(std::list<MTXW_DATA_BUFFER_STRU*>::iterator it)
{
    MTXW_H264_RTP_PAYLOAD_HEADER_UNION Head = (*it)->header.h264_fragment_header.rtpPayloadHeader;
    if(Head.fu_A.fu_indicator.bitsType >= MTXW_NALU_TYPE_1_NON_IDR 
        && Head.fu_A.fu_indicator.bitsType <= MTXW_NALU_TYPE_12_FILLER_DATA)
    {
        return MTXW_COMPLETE_NALU;
    }
    else if(Head.fu_A.fu_indicator.bitsType == MTXW_NALU_TYPE_28_FU_A)
    {
        if(Head.fu_A.fu_header.bitS == true)
        {
            return MTXW_FU_A_START;
        }
        else if(Head.fu_A.fu_header.bitE == true)
        {
            return MTXW_FU_A_END;
        }
        else if((false == Head.fu_A.fu_header.bitS )&& (false == Head.fu_A.fu_header.bitE) )
        {
            return MTXW_FU_A_NO_START_END;
        }
    }

    return MTXW_FU_A_MAX;

}

void MTXW_VideoInstance::ClearRcvBuffer()
{
    while(this->h264RcvBuffer.RcvFrameList.begin() != this->h264RcvBuffer.RcvFrameList.end())
    {
		std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator it=this->h264RcvBuffer.RcvFrameList.begin();
        while(it->FragmentDataList.begin() != it->FragmentDataList.end())
        {
            mtxwMemFree(*it->FragmentDataList.begin());
            it->FragmentDataList.pop_front();
        }
        
        this->h264RcvBuffer.RcvFrameList.pop_front();
    }
}

void MTXW_VideoInstance::ClearOneFrameList(std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator it)
{
    while(it->FragmentDataList.begin()!=it->FragmentDataList.end())
    {
        mtxwMemFree(*it->FragmentDataList.begin());
        it->FragmentDataList.pop_front();                
    }
}

/*******************************************************************************************
*��H264��RTP payload��ͷ���ȡNALU ����
*
*2017.11.14  wuyunfei
*********************************************************************************************/
MTXW_NALU_TYPE_ENUM MTXW_VideoInstance::GetNaluType(MTXW_H264_FRAGMENT_HEADER_STRU fragHdr)
{

    if(fragHdr.rtpPayloadHeader.fu_A.fu_indicator.bitsType >= MTXW_NALU_TYPE_1_NON_IDR
    &&fragHdr.rtpPayloadHeader.fu_A.fu_indicator.bitsType <= MTXW_NALU_TYPE_12_FILLER_DATA)
    {
        return (MTXW_NALU_TYPE_ENUM) fragHdr.rtpPayloadHeader.fu_A.fu_indicator.bitsType;
        
    }else if(fragHdr.rtpPayloadHeader.fu_A.fu_indicator.bitsType == MTXW_NALU_TYPE_28_FU_A)
    {
        return (MTXW_NALU_TYPE_ENUM)fragHdr.rtpPayloadHeader.fu_A.fu_header.bitsNalu_type;
        
    }else
    {
        return MTXW_NALU_TYPE_0;
    }
}

MTXW_ENUM_FRAME_TYPE MTXW_VideoInstance::IsIDRFrame(std::list<MTXW_DATA_BUFFER_STRU*>::iterator it)
{
    
    if((*it)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsType >= MTXW_NALU_TYPE_1_NON_IDR
        && (*it)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsType <= MTXW_NALU_TYPE_12_FILLER_DATA)
    {
        if((*it)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsType == MTXW_NALU_TYPE_5_IDR
            || (*it)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsType == MTXW_NALU_TYPE_7_SPS
            || (*it)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsType == MTXW_NALU_TYPE_8_PPS)
        {
            return MTXW_FRAME_TYPE_IDR;
        }
        else
        {
            return MTXW_FRAME_TYPE_NON_IDR;
        }
      
        
    }
    else if((*it)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsType == MTXW_NALU_TYPE_28_FU_A)
    {
        if((*it)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_header.bitsNalu_type == MTXW_NALU_TYPE_5_IDR
            || (*it)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_header.bitsNalu_type == MTXW_NALU_TYPE_7_SPS
            || (*it)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_header.bitsNalu_type == MTXW_NALU_TYPE_8_PPS)
        {
            return MTXW_FRAME_TYPE_IDR;
        }
        else
        {
            return MTXW_FRAME_TYPE_NON_IDR;
        }
    }
    else
    {
        return MTXW_FRAME_TYPE_MAX;
    }
    
}



INT32 MTXW_VideoInstance::UpdateFrameToPlayQueue()
{
    INT32 temp =0;
    
    std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator it_begin;
    while(true)
    {
        it_begin = this->h264RcvBuffer.RcvFrameList.begin();

        if(it_begin==this->h264RcvBuffer.RcvFrameList.end())
        {
            break;
        }

        if(it_begin->bCompleted == false)
        {
            break;
        }
        MTXW_queue<MTXW_DATA_BUFFER_STRU*>& playQueue = GetPlayQueue();
        {
            MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_VideoInstance::UpdateFrameToPlayQueue");
            if(playQueue.size() >= GetPlayQueueLengthMax())
            {
                break;
            }
        }    	

        temp = PutHeadFramListToPlayQueue();
        if(temp == -1)
        {
            MTXW_LOGI("MTXW_VideoInstance::UpdateFrameToPlayQueue Put HeadFramList to PlayQueue fail!!!!!");
            return -1;

        }
    }
    
    return temp;
   
}
INT32 MTXW_VideoInstance:: PutHeadFramListToPlayQueue()
{
    MTXW_FUNC_TRACE();
    
    std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator it_begin=this->h264RcvBuffer.RcvFrameList.begin();
    if(it_begin==this->h264RcvBuffer.RcvFrameList.end())
    {
        MTXW_LOGE("MTXW_VideoInstance::PutHeadFramListToPlayQueue: no frame");
        return -1;
    }
    std::list<MTXW_DATA_BUFFER_STRU*>::iterator itFragmentFirst=it_begin->FragmentDataList.begin();
    if(itFragmentFirst==it_begin->FragmentDataList.end())//��֡û���κ�����
    {
        this->h264RcvBuffer.RcvFrameList.pop_front();
        return 0;
    }
    
    if(this->GetPopExpectSN() != (*itFragmentFirst)->header.h264_fragment_header.SN)
    {
        //����ǵ�һ��TimeStamp FramList�����ж�
        if(this->GetPopExpectSN() == -1)
        {
            this->bContinueDropFrame = MTXW_STOP_DROP_FRAME;
        }
        else
        {
            this->bContinueDropFrame = MTXW_CONTINUE_DROP_DAMAGE_FRAME;
            MTXW_LOGE("ulTimStamp = %d, PopExpectSN = %d, itFragmentFirstSN = %d ", it_begin->ulTimStamp, this->GetPopExpectSN(), (*itFragmentFirst)->header.h264_fragment_header.SN);
        }        
    }

    if(it_begin->bCompleted == true)
    {
        
        if(MTXW_FRAME_TYPE_IDR == IsIDRFrame(it_begin->FragmentDataList.begin()))
        {
            this->bContinueDropFrame = MTXW_STOP_DROP_FRAME;
        }

        //����û����õ�����֡����ģʽ������ǰ����ж�,ֱ�Ӳ���H264֡
        if(!this->Getbdiscardflag())
        {
            this->bContinueDropFrame = MTXW_STOP_DROP_FRAME;
        }

        if((MTXW_CONTINUE_DROP_DAMAGE_FRAME == this->bContinueDropFrame) || (MTXW_CONTINUE_DROP_FULL_FRAME == this->bContinueDropFrame))
        {
            MTXW_LOGE("MTXW_VideoInstance:: PutHeadFramListToPlayQueue, this->bContinueDropFrame =%d, the Drop timeStame=%d",this->bContinueDropFrame, it_begin->ulTimStamp);
            GetVedioCount()->rtp_drop_cnt += it_begin->FragmentDataList.size();

            SetDropPacketNum(GetDropPacketNum()+it_begin->FragmentDataList.size());
            
            //���½��մ��±�Ե
            SetTimeStampWndStart(it_begin->ulTimStamp);

            //���µ�ǰtimeStamp List�����һ�����ݰ�������SN
            SetPopExpectSN(it_begin->ulExpectSN);

            /*ɾ��framlist�е�fragmentDataList���ڴ�*/
            while(it_begin->FragmentDataList.begin()!=it_begin->FragmentDataList.end())
            {
                mtxwMemFree(*it_begin->FragmentDataList.begin());
                it_begin->FragmentDataList.pop_front();                
            }        
            this->h264RcvBuffer.RcvFrameList.pop_front();

            if((MTXW_CONTINUE_DROP_DAMAGE_FRAME == this->bContinueDropFrame))
            {
                GetVedioCount()->frame_drop_for_damage_cnt++;
            }
            else
            {
                GetVedioCount()->frame_drop_for_full_cnt++;
            }
            
            return 0;

        }

        MTXW_DATA_BUFFER_STRU* pBuffer = (MTXW_DATA_BUFFER_STRU*)mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+it_begin->ulLengthOfFragmentData+4+512);//��չ512�ֽڣ��Ա����SPS��PPS��Ԫ
        if(!pBuffer)
        {
            //�����ڴ�ʧ��
            MTXW_LOGE("MTXW_VideoInstance::MTXW_VideoInstance allocate mem for m_pPackBuffer fail!");
            return -1;
        }
        MTXW_LOGD("framelist timeStamp =%d, it_begin->ulLengthOfFragmentData=%d\n", it_begin->ulTimStamp, it_begin->ulLengthOfFragmentData);

        pBuffer->header.h264_data_header.uRotatedAngle = it_begin->ulRotateAngle;
        pBuffer->header.h264_data_header.uWidth = this->ulRcvFrameWidth;
        pBuffer->header.h264_data_header.uHeight = this->ulRcvFrameHeight;
        pBuffer->header.h264_data_header.ulSsrc = this->GetRcvSSRC();
        //2017.11.14  added
        INT32 firstNaluType =  GetNaluType((*it_begin->FragmentDataList.begin())->header.h264_fragment_header);
        pBuffer->header.h264_data_header.isNonIDRFrame =  (MTXW_NALU_TYPE_1_NON_IDR==firstNaluType);
        pBuffer->uSize = 0;
        if(firstNaluType==MTXW_NALU_TYPE_5_IDR)
        {
            //���ΪI֡�������SPS��PPS NALU��Ԫ
            if(this->mRcvSpsLength>0 && this->mRcvPpsLength>0)
            {
                mtxwMemCopy(&pBuffer->pData[0], this->mRcvSps, this->mRcvSpsLength);
                pBuffer->uSize +=  this->mRcvSpsLength;
                mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], this->mRcvPps, this->mRcvPpsLength);
                pBuffer->uSize +=  this->mRcvPpsLength;
            }
        }else
        {
            pBuffer->uSize = 0;
        }
        UINT8 startCode[4] = {0x00,0x00,0x00,0x01};

        mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], &startCode, 4);

        pBuffer->uSize += 4;
        //��ͷ��־λ
        bool naluheadflag = true;
        std::list<MTXW_DATA_BUFFER_STRU*>::iterator itF=it_begin->FragmentDataList.begin();
        std::list<MTXW_DATA_BUFFER_STRU*>::iterator itFragmentend=it_begin->FragmentDataList.end();
        
        for(;itF != itFragmentend;itF++)
        {
            if(MTXW_COMPLETE_NALU == FragmentType(itF))
            {
                MTXW_NALU_HEADER_STRU NaluHead;
                NaluHead.bitF = (*itF)->header.h264_fragment_header.rtpPayloadHeader.nalu_header.bitF;
                NaluHead.bitsNRI = (*itF)->header.h264_fragment_header.rtpPayloadHeader.nalu_header.bitsNRI;
                NaluHead.bitsType = (*itF)->header.h264_fragment_header.rtpPayloadHeader.nalu_header.bitsType;

                //mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], &NaluHead, 1);
                pBuffer->pData[pBuffer->uSize] = NaluHead.bitF<<7 | NaluHead.bitsNRI<<5 | NaluHead.bitsType;
                pBuffer->uSize += 1;
                mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], &((*itF)->pData[0]), (*itF)->uSize);
                pBuffer->uSize += (*itF)->uSize;
                mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], &startCode, 4);
                pBuffer->uSize += 4;
            }
            //-------------------------
            else
            {
                 MTXW_NALU_HEADER_STRU NaluHead;
                 if(naluheadflag)
                 {
                     //-1- pack NALU head
                    NaluHead.bitF = (*itF)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitF;
                    NaluHead.bitsNRI = (*itF)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsNRI;
                    NaluHead.bitsType = (*itF)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_header.bitsNalu_type;
                    
    	             pBuffer->pData[pBuffer->uSize] = NaluHead.bitF<<7 | NaluHead.bitsNRI<<5 | NaluHead.bitsType;
                    pBuffer->uSize += 1;                    
                     //-2- set naluheadflag to false
                     naluheadflag = false;
                 }

                 if(MTXW_FU_A_START == FragmentType(itF))
                 {
                     if(naluheadflag)
                     {
                         //means miss a END, if go to here                         

                         //pack start code
                         mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], &startCode, 4);
                         pBuffer->uSize += 4;
                         // pack NALU head
                         
                         NaluHead.bitF = (*itF)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitF;
                         NaluHead.bitsNRI = (*itF)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_indicator.bitsNRI;
                         NaluHead.bitsType = (*itF)->header.h264_fragment_header.rtpPayloadHeader.fu_A.fu_header.bitsNalu_type;
	                  pBuffer->pData[pBuffer->uSize] = NaluHead.bitF<<7 | NaluHead.bitsNRI<<5 | NaluHead.bitsType;
                         pBuffer->uSize += 1;
                         //set naluheadflag to false
                         naluheadflag = false;                                                  
                     }

                    mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], &(*itF)->pData[0], (*itF)->uSize);
                    pBuffer->uSize += (*itF)->uSize;
                     
                 }
                 else if(MTXW_FU_A_END == FragmentType(itF))
                 {                 
                     // set naluheadflag to true
                     naluheadflag = true;
                     
                     mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], &(*itF)->pData[0], (*itF)->uSize);
                     pBuffer->uSize += (*itF)->uSize;
                     mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], &startCode, 4);
                     pBuffer->uSize += 4;                     
                 }
                 else if(MTXW_FU_A_NO_START_END == FragmentType(itF))
                 {
                     mtxwMemCopy(&pBuffer->pData[pBuffer->uSize], &(*itF)->pData[0], (*itF)->uSize);
                     pBuffer->uSize += (*itF)->uSize;                
                 }
                 else if(MTXW_FU_A_MAX == FragmentType(itF))
                 {
                     MTXW_LOGE("MTXW_VideoInstance::PutHeadFramListTOPlayQueue FragmentType Is Invali!!!!");
                     continue;                    
                 }
                 
            }
            //------------------
                        
        }

        //ȥ�����һ��startCode
        pBuffer->uSize -= 4;	 
        MTXW_LOGI("put the framelist into playQueue, framelist timeStamp =%d, pBuffer->uSize=%d\n", it_begin->ulTimStamp, pBuffer->uSize);
        
        //���½��մ��±�Ե
        SetTimeStampWndStart(it_begin->ulTimStamp);

        //���µ�ǰtimeStamp List�����һ�����ݰ�������SN
        SetPopExpectSN(it_begin->ulExpectSN);

        /*ɾ��framlist�е�fragmentDataList���ڴ�*/
        while(it_begin->FragmentDataList.begin()!=it_begin->FragmentDataList.end())
        {
            mtxwMemFree(*it_begin->FragmentDataList.begin());
            it_begin->FragmentDataList.pop_front();                
        }        
        this->h264RcvBuffer.RcvFrameList.pop_front();
        //����
        GetVedioCount()->rtp_buffer_size--;
        MTXW_LOGI("Video.rtp.buffer_size: %d", GetVedioCount()->rtp_buffer_size);

        
        MTXW_queue<MTXW_DATA_BUFFER_STRU*>& playQueue = GetPlayQueue();
        {
            MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_VideoInstance::ReceiveH264RtpPacket:playQueue");
            if(playQueue.size() >= GetPlayQueueLengthMax())
            {
                /*--2017.11.14 �޸�: ��������Ѿ���������NON-IDRֱ֡�Ӷ���
                * ����IDR֡����ֱ����յ�ǰ���Ŷ���(�൱�ڿ��)
                * 
                *****************************************************************************/
                if(pBuffer->header.h264_data_header.isNonIDRFrame)
                {
                    MTXW_LOGE("MTXW_VideoInstance::ReceiveH264RtpPacket playQueue is full!! discard this frame ");
                    mtxwMemFree(pBuffer);
                    this->bContinueDropFrame = MTXW_CONTINUE_DROP_FULL_FRAME;
                    GetVedioCount()->frame_drop_for_full_cnt++;
                    return -1;
                }else{
                    //��ն���
                	while(!playQueue.empty())
	              {
		          mtxwMemFree(playQueue.front());
		          playQueue.pop();
	              }
                }
            }
         
            playQueue.push(pBuffer);
            //����ͳ��
            GetVedioCount()->frame_buffer_size = playQueue.size();
            MTXW_LOGI("Video.Frame.buffer_size: %d", GetVedioCount()->frame_buffer_size);

        }    		

    }
    
    return 0;
}
INT32 MTXW_VideoInstance::ReceiveRtpPacket(MTXW_DATA_BUFFER_STRU* pInputData)
{
    MTXW_FUNC_TRACE();
    UINT32 RtpHeadLength = 0;    
    UINT32 rcvTimeStamp = 0;
    UINT32 gapTimeStamp = 0;

    MTXW_Rtp rcvRtpPact;
   
         
    RtpHeadLength = this->pRtpObj->UnpackRtpHeader(pInputData, &rcvRtpPact);

    if(RtpHeadLength <= 0 || pInputData->uSize<RtpHeadLength)
    {
        MTXW_LOGE("MTXW_VideoInstance::ReceiveRtpPacket UnpackRtpHeader Error!");
        return -1;
    }
    //�жϽ��յ����ݰ���payloadtype�Ƿ�Ϸ�	
    if(rcvRtpPact.payloadType <96 || rcvRtpPact.payloadType >128)
    {
        MTXW_LOGV("MTXW_VideoInstance:the Receive Rtp Packert payLoadType is Invalid, payloadType=%d", rcvRtpPact.payloadType);
        return -1;
    }
    MTXW_LOGD("MTXW_VideoInstance::ReceiveRtpPacket fec debug rtp SN = %d",rcvRtpPact.GetSeq());
    
    GetVedioCount()->rtp_rcv_cnt++;

    rcvTimeStamp = rcvRtpPact.timeStamp;
    UINT32 ssrc = rcvRtpPact.GetSSRC();

    if(this->bFirstRcvPackt==false)
    {
        // do somthing
        SetRcvSSRC(ssrc);
        SetTimeStampWndStart(rcvTimeStamp-1);
    }
   
    this->bFirstRcvPackt = true;

    if(this->GetRcvSSRC() != ssrc)
    {
        SetRcvSSRC(ssrc);
	 SetTimeStampWndStart(rcvTimeStamp-1);
        ClearRcvBuffer();
        SetPopExpectSN(-1);                       
    }
    
    if(GetTimeStampWndStart() > rcvRtpPact.timeStamp)
    {        
        gapTimeStamp = (0xFFFFFFFF - GetTimeStampWndStart()) + (rcvTimeStamp -0 + 1);       
    }
    else
    {
        gapTimeStamp = rcvTimeStamp - GetTimeStampWndStart();
    }

    if(this->bRcvWndActive)
    {

        if(gapTimeStamp> this->GetTimeStampRcvWnd() || gapTimeStamp == 0)
        {                             
            MTXW_LOGA("\n the Receive Video RtpPacket TimeStamp is Not In Window ,Drop!! gapTimeStamp=%d,  rcvTimeStamp=%d, TimeStampRcvWnd=%d",
                                     gapTimeStamp, rcvRtpPact.timeStamp, GetTimeStampWndStart());
            GetVedioCount()->rtp_drop_cnt++;
            SetDropPacketNum(GetDropPacketNum()+1);
            
            return -1;
        }


    }
     //---2018.01.17 ����jitter����
    this->mJitter.onRecv(rcvRtpPact.timeStamp);
    MTXW_LOGI(" video jitter = %d meanJitter = %d",this->mJitter.getJitter(),this->mJitter.getMeanJitter());
    
    switch(this->enCodecType)
    {
        case MTXW_VIDEO_H264:
        {
            ReceiveH264RtpPacket(pInputData, RtpHeadLength, rcvRtpPact);
            return 0;
        }
        default:
        {
            MTXW_LOGE("MTXW_VideoInstance::ReceiveRtpPacket unknow enCodeType:%d",this->enCodecType);
            return -1;
        }
    }
                          
}
INT32 MTXW_VideoInstance::ReceiveH264RtpPacket(MTXW_DATA_BUFFER_STRU* pInputData, UINT32 RtpHeadLength, MTXW_Rtp rcvRtpPact)
{
    MTXW_FUNC_TRACE();
    MTXW_H264_RTP_PAYLOAD_HEADER_UNION PayloadHdr = {0};
    UINT8 PayloadHdrLength = 0;
    
    if(pInputData == NULL)
    {   
        MTXW_LOGE("MTXW_VideoInstance::ReceiveH264RtpPacket pInputData is NULL!!!!");
        return -1;
    }
    
    MTXW_H264Rtp *pH264Rtp = dynamic_cast<MTXW_H264Rtp *>(this->pRtpObj);
    if(pH264Rtp)
    {                
        if(0 != pH264Rtp->UnPack_H264_Payload(&pInputData->pData[RtpHeadLength], &PayloadHdr, &PayloadHdrLength))
        {
            MTXW_LOGE("MTXW_VideoInstance::ReceiveH264RtpPacket UnPack_H264_Payload Error!");
            return -1;                               
        }        
       
    }
    else
    {
        MTXW_LOGE("MTXW_VideoInstance::ReceiveH264RtpPacket failed to get H264 rtp object!");
        return -1;
    }

    //����SPS���ݰ��ʹ洢��Ӧ�Ŀ��
    if(PayloadHdr.fu_A.fu_indicator.bitsType == MTXW_NALU_TYPE_7_SPS)
    {
        SequenceParameterSet sps(&pInputData->pData[RtpHeadLength+PayloadHdrLength], pInputData->uSize - RtpHeadLength - PayloadHdrLength);
        this->ulRcvFrameWidth = sps.getWidth();
        this->ulRcvFrameHeight = sps.getHeight();

        //����SPS��Ϣ,Ϊ�˱��ں���ʹ�ã�����������startCode + naluHeader
        //|--startcode--|-naluHdr-|----------------sps data-------------|

        if(pInputData->uSize<256) //��һ���򵥵ĺϷ����жϣ�һ��SPS���Ǽ�ʮ���ֽ�
        {
        UINT8 naluHdr = PayloadHdr.fu_A.fu_indicator.bitF<<7 | PayloadHdr.fu_A.fu_indicator.bitsNRI<<5|PayloadHdr.fu_A.fu_indicator.bitsType;
        UINT8 startCodeAndHeader[5]={0x00,0x00,0x00,0x01,naluHdr};
        this->mRcvSpsLength = pInputData->uSize - RtpHeadLength - PayloadHdrLength;
        mtxwMemCopy(this->mRcvSps+5, &pInputData->pData[RtpHeadLength+PayloadHdrLength], this->mRcvSpsLength);
        mtxwMemCopy(this->mRcvSps, startCodeAndHeader, 5);
        this->mRcvSpsLength += 5; //����startcode��naluheader�ĳ���
        //MTXW_LOGE("wyf-debug  ReceiveH264RtpPacket  found sps (%d bytes)",this->mRcvSpsLength);
        }
        
    }

    //����PPS���ݰ���Ϣ
    if(PayloadHdr.fu_A.fu_indicator.bitsType == MTXW_NALU_TYPE_8_PPS)
    {
        if(pInputData->uSize<256) //��һ���򵥵ĺϷ����жϣ�һ��PPS���Ǽ�ʮ���ֽ�
        {
        UINT8 naluHdr = PayloadHdr.fu_A.fu_indicator.bitF<<7 | PayloadHdr.fu_A.fu_indicator.bitsNRI<<5|PayloadHdr.fu_A.fu_indicator.bitsType;
        UINT8 startCodeAndHeader[5]={0x00,0x00,0x00,0x01,naluHdr};
        
        this->mRcvPpsLength = pInputData->uSize - RtpHeadLength - PayloadHdrLength;
        mtxwMemCopy(this->mRcvPps+5, &pInputData->pData[RtpHeadLength+PayloadHdrLength], this->mRcvPpsLength);
        mtxwMemCopy(this->mRcvPps, startCodeAndHeader, 5);
        this->mRcvPpsLength += 5;//����startcode��naluheader�ĳ���
        //MTXW_LOGE("wyf-debug  ReceiveH264RtpPacket  found pps (%d bytes) ",this->mRcvPpsLength);
        }
  
    }
    
    UINT32 rcvTimeStamp = rcvRtpPact.timeStamp;
    UINT32 ssrc = rcvRtpPact.GetSSRC();
    
    bool bDiscard = false;

    MTXW_LOGV("ReceiveH264RtpPacket: the Seqnumber = %d, the rcvRtpPacket rcvTimeStamp=%d\n", rcvRtpPact.seq, rcvTimeStamp);

    //---1---��h264RcvBuffer������ͬtimestamp�Ľڵ�
    std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator it=this->h264RcvBuffer.RcvFrameList.begin();
    std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator it_end=this->h264RcvBuffer.RcvFrameList.end();

    for(;it!=it_end;it++)
    {
         if(it->ulTimStamp == rcvTimeStamp)
         {
             MTXW_LOGV("find the TimeStamp,  the Seqnumber = %d, the rcvRtpPacket rcvTimeStamp=%d\n", rcvRtpPact.seq, rcvTimeStamp);
             break;
         }
    }

    //-----���û���ҵ�����Ҫ������timestamp��FrameList�ڵ㣬Ȼ���ڸýڵ��б�����Ƭ
    
    if(it==it_end)
    {
        this->ulRcvFrameNum ++;//���յ��µ�һ֡����

        //----1---����pFragmentData---------------------------
        UINT32 rtpPayloadContentLength = pInputData->uSize - RtpHeadLength - PayloadHdrLength;
        MTXW_DATA_BUFFER_STRU*  pFragmentData = (MTXW_DATA_BUFFER_STRU*)mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+rtpPayloadContentLength);

        if(!pFragmentData)
        {
            return -1;
        }
       pFragmentData->header.h264_fragment_header.SN = rcvRtpPact.seq;
       pFragmentData->header.h264_fragment_header.bMark = rcvRtpPact.bitMaker;
       pFragmentData->header.h264_fragment_header.rtpPayloadHeader = PayloadHdr;
       UINT32 ulOffset = RtpHeadLength+PayloadHdrLength;
       mtxwMemCopy(pFragmentData->pData, &pInputData->pData[ulOffset], rtpPayloadContentLength);
       pFragmentData->uSize = rtpPayloadContentLength;

       MTXW_LOGV("ReceiveH264RtpPacket: the pFragmentData is the frameList head the rcvRtpPacket Seqnumber=%d ,rcvTimeStamp=%d, rtpPayloadContentLength=%d\n", rcvRtpPact.seq, rcvTimeStamp, rtpPayloadContentLength);

        //----2----����framelist-----------------------------------
        MTXW_H264_RCV_FRAME_LIST_STRU   framelist;
        framelist.ulTimStamp = rcvTimeStamp;
        framelist.ulExpectSN = rcvRtpPact.seq+1;


        //����RotateAngle ����RTP��չͷЯ������
        /***************************************************
              |----tag-----|---length---| //tag=0x0000, length=0x02
              |---  00    09    29  A5 -----|  //MagicNumber=0x000929A5
              |srvId|--angle----|--rsv---|  //srvId=0x01, rsv=0x00, angle=0,90,180,270
        **********************************************************/
        if(rcvRtpPact.HasHeaderExtension())
        {
            UINT16 tag = 0x0000;
            UINT32 MagicNumber = 0x000929A5;
            UINT8 SrvId = 0x01;
            UINT32 RcvMagicNumber = mtxwNtohl(*((UINT32*)&pInputData->pData[rcvRtpPact.GetHeadExtensionOffset()]));
            UINT8 RcvSrvId = *((UINT8*)&pInputData->pData[rcvRtpPact.GetHeadExtensionOffset()+4]);
            
            if(tag == rcvRtpPact.GetHeadExtensionTag())
            {
                if(rcvRtpPact.GetHeadExtensionLength()>0)
                {
                    if(MagicNumber == RcvMagicNumber)
                    {
                        if(SrvId == RcvSrvId)
                        {
                            framelist.ulRotateAngle = mtxwNtohs(*((UINT16*)&pInputData->pData[rcvRtpPact.GetHeadExtensionOffset()+5]));
                        }
                    }
                    else
                    {
                        MTXW_LOGI("the Rtp extension head RcvMagicNumber = %d", RcvMagicNumber);
                    }
                }
            }
        }
        
        //ulLengthOfFragmentDataֻ���������ڴ�
        framelist.ulLengthOfFragmentData += 1 + rtpPayloadContentLength+4;
        
        framelist.bCompleted = rcvRtpPact.bitMaker;
        framelist.FragmentDataList.push(pFragmentData);       

        //������ն��е�Buffer����
        if(this->h264RcvBuffer.RcvFrameList.size() == this->GetH264RcvBufferMax())
        {
            std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator itFirst = this->h264RcvBuffer.RcvFrameList.begin();

            //�ж��Ƿ񲥷���֡
            if(!this->Getbdiscardflag())
            {                
                itFirst->bCompleted = true;
                if(0 != PutHeadFramListToPlayQueue())
                {
                    
                    MTXW_LOGI("Put HeadFramList to PlayQueue fail!!!!!");
                    return -1;
                }
            }
            else
            {
                //��������֡
                std::list<MTXW_DATA_BUFFER_STRU*>::iterator itdamage = itFirst->FragmentDataList.begin();

                GetVedioCount()->frame_drop_for_damage_cnt++;
                GetVedioCount()->rtp_drop_cnt += itFirst->FragmentDataList.size();
                SetDropPacketNum(GetDropPacketNum()+itFirst->FragmentDataList.size());

                //��ɾ��ͷframeList
                SetTimeStampWndStart(itFirst->ulTimStamp);
                //���µ�ǰtimeStamp frameList�����һ�����ݰ�������SN
                SetPopExpectSN(itFirst->ulExpectSN);
                
                MTXW_LOGE("this FrameList Is damage ,Drop the FrameList !!!! drop TimeStamp = %d, FrameType = %d", itFirst->ulTimStamp, IsIDRFrame(itdamage));
                
                ClearOneFrameList(itFirst);                              
                this->h264RcvBuffer.RcvFrameList.pop_front();
                
                //ѭ��Buffer�е�frameList����������µ�I֡��ֹͣ����
                while(this->h264RcvBuffer.RcvFrameList.begin()!= this->h264RcvBuffer.RcvFrameList.end())
                {
                    std::list<MTXW_H264_RCV_FRAME_LIST_STRU>::iterator iterm = this->h264RcvBuffer.RcvFrameList.begin();

                    //���������д���I֡��ֱ���˳����ѭ��
                    if(MTXW_FRAME_TYPE_IDR == IsIDRFrame(iterm->FragmentDataList.begin()))
                    {
                        break;
                    }
                    
                    //������һ��frameListҲ����I֡������¼���������־λΪ1
                    if(this->h264RcvBuffer.RcvFrameList.begin() == (--this->h264RcvBuffer.RcvFrameList.end()))
                    {
                        this->bContinueDropFrame = MTXW_CONTINUE_DROP_DAMAGE_FRAME;
                    }
                    
                    GetVedioCount()->frame_drop_for_damage_cnt++;
                    SetDropPacketNum(GetDropPacketNum()+1);
                    GetVedioCount()->rtp_drop_cnt += iterm->FragmentDataList.size();
                    SetTimeStampWndStart(iterm->ulTimStamp);
                    //���µ�ǰtimeStamp frameList�����һ�����ݰ�������SN
                    SetPopExpectSN(iterm->ulExpectSN);
                    MTXW_LOGE("this FrameList is Not useless, Because  we have drop the frameList In this Frame!!!! drop TimeStamp = %d", iterm->ulTimStamp);
                    
                    ClearOneFrameList(iterm);              
                    this->h264RcvBuffer.RcvFrameList.pop_front();     

                }
                            
            }
			
        }
        
        //����֡����ͳ����

        GetVedioCount()->rtp_buffer_size++;
        MTXW_LOGI("Video.rtp.buffer_size: %d", GetVedioCount()->rtp_buffer_size);

        this->h264RcvBuffer.RcvFrameList.push(framelist);
        
#ifndef WIN32
            //***********************************����֡���ͳ���㷨**************************************************
            struct timespec currRecvFrameTime = {0,0};

            double newDelta;

            clock_gettime(CLOCK_MONOTONIC, &currRecvFrameTime); //��ȡ��ǰʱ��
            //������֡��ǰһ֡��ʱ���
            if(currRecvFrameTime.tv_nsec > lastRecvFrameTime.tv_nsec)
            {
                newDelta = (currRecvFrameTime.tv_sec - lastRecvFrameTime.tv_sec)*1000000000 + (currRecvFrameTime.tv_nsec - lastRecvFrameTime.tv_nsec);//nano second
            }
            else
            {
                newDelta = (currRecvFrameTime.tv_sec - lastRecvFrameTime.tv_sec - 1)*1000000000 + (currRecvFrameTime.tv_nsec + 1000000000 - lastRecvFrameTime.tv_nsec);//nano second
            }           
            
            if((lastRecvFrameTime.tv_sec == 0 && lastRecvFrameTime.tv_nsec == 0) || newDelta > 1000000000)
            {
                //��������detal����
            }
            else
            {
                 delta = delta * (1.0-smoothFactor) + smoothFactor * newDelta;
            }

            lastRecvFrameTime = currRecvFrameTime;
            //******************************************����֡���ͳ���㷨********************************************
#endif   
        

		/*--�ж��Ƿ�Ϊ����֡
        ͬʱ������������������
        1)֡�б�����һ��RTP��Я����MARK=TRUE��
        2)expectSN�������һ��RTP����SN+1
        3)��һ��RTPΪ������NALU����start=1��FU��Ƭ
        */
        std::list<MTXW_DATA_BUFFER_POINTER>::iterator itFragmentbegin =  framelist.FragmentDataList.begin();
        std::list<MTXW_DATA_BUFFER_POINTER>::iterator itFragmentend =  framelist.FragmentDataList.end();
		itFragmentend--;

        if((*itFragmentend)->header.h264_fragment_header.bMark == true 
            && (framelist.ulExpectSN == ((*itFragmentend)->header.h264_fragment_header.SN+1)) 
            && IsCompletNaluOrStart1Fu(*itFragmentbegin))
        {
            framelist.bCompleted = true;

            MTXW_LOGI("the framelist is Complete, framelist timeStamp =%d, rtpPayloadContentLength =%d\n", framelist.ulTimStamp, framelist.ulLengthOfFragmentData);
           

        }

    }
       //---1.1--����ҵ������ڸýڵ���б��в����Ƭ
    else
    {
        MTXW_DATA_BUFFER_STRU DataBuffer;
        DataBuffer.header.h264_fragment_header.SN = rcvRtpPact.seq;
        if(rcvRtpPact.seq > it->ulExpectSN)
        {
            if(it->FragmentDataList.iscontain(&DataBuffer))
            {
                bDiscard = true;//�ظ���
            }
            else
            {
                //doNothing
            }
        }
        else
        {
            if(rcvRtpPact.seq == it->ulExpectSN)
            {
                it->ulExpectSN++;
            }
            else
            {
                if(it->FragmentDataList.iscontain(&DataBuffer))
                {
                    bDiscard = true;//�ظ���
                }
                else
                {
                    it->ulExpectSN++;
                }
            }
        }

        if(bDiscard == true)
        {  
            GetVedioCount()->rtp_drop_cnt++;
            SetDropPacketNum(GetDropPacketNum()+1);
            MTXW_LOGW("MTXW_VideoInstance::ReceiveH264RtpPacket. Duplicate packet! Discard the packet");
            return -1;
        }
        else
        {
            //----1---����pFragmentData---------------------------
            UINT32 rtpPayloadContentLength = pInputData->uSize - RtpHeadLength - PayloadHdrLength;
            MTXW_DATA_BUFFER_STRU*  pFragmentData = (MTXW_DATA_BUFFER_STRU*)mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+rtpPayloadContentLength);

            if(!pFragmentData)
            {
                MTXW_LOGE("MTXW_VideoInstance::ReceiveH264RtpPacket. pFragmentData is null!");
                return -1;
            }
            pFragmentData->header.h264_fragment_header.SN = rcvRtpPact.seq;
            pFragmentData->header.h264_fragment_header.bMark = rcvRtpPact.bitMaker;
            pFragmentData->header.h264_fragment_header.rtpPayloadHeader = PayloadHdr;
            pFragmentData->uSize = rtpPayloadContentLength;

            UINT32 ulOffset = RtpHeadLength+PayloadHdrLength;
            mtxwMemCopy(pFragmentData->pData, &pInputData->pData[ulOffset], rtpPayloadContentLength);
            it->FragmentDataList.push(pFragmentData);

            MTXW_LOGV("the framelist push a pFragmentData, the framelist timeStamp = %d,  the pFragmentData.seq = %d", 
                                    it->ulTimStamp, pFragmentData->header.h264_fragment_header.SN);
            MTXW_LOGV("ReceiveH264RtpPacket: rtpPayloadContentLength=%d\n", rtpPayloadContentLength);
              
            // -----1:������װH264֡ʱ��NALU��Header�ĳ��ȣ�4:����startCode�ĳ���
            it->ulLengthOfFragmentData += 1 + rtpPayloadContentLength+4;
            
        }

        while(true)
        {
            DataBuffer.header.h264_fragment_header.SN = it->ulExpectSN;
            if(!(it->FragmentDataList.iscontain(&DataBuffer)))
            {
                break;
            }
            else
            {
                it->ulExpectSN++;
            }
        }
        /*--�ж��Ƿ�Ϊ����֡
        ͬʱ������������������
        1)֡�б�����һ��RTP��Я����MARK=TRUE��
        2)expectSN�������һ��RTP����SN+1
        3)��һ��RTPΪ������NALU����start=1��FU��Ƭ
        */
        std::list<MTXW_DATA_BUFFER_POINTER>::iterator itFragmentbegin =  it->FragmentDataList.begin();
        std::list<MTXW_DATA_BUFFER_POINTER>::iterator itFragmentend =  it->FragmentDataList.end();
		itFragmentend--;

        if((*itFragmentend)->header.h264_fragment_header.bMark == true 
            && (it->ulExpectSN == ((*itFragmentend)->header.h264_fragment_header.SN+1)) 
            && IsCompletNaluOrStart1Fu(*itFragmentbegin))
        {
            it->bCompleted = true;

            MTXW_LOGI("the framelist is Complete, framelist timeStamp =%d, rtpPayloadContentLength =%d\n", it->ulTimStamp, it->ulLengthOfFragmentData);

        }
                    
    }            

    //---2---- --�ж�h264RcvBuffer�б�ͷ��֡�Ƿ������������������װh264֡�ŵ����Ŷ��У�Ȼ��ɾ����Ӧ���ڴ�
    
    if(0 != UpdateFrameToPlayQueue())
    {
        MTXW_LOGI("MTXW_VideoInstance::ReceiveH264RtpPacket->UpdateFrameToPlayQueue Put HeadFramList to PlayQueue fail!!!!!");
        return -1;
    }
    
        
    return 0;
    
}


INT32 MTXW_VideoInstance::Decode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData)
{
        if(XwFfmpegDecodeH264Frame(pInData->pData,pInData->uSize,pOutData->pData)<0)
        {
            return -1;
        }else
        {
            pOutData->uSize = pInData->header.h264_data_header.uWidth*pInData->header.h264_data_header.uHeight*3/2;
            return 0;
        }
}

INT32 MTXW_VideoInstance::Play(MTXW_DATA_BUFFER_STRU *pH264Data)
{
    //MTXW_LOGI("MTXW_VideoInstance::Play()   this->playYuvflag=%d ulRcvFrameWidth=%d ",this->playYuvflag,this->ulRcvFrameWidth);
    //-----����YUV����-------------
    if(this->playYuvflag>0)
    {
        if(this->ulRcvFrameWidth==0)
        {
            //ͨ������������ж��Ƿ��Ѿ����յ���SPS��PPS֡
            //��Ϊֻ���յ���SPS֡�����ܻ���������
            //�����û��SPS֡PPS֡�Ļ������޷����� �ģ�ֱ�Ӷ���������
            return 0;
        }
        int yuvsize = pH264Data->header.h264_data_header.uWidth*pH264Data->header.h264_data_header.uHeight*3/2;

        if(0!=m_pDecodeBuffer)
        {
           if(m_pDecodeBuffer->uSize<yuvsize)
           {
               mtxwMemFree(this->m_pDecodeBuffer);
               this->m_pDecodeBuffer = 0;
           }
        }

        if(0==m_pDecodeBuffer)
        {
           m_pDecodeBuffer = (MTXW_DATA_BUFFER_STRU*) mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+yuvsize);
           if(m_pDecodeBuffer)
           {
               m_pDecodeBuffer->uSize=yuvsize;
           }
        }

        if(m_pDecodeBuffer)
        {
           int rslt=0;
           rslt = Decode(pH264Data,m_pDecodeBuffer);

           if(0==rslt)
           {
               if(0 != mPlayBuffer.pBuffer && mPlayBuffer.ulSize >= yuvsize)
               {
                   mtxwMemCopy(mPlayBuffer.pBuffer, m_pDecodeBuffer->pData, yuvsize);
                   MTXW_CB::PlayVideoYUV(this->GetInstance(),
                                                       0,
                                                       yuvsize,
                                                       pH264Data->header.h264_data_header.uRotatedAngle,
                                                       pH264Data->header.h264_data_header.uWidth,
                                                       pH264Data->header.h264_data_header.uHeight,
                                                       pH264Data->header.h264_data_header.ulSsrc);
               }
               /*
               MTXW_CB::PlayVideoYUV(this->GetInstance(),
                                                       m_pDecodeBuffer->pData,
                                                       yuvsize,
                                                       pH264Data->header.h264_data_header.uRotatedAngle,
                                                       pH264Data->header.h264_data_header.uWidth,
                                                       pH264Data->header.h264_data_header.uHeight,
                                                       pH264Data->header.h264_data_header.ulSsrc);
                                                       */
           }
        } 
    }
    else
    {
        //---����H264֡����
        if(0 != mPlayBuffer.pBuffer && mPlayBuffer.ulSize >= pH264Data->uSize)
        {
               mtxwMemCopy(mPlayBuffer.pBuffer, pH264Data->pData, pH264Data->uSize);
               MTXW_CB::PlayVideoFrameData(this->GetInstance(),
                                                       0,
                                                       pH264Data->uSize,
                                                       pH264Data->header.h264_data_header.uRotatedAngle,
                                                       pH264Data->header.h264_data_header.uWidth,
                                                       pH264Data->header.h264_data_header.uHeight,
                                                       pH264Data->header.h264_data_header.ulSsrc);
        }
        /*
        MTXW_CB::PlayVideoFrameData(this->GetInstance(),
                                                       pH264Data->pData,
                                                       pH264Data->uSize,
                                                       pH264Data->header.h264_data_header.uRotatedAngle,
                                                       pH264Data->header.h264_data_header.uWidth,
                                                       pH264Data->header.h264_data_header.uHeight,
                                                       pH264Data->header.h264_data_header.ulSsrc);*/
    }

    return 0;

}


