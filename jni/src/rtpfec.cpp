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

//----内存操作函数----------------//
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
	t->tv_sec = v/1000; //秒
	t->tv_msec = v%1000;//毫秒
#else
    struct timespec currtime = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &currtime); //获取当前时间
	t->tv_sec = currtime.tv_sec;
	t->tv_msec = currtime.tv_nsec / (1000*1000); //nsec->msec
#endif
}

//返回值为两个时间只差，单位为毫秒
static long long DiffTime(TimeVal &t_start,TimeVal &t_end){
	return (t_end.tv_sec*1000+t_end.tv_msec)-(t_start.tv_sec*1000+t_start.tv_msec);
    
}

/************************************************************************
*
*@param rtpFecEncodeOutputCb:  编码回调函数
*@param pUserData: 调用者的数据，再调用编码回调输出时会将此参数扔回给调用者
*@param rank:  冗余阶数，取值[0,1,2,3]，建议取值3
*@param usGDLI:  编码组长度，取值[0,1,2]，建议取值1
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

//输出冗余数据
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
	    if(cqimsg.bAcqi && INVALID_CQI_VALUE != cqimsg.acqi.RI)//有关联信道CQI
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
		//输出冗余数据
		for(unsigned int i=0;i<this->usCurrRank;i++)
		{
			//--构造Padding2 Msg-----------------------------------------------
			padMsg[1] = (padMsg[1] &0xF8) | (i&0x07); //更新RI值
			crc8 = crc8Calculator.calc(padMsg,PadMsgLen-1);//计算CRC
	        padMsg[PadMsgLen-1] = crc8; //更新CRC8

			//--将Msg拷贝到冗余数据后面
			memcpy(&pRDBuffer[i*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + this->usMaxLen],padMsg,PadMsgLen);

			pRDBuffer[i*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + this->usMaxLen +PadMsgLen] =0x01;//插入Padding字段

			//输出冗余数据RTP包
			this->outputCb(this->pUserData,
				true,
				&pRDBuffer[i*MAX_RD_BUFF_LEN],
				RTP_HEADER_LEN + this->usMaxLen + PadMsgLen + 1);
		}
	}else{
		crc8 = crc8Calculator.calc(padMsg,PadMsgLen-1);//计算CRC
	    padMsg[PadMsgLen-1] = crc8; //更新CRC8

		//--将Msg拷贝到RTP Header后面(0阶冗余，也就是没有冗余数据，只需要携带FEC参数即可)
		memcpy(&pRDBuffer[0*MAX_RD_BUFF_LEN + RTP_HEADER_LEN],padMsg,PadMsgLen);
		pRDBuffer[0*MAX_RD_BUFF_LEN + RTP_HEADER_LEN +PadMsgLen] =0x01;//插入Padding字段
		//输出冗余数据RTP包
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

	//--RTP数据包没有Padding，无法进行FEC编解码处理，因为解码放需要Padding作为解码数据的边界
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
		//新的一个解码组开始,初始化冗余数据缓存
		initalRDBuffer();

		this->usMaxLen = 0;

		if(usRtpSN%this->usSettingGroupDataLen==0){
		    this->usCurrRank = this->usSettingRank;//更新冗余阶数
			this->usCurrGroupDataLen = this->usSettingGroupDataLen; //更新编码组数据包个数
		}
	}

	this->usCurrGroupId = usRtpSN/this->usCurrGroupDataLen;
	this->usMaxLen = (this->usMaxLen<len)?len:this->usMaxLen;

	//无冗余的情况，无需计算冗余数据，直接输出数据
	if(this->usCurrRank==0)
	{
		//
	}else
	{
		//计算冗余数据
		unsigned char I = usRtpSN%this->usCurrGroupDataLen;
		for(unsigned int i=0;i<len;i++)
		{
			//-计算一阶冗余数据
			this->pRDBuffer[0*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + i] ^= pRawRtpData[i];

		    //-计算二阶冗余数据
			if(this->usCurrRank>=2)
			{
				this->pRDBuffer[1*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + i] ^= gfm(I+1,pRawRtpData[i]);
			}

		    //-计算三阶冗余数据
			if(this->usCurrRank>=3)
			{
				this->pRDBuffer[2*MAX_RD_BUFF_LEN + RTP_HEADER_LEN + i] ^= gfm(gfm(I+1,I+1),pRawRtpData[i]);
			}
		}

	}

	this->outputCb(this->pUserData,false,pRawRtpData,len);

	if((usRtpSN+1)%this->usCurrGroupDataLen==0)
	{//输出冗余数据
		outputRedundantData();
	}

	return 0;
}

/************************************************************************
*
*@param rtpFecDecodeOutputCb: 解码回调函数
*@param pUserData: 调用者的数据，再调用解码回调输出时会将此参数扔回给调用者
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

		if(T==0){ //FEC info存在
			pPiggyMsg->fecBit = 1;
			pPiggyMsg->fecInfo.ucGDLI=(pMsg[pos]>>6)&0x03;
			pPiggyMsg->fecInfo.ucRV=(pMsg[pos]>>3) &0x07;
			pPiggyMsg->fecInfo.ucRI=(pMsg[pos]) &0x07;
			pPiggyMsg->fecInfo.usGID=pMsg[pos+1]<<8|pMsg[pos+2];
		}else  if(T==1){//CQI 存在 2018.4.16
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
	//先不考虑扩展头，直接返回固定长度
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
	UINT16 groupDataLen=0;//组长度
	std::vector<UINT16> vecLostPos; //丢包位置
	UINT16 rcvRDCnt=0;//接收到的冗余包个数
	UINT16 RDLen = 0;//冗余包的数据长度

	UINT8 a1,a2,a3;
	UINT8 fz,fm;

	if(!this->fecValid){
		return;
	}

	groupDataLen = this->getGroupDataLenByGDLI(this->bitGDLI);

	//计算当前解码组内丢失的数据量
	for(UINT16 i=0;i<groupDataLen;i++){ if(0 == this->vecDecodeBuffer[i]->usDataLen){vecLostPos.push_back(i);}}


	//计算当前解码组内接收到的冗余包数量
	for(UINT16 i=0;i<this->vecAlpha.size();i++){if(0 < this->vecAlpha[i]->usDataLen){rcvRDCnt++;RDLen=this->vecAlpha[i]->usDataLen;}}

	//--2018.4.16--CQI评估参数输入调用
	cqiEvaluater.input(vecLostPos.size()+(this->bitRV-rcvRDCnt),this->bitGDLI);

	if((vecLostPos.size()==0) ||(rcvRDCnt<vecLostPos.size())){//如果数据没有丢失，或者丢失太多无法恢复，则直接返回
	       MTXW_LOGD("(%s)-2 fec recovered data lost packet number =%d  rcvRDCnt = %d in this group",(RDLen<200)?"audio":"vedio",vecLostPos.size(),rcvRDCnt);
		return;
	}else{
	    //定位打印
	    MTXW_LOGD("(%s)-2 fec recovered data lost packet number =%d in this group",(RDLen<200)?"audio":"vedio",vecLostPos.size());
	    
	}

    //计算Alpha值
	for(unsigned int i = 0;i<groupDataLen;i++)
	{
		if(0 == this->vecDecodeBuffer[i]->usDataLen){
			continue;
		}

		//--计算一阶alpha值
		if(this->vecAlpha[0]->usDataLen > 0)
		{
			for(unsigned int j=0;j<this->vecDecodeBuffer[i]->usDataLen;j++)
			{
				this->vecAlpha[0]->pData[j] ^= this->vecDecodeBuffer[i]->pData[j];
			}
		}
		
		//-计算二阶alpha值
		if(this->vecAlpha[1]->usDataLen > 0){
			for(unsigned int j=0;j<this->vecDecodeBuffer[i]->usDataLen;j++)
			{
				this->vecAlpha[1]->pData[j] ^= gfm(i+1,this->vecDecodeBuffer[i]->pData[j]);
			}
		}
		//-计算三阶alpha值
		if(this->vecAlpha[2]->usDataLen > 0){
			for(unsigned int j=0;j<this->vecDecodeBuffer[i]->usDataLen;j++)
			{
				this->vecAlpha[2]->pData[j] ^= gfm(gfm(i+1,i+1),this->vecDecodeBuffer[i]->pData[j]);
			}
		}
	}

	//----下面是利用上面计算出来的alpha值进行丢失数据恢复计算-----

	//判断内存是否足以容纳恢复数据，如果太小，则申请足够的内存
	for(UINT16 i=0;i<vecLostPos.size();i++){
		if(this->vecDecodeBuffer[vecLostPos[i]]->usBuffLen<RDLen){
			fecMemFree(this->vecDecodeBuffer[vecLostPos[i]]);
			this->vecDecodeBuffer[vecLostPos[i]] = (FecBuffStru*)fecMemAlloc(sizeof(FecBuffStru)+MAX_ENCODE_DATA_LEN);
			this->vecDecodeBuffer[vecLostPos[i]]->usBuffLen = MAX_ENCODE_DATA_LEN;
			this->vecDecodeBuffer[vecLostPos[i]]->usDataLen = 0;
		}
		this->vecDecodeBuffer[vecLostPos[i]]->usOffset = 0;
	}

	if(vecLostPos.size()==1)//丢失一个数据的场景
	{
		unsigned int lostI = vecLostPos[0];
		for(unsigned int r = 0;r<this->vecAlpha.size();r++)
		{
			if(this->vecAlpha[r]->usDataLen>0)
			{
				if(r==0){//使用一阶冗余数据进行恢复
					memcpy(this->vecDecodeBuffer[lostI]->pData,this->vecAlpha[r]->pData,this->vecAlpha[r]->usDataLen);
				}else if(r==1){//使用二阶冗余数据进行恢复
					for(int i=0;i<this->vecAlpha[r]->usDataLen;i++){
						this->vecDecodeBuffer[lostI]->pData[i] = gfd(this->vecAlpha[r]->pData[i],lostI+1);
					}
				}else if(r==2){//使用三阶冗余数据进行恢复
					for(int i=0;i<this->vecAlpha[r]->usDataLen;i++){
					    this->vecDecodeBuffer[lostI]->pData[i] = gfd(this->vecAlpha[r]->pData[i],gfm(lostI+1,lostI+1));
					}
				}
				break;
			}
		}

	}else if(vecLostPos.size()==2)//丢失两个数据包的场景
	{
		unsigned int lostI = vecLostPos[0],lostJ = vecLostPos[1];

		if(this->vecAlpha[0]->usDataLen>0 && this->vecAlpha[1]->usDataLen>0){//使用第一、二个冗余包进行数据恢复计算
			for(int i=0;i<RDLen;i++){
				a1 = this->vecAlpha[0]->pData[i];
				a2 = this->vecAlpha[1]->pData[i];
				this->vecDecodeBuffer[lostI]->pData[i]=gfd(gfm(lostJ+1,a1)^a2,(lostJ+1)^(lostI+1));
	            this->vecDecodeBuffer[lostJ]->pData[i]=gfd(gfm(lostI+1,a1)^a2,(lostJ+1)^(lostI+1));
			}

		}else if(this->vecAlpha[0]->usDataLen>0  && this->vecAlpha[2]->usDataLen>0 ){//利用第一、三个冗余包恢复数据
			for(int i=0;i<RDLen;i++){
				a1 = this->vecAlpha[0]->pData[i];
				a3 = this->vecAlpha[2]->pData[i];
				fz = gfm(lostJ+1,lostJ+1)^gfm(lostI+1,lostI+1);
				fm = gfm(gfm(lostJ+1,lostJ+1),a1)^a3;
				this->vecDecodeBuffer[lostI]->pData[i]=gfd(fm,fz);
				fm = gfm(gfm(lostI+1,lostI+1),a1)^a3;
	            this->vecDecodeBuffer[lostJ]->pData[i]=gfd(fm,fz);
			}

		}else if(this->vecAlpha[1]->usDataLen>0  && this->vecAlpha[2]->usDataLen>0 ){//利用第二、三个冗余包恢复数据		
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

		//移除数据恢复时产生的多余"0"，由于已经要求所有进行编码的RTP包都要有填充比特，所以数据的最后一个字节必然不是"0"
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
    if((pRawRtpData[0]&0x20) == 0 || len <12){//RTP头的P标志位未置位，或者数据包长度小于一个RTP头
    	this->outputCb(this->pUserData,pRawRtpData,len);
    	return 0;
    }
    UINT16 usRtpHeaderLen;
    unsigned char PadLen = pRawRtpData[len-1];
    UINT16 usRtpSN = (pRawRtpData[2]<<8)|pRawRtpData[3];
    UINT32 ulRcvSSRC = pRawRtpData[8]<<24|pRawRtpData[9]<<16|pRawRtpData[10]<<8|pRawRtpData[11];
    UINT8  ucPT = pRawRtpData[1]&0x7F;

    //SSRC变化时，重置一下解码器，SSRC变化了说明已经不是同一条数据流了
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
	if(ucPT==0){//冗余包的PT==0
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
		if(crc_calc!=ucRcvCrc){//CRC校验错误，说明此包并不是冗余包
			this->outputCb(this->pUserData,pRawRtpData,len);
			return 0;
		}
		PiggyMsgStru PiggyMsg;
		DecodePiggyMsg(pPiggyMsg,ucPiggyMsgLen-2,&PiggyMsg);
		if(PiggyMsg.fecBit==1){
			//走到这里，说明本次收到的数据包就是FEC冗余包

			//--2018.4.16 通知上层接收到的CQI信息
			if(PiggyMsg.cqiBit==1){
				reportRecvCQI(PiggyMsg.cqi);
			}
			if(this->fecValid==false){
				//第一次收到FEC 信息
				this->bitGDLI=PiggyMsg.fecInfo.ucGDLI;
				this->bitRV=PiggyMsg.fecInfo.ucRV;
				this->fecValid = true;
				//更新期望递交SN，下一个编解码组生效
				this->usDeliveredSN = (PiggyMsg.fecInfo.usGID+1)*this->getGroupDataLenByGDLI(PiggyMsg.fecInfo.ucGDLI);
			}else{
				//如果FEC组长发生变化，则将当前解码缓存数据直接输出，FEC更新在下个编码组生效
				if(PiggyMsg.fecInfo.ucGDLI!=this->bitGDLI){
					flush(); //解码缓存数据输出
					//更新期望递交SN，下一个编解码组生效
					this->usDeliveredSN = (PiggyMsg.fecInfo.usGID+1)*this->getGroupDataLenByGDLI(PiggyMsg.fecInfo.ucGDLI);
					this->bitGDLI=PiggyMsg.fecInfo.ucGDLI;
				    this->bitRV=PiggyMsg.fecInfo.ucRV;
					this->fecValid = true;
					
				}else{//编码组长度没有变化
					if(PiggyMsg.fecInfo.usGID != this->usDeliveredSN/this->getGroupDataLenByGDLI(this->bitGDLI)){//不属于当前组，则丢弃该包
						//DO NOTHING
					}else{
						
						if(this->bitRV>0 && PiggyMsg.fecInfo.ucRV==0){//冗余阶数变为0，则直接将接收缓存的数据输出
							flush();
							this->usDeliveredSN = (PiggyMsg.fecInfo.usGID+1)*getGroupDataLenByGDLI(this->bitGDLI);

						}else{
							this->bitRV=PiggyMsg.fecInfo.ucRV;//更新ucRV；
							//计算冗余数据内容的长度
							UINT16 usRDLen = len - usRtpHeaderLen - ucPiggyMsgLen - 1 - PadLen;//"-1": msg crc 
							if(usRDLen>0){
								write2buff(this->vecAlpha,PiggyMsg.fecInfo.ucRI,pRawRtpData+usRtpHeaderLen,usRDLen);
							}
						}
					}
					
				}
			}
		}else{//这个数据包不是FEC冗余包，直接输出
			this->outputCb(this->pUserData,pRawRtpData,len);
		    return 0;
		}
	}else{//非冗余包(PT!=0)
		if(!this->fecValid){//尚未收到FEC信息，无法解码
			this->outputCb(this->pUserData,pRawRtpData,len);
		    return 0;
		}

		UINT16 dst = (this->usDeliveredSN<=usRtpSN)?(usRtpSN-this->usDeliveredSN):(MAX_SN-this->usDeliveredSN+usRtpSN);
		UINT16 groupDataLen = getGroupDataLenByGDLI(this->bitGDLI);
		UINT16 pos = usRtpSN%groupDataLen;
		UINT16 currGroupId = this->usDeliveredSN/groupDataLen;

		UINT8  cqi_RI,cqi_GDLI;

		if(dst<MAX_SN/2){//在接收窗内
			if(usRtpSN/groupDataLen == currGroupId){//接收的数据包属于当前解码组
				write2buff(this->vecDecodeBuffer,pos,pRawRtpData,len);
				if(this->usDeliveredSN == usRtpSN){
					this->outputCb(this->pUserData,pRawRtpData,len);
					this->usDeliveredSN++;
					if(currGroupId != this->usDeliveredSN/groupDataLen){//当前解码组所有数据都已经输出，
						resetBuffer();
						cqiEvaluater.input(0,this->bitGDLI);
						cqiEvaluater.getEvaluatedResult(cqi_RI,cqi_GDLI);
						this->updateCQI(cqi_GDLI,cqi_RI);
					}
				}
			}else{//数据包不属于当前解码组，说明新的解码组已经到来，当前解码组数据立刻输出
				recoverData();
				flush();//将数据输出

				cqiEvaluater.getEvaluatedResult(cqi_RI,cqi_GDLI);
				this->updateCQI(cqi_GDLI,cqi_RI);

				//下面进入新的解码组
				write2buff(this->vecDecodeBuffer,pos,pRawRtpData,len);//数据保存到缓存，后续数据恢复需要
				currGroupId = usRtpSN/groupDataLen;
				this->usDeliveredSN = currGroupId*groupDataLen;
				
				if(this->usDeliveredSN == usRtpSN){
					this->outputCb(this->pUserData,pRawRtpData,len);
					this->usDeliveredSN++;
				}
			}
			
		}else{//出窗了(一般是网络传输中的乱序导致)，则直接输出，不参与当前的解码组
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
		this->eRI = (lostCnt>=3)?3:lostCnt; //快升
		if(lostCnt>this->eRI){this->eCount ++;}
		this->eChange = tv;
	}else{
		long long diff = DiffTime(this->eChange,tv);
		printf("\r\ndiff=%lld period=%lld",diff,period[this->eRI]);
		if(diff>period[this->eRI]){ //慢降：持续一段时间当前eRI都比丢包数小时才会通知发送端降阶，而且逐阶下降
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


