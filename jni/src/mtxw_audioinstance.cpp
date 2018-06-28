
#include "mtxw_audioinstance.h"
#include "mtxw_amrrtp.h"
#include "mtxw_amrcodec.h"
#include "mtxw_g729codec.h"
#include "mtxw_controlblock.h"
#include "vector"

#define BYTE_SPEACH_BLOCK_LEN   320
#define SHORT_SPEACH_BLOCK_LEN   (BYTE_SPEACH_BLOCK_LEN/2)
#define BYTE_G729_BLOCK_LEN  20

extern int g_MTXWSpeechEnLibDTime;


/***********************************************************/
/*                   libSpeechEngineEnhance.so                                        */
/***********************************************************/

static bool MTXW_IsStartLibSpeech= false;

extern bool g_MTXWActiveSpeechLib;

#ifndef MTXW_NASE
//speech engine 
extern bool CreateEngine(void);  //create the engine

extern void InitEngine(void);    //initialize the engine

extern void SetSampleRate(int nSample, int nDeviceSamle);  //set the sampling rate

extern void SetExtraDelayTime(int nExtraDelay);  //set the extra delya time


/*
一次处理10ms的数据，8K的采样率，就是一次处理80个数据。

上一次说的需要修改的第三点： Nearprocess()接口中最后一个参数是数据长度，

每次传80长度，160的话请分2次调用。

*/
extern bool NearProcess(short *nInData, short *nOutDdata, int nLength);  //process the near-end speech(recording-end), return value: 0: have not speech, 1: have speech

extern void FarProcess(short *nData, int nLength); //process the far-end speech(playing-end)

extern void DesdroyEngine(void);  //destroy the engine

#else
bool CreateEngine(void){return false;}
void InitEngine(void){return;}
void SetSampleRate(int nSample, int nDeviceSamle){return;}
void SetExtraDelayTime(int nExtraDelay){return;}

bool NearProcess(short *nInData, short *nOutDdata, int nLength){return true;}

void FarProcess(short *nData, int nLength){return;}

void DesdroyEngine(void){return;}

#endif
/***********************************************************/
/*                   libSpeechEngineEnhance.so                                        */
/***********************************************************/


MTXW_AudioInstance::MTXW_AudioInstance(INT32 uInstanceId,INT32 callId):MTXW_Instance(true,uInstanceId,callId)
{
	MTXW_FUNC_TRACE();
    this->enCodecType = MTXW_AUDIO_AMR; //音频编码类型
    this->sampleRate=8000;//每秒采样8000次
    this->frameTime=20; //20ms
    this->enAlignMode = MTXW_AMR_BE;//节省带宽
    this->bitRate = 12200;//AMR编码码率12.2k
    this->usLastRcvSeqNum = -1;
    this->ulRcvSSRC = 0;
    this->bFirstRcvPackt = false;
    this->usRcvWnd = 0xF500;
     	
    pRtpObj = 0;
    pAmrcodec = 0;
    pG729codec = 0;
}
MTXW_AudioInstance::~MTXW_AudioInstance()
{
        MTXW_FUNC_TRACE();
        
        if(pRtpObj)
        {
            delete pRtpObj;
            pRtpObj = 0;
        }

        if(pAmrcodec)
        {
            delete pAmrcodec;	
            pAmrcodec = 0;
        }

        if(pG729codec)
        {
            delete pG729codec;
            pG729codec = 0;
        }
}
INT32 MTXW_AudioInstance::SetAudioCodecType(MTXW_ENUM_AUDIO_CODEC_TYPE type)
{
    MTXW_FUNC_TRACE();
    if(IsRunning())
    {
        MTXW_LOGE("SetAudioCodecType() failed ,because instance:%d is running now!",GetInstance());
        return -1;
    }

    this->enCodecType= type;
    return 0;
}
INT32 MTXW_AudioInstance::SetAudioSampleRate(UINT32 sampleRate)
{
    MTXW_FUNC_TRACE();
    if(IsRunning())
    {
        MTXW_LOGE("SetAudioSampleRate() failed ,because instance is running now!");
        return -1;
    }

    this->sampleRate= sampleRate;
    return 0;
}
INT32 MTXW_AudioInstance::SetG729Param(UINT32 frameTime)
{
    MTXW_FUNC_TRACE();
    if(IsRunning())
    {
        MTXW_LOGE("SetG729Param() failed ,because instance is running now!");
        return -1;
    }

    this->frameTime = frameTime;

    return 0;
}
INT32 MTXW_AudioInstance::SetAmrParam(UINT32 bitRate,
               UINT32 frameTime,
               MTXW_ENUM_AMR_ALIGN_MODE enAlignMode)
{
    MTXW_FUNC_TRACE();
    if(IsRunning())
    {
        MTXW_LOGE("SetAmrParam() failed ,because instance is running now!");
        return -1;
    }

    this->bitRate = bitRate;//取值如12200(12.2k) 、4750(4.75k)
    this->frameTime = frameTime;
    this->enAlignMode = enAlignMode;
    return 0;				   
}
INT32 MTXW_AudioInstance::Send(UINT8 *pPCMData,UINT32 uLength)
{
    MTXW_FUNC_TRACE();

    MTXW_LOGD("MTXW_AudioInstance::Send length:%d",uLength);

    //MTXW_queue<MTXW_DATA_BUFFER_STRU*>& sendQueue = GetSendQueue();
    if(!this->IsRunning())
    {
        MTXW_LOGE("MTXW_AudioInstance::Send sendthread is not running");
        return -1;
    }

    if(MTXW_DIRECTION_RECEIVE_ONLY == this->GetDirection())
    {
        MTXW_LOGW("MTXW_AudioInstance::Send() Receive only, forbidden sending");
        return -1;
    }
    
    if(pPCMData==NULLPTR)
    {
        MTXW_LOGE("MTXW_AudioInstance::Send pPCMData is Null");
        return -1;
    }

    //--判断数据长度是否合法:  数据长度=(采样率*帧间隔/1000)*每个采样数据的长度
    //uLength = 0是静音包
    if((uLength != (this->frameTime*this->sampleRate/1000)*2 ) && (uLength!= 0))
    {
        MTXW_LOGE("MTXW_AudioInstance::Send length:%d != %d",uLength,(this->frameTime*this->sampleRate/1000)*2);
        return -1;
    }
    MTXW_DATA_BUFFER_STRU* pBuffer = (MTXW_DATA_BUFFER_STRU*)mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+uLength);
    if(!pBuffer)
    {
        MTXW_LOGE("MTXW_AudioInstance::Send mtxwMemAlloc fail");
        return -1;
    }

    //*******************静音包处理***********************
    if(uLength == 0)
    {
        //数据进入发送队列
        pBuffer->uSize = uLength;
        mtxwMemCopy(pBuffer->pData, pPCMData, uLength);

        {
            MTXW_lock   lock(mSendQueue.getMutex(),"MTXW_AudioInstance::Send");
            mSendQueue.push(pBuffer);
            mSendQueue.signal();
        }
        return 0;
    }
    //*******************静音包处理************************
    if(MTXW_IsStartLibSpeech)
    {
        int processlength = ((this->sampleRate)/1000)*10;

        for(int i = 0;i<((this->frameTime)/10);i++)
        {
         //NearProcess function only process 10ms frame one time;
         NearProcess((short*)pPCMData+(processlength*i),
                             (short*)pPCMData+(processlength*i),
                             processlength);    
        }
    }
    //--现对发送队列上锁，保证数据线程同步------------

    //数据进入发送队列
    pBuffer->uSize = uLength;
    mtxwMemCopy(pBuffer->pData, pPCMData, uLength);

    {
        MTXW_lock   lock(mSendQueue.getMutex(),"MTXW_AudioInstance::Send");
        mSendQueue.push(pBuffer);
        mSendQueue.signal();
    }

    return 0;



}
INT32 MTXW_AudioInstance::Start()
{
    MTXW_FUNC_TRACE();

    if(IsRunning())
    {
        MTXW_LOGE("\n MTXW_AudioInstance is already started!");
        return MTXW_Instance::Start();
    }

    MTXW_LOGI("\n this->enCodecType:%d",this->enCodecType);
    this->bFirstRcvPackt = false;

    this->mJitter.setFrequency(this->sampleRate);
    
    switch(this->enCodecType)
    {
        case MTXW_AUDIO_AMR:
        {
            this->pRtpObj = new (std::nothrow) MTXW_AmrRtp();
            if(!this->pRtpObj)
            {
                //创建实例失败
                return -1;
            }
            this->pAmrcodec= new (std::nothrow) Amr_Codec();
            if(!this->pAmrcodec)
            {
                //创建实例失败
                
                delete this->pRtpObj;
                this->pRtpObj = 0;
				
				
				
                return -1;
            }

            Mode mode = GetMode();
            MTXW_AmrRtp *pAmrRtp = dynamic_cast<MTXW_AmrRtp *>(this->pRtpObj);            

            if(pAmrRtp)
            {
                pAmrRtp->SetAMR_ALIGN_MODE(this->enAlignMode);
                pAmrRtp->SetAMR_BitRateMode(mode);
            }
            
            break;
        }
        case MTXW_AUDIO_G729:
        {
            this->pRtpObj = new (std::nothrow) MTXW_G729Rtp();
            if(!this->pRtpObj)
            {
                //创建实例失败
                return -1;
            }
            
            this->pG729codec = new (std::nothrow) G729_Codec();
            if(!this->pG729codec)
            {
                //创建实例失败

                delete this->pRtpObj;
                this->pRtpObj = 0;
                
                return -1;
            }
            
            break;
        }
        default:
        {
            //invalid type
            return -1;
        } 

    }
    
    MTXW_StartSpeechEnhanceLib();

     //--最后一定要调用父类的Start启动各种线程
     return  MTXW_Instance::Start();

     
}


INT32 MTXW_AudioInstance::Stop()
{
    MTXW_FUNC_TRACE();

      //--调用父类的Stop停止各种线程,只有接收发送线程停止后，才能释放pRtpObj和编码器
    MTXW_Instance::Stop();
    if(MTXW_IsStartLibSpeech)
    {
        DesdroyEngine();
    }
    MTXW_IsStartLibSpeech = false;
    
    if(this->pRtpObj)
    {
        delete this->pRtpObj;
        this->pRtpObj = 0;
    }

    if(pAmrcodec)
    {
        delete pAmrcodec;
        pAmrcodec = 0;
    }

    if(pG729codec)
    {
    	delete pG729codec;
    	pG729codec = 0;
    }


     return  0;
}

//codec编码，pInData-入参，内容为原始采样数据(如PCM)  pOutData-出参，内容为编码后的输出(如AMR)，调用者要负责buffer足够大
INT32 MTXW_AudioInstance::Encode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData)
{
    MTXW_FUNC_TRACE();
    int payloadLen = 0;
    Mode mode;
    
    if((NULLPTR == pInData) || (NULLPTR == pOutData))
    {
        MTXW_LOGE("MTXW_AudioInstance::Encode pInData or pOutData is NULL");
        return -1;
    }

    //静音包处理
    if(pInData->uSize == 0)
    {
        MTXW_LOGI("MTXW_AudioInstance,Encode send mute packet ");
        return 0;
    }

    if(MTXW_AUDIO_AMR == this->enCodecType)
    {
               
	mode = GetMode();
	if(mode>= N_MODES)
	{
	    MTXW_LOGE("AMR_ENCODE the BitRate is Not Standar");
	    return -1;
	}

      if(NULL == pAmrcodec)
      {
             MTXW_LOGE("MTXW_AudioInstance::Encode pAmrcodec == NULL");
             return -1;
      }
	
	payloadLen = pAmrcodec->Encode(mode, (short *)&pInData->pData[0], pOutData->pData, 0);

       MTXW_LOGV("MTXW_AudioInstance::Encode payloadLen %d ,mode:%d",payloadLen,mode);

	if(payloadLen<=0)
	{
	    pOutData->uSize = 0;
	    MTXW_LOGE("MTXW_AudioInstance::Encode payloadLen Error!which is%d",payloadLen);
	    return -1;
	}
	pOutData->uSize = payloadLen;					
		
	
	return payloadLen;
    	
    }
    else if(this->enCodecType == MTXW_AUDIO_G729)
    {
    	payloadLen = pG729codec->encode((short *)&pInData->pData[0], pOutData->pData, SHORT_SPEACH_BLOCK_LEN);

    	MTXW_LOGV("MTXW_AudioInstance::Encode payloadLen %d ",payloadLen);
    	
    	if(payloadLen<=0)
    	{
    		pOutData->uSize = 0;
    		MTXW_LOGE("MTXW_AudioInstance::Encode payloadLen Error!which is%d",payloadLen);
    		return -1;
    	}
    	pOutData->uSize = payloadLen;
    	
    }
    else
    {
       MTXW_LOGE("MTXW_AudioInstance::Encode enCodecType Error!which is %d",this->enCodecType);
    	return -1;
    }
    
    
    
    return payloadLen;
}

//decode解码，pInData-入参，内容为编码后的数据(如AMR)  pOutData-出参，内容为解码后的输出(如PCM)，调用者要确保足够大的buffer
INT32 MTXW_AudioInstance::Decode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData)
{
    MTXW_FUNC_TRACE();
    return 0;
}



/*打包函数，输入是未打包的数据，输出是打包后的数据，函数调用者负责释放vecPackedData里的内存
   pPackedData->pData的数据的数据格式为: |-len0-|--pack0--|-len1-|--pack1--|-len2-|--pack2--|......
       lenX的长度为2字节，主机字节序

*/
INT32 MTXW_AudioInstance::Pack(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_DATA_BUFFER_STRU* pPackedData)
{
	MTXW_FUNC_TRACE();
    	INT32 length = 0;
    	INT32 lengthOfAmr = 0;
       INT32 lengthOfG729 = 0;
       INT32 lengthOfRtpHead = 0;

    if((NULLPTR == pInputData) || (NULLPTR == pPackedData))
    {
        MTXW_LOGE("MTXW_AudioInstance::Pack pInputData or pPackedData is NULL");
        return -1;
    }

    //静音包处理
    if(pInputData->uSize == 0)
    {
        MTXW_LOGI("MTXW_AudioInstance,Pack send mute packet ");
        length = 0;
        mtxwMemCopy(&pPackedData->pData[0], &length, 2);
        pPackedData->uSize = length+2;
        return 0;
    }

	if(MTXW_AUDIO_AMR == this->enCodecType)
	{
	       
		MTXW_AmrRtp *pAmrRtp = dynamic_cast<MTXW_AmrRtp *>(this->pRtpObj);
		if(pAmrRtp)
		{
		   lengthOfAmr =  pAmrRtp->PackRtpPayload(pInputData->pData, pInputData->uSize, &pPackedData->pData[RTP_HEAD_LENGTH+2]);		   
		}

               if(this->pRtpObj)
               {
    	            this->pRtpObj->SetbitMaker(true);
	
	            this->pRtpObj->SetTimeStamp(this->pRtpObj->timeStamp+160);
	
	            lengthOfRtpHead = this->pRtpObj->PackRtpHeader(&pPackedData->pData[2], this->pRtpObj->timeStamp, this->pRtpObj->bitMaker);

                   //--增加padding
	            pPackedData->pData[2] |= 0x20;
	            pPackedData->pData[2+lengthOfRtpHead+lengthOfAmr] = 0x01;
	             
	            length = lengthOfRtpHead+lengthOfAmr+1;
	            

	            mtxwMemCopy(&pPackedData->pData[0], &length, 2);
	            pPackedData->uSize = length+2;
               }

		
	}
	else if(MTXW_AUDIO_G729 == this->enCodecType)
	{

		if(this->pRtpObj)
		{
		       lengthOfG729 = pInputData->uSize;
		       mtxwMemCopy(&pPackedData->pData[RTP_HEAD_LENGTH+2], pInputData->pData, lengthOfG729);
		       
			this->pRtpObj->SetbitMaker(true);
			this->pRtpObj->SetTimeStamp(this->pRtpObj->timeStamp+160);	
			
			lengthOfRtpHead = this->pRtpObj->PackRtpHeader(&pPackedData->pData[2], this->pRtpObj->timeStamp, this->pRtpObj->bitMaker);

                    //--增加padding
                    pPackedData->pData[2] |= 0x20;
                    pPackedData->pData[2+lengthOfRtpHead+lengthOfG729] = 0x01;
	            
			length = lengthOfRtpHead + lengthOfG729+1;
			
			mtxwMemCopy(&pPackedData->pData[0], &length, 2);
			
			pPackedData->uSize = length+2;
		}
	}
	else
	{
	       MTXW_LOGE("MTXW_AudioInstance::Pack this->enCodecType is Unknow Value!");
		return -1;
	}

	return length;
}

/*
接收到的RTP包处理函数，

输入是打包后的数据，解包后的数据输出到播放队列vecPlayQueue，供播放线程使用。

*/
INT32 MTXW_AudioInstance::ReceiveRtpPacket(MTXW_DATA_BUFFER_STRU* pInputData)
{
    MTXW_FUNC_TRACE();
    UINT32 ulRslt =0;
    UINT32 RtpHeadLength = 0;

    UINT16 seq = 0;
    UINT32 ssrc = 0;
    UINT32 gap = 0;
    
    if(!this->pRtpObj)
    {
        MTXW_LOGE("MTXW_AudioInstance::ReceiveRtpPacket this->pRtpObj == null!");
        return -1;
        
    }

    MTXW_Rtp rcvRtpPact;
    RtpHeadLength = this->pRtpObj->UnpackRtpHeader(pInputData, &rcvRtpPact);
    
    if(RtpHeadLength <= 0)
    {
        MTXW_LOGE("MTXW_AudioInstance::ReceiveRtpPacket RtpHeadLength<=0!");
        return -1;
    }
    if(rcvRtpPact.GetVersion()!=2 || rcvRtpPact.GetPayloadType()==0)
    {
        MTXW_LOGE("MTXW_AudioInstance::ReceiveRtpPacket invalid version(%d) or payload-type(%d)",rcvRtpPact.GetVersion(),rcvRtpPact.GetPayloadType());
        return -1;
    }
    //临时增加定位打印
     MTXW_LOGD("MTXW_AudioInstance::ReceiveRtpPacket SN = %d",rcvRtpPact.GetSeq());
    seq = rcvRtpPact.GetSeq();
    ssrc = rcvRtpPact.GetSSRC();

    if(this->bFirstRcvPackt==false)
    {
        // do somthing
        SetLastRcvSeqNum(seq -1);

        SetRcvSSRC(ssrc);

    }
   
    this->bFirstRcvPackt = true;

    if(GetRcvSSRC()!=ssrc)
    {
         SetLastRcvSeqNum( seq -1);
         SetRcvSSRC(ssrc);
    }

    //判断数据包是否在窗内
    if(GetLastRcvSeqNum() == seq)
    {
        MTXW_LOGI("\n the Receive RtpPacket SN is Same with LastPacket SN ,Drop");
        return -1;
    }
    
    if(GetLastRcvSeqNum() > seq)
    {        
        gap = seq + 0xFFFF - GetLastRcvSeqNum();       
    }
    else
    {
        gap = seq - GetLastRcvSeqNum();
    }
    
    /*
    if(gap>this->GetRcvWnd())
    {
        MTXW_LOGI("\n the Receive RtpPacket SN is Not In Window ,Drop");
        return -1;
    }
    */
    
    this->SetLastRcvSeqNum(seq);


    //---2018.01.16 增加jitter处理
    this->mJitter.onRecv(rcvRtpPact.timeStamp);
    MTXW_LOGI(" audio jitter = %d meanJitter = %d",this->mJitter.getJitter(),this->mJitter.getMeanJitter());

    switch(this->enCodecType)
    {
        case MTXW_AUDIO_AMR:
        {
            //判断AMR的长度合法性，判断payloadType的合法性,协议规定最大能带4个AMR包
            if(((pInputData->uSize-RtpHeadLength)> 11 && (pInputData->uSize-RtpHeadLength)<= 32*4) && (rcvRtpPact.payloadType >=96 && rcvRtpPact.payloadType<=127))
            {
                ulRslt=ReceiveAmrRtpPacket(pInputData, RtpHeadLength);
                this->ulRcvFrameNum ++;
            }
            else
            {
                MTXW_LOGE("the AUDIO MODE is AMR ,But the RtpPayLoadLength is invalid , RtpPayLoadLength=%d, rcvRtpPact.payloadType=%d", pInputData->uSize-RtpHeadLength, rcvRtpPact.payloadType);
                return -1;

            }
            break;
        }

        case MTXW_AUDIO_G729:
        {
            //判断G729的长度合法性，判断G729 payloadType的合法性
            if(((pInputData->uSize-RtpHeadLength)!= BYTE_G729_BLOCK_LEN) && (rcvRtpPact.payloadType != 18))
            {
                MTXW_LOGE("the AUDIO MODE is G729 ,But the RtpPayLoadLength is not 20 Or rcvRtpPact.payloadType is Not 18, RtpPayLoadLength=%d, rcvRtpPact.payloadType=%d", pInputData->uSize-RtpHeadLength, rcvRtpPact.payloadType);
                return -1;
            }
            ulRslt=ReceiveG729RtpPacket(pInputData, RtpHeadLength);
            this->ulRcvFrameNum ++;
            break;
        }
        default:
        {
            return -1;
        }
    }


    return ulRslt;

    
}

extern AMR_DECRIPTION gRtpAmrDscrpt[];
INT32 MTXW_AudioInstance::ReceiveAmrRtpPacket(MTXW_DATA_BUFFER_STRU* pInputData, UINT32 RtpHeadLength)
{
	MTXW_FUNC_TRACE();
	UINT8 ucToc = 0;
	UINT8 ft = 0;
	UINT8 q = 0;
	UINT8 f = 1;
	UINT16 Toc_length = 0;
	INT32 tocsize = 0;
	UINT32 i =0;
	UINT8 frame_len = 0;
	MTXW_AMR_FRAME_STRU AmrFrame;
		
	//--1-RTP解包--------------
	if(NULLPTR == pInputData)
	{
	       MTXW_LOGE("MTXW_AudioInstance::ReceiveAmrRtpPacket pInputData is NULL!");
	   	return -1;
	}
	
        
	MTXW_AmrRtp *pAmrRtp = dynamic_cast<MTXW_AmrRtp *>(this->pRtpObj);
	if(pAmrRtp)
	{
	    	if(this->enAlignMode == MTXW_AMR_BE)
		{
			if(0 != pAmrRtp->UnpackRtpPayload_BE(&pInputData->pData[RtpHeadLength],  (pInputData->uSize - RtpHeadLength),  &AmrFrame))
			{
				//decode error!!
				MTXW_LOGE("the RTP data stream received is decoded error, please check the data is standard bandwith efficent format?");
				
				return -1;
			}
			
			Toc_length = 0;
			tocsize = AmrFrame.toc_table.size();
			for(i = 0; i<tocsize && i<AmrFrame.frame_block_table.size(); i++)
			{
				//ALOGD("decode AmrFrame FT : %d", AmrFrame.toc_table[i].FT);
				if(AmrFrame.toc_table[i].FT > 7)
				{
					//ALOGD("decode AmrFrame FT : %d > 7", AmrFrame.toc_table[i].FT);
					continue;
				}
				
				/*防溢出*/
				if(160 * (Toc_length+1) > 1024)
				{
					break;
				}

				/*Decode the packer*/
				frame_len = (gRtpAmrDscrpt[AmrFrame.toc_table[i].FT].Total_number_of_bits+7)>>3;
				/*组装成TOC+Content的形式*/
				mtxwMemMove(AmrFrame.frame_block_table[i].data + 1, AmrFrame.frame_block_table[i].data, frame_len);
				AmrFrame.frame_block_table[i].data[0] = (AmrFrame.toc_table[i].F << 7 ) | (AmrFrame.toc_table[i].FT << 3) | (AmrFrame.toc_table[i].Q << 2);
				//ALOGD("AmrFrame F : %d  FT : %d Q : %d", AmrFrame.toc_table[i].F, AmrFrame.toc_table[i].FT, AmrFrame.toc_table[i].Q);
				//ALOGD("AmrFrame toc : %d", AmrFrame.frame_block_table[i].data[0]);			
				MTXW_DATA_BUFFER_STRU* pBuffer = (MTXW_DATA_BUFFER_STRU*)mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+BYTE_SPEACH_BLOCK_LEN);
               
				if(!pBuffer)
				{
				       MTXW_LOGE("MTXW_AudioInstance::ReceiveAmrRtpPacket pBuffer is NULL!");
					return -1;
				}
				pAmrcodec->Decode(AmrFrame.frame_block_table[i].data,  (short*)&pBuffer->pData[0], 0);
				pBuffer->uSize = BYTE_SPEACH_BLOCK_LEN;

                            /*if(MTXW_IsStartLibSpeech)
                            {
                            	int processlength = 0;//((this->sampleRate)/1000)*10;
                            	while(processlength<(BYTE_SPEACH_BLOCK_LEN/2))
                            	{
                            	     //FarProcess function only process 10ms frame one time;
                            	    FarProcess((short *)&pBuffer->pData[0+(processlength*2)],((this->sampleRate)/1000)*10);
                            	    processlength +=((this->sampleRate)/1000)*10;
                            	}
                            }*/
				 MTXW_queue<MTXW_DATA_BUFFER_STRU*>& playQueue = GetPlayQueue();
				 {
				     MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_AudioInstance::ReceiveAmrRtpPacket:playQueue");
				     if(playQueue.size() >= GetPlayQueueLengthMax())
				     {
                                        MTXW_LOGE("MTXW_AudioInstance::ReceiveAmrRtpPacket playQueue is full!! discard this frame ");
                                        mtxwMemFree(pBuffer);
                                        return -1;
				     }
				     playQueue.push(pBuffer);
				 }
	
				
				Toc_length++;
				
			
			}
		}
		else 
		{
			Toc_length = 0;


			if(0!= pAmrRtp->UnpackRtpPayload_OA(&pInputData->pData[RtpHeadLength],  (pInputData->uSize - RtpHeadLength),  &AmrFrame))
			{
				//decode error!!
				MTXW_LOGE("the RTP data stream received is decoded error, please check the data is standard octect algined ?");
				//goto END;
				return -1;
			}
			
			tocsize = AmrFrame.toc_table.size();

			for(i = 0; i < tocsize && i<AmrFrame.frame_block_table.size();i++)
			{

				//ALOGD("decode AmrFrame FT : %d i : %d tocsize : %d blocksize : %d Toc_length : %d", AmrFrame.toc_table[i].FT, i, tocsize, AmrFrame.frame_block_table.size(),Toc_length);


				if(AmrFrame.toc_table[i].FT > 7)
				{

					MTXW_LOGW("decode AmrFrame FT : %d > 7", AmrFrame.toc_table[i].FT);

					continue;
				}

				/*防溢出--临时*/
				if(160 * (Toc_length+1)>1024)
				{
					break;
				}

				/* Decode the packet */
				frame_len = (gRtpAmrDscrpt[AmrFrame.toc_table[i].FT].Total_number_of_bits+7) >> 3;

				mtxwMemMove(AmrFrame.frame_block_table[i].data + 1,AmrFrame.frame_block_table[i].data,frame_len);
				AmrFrame.frame_block_table[i].data[0] = AmrFrame.toc_table[i].F << 7 |
				AmrFrame.toc_table[i].FT << 3 | AmrFrame.toc_table[i].Q << 2;							
				MTXW_DATA_BUFFER_STRU* pBuffer = (MTXW_DATA_BUFFER_STRU*)mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+BYTE_SPEACH_BLOCK_LEN);
               
				if(!pBuffer)
				{
					return -1;
				}
				pAmrcodec->Decode(AmrFrame.frame_block_table[i].data,  (short*)&pBuffer->pData[0], 0);
				pBuffer->uSize = BYTE_SPEACH_BLOCK_LEN;

                            if(MTXW_IsStartLibSpeech)
                            {
    				       int processlength = 0;//((this->sampleRate)/1000)*10;
                            	while(processlength<(BYTE_SPEACH_BLOCK_LEN/2))
                            	{
                            	     //FarProcess function only process 10ms frame one time;
                            	    FarProcess((short *)&pBuffer->pData[0+(processlength*2)],((this->sampleRate)/1000)*10);
                            	    processlength +=((this->sampleRate)/1000)*10;
                            	}
                        	}
				MTXW_queue<MTXW_DATA_BUFFER_STRU*>& playQueue = GetPlayQueue();
				{
				     MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_AudioInstance::ReceiveAmrRtpPacket:playQueue");
     				     if(playQueue.size() >= GetPlayQueueLengthMax())
				     {
                                        MTXW_LOGE("MTXW_AudioInstance::ReceiveAmrRtpPacket playQueue is full!! discard this frame ");
                                        mtxwMemFree(pBuffer);
                                        return -1;
				     }
				     playQueue.push(pBuffer);
				}
				
				Toc_length++;
			}
		}
	}
	else
	{
		return -1;
	}

	//mtxwMemCopy(dest, pcm, size)

	
	    
	//--2-检查包的合法性--	


      //--3-decode amr to pcm, and put pcm to playbuffer---------
    

	return 0;
}

INT32 MTXW_AudioInstance::ReceiveG729RtpPacket(MTXW_DATA_BUFFER_STRU* pInputData, UINT32 RtpHeadLength)
{
	MTXW_FUNC_TRACE();
    	//--1-RTP解包--------------
       if(NULLPTR == pInputData)
	{
	    MTXW_LOGE("the Receive G729RtpPacket is NULLPTR");	        
	    return -1;
	}
	MTXW_DATA_BUFFER_STRU* pBuffer = (MTXW_DATA_BUFFER_STRU*)mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+BYTE_SPEACH_BLOCK_LEN);
   
	if(!pBuffer)
	{
	       MTXW_LOGE("ReceiveG729RtpPacket: mtxwMemAlloc fail");
		return -1;
	}
	UINT32 lengthOfG729 = pInputData->uSize - RtpHeadLength;
	INT32 lengthOfPcm = 0;

	/*
	    lengthOfPcm is the short Array pcm length, and pBuffer->uSize indicate the  pBuffer->pData length
	*/
	lengthOfPcm = pG729codec->decode(&pInputData->pData[RtpHeadLength], (short *)&pBuffer->pData[0], lengthOfG729);
	/*if(MTXW_IsStartLibSpeech)
      {
        	int processlength = 0;//((this->sampleRate)/1000)*10;
        	while(processlength<lengthOfPcm)
        	{
        	    //FarProcess function only process 10ms frame one time;
        	    FarProcess((short *)&pBuffer->pData[0+(processlength*2)],((this->sampleRate)/1000)*10);
        	    processlength +=((this->sampleRate)/1000)*10;
        	}
	}*/
	
	pBuffer->uSize = lengthOfPcm*2;
	MTXW_queue<MTXW_DATA_BUFFER_STRU *> &playQueue = GetPlayQueue();
	{            
            MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_AudioInstance::ReceiveG729RtpPacket:playQueue");
            if(playQueue.size() >= GetPlayQueueLengthMax())
            {
                MTXW_LOGE("MTXW_AudioInstance::ReceiveAmrRtpPacket playQueue is full!! discard this frame ");
                mtxwMemFree(pBuffer);
                return -1;
            }
		playQueue.push(pBuffer);
	}
	

    //--2-检查包的合法性--

    //--3-decode G729 to pcm, and put pcm to playbuffer---------

	return 0;
}

Mode MTXW_AudioInstance::GetMode()
{
    MTXW_FUNC_TRACE();
    Mode mode = MR122;
    switch(this->bitRate)
       {
            case 12200:
            {
                mode = MR122;
                break;   
            }            
            case 10200:
            {
                mode = MR102;
                break;   
            }
            case 7950:
            {
                mode = MR795;
                break;  
            }
            case 7400:
            {
                mode = MR74;
                break;  
            }
            case 6700:
            {
                mode = MR67;
                break;  
            }
            case 5900:
            {
                mode = MR59;
                break;  
            }
            case 5150:
            {
                mode = MR515;
                break;  
            }
            case 4750:
            {
                mode = MR475;
                break;  
            }
            default:
            {
                return N_MODES;
            }
            
       }

       return mode;
}


bool MTXW_AudioInstance::MTXW_StartSpeechEnhanceLib()
{
        MTXW_FUNC_TRACE();
            
        if(!g_MTXWActiveSpeechLib)
        {
            MTXW_LOGE("MTXW_StartSpeechEnhanceLib Lib not Active!");
            return 0;
        }
        
        if(MTXW_IsStartLibSpeech)
        {
            MTXW_LOGI("Engine has already started!");
            return MTXW_IsStartLibSpeech;
        }
        else
        {
            MTXW_IsStartLibSpeech = CreateEngine();
            InitEngine();
            SetSampleRate(this->sampleRate,this->sampleRate);
            MTXW_LOGI("MTXW_AudioInstance::MTXW_StartSpeechEnhanceLib g_MTXWSpeechEnLibDTime:%d",g_MTXWSpeechEnLibDTime);
            SetExtraDelayTime(g_MTXWSpeechEnLibDTime);
            MTXW_LOGA("MTXW_StartSpeechEnhanceLib g_MTXWSpeechEnLibDTime = %d",g_MTXWSpeechEnLibDTime);
            return MTXW_IsStartLibSpeech;
        }
}

/**************************************************************
*pData: pcm数据
****************************************************************/
INT32 MTXW_AudioInstance::Play(MTXW_DATA_BUFFER_STRU *pPlayData)
{  
    if(MTXW_IsStartLibSpeech)
   {
     int processlength = 0;//((this->sampleRate)/1000)*10;
     while(processlength<(BYTE_SPEACH_BLOCK_LEN/2))
              {
                //FarProcess function only process 10ms frame one time;
                FarProcess((short *)&pPlayData->pData[0+(processlength*2)],((this->sampleRate)/1000)*10);
                processlength +=((this->sampleRate)/1000)*10;
               }
    }
    //MTXW_LOGE("MTXW_AudioInstance::Play() mPlayBuffer.ulSize=%d pPlayData->uSize=%d ",mPlayBuffer.ulSize,pPlayData->uSize);
    if(0 != mPlayBuffer.pBuffer && mPlayBuffer.ulSize >= pPlayData->uSize)
    {
        mtxwMemCopy(mPlayBuffer.pBuffer, pPlayData->pData, pPlayData->uSize);
        MTXW_CB::PlayAudioPcmData(this->GetInstance(),0,pPlayData->uSize);
    }
    /*
    MTXW_CB::PlayAudioPcmData(this->GetInstance(),pPlayData->pData,pPlayData->uSize);
    */

    return 0;
}

