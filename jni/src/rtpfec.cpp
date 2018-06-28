/*********************************************************************
*
*
*
***********************************************************************/
#include "rtpfec.h"
#include "Crc8.h"

#include "Galois.h"



#define gfm(a,b) glByteMul(a,b)
#define gfd(a,b) glByteDiv(a,b)


static UINT32 gMemCnt = 0;

//----�ڴ��������----------------//
void *fecMemAlloc(UINT32 usize)
{
	void *p=0;
	p = malloc(usize);
	if(p)
	{
		gMemCnt++;
	}
	return p;
}
void fecMemFree(void* p)
{
	if(p==0){return;}
	
	free(p);
	gMemCnt--;
}
INT32 fecGetMemCnt()
{
    return gMemCnt;
}

void RtpFecEncoder::initalRDBuffer(){
	if(pRDBuffer){
		memset(pRDBuffer,0,MAX_RD_BUFF_LEN*MAX_RANK);
	}
	for(int i=0;i<MAX_RANK;i++){
	    pRDBuffer[i*MAX_RD_BUFF_LEN+0]=0x20;//v=(00)0 p=1,x=0,cc=(0000)2
		pRDBuffer[i*MAX_RD_BUFF_LEN+1]=0x80;//M=1,PT=0;

		//--SSRC ID
		pRDBuffer[i*MAX_RD_BUFF_LEN+8]=this->usSSRC[0];
		pRDBuffer[i*MAX_RD_BUFF_LEN+9]=this->usSSRC[1];
		pRDBuffer[i*MAX_RD_BUFF_LEN+10]=this->usSSRC[2];
		pRDBuffer[i*MAX_RD_BUFF_LEN+11]=this->usSSRC[3];
	}

}

UINT16 RtpFecEncoder::getGroupDataLenByGDLI(UINT16 usGDLI)
{
	switch(usGDLI)
	{
	case 0:
		return 8;
	case 1:
		return 16;
	case 2:
		return 32;
	case 3:
		return 64;
	default:
		return 16;
	}
}
UINT8 RtpFecEncoder::getGDLI()
{
	switch(this->usCurrGroupDataLen)
	{
	case 8:
		return 0;
	case 16:
		return 1;
	case 32:
		return 2;
	case 64:
		return 3;
	default:
		return 0;
	}

}



void static GetTime(TimeVal *t){
#ifdef WIN32
	DWORD v = GetTickCount();
	t->tv_sec = v/1000; //��
	t->tv_msec = v%1000;//����
#else
    struct timespec currtime = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &currtime); //��ȡ��ǰʱ��
	t->tv_sec = currtime.tv_sec;
	t->tv_msec = currtime.tv_nsec / (1000*1000); //nsec->msec
#endif
}

//����ֵΪ����ʱ��ֻ���λΪ����
static long long DiffTime(TimeVal &t_start,TimeVal &t_end){
	return (t_end.tv_sec*1000+t_end.tv_msec)-(t_start.tv_sec*1000+t_start.tv_msec);
    
}

/************************************************************************
*
*@param rtpFecEncodeOutputCb:  ����ص�����
*@param pUserData: �����ߵ����ݣ��ٵ��ñ���ص����ʱ�Ὣ�˲����ӻظ�������
*@param rank:  ���������ȡֵ[0,1,2,3]������ȡֵ3
*@param usGDLI:  �����鳤�ȣ�ȡֵ[0,1,2]������ȡֵ1
*
*************************************************************************/
RtpFecEncoder::RtpFecEncoder(RtpFecEncodeOutputCallBack rtpFecEncodeOutputCb, void* pUserData,UINT16 rank, UINT16 usGDLI)
{
	this->outputCb = rtpFecEncodeOutputCb;
	this->pUserData = pUserData;
	this->usCurrGroupDataLen = this->usSettingGroupDataLen = getGroupDataLenByGDLI(usGDLI);
	this->usCurrGroupId = 0xFFFF;
	this->usCurrRank = this->usSettingRank = (rank<=3)?rank:1;

	this->usSSRC[0]= this->usSSRC[1]= this->usSSRC[2]= this->usSSRC[3]= 0;
	this->usMaxLen = 0;

	createRDBuffer();

}
RtpFecEncoder::~RtpFecEncoder()
{
	deleteRDBuffer();
}

//�����������
void RtpFecEncoder::outputRedundantData()
{
	CRC8 crc8Calculator;
	UINT8 padMsg[32]={0};
	UINT8 PadMsgLen=0,crc8=0;
	CqiStru  cqimsg = getCQI();

       //-------------FEC info msg packing----------------------------------------------
	padMsg[PadMsgLen]=3; // T= (0000)2 L=(3)
	PadMsgLen++;
	//--V--
	padMsg[PadMsgLen]=this->getGDLI()<<6|this->usCurrRank<<3|(UINT8)0;//GDLI|RV|RI
	PadMsgLen++;
	padMsg[PadMsgLen]=this->usCurrGroupId>>8;
	PadMsgLen++;
	padMsg[PadMsgLen]=this->usCurrGroupId&0xFF;
	PadMsgLen++;

	//-------------CQI msg packing--------------------------------------------------
	if(INVALID_CQI_VALUE != cqimsg.cqi.RI)
	{
	    if(cqimsg.bAcqi && INVALID_CQI_VALUE != cqimsg.acqi.RI)//�й����ŵ�CQI
	    {
	        padMsg[PadMsgLen++]=0x14; // T= (0001)2 L=(4)

	        //local channel CQI
	        padMsg[PadMsgLen++]= (0x01<<5) | ((cqimsg.cqi.GDLI&0x03)<<3) | (cqimsg.cqi.RI&0x07);
	        padMsg[PadMsgLen++] = cqimsg.cqi.RFR;
               //associate channel CQI
	        padMsg[PadMsgLen++]= ((cqimsg.acqi.GDLI&0x03)<<3) | (cqimsg.acqi.RI&0x07);
	        padMsg[PadMsgLen++] = cqimsg.acqi.RFR;
	        
	        
	    }else
	    {
	        padMsg[PadMsgLen++]=0x12; // T= (0001)2 L=(2)
	        padMsg[PadMsgLen++]= ((cqimsg.cqi.GDLI&0x03)<<3) | (cqimsg.cqi.RI&0x07);
	        padMsg[PadMsgLen++] = cqimsg.cqi.RFR;
	    }
	    
	}

	//-------------Magic Number / Lenghth / CRC packing-------------------------------
	padMsg[PadMsgLen]=0xA5; //Magic Number
	PadMsgLen++;
	padMsg[PadMsgLen] = PadMsgLen+1; //Length of Msg
	PadMsgLen++;
	padMsg[PadMsgLen] = crc8; //CRC8
	PadMsgLen++;

	if(this->usCurrRank>0)
	{
		//�����������
		for(unsigned int i=0;i<this->usCurrRank;i++)
		{
			//--����Padding2 Msg-----------------------------------------------
			padMsg[1] = (padMsg[1] &0xF8) | (i&0x07); //����RIֵ
			crc8 = crc8Calculator.calc(padMsg,PadMsgLen-1);//����CRC
	        padMsg[PadMsgLen-1] = crc8; //����CRC8

			//--��Msg�������������ݺ���
			memcpy(&pRDBuffer[i*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + this->usMaxLen],padMsg,PadMsgLen);

			pRDBuffer[i*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + this->usMaxLen +PadMsgLen] =0x01;//����Padding�ֶ�

			//�����������RTP��
			this->outputCb(this->pUserData,
				true,
				&pRDBuffer[i*MAX_RD_BUFF_LEN],
				RTP_HEADER_LEN + this->usMaxLen + PadMsgLen + 1);
		}
	}else{
		crc8 = crc8Calculator.calc(padMsg,PadMsgLen-1);//����CRC
	    padMsg[PadMsgLen-1] = crc8; //����CRC8

		//--��Msg������RTP Header����(0�����࣬Ҳ����û���������ݣ�ֻ��ҪЯ��FEC��������)
		memcpy(&pRDBuffer[0*MAX_RD_BUFF_LEN + RTP_HEADER_LEN],padMsg,PadMsgLen);
		pRDBuffer[0*MAX_RD_BUFF_LEN + RTP_HEADER_LEN +PadMsgLen] =0x01;//����Padding�ֶ�
		//�����������RTP��
		this->outputCb(this->pUserData,
				true,
				&pRDBuffer[0*MAX_RD_BUFF_LEN],
				RTP_HEADER_LEN + PadMsgLen + 1);

	}
}

UINT32 RtpFecEncoder::input(UINT8 *pRawRtpData, UINT16 len)
{
	UINT16 usRtpSN;

	if(pRawRtpData==0){
		return -1;
	}
	if(len>MAX_ENCODE_DATA_LEN || len<13){
		this->outputCb(this->pUserData,false,pRawRtpData,len);
		return 0;
	}

	//--RTP���ݰ�û��Padding���޷�����FEC����봦����Ϊ�������ҪPadding��Ϊ�������ݵı߽�
	if((pRawRtpData[0]&0x20) == 0){
		this->outputCb(this->pUserData,false,pRawRtpData,len);
		return 0;
	}

	usRtpSN = (pRawRtpData[2]<<8)|pRawRtpData[3];
	this->usSSRC[0]= pRawRtpData[8];
	this->usSSRC[1]= pRawRtpData[9];
	this->usSSRC[2]= pRawRtpData[10];
	this->usSSRC[3]= pRawRtpData[11];

	if(usRtpSN%this->usCurrGroupDataLen!=0 && this->usCurrGroupId==0xFFFF){
		this->outputCb(this->pUserData,false,pRawRtpData,len);
        return 0;
	}



	if(usRtpSN%this->usCurrGroupDataLen==0)
	{
		//�µ�һ�������鿪ʼ,��ʼ���������ݻ���
		initalRDBuffer();

		this->usMaxLen = 0;

		if(usRtpSN%this->usSettingGroupDataLen==0){
		    this->usCurrRank = this->usSettingRank;//�����������
			this->usCurrGroupDataLen = this->usSettingGroupDataLen; //���±��������ݰ�����
		}
	}

	this->usCurrGroupId = usRtpSN/this->usCurrGroupDataLen;
	this->usMaxLen = (this->usMaxLen<len)?len:this->usMaxLen;

	//��������������������������ݣ�ֱ���������
	if(this->usCurrRank==0)
	{
		//
	}else
	{
		//������������
		unsigned char I = usRtpSN%this->usCurrGroupDataLen;
		for(unsigned int i=0;i<len;i++)
		{
			//-����һ����������
			this->pRDBuffer[0*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + i] ^= pRawRtpData[i];

		    //-���������������
			if(this->usCurrRank>=2)
			{
				this->pRDBuffer[1*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + i] ^= gfm(I+1,pRawRtpData[i]);
			}

		    //-����������������
			if(this->usCurrRank>=3)
			{
				this->pRDBuffer[2*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + i] ^= gfm(gfm(I+1,I+1),pRawRtpData[i]);
			}
		}

	}

	this->outputCb(this->pUserData,false,pRawRtpData,len);

	if((usRtpSN+1)%this->usCurrGroupDataLen==0)
	{//�����������
		outputRedundantData();
	}

	return 0;
}

/************************************************************************
*
*@param rtpFecDecodeOutputCb: ����ص�����
*@param pUserData: �����ߵ����ݣ��ٵ��ý���ص����ʱ�Ὣ�˲����ӻظ�������
*
*************************************************************************/
RtpFecDecoder::RtpFecDecoder(RtpFecDecodeOutputCallBack rtpFecDecodeOutputCb, 
	                        RtpFecDecodeRcvCQICallBack rtpFecDecodeRcvCqiCb,
	                        RtpFecDecodeUpdCQICallBack rtpFecDecodeUpdCqiCb,
	                        void* pUserData)
{
	this->pUserData = pUserData;
	this->outputCb = rtpFecDecodeOutputCb;
	this->rcvCqiCb = rtpFecDecodeRcvCqiCb;
	this->updtCqiCb = rtpFecDecodeUpdCqiCb;
	this->fecValid = false;
	this->usDeliveredSN = 0;
	this->usRcvCnt = 0;
	this->ulSSRC = 0xFFFFFFFF;
	this->bitGDLI = 0;
	this->bitRV = 0;

	createBuffer();
	
}
RtpFecDecoder::~RtpFecDecoder()
{
	releaseBuffer();
}

void RtpFecDecoder::reset()
{
	resetBuffer();
	this->fecValid = false;
	this->usDeliveredSN = 0;
	this->usRcvCnt = 0;
	this->ulSSRC = 0xFFFFFFFF;
	this->bitGDLI = 0;
	this->bitRV = 0;
}
UINT8 RtpFecDecoder::DecodePiggyMsg(UINT8* pMsg, UINT8 msgLen,PiggyMsgStru* pPiggyMsg)
{
	UINT8 pos= 0;
	UINT8 T,L;
	pPiggyMsg->cqiBit = pPiggyMsg->fecBit = 0;
	
	while(pos < msgLen){
		T = pMsg[pos]>>4;
		L = pMsg[pos]&0x0F;
		pos++;

		if(pos+L>msgLen){
			break;
		}

		if(T==0){ //FEC info����
			pPiggyMsg->fecBit = 1;
			pPiggyMsg->fecInfo.ucGDLI=(pMsg[pos]>>6)&0x03;
			pPiggyMsg->fecInfo.ucRV=(pMsg[pos]>>3) &0x07;
			pPiggyMsg->fecInfo.ucRI=(pMsg[pos]) &0x07;
			pPiggyMsg->fecInfo.usGID=pMsg[pos+1]<<8|pMsg[pos+2];
		}else  if(T==1){//CQI ���� 2018.4.16
		      bool aCqiExist = (pMsg[pos]&0x20)==0x20;
		      if( ((L==2) && (!aCqiExist))
		      || ((L==4) && aCqiExist)){
		          pPiggyMsg->cqiBit = 1;
		          pPiggyMsg->cqi.cqi.GDLI = (pMsg[pos]>>3) &0x03;
		          pPiggyMsg->cqi.cqi.RI = pMsg[pos] &0x07;
		          pPiggyMsg->cqi.cqi.RFR = pMsg[pos+1];
		          
		          if(aCqiExist){
		              pPiggyMsg->cqi.acqi.GDLI = (pMsg[pos+2]>>3) &0x03;
		              pPiggyMsg->cqi.acqi.RI = pMsg[pos+2] &0x07;
		              pPiggyMsg->cqi.acqi.RFR = pMsg[pos+3];
		              pPiggyMsg->cqi.bAcqi = true;
		              
		          }else{
		              pPiggyMsg->cqi.acqi.GDLI = INVALID_CQI_VALUE;
		              pPiggyMsg->cqi.acqi.RI = INVALID_CQI_VALUE;
		              pPiggyMsg->cqi.acqi.RFR = INVALID_CQI_VALUE;
		              pPiggyMsg->cqi.bAcqi = false;
		          }
		      }
		      

		}
		
		pos += L;
	}
	return 0;
}
UINT16 RtpFecDecoder::getGroupDataLenByGDLI(UINT16 usGDLI)
{
	switch(usGDLI)
	{
	case 0:
		return 8;
	case 1:
		return 16;
	case 2:
		return 32;
	case 3:
		return 64;
	default:
		return 16;
	}
}

UINT16 RtpFecDecoder::getRtpHeaderLen(UINT8 *pRawRtpData,UINT16 len)
{
	//�Ȳ�������չͷ��ֱ�ӷ��ع̶�����
	return RTP_HEADER_LEN;
}

void RtpFecDecoder::flush()
{
	UINT16 groupDataLen;
	if(this->fecValid){
		groupDataLen = this->getGroupDataLenByGDLI(this->bitGDLI);
		for(UINT16 i=this->usDeliveredSN%groupDataLen; i<groupDataLen && i<this->vecDecodeBuffer.size();i++){
			if(this->vecDecodeBuffer[i]->usDataLen>0){
				this->outputCb(this->pUserData,
					this->vecDecodeBuffer[i]->pData+this->vecDecodeBuffer[i]->usOffset,
					this->vecDecodeBuffer[i]->usDataLen);
			}
		}
	}

	resetBuffer();

}

bool RtpFecDecoder::write2buff(std::vector<FecBuffStru*> &vecBuff,UINT16 pos,UINT8 *pData,UINT16 len){
	if(vecBuff.size() <= pos)
	{
	       MTXW_LOGE("fec decode write2buff: vecBuff.size() (=%d) <= pos(%d) ",vecBuff.size(),pos);
		return false;
	}

	if(len > MAX_ENCODE_DATA_LEN){
	       MTXW_LOGE("fec decode write2buff: len > %d ",MAX_ENCODE_DATA_LEN);
		return false;
	}

	if(vecBuff[pos]->usBuffLen < len){
		fecMemFree(vecBuff[pos]);
		vecBuff[pos] = (FecBuffStru*)fecMemAlloc(sizeof(FecBuffStru)+MAX_ENCODE_DATA_LEN);
		vecBuff[pos]->usBuffLen = MAX_ENCODE_DATA_LEN;
		vecBuff[pos]->usDataLen = 0;
		vecBuff[pos]->usOffset = 0;
	}
	fecMemCopy(vecBuff[pos]->pData + vecBuff[pos]->usOffset, pData, len);
	vecBuff[pos]->usDataLen = len;
	return true;
}
void RtpFecDecoder::recoverData()
{
	UINT16 groupDataLen=0;//�鳤��
	std::vector<UINT16> vecLostPos; //����λ��
	UINT16 rcvRDCnt=0;//���յ������������
	UINT16 RDLen = 0;//����������ݳ���

	UINT8 a1,a2,a3;
	UINT8 fz,fm;

	if(!this->fecValid){
		return;
	}

	groupDataLen = this->getGroupDataLenByGDLI(this->bitGDLI);

	//���㵱ǰ�������ڶ�ʧ��������
	for(UINT16 i=0;i<groupDataLen;i++){ if(0 == this->vecDecodeBuffer[i]->usDataLen){vecLostPos.push_back(i);}}


	//���㵱ǰ�������ڽ��յ������������
	for(UINT16 i=0;i<this->vecAlpha.size();i++){if(0 < this->vecAlpha[i]->usDataLen){rcvRDCnt++;RDLen=this->vecAlpha[i]->usDataLen;}}

	//--2018.4.16--CQI���������������
	cqiEvaluater.input(vecLostPos.size()+(this->bitRV-rcvRDCnt),this->bitGDLI);

	if((vecLostPos.size()==0) ||(rcvRDCnt<vecLostPos.size())){//�������û�ж�ʧ�����߶�ʧ̫���޷��ָ�����ֱ�ӷ���
	       MTXW_LOGD("(%s)-2 fec recovered data lost packet number =%d  rcvRDCnt = %d in this group",(RDLen<200)?"audio":"vedio",vecLostPos.size(),rcvRDCnt);
		return;
	}else{
	    //��λ��ӡ
	    MTXW_LOGD("(%s)-2 fec recovered data lost packet number =%d in this group",(RDLen<200)?"audio":"vedio",vecLostPos.size());
	    
	}

    //����Alphaֵ
	for(unsigned int i = 0;i<groupDataLen;i++)
	{
		if(0 == this->vecDecodeBuffer[i]->usDataLen){
			continue;
		}

		//--����һ��alphaֵ
		if(this->vecAlpha[0]->usDataLen > 0)
		{
			for(unsigned int j=0;j<this->vecDecodeBuffer[i]->usDataLen;j++)
			{
				this->vecAlpha[0]->pData[j] ^= this->vecDecodeBuffer[i]->pData[j];
			}
		}
		
		//-�������alphaֵ
		if(this->vecAlpha[1]->usDataLen > 0){
			for(unsigned int j=0;j<this->vecDecodeBuffer[i]->usDataLen;j++)
			{
				this->vecAlpha[1]->pData[j] ^= gfm(i+1,this->vecDecodeBuffer[i]->pData[j]);
			}
		}
		//-��������alphaֵ
		if(this->vecAlpha[2]->usDataLen > 0){
			for(unsigned int j=0;j<this->vecDecodeBuffer[i]->usDataLen;j++)
			{
				this->vecAlpha[2]->pData[j] ^= gfm(gfm(i+1,i+1),this->vecDecodeBuffer[i]->pData[j]);
			}
		}
	}

	//----����������������������alphaֵ���ж�ʧ���ݻָ�����-----

	//�ж��ڴ��Ƿ��������ɻָ����ݣ����̫С���������㹻���ڴ�
	for(UINT16 i=0;i<vecLostPos.size();i++){
		if(this->vecDecodeBuffer[vecLostPos[i]]->usBuffLen<RDLen){
			fecMemFree(this->vecDecodeBuffer[vecLostPos[i]]);
			this->vecDecodeBuffer[vecLostPos[i]] = (FecBuffStru*)fecMemAlloc(sizeof(FecBuffStru)+MAX_ENCODE_DATA_LEN);
			this->vecDecodeBuffer[vecLostPos[i]]->usBuffLen = MAX_ENCODE_DATA_LEN;
			this->vecDecodeBuffer[vecLostPos[i]]->usDataLen = 0;
		}
		this->vecDecodeBuffer[vecLostPos[i]]->usOffset = 0;
	}

	if(vecLostPos.size()==1)//��ʧһ�����ݵĳ���
	{
		unsigned int lostI = vecLostPos[0];
		for(unsigned int r = 0;r<this->vecAlpha.size();r++)
		{
			if(this->vecAlpha[r]->usDataLen>0)
			{
				if(r==0){//ʹ��һ���������ݽ��лָ�
					memcpy(this->vecDecodeBuffer[lostI]->pData,this->vecAlpha[r]->pData,this->vecAlpha[r]->usDataLen);
				}else if(r==1){//ʹ�ö����������ݽ��лָ�
					for(int i=0;i<this->vecAlpha[r]->usDataLen;i++){
						this->vecDecodeBuffer[lostI]->pData[i] = gfd(this->vecAlpha[r]->pData[i],lostI+1);
					}
				}else if(r==2){//ʹ�������������ݽ��лָ�
					for(int i=0;i<this->vecAlpha[r]->usDataLen;i++){
					    this->vecDecodeBuffer[lostI]->pData[i] = gfd(this->vecAlpha[r]->pData[i],gfm(lostI+1,lostI+1));
					}
				}
				break;
			}
		}

	}else if(vecLostPos.size()==2)//��ʧ�������ݰ��ĳ���
	{
		unsigned int lostI = vecLostPos[0],lostJ = vecLostPos[1];

		if(this->vecAlpha[0]->usDataLen>0 && this->vecAlpha[1]->usDataLen>0){//ʹ�õ�һ������������������ݻָ�����
			for(int i=0;i<RDLen;i++){
				a1 = this->vecAlpha[0]->pData[i];
				a2 = this->vecAlpha[1]->pData[i];
				this->vecDecodeBuffer[lostI]->pData[i]=gfd(gfm(lostJ+1,a1)^a2,(lostJ+1)^(lostI+1));
	            this->vecDecodeBuffer[lostJ]->pData[i]=gfd(gfm(lostI+1,a1)^a2,(lostJ+1)^(lostI+1));
			}

		}else if(this->vecAlpha[0]->usDataLen>0  && this->vecAlpha[2]->usDataLen>0 ){//���õ�һ������������ָ�����
			for(int i=0;i<RDLen;i++){
				a1 = this->vecAlpha[0]->pData[i];
				a3 = this->vecAlpha[2]->pData[i];
				fz = gfm(lostJ+1,lostJ+1)^gfm(lostI+1,lostI+1);
				fm = gfm(gfm(lostJ+1,lostJ+1),a1)^a3;
				this->vecDecodeBuffer[lostI]->pData[i]=gfd(fm,fz);
				fm = gfm(gfm(lostI+1,lostI+1),a1)^a3;
	            this->vecDecodeBuffer[lostJ]->pData[i]=gfd(fm,fz);
			}

		}else if(this->vecAlpha[1]->usDataLen>0  && this->vecAlpha[2]->usDataLen>0 ){//���õڶ�������������ָ�����		
			for(int i=0;i<RDLen;i++){
				a2 = this->vecAlpha[1]->pData[i];
				a3 = this->vecAlpha[2]->pData[i];
				fz = gfm(lostI+1,lostJ+1)^gfm(lostI+1,lostI+1);
				fm = gfm(lostJ+1,a2)^a3;
				this->vecDecodeBuffer[lostI]->pData[i] = gfd(fm,fz);
				fz = gfm(lostI+1,lostJ+1)^gfm(lostJ+1,lostJ+1);
				fm = a3^gfm(lostI+1,a2);
				this->vecDecodeBuffer[lostJ]->pData[i] = gfd(fm,fz);
			}
		}
	}else if(vecLostPos.size()==3 ){
		if(this->vecAlpha[0]->usDataLen>0 && this->vecAlpha[1]->usDataLen>0 && this->vecAlpha[2]->usDataLen>0){

			unsigned int lostI = vecLostPos[0]+1,lostJ = vecLostPos[1]+1,lostK = vecLostPos[2]+1;
			unsigned char I_s=gfm(lostI,lostI);
			unsigned char J_s=gfm(lostJ,lostJ);
			unsigned char K_s=gfm(lostK,lostK);
			unsigned char d=gfm(lostJ^lostI,K_s)^gfm(lostK^lostJ,I_s) ^ gfm(lostI^lostK,J_s);
			unsigned char belta=gfd(1,d);
			unsigned char recoverI,recoverJ,recoverK;
			for(unsigned int i=0;i<RDLen;i++){
				a1 = this->vecAlpha[0]->pData[i];
				a2 = this->vecAlpha[1]->pData[i];
				a3 = this->vecAlpha[2]->pData[i];
				recoverI=gfm(a1,gfm(lostJ,K_s)^gfm(lostK,J_s)) ^ gfm(a2,J_s^K_s) ^ gfm(a3,lostK^lostJ);
				recoverJ=gfm(a1,gfm(lostK,I_s)^gfm(lostI,K_s)) ^ gfm(a2,K_s^I_s) ^ gfm(a3,lostI^lostK);
				recoverK=gfm(a1,gfm(lostI,J_s)^gfm(lostJ,I_s)) ^ gfm(a2,I_s^J_s) ^ gfm(a3,lostJ^lostI);
				recoverI=gfm(belta,recoverI);
				recoverJ=gfm(belta,recoverJ);
				recoverK=gfm(belta,recoverK);
				this->vecDecodeBuffer[lostI-1]->pData[i]=recoverI;
				this->vecDecodeBuffer[lostJ-1]->pData[i]=recoverJ;
				this->vecDecodeBuffer[lostK-1]->pData[i]=recoverK;
			}
		}
	}

	for(UINT16 i=0;i<vecLostPos.size();i++){
		UINT16 pos = vecLostPos[i];
		this->vecDecodeBuffer[pos]->usDataLen = RDLen;

		//�Ƴ����ݻָ�ʱ�����Ķ���"0"�������Ѿ�Ҫ�����н��б����RTP����Ҫ�������أ��������ݵ����һ���ֽڱ�Ȼ����"0"
		while(this->vecDecodeBuffer[pos]->usDataLen > 0
			&& this->vecDecodeBuffer[pos]->pData[this->vecDecodeBuffer[pos]->usDataLen-1] == 0){
			this->vecDecodeBuffer[pos]->usDataLen --;
		}

	}

	

        for(UINT16 i=0; i<vecLostPos.size();i++){
             FecBuffStru* pBuff = vecDecodeBuffer[vecLostPos[i]];
             MTXW_LOGD("(%s)-2 fec recovered data SN=%d",(RDLen<200)?"audio":"vedio",(pBuff->pData[pBuff->usOffset+2]<<8)|pBuff->pData[pBuff->usOffset+3]);
        }


}

UINT32 RtpFecDecoder::input(UINT8 *pRawRtpData, UINT16 len)
{
    if((pRawRtpData[0]&0x20) == 0 || len <12){//RTPͷ��P��־λδ��λ���������ݰ�����С��һ��RTPͷ
    	this->outputCb(this->pUserData,pRawRtpData,len);
    	return 0;
    }
    UINT16 usRtpHeaderLen;
    unsigned char PadLen = pRawRtpData[len-1];
    UINT16 usRtpSN = (pRawRtpData[2]<<8)|pRawRtpData[3];
    UINT32 ulRcvSSRC = pRawRtpData[8]<<24|pRawRtpData[9]<<16|pRawRtpData[10]<<8|pRawRtpData[11];
    UINT8  ucPT = pRawRtpData[1]&0x7F;

    //SSRC�仯ʱ������һ�½�������SSRC�仯��˵���Ѿ�����ͬһ����������
    if(this->ulSSRC!=0xFFFFFFFF && ulRcvSSRC!=this->ulSSRC){
        MTXW_LOGE("fec decode input: ssrc changed from(%d) to (%d)",this->ulSSRC,ulRcvSSRC);
        flush();
        reset();
    }
    this->ulSSRC = ulRcvSSRC;

	if(PadLen+12>len){
		this->outputCb(this->pUserData,pRawRtpData,len);
		return 0;
	}
	usRtpHeaderLen = getRtpHeaderLen(pRawRtpData,len);
	if(ucPT==0){//�������PT==0
		UINT8 ucRcvCrc = pRawRtpData[len - PadLen - 1];
		UINT8 ucPiggyMsgLen = pRawRtpData[len - PadLen - 2];
		UINT8 ucMagicNumber = pRawRtpData[len - PadLen - 3];
		UINT8 *pPiggyMsg = &pRawRtpData[len - PadLen - 2-ucPiggyMsgLen+1];

		if(ucPiggyMsgLen+1+PadLen+12>len){
			this->outputCb(this->pUserData,pRawRtpData,len);
		    return 0;
		}
		if(ucMagicNumber!=0xA5){
			this->outputCb(this->pUserData,pRawRtpData,len);
		    return 0;
		}
		CRC8 crc8Calculator;
	    UINT8 crc_calc = crc8Calculator.calc(pPiggyMsg,ucPiggyMsgLen);
		if(crc_calc!=ucRcvCrc){//CRCУ�����˵���˰������������
			this->outputCb(this->pUserData,pRawRtpData,len);
			return 0;
		}
		PiggyMsgStru PiggyMsg;
		DecodePiggyMsg(pPiggyMsg,ucPiggyMsgLen-2,&PiggyMsg);
		if(PiggyMsg.fecBit==1){
			//�ߵ����˵�������յ������ݰ�����FEC�����

			//--2018.4.16 ֪ͨ�ϲ���յ���CQI��Ϣ
			if(PiggyMsg.cqiBit==1){
				reportRecvCQI(PiggyMsg.cqi);
			}
			if(this->fecValid==false){
				//��һ���յ�FEC ��Ϣ
				this->bitGDLI=PiggyMsg.fecInfo.ucGDLI;
				this->bitRV=PiggyMsg.fecInfo.ucRV;
				this->fecValid = true;
				//���������ݽ�SN����һ�����������Ч
				this->usDeliveredSN = (PiggyMsg.fecInfo.usGID+1)*this->getGroupDataLenByGDLI(PiggyMsg.fecInfo.ucGDLI);
			}else{
				//���FEC�鳤�����仯���򽫵�ǰ���뻺������ֱ�������FEC�������¸���������Ч
				if(PiggyMsg.fecInfo.ucGDLI!=this->bitGDLI){
					flush(); //���뻺���������
					//���������ݽ�SN����һ�����������Ч
					this->usDeliveredSN = (PiggyMsg.fecInfo.usGID+1)*this->getGroupDataLenByGDLI(PiggyMsg.fecInfo.ucGDLI);
					this->bitGDLI=PiggyMsg.fecInfo.ucGDLI;
				    this->bitRV=PiggyMsg.fecInfo.ucRV;
					this->fecValid = true;
					
				}else{//�����鳤��û�б仯
					if(PiggyMsg.fecInfo.usGID != this->usDeliveredSN/this->getGroupDataLenByGDLI(this->bitGDLI)){//�����ڵ�ǰ�飬�����ð�
						//DO NOTHING
					}else{
						
						if(this->bitRV>0 && PiggyMsg.fecInfo.ucRV==0){//���������Ϊ0����ֱ�ӽ����ջ�����������
							flush();
							this->usDeliveredSN = (PiggyMsg.fecInfo.usGID+1)*getGroupDataLenByGDLI(this->bitGDLI);

						}else{
							this->bitRV=PiggyMsg.fecInfo.ucRV;//����ucRV��
							//���������������ݵĳ���
							UINT16 usRDLen = len - usRtpHeaderLen - ucPiggyMsgLen - 1 - PadLen;//"-1": msg crc 
							if(usRDLen>0){
								write2buff(this->vecAlpha,PiggyMsg.fecInfo.ucRI,pRawRtpData+usRtpHeaderLen,usRDLen);
							}
						}
					}
					
				}
			}
		}else{//������ݰ�����FEC�������ֱ�����
			this->outputCb(this->pUserData,pRawRtpData,len);
		    return 0;
		}
	}else{//�������(PT!=0)
		if(!this->fecValid){//��δ�յ�FEC��Ϣ���޷�����
			this->outputCb(this->pUserData,pRawRtpData,len);
		    return 0;
		}

		UINT16 dst = (this->usDeliveredSN<=usRtpSN)?(usRtpSN-this->usDeliveredSN):(MAX_SN-this->usDeliveredSN+usRtpSN);
		UINT16 groupDataLen = getGroupDataLenByGDLI(this->bitGDLI);
		UINT16 pos = usRtpSN%groupDataLen;
		UINT16 currGroupId = this->usDeliveredSN/groupDataLen;

		UINT8  cqi_RI,cqi_GDLI;

		if(dst<MAX_SN/2){//�ڽ��մ���
			if(usRtpSN/groupDataLen == currGroupId){//���յ����ݰ����ڵ�ǰ������
				write2buff(this->vecDecodeBuffer,pos,pRawRtpData,len);
				if(this->usDeliveredSN == usRtpSN){
					this->outputCb(this->pUserData,pRawRtpData,len);
					this->usDeliveredSN++;
					if(currGroupId != this->usDeliveredSN/groupDataLen){//��ǰ�������������ݶ��Ѿ������
						resetBuffer();
						cqiEvaluater.input(0,this->bitGDLI);
						cqiEvaluater.getEvaluatedResult(cqi_RI,cqi_GDLI);
						this->updateCQI(cqi_GDLI,cqi_RI);
					}
				}
			}else{//���ݰ������ڵ�ǰ�����飬˵���µĽ������Ѿ���������ǰ�����������������
				recoverData();
				flush();//���������

				cqiEvaluater.getEvaluatedResult(cqi_RI,cqi_GDLI);
				this->updateCQI(cqi_GDLI,cqi_RI);

				//��������µĽ�����
				write2buff(this->vecDecodeBuffer,pos,pRawRtpData,len);//���ݱ��浽���棬�������ݻָ���Ҫ
				currGroupId = usRtpSN/groupDataLen;
				this->usDeliveredSN = currGroupId*groupDataLen;
				
				if(this->usDeliveredSN == usRtpSN){
					this->outputCb(this->pUserData,pRawRtpData,len);
					this->usDeliveredSN++;
				}
			}
			
		}else{//������(һ�������紫���е�������)����ֱ������������뵱ǰ�Ľ�����
			this->outputCb(this->pUserData,pRawRtpData,len);
		}

	}
	return 0;
}


CQIEvaluate::CQIEvaluate(){
	reset();
}
CQIEvaluate::~CQIEvaluate(){
}
void CQIEvaluate::input(UINT8 lostCnt,UINT8 GDLI){
	static long long period[]={0xFFFFFFFFll,40*1000,20*1000,10*1000};
	TimeVal tv;
	GetTime(&tv);

	if(this->eChange.tv_msec==0 && this->eChange.tv_sec==0){
		this->eChange = tv;
	}

	this->eGDLI = GDLI;

	if(lostCnt>=this->eRI){
		this->eRI = (lostCnt>=3)?3:lostCnt; //����
		if(lostCnt>this->eRI){this->eCount ++;}
		this->eChange = tv;
	}else{
		long long diff = DiffTime(this->eChange,tv);
		printf("\r\ndiff=%lld period=%lld",diff,period[this->eRI]);
		if(diff>period[this->eRI]){ //����������һ��ʱ�䵱ǰeRI���ȶ�����Сʱ�Ż�֪ͨ���Ͷ˽��ף���������½�
			this->eRI = (this->eRI>0)?(this->eRI-1):0;
			this->eChange = tv;
		}

	}
}
bool CQIEvaluate::getEvaluatedResult(UINT8 & RI,UINT8 &GDLI){
	if(this->eChange.tv_msec==0 && this->eChange.tv_sec==0){
		return false;
	}else{
		RI = this->eRI;
		GDLI = this->eGDLI;
		return true;
	}
}

void CQIEvaluate::reset(){
	this->eCount = 0;
	this->eGDLI = 1;
	this->eRI = 3;
	this->eChange.tv_sec=0;
	this->eChange.tv_msec=0;
}


