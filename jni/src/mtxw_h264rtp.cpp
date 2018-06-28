#include "mtxw_h264rtp.h"

#define gMTXWH264MTU 1400  

MTXW_H264Rtp::MTXW_H264Rtp()
{
        MTXW_FUNC_TRACE();
        this->payloadType = 98;
        this->ulH264FrameRate = 25;
        SPS.uLength = 0;
        PPS.uLength = 0;
}

MTXW_H264Rtp::~MTXW_H264Rtp()
{
        MTXW_FUNC_TRACE();
}

void MTXW_H264Rtp::SetH264FrameRate(UINT32 FrameRate)
{
    MTXW_FUNC_TRACE();
    this->ulH264FrameRate = FrameRate;
}
INT32 MTXW_H264Rtp::Pack_H264_Payload(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_DATA_BUFFER_STRU* pPackedData)
{
        MTXW_FUNC_TRACE();
        UINT32  StartCodeLength = 0;
        UINT32  ulInputDataCursor = 0;   //the cursor of input data.
        UINT32  ulPackedDataCursor = 0;  //the cursor of paked data.
        UINT32  Offset = 0; //
        UINT32  LengthOfNal = 0;        //the length of One NAL
        UINT32  PacketLength = 0;
        UINT32  ulRtpHeadLen = 0;     //length of Rtp Head. 
        UINT32 ulPayloadHeadLen = 0;  //length of payload head. 
        MTXW_NALU_HEADER_STRU    uNalU_head;
        MTXW_FU_INDICATOR_STRU    uFu_Indicator;
        MTXW_FU_HEADER_STRU    uFu_Head;

        UINT32 ulIndex = 0;

        //set the FrameRate of this Frame,each Frame has the same FrameRate. 
        this->SetTimeStamp(this->timeStamp+(90000/(this->ulH264FrameRate)));
        this->SetbitMaker(false);
            
        //get start code length ,which is always 3 bytes or 4 bytes.
        StartCodeLength = 0;
        this->GetNextNalLocation(&(pInputData->pData[0]), pInputData->uSize,&StartCodeLength);
        
        if(0 == StartCodeLength||((3!=StartCodeLength)&&(4!=StartCodeLength)))
        {
            MTXW_LOGE("MTXW_H264Rtp::Pack_H264_Payload. StartCodeLength:%d is not found or Error!",StartCodeLength);
            return -1;
        }
   
        ulInputDataCursor = StartCodeLength;

        //Get nalu value.
        uNalU_head.bitsType = (pInputData->pData[ulInputDataCursor])&0x1F;
        uNalU_head.bitsNRI = ((pInputData->pData[ulInputDataCursor])>>5)&0x03;
        uNalU_head.bitF  = ((pInputData->pData[ulInputDataCursor])>>7)&0x01;

        if(MTXW_NALU_TYPE_5_IDR == uNalU_head.bitsType)
        {
            if( (0!=SPS.uLength) &&(0!=PPS.uLength) )
            {
                ulPackedDataCursor += SendSPSorPPS(&pPackedData->pData[ulPackedDataCursor],
                                                                       SPS,
                                                                       this->timeStamp,
                                                                       this->bitMaker,
                                                                       pInputData->header.h264_data_header.uRotatedAngle);  
                                                                       
               ulPackedDataCursor += SendSPSorPPS(&pPackedData->pData[ulPackedDataCursor],
                                                           PPS,
                                                           this->timeStamp,
                                                           this->bitMaker,
                                                           pInputData->header.h264_data_header.uRotatedAngle);  
           }
        }         

         do
         {
            if(pInputData->uSize <= ulInputDataCursor)
            {
                MTXW_LOGE("pInputData->uSize:%d < pInputDataCursor:%d!",pInputData->uSize ,ulInputDataCursor);
                return -1;
            }

            //Get the next nal location ,step over the startcode.
            StartCodeLength = 0;
            Offset  = this->GetNextNalLocation(&(pInputData->pData[ulInputDataCursor]), pInputData->uSize - ulInputDataCursor,&StartCodeLength);

            
            if((-1)== Offset) //this is the last nal
            {
               LengthOfNal = pInputData->uSize - ulInputDataCursor;      
            }
            else
            {
                LengthOfNal = Offset -StartCodeLength ;
            }

            

            //if the nal length is small than mtu or equal mtu ,then send this nal directly
            if(LengthOfNal <= (gMTXWH264MTU-ulRtpHeadLen))
            {
                //Get nalu value.
                uNalU_head.bitsType = (pInputData->pData[ulInputDataCursor])&0x1F;
                uNalU_head.bitsNRI = ((pInputData->pData[ulInputDataCursor])>>5)&0x03;
                uNalU_head.bitF  = ((pInputData->pData[ulInputDataCursor])>>7)&0x01;
                
                
                if(MTXW_NALU_TYPE_7_SPS == uNalU_head.bitsType)
                {
                    SPS.uLength = LengthOfNal;
                    mtxwMemCopy(SPS.pData, &pInputData->pData[ulInputDataCursor], SPS.uLength);
                }
                else if(MTXW_NALU_TYPE_8_PPS == uNalU_head.bitsType)
                {
                    PPS.uLength = LengthOfNal;
                    mtxwMemCopy(PPS.pData, &pInputData->pData[ulInputDataCursor], PPS.uLength);
                }
              


                //--1--set rtp_head value and put into pPackedData->pData[ulPackedDataCursor+2]
                if((-1)== Offset) //this is the last nal
                {
                    this->SetbitMaker(true);
                }
                else
                {
                     this->SetbitMaker(false);
                }

    	         ulRtpHeadLen = this->PackH264RtpHeader(&pPackedData->pData[ulPackedDataCursor+2],
    	                                                                        this->timeStamp, 
    	                                                                        this->bitMaker,
    	                                                                        pInputData->header.h264_data_header.uRotatedAngle);
                //--修改padding标志位为true
                pPackedData->pData[ulPackedDataCursor+2] |= 0x20;
                 //--2--the first two bytes are the length of each paket.
                 
                PacketLength = LengthOfNal + ulRtpHeadLen;       //packet length = nal_len + rtp_head_len

                //-增加padding字段的长度
                PacketLength++;
     
                mtxwMemCopy(&pPackedData->pData[ulPackedDataCursor], &PacketLength, 2); 

                ulPackedDataCursor += ulRtpHeadLen+2;
    	                                                        
                //--3--set nal value and put into  pPackedData->pData[ulPackedDataCursor]
                
                mtxwMemCopy(&pPackedData->pData[ulPackedDataCursor], &pInputData->pData[ulInputDataCursor], LengthOfNal);
                                                 
                ulPackedDataCursor +=  LengthOfNal;

                //---最后增加padding字段
                pPackedData->pData[ulPackedDataCursor] = 0x01;
                ulPackedDataCursor +=1;
                
                
                ulInputDataCursor += LengthOfNal + StartCodeLength;  //need to skip the start code.

            }
            else
            { 
                //Get nalu value.
                uNalU_head.bitsType = (pInputData->pData[ulInputDataCursor])&0x1F;
                uNalU_head.bitsNRI = ((pInputData->pData[ulInputDataCursor])>>5)&0x03;
                uNalU_head.bitF  = ((pInputData->pData[ulInputDataCursor])>>7)&0x01;
                  
                uFu_Indicator.bitsType = 0x1C;   //FU-A TYPE:28
                uFu_Indicator.bitsNRI = uNalU_head.bitsNRI ;
                uFu_Indicator.bitF=  uNalU_head.bitF;

                uFu_Head.bitsNalu_type = uNalU_head.bitsType;
                uFu_Head.bitR= 0;
                uFu_Head.bitS= 0;
                uFu_Head.bitE = 0;

                ulPayloadHeadLen = 2;
                
                //the first byte is Nal unit Type
                ulInputDataCursor = ulInputDataCursor+1;

                LengthOfNal = LengthOfNal -1;

                ulIndex = 0;

                //If LengthOfNal + 2(uFu_Indicator+uFu_Head) + RTP_HEAD_LENGTH > MTU.The packet need to slice.
                while((LengthOfNal+ulPayloadHeadLen) > (gMTXWH264MTU-ulRtpHeadLen))   
                {
                    if(0 ==ulIndex)
                    {
                        uFu_Head.bitS= 1;  //set start bit
                    }
                    else
                    {
                        uFu_Head.bitS = 0;
                    }
                    
                     //--1--set rtp_head value and put into pPackedData->pData[ulPackedDataCursor+2]

                    ulRtpHeadLen = this->PackH264RtpHeader(&pPackedData->pData[ulPackedDataCursor+2],
                                                                        this->timeStamp, 
                                                                        this->bitMaker,
                                                                        pInputData->header.h264_data_header.uRotatedAngle);

                    //--修改padding标志位为true
                    pPackedData->pData[ulPackedDataCursor+2] |= 0x20;
                    
                     //--2--the first two bytes are the length of each paket.
                     
                    PacketLength = gMTXWH264MTU;      //packet length = nal_len + rtp_head_len

                    //-增加padding字段的长度
                    PacketLength++;

                    mtxwMemCopy(&pPackedData->pData[ulPackedDataCursor], &PacketLength, 2); 

                    ulPackedDataCursor += ulRtpHeadLen+2;


                    //--3--set nal value and put into  pPackedData->pData[ulPackedDataCursor]

                    pPackedData->pData[ulPackedDataCursor] = (uFu_Indicator.bitF<<7)|(uFu_Indicator.bitsNRI<<5)|(uFu_Indicator.bitsType);

                    ulPackedDataCursor +=1;

                    pPackedData->pData[ulPackedDataCursor] = (uFu_Head.bitS<<7)|(uFu_Head.bitE<<6)|(uFu_Head.bitR<<5)|(uFu_Head.bitsNalu_type);  

                    ulPackedDataCursor +=1;
                    
                    mtxwMemCopy(&pPackedData->pData[ulPackedDataCursor], 
                                            &pInputData->pData[ulInputDataCursor], 
                                            gMTXWH264MTU-ulRtpHeadLen-ulPayloadHeadLen);

                    ulPackedDataCursor +=  gMTXWH264MTU-ulRtpHeadLen -ulPayloadHeadLen;
                    //---最后增加padding字段
                    pPackedData->pData[ulPackedDataCursor] = 0x01;
                    ulPackedDataCursor +=1;

                    ulInputDataCursor += gMTXWH264MTU-ulRtpHeadLen -ulPayloadHeadLen;

                    LengthOfNal = LengthOfNal - (gMTXWH264MTU - ulRtpHeadLen -ulPayloadHeadLen);

                    ulIndex++;
                                               
                }
                    uFu_Head.bitS = 0;
                    
                    /* the rest of the data is the end of FU-A*/
                    uFu_Head.bitE = 1;

                    //--1--set rtp_head value and put into pPackedData->pData[ulPackedDataCursor+2]
                    if((-1)== Offset) //this is the last nal
                    {
                        this->SetbitMaker(true);
                    }
                    else
                    {
                        this->SetbitMaker(false);
                    }

                     ulRtpHeadLen = this->PackH264RtpHeader(&pPackedData->pData[ulPackedDataCursor+2],
                                                                                    this->timeStamp, 
                                                                                    this->bitMaker,
                                                                                    pInputData->header.h264_data_header.uRotatedAngle);
                    //--修改padding标志位为true
                    pPackedData->pData[ulPackedDataCursor+2] |= 0x20;
                    
                     //--2--the first two bytes are the length of each paket.
                     
                    PacketLength = LengthOfNal + ulRtpHeadLen + ulPayloadHeadLen;  //nal_len + rtp_head_len + uFu_Indicator +uFu_Head

                    //-增加padding字段的长度
                    PacketLength++;
                    
                    mtxwMemCopy(&pPackedData->pData[ulPackedDataCursor], &PacketLength, 2); 

                    ulPackedDataCursor += ulRtpHeadLen+2;
       

                    //--3--set nal value and put into  pPackedData->pData[ulPackedDataCursor]
                    
                    pPackedData->pData[ulPackedDataCursor] = (uFu_Indicator.bitF<<7)|(uFu_Indicator.bitsNRI<<5)|(uFu_Indicator.bitsType);

                    ulPackedDataCursor +=1;

                    pPackedData->pData[ulPackedDataCursor] = (uFu_Head.bitS<<7)|(uFu_Head.bitE<<6)|(uFu_Head.bitR<<5)|(uFu_Head.bitsNalu_type);  

                    ulPackedDataCursor +=1;
                    
                    mtxwMemCopy(&pPackedData->pData[ulPackedDataCursor], 
                                         &pInputData->pData[ulInputDataCursor], 
                                         LengthOfNal);                     

                    ulPackedDataCursor +=  LengthOfNal;
                     //---最后增加padding字段
                    pPackedData->pData[ulPackedDataCursor] = 0x01;
                    ulPackedDataCursor +=1;

                    ulInputDataCursor += LengthOfNal+ StartCodeLength;
                    
            }
         }
         while((-1)!= Offset);
         
         pPackedData->header.h264_data_header.uRotatedAngle = pInputData->header.h264_data_header.uRotatedAngle;
         pPackedData->uSize = ulPackedDataCursor;

         /*如果当关键参数没有获取到的时候，不给对端发包*/
         if( (0 == SPS.uLength) || (0 == PPS.uLength) )
         {
             pPackedData->uSize = 0;
             pPackedData->pData[0] = 0;
             pPackedData->pData[1] = 0;
             
         }

    return 0;
}


INT32 MTXW_H264Rtp::UnPack_H264_Payload(const UINT8* pInputData, MTXW_H264_RTP_PAYLOAD_HEADER_UNION * pPayloadHdr, UINT8 *ucLength)
{
    
    if(NULLPTR == pInputData || NULLPTR == pPayloadHdr)
    {
    
        MTXW_LOGE("MTXW_H264Rtp::UnPack_H264_Payload, pInputData or pPayloadHdr is NULL!!!!");
        return -1;
    }
    
    MTXW_FU_INDICATOR_STRU tempHeader = {0};
    UINT8 j = 0;
   
    tempHeader.bitF = (pInputData[0]&0x80)>>7;
    tempHeader.bitsNRI = (pInputData[0]&0x60)>>5;
    tempHeader.bitsType = pInputData[0]&0x1F;

    if(tempHeader.bitsType >= MTXW_NALU_TYPE_1_NON_IDR && tempHeader.bitsType <= MTXW_NALU_TYPE_12_FILLER_DATA)
    {
        pPayloadHdr->nalu_header.bitF = tempHeader.bitF;
        pPayloadHdr->nalu_header.bitsNRI = tempHeader.bitsNRI;
        pPayloadHdr->nalu_header.bitsType = tempHeader.bitsType;
        j++;
        
        *ucLength = j; 
    }
    else if(tempHeader.bitsType == MTXW_NALU_TYPE_28_FU_A)
    {
        pPayloadHdr->fu_A.fu_indicator.bitF = tempHeader.bitF;
        pPayloadHdr->fu_A.fu_indicator.bitsNRI = tempHeader.bitsNRI;
        pPayloadHdr->fu_A.fu_indicator.bitsType = tempHeader.bitsType;
        j++;

        pPayloadHdr->fu_A.fu_header.bitS = (pInputData[1]&0x80)>>7;
        pPayloadHdr->fu_A.fu_header.bitE = (pInputData[1]&0x40)>>6;
        pPayloadHdr->fu_A.fu_header.bitR = (pInputData[1]&0x20)>>5;
        pPayloadHdr->fu_A.fu_header.bitsNalu_type = (pInputData[1]&0x1F);
        j++;  

        *ucLength = j; 
    }
    else
    {
        //is not complete nalu or Fu_A 

        MTXW_LOGE("the receice packet is not Complete Nalu or Fu_A, Drop the packet!!!!, bitsType=%d", tempHeader.bitsType);
        
        return -1;
    }
            
    return 0;
}



UINT32 MTXW_H264Rtp::GetNextNalLocation(unsigned char* pStartBuff,UINT32 ulLength,UINT32 *pulStartCodeLength)
{
        MTXW_FUNC_TRACE();
        UINT32 ulIndex;

        if(NULL == pStartBuff)
        {
                MTXW_LOGE("MTXW_H264Rtp::GetNextNalLocation pStartBuff is NULL!");
                return (-1);
        }
        
        if(3 >= ulLength)
        {
                MTXW_LOGE("MTXW_H264Rtp::GetNextNalLocation ulLength is too Small!");
                return  (-1);
        }
        
        for(ulIndex = 0 ;ulIndex < ulLength - 3 ;ulIndex++)
        {
                if((0x00 == pStartBuff[ulIndex])
                 &&(0x00 == pStartBuff[ulIndex+1])
                 &&(0x01 == pStartBuff[ulIndex+2]))
                {
                    if(pulStartCodeLength)
                    {
                        if((ulIndex>0)&&(0x00 == pStartBuff[ulIndex-1]))
                        {
                            *pulStartCodeLength = 4;
                        }
                        else
                        {
                            *pulStartCodeLength = 3;
                        }
                    }
                    return ulIndex+3;
                }
        }
        
//        MTXW_LOGW("MTXW_H264Rtp::GetNextNalLocation Not find nal!");
        
        return  (-1);
        
}


INT32 MTXW_H264Rtp::PackH264RtpHeader (UINT8 *pData, UINT32 timeStamp, bool maker,UINT32 ulRotatedAngle)
{
	UINT16 temp = 0;
	UINT16 lengthOfRtp=0;
	UINT32 temp1 = 0;
	UINT16 tag =0;
	UINT16 length=2;
	UINT32 MagicNumber=0x000929A5;
	UINT8 srvId=1;
	UINT8 rsv=0;

	UINT16 usRotatedAngle = (UINT16)ulRotatedAngle;


	if(NULL == pData)
	{
		return -1;
	}

	if(0 == usRotatedAngle)
	{
            SetHeaderExtension(false);
	}
	else
	{
	    SetHeaderExtension(true);
	}
	
	pData[lengthOfRtp] = (GetVersion() << 6) | (GetPadding() << 5) |(HasHeaderExtension() << 4) |GetCSRCCount();
	lengthOfRtp++;
	
	if(maker)
	{
		pData[lengthOfRtp] = 0x80;
	}
	else
	{
		pData[lengthOfRtp] = 0;
	}
	
	pData[lengthOfRtp] =pData[lengthOfRtp] |payloadType;
	lengthOfRtp++;
	
	
	temp = mtxwHtons(seq);
	mtxwMemCopy(&pData[lengthOfRtp], &temp, 2);
	seq++;
	lengthOfRtp += 2;

	temp1 = mtxwHtonl(timeStamp);
	mtxwMemCopy(&pData[lengthOfRtp], &temp1, 4);
	lengthOfRtp += 4;

	temp1 = mtxwHtonl(GetSSRC());
	mtxwMemCopy(&pData[lengthOfRtp], &temp1, 4);
	lengthOfRtp += 4;

       /*******************************************************************
        |----tag-----|---length---| //tag=0x0000, length=0x02
        |---  00  09  29  A5 -----|  //MagicNumber=0x000929A5
        |srvId|--angle----|--rsv--|  //srvId=0x01, rsv=0x00, angle=0,90,180,270
        *******************************************************************/
	if(HasHeaderExtension())
	{
	    temp1= mtxwHtonl((tag<<16)|length);
	    mtxwMemCopy(&pData[lengthOfRtp], &temp1, 4);
	    lengthOfRtp +=4;
	    
	    temp1= mtxwHtonl(MagicNumber);
	    mtxwMemCopy(&pData[lengthOfRtp], &temp1, 4);
	    lengthOfRtp +=4;
	    
	    temp1= mtxwHtonl((srvId<<24)|(usRotatedAngle<<8)|rsv);
	    mtxwMemCopy(&pData[lengthOfRtp], &temp1, 4);
	    lengthOfRtp +=4;
	}
	return lengthOfRtp;
}


INT32 MTXW_H264Rtp::SendSPSorPPS (UINT8 * pPackedData,MTXW_SPS_PPS_BUFFER SPS_PPS,UINT32  timeStamp,UINT32  bitMaker,UINT16 uRotatedAngle)
{
     UINT32  ulRtpHeadLen = 0;     //length of Rtp Head. 
     UINT32  ulSendLength = 0;  //the cursor of paked data.
     UINT32  PacketLength = 0;
     
     ulRtpHeadLen = this->PackH264RtpHeader(&pPackedData[2],
    	                                                          timeStamp, 
    	                                                          bitMaker,
    	                                                          uRotatedAngle);

      //--2--the first two bytes are the length of each paket.
                 
     PacketLength = SPS_PPS.uLength + ulRtpHeadLen;       //packet length = nal_len + rtp_head_len

     //-增加padding字段的长度
      PacketLength++;
     
      mtxwMemCopy(&pPackedData[0] ,&PacketLength, 2); 

      ulSendLength += ulRtpHeadLen+2;
    	                                                        
      //--3--set nal value and put into  pPackedData->pData[ulPackedDataCursor]
                
     mtxwMemCopy(&pPackedData[ulSendLength], SPS_PPS.pData, SPS_PPS.uLength);
                                                 
     ulSendLength +=  SPS_PPS.uLength;

     //--增加padding
     pPackedData[2] |= 0x20;
     pPackedData[ulSendLength] = 0x01;
     ulSendLength+=1;
     

     return ulSendLength;
}
  
