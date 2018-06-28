
#include "mtxw_amrrtp.h"
#include "mtxw_comm.h"
#include "mtxw_amrcodec.h"
#include "mtxw_g729codec.h"

AMR_DECRIPTION gRtpAmrDscrpt[] =
{
  {0,  0,  0,  95,  42, 53,  0,  "AMR 4.75 kbit/s"},
  {1,  1,  1,  103, 49, 54,  0,  "AMR 5.15 kbit/s"},
  {2,  2,  2,  118, 55, 63,  0,  "AMR 5.90 kbit/s"},
  {3,  3,  3,  134, 58, 76,  0,  "AMR 6.70 kbit/s"},
  {4,  4,  4,  148, 61, 87,  0,  "AMR 7.40 kbit/s"},
  {5,  5,  5,  159, 75, 84,  0,  "AMR 7.95 kbit/s"},
  {6,  6,  6,  204, 65, 99,  40, "AMR 10.2 kbit/s"},
  {7,  7,  7,  244, 81, 103, 60, "AMR 12.2 kbit/s"},
  {8,  255,255,0,   0,  0,   0,  "AMR SID"},
  {9,  255,255,0,   0,  0,   0,  "GSM-EFR SID"},
  {10, 255,255,0,   0,  0,   0,  "TDMA-EFR SID"},
  {11, 255,255,0,   0,  0,   0,  "PDC-EFR SID"},
  {12, 255,255,0,   0,  0,   0,  "for futrue use"},
  {13, 255,255,0,   0,  0,   0,  "for futrue use"},
  {14, 255,255,0,   0,  0,   0,  "for futrue use"},
  {15, 255,255,0,   0,  0,   0,  "NO Data"}
};

MTXW_AmrRtp::MTXW_AmrRtp()
{
       MTXW_FUNC_TRACE();
	this->enAlignMode = MTXW_AMR_BE;
	this->payloadType = 126;
	this->mode = MR122;
}

MTXW_AmrRtp::~MTXW_AmrRtp()
{
	MTXW_FUNC_TRACE();
}
INT32 MTXW_AmrRtp::UnpackRtpPayload_BE(UINT8* pInputData, UINT32 ulength,  MTXW_AMR_FRAME_STRU *pAmrFrame)
{
       MTXW_FUNC_TRACE();
	if(NULLPTR == pInputData || NULLPTR == pAmrFrame)
	{
		return -1;
	}
	
      return decode_BE_amrframe(pInputData, ulength, pAmrFrame);
      
}

INT32 MTXW_AmrRtp::UnpackRtpPayload_OA(UINT8* pInputData, UINT32 ulength,  MTXW_AMR_FRAME_STRU *pAmrFrame)
{
       MTXW_FUNC_TRACE();
	if(NULLPTR == pInputData || NULLPTR == pAmrFrame)
	{
		return -1;
	}
	
      return decode_OA_amrframe(pInputData, ulength, pAmrFrame);
      
}
INT32 MTXW_AmrRtp:: PackRtpPayload(UINT8 *pAmrFrame, UINT32 payloadLen, UINT8 *pBuff)
{
	MTXW_FUNC_TRACE();
	INT32 lengthOfAmr = 0;
	UINT32 payloadOffset = 0;
	UINT8 temp0 =0;
	UINT8 temp1 = 0;
	UINT8 FT = 0;
	UINT8 ucToc = 0;
	UINT32 ulFrameBitLen = 0;
	UINT32 i;
	
	if(NULLPTR == pAmrFrame || NULLPTR == pBuff)
	{
		return -1;
	}

        
	if(this->enAlignMode == MTXW_AMR_BE)
	{
	       /*
	       pAmrFrame
              0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 
              |    toc     | speech data ...
              |F| FT  |Q|00|payload.....
	       */
		mtxwMemCopy(pBuff, pAmrFrame, payloadLen);
		temp0 = pBuff[payloadOffset];
		ucToc = temp0 >> 2;//toc头
		FT = (pBuff[payloadOffset] >> 3)&0xF;		
		pBuff[payloadOffset] = ((UINT8)this->mode) << 4;//cmr
		pBuff[payloadOffset] |= (ucToc >> 2) &0x0F;//cmr: 4bit, F+FT:1bit+3bit
		payloadOffset++;

		temp0 = pBuff[payloadOffset];
		temp1 = pBuff[payloadOffset];

		pBuff[payloadOffset] = (ucToc & 0x03) << 6;// 2bit(FT_1bit + Q_1bit)

		for(i = 1; i< payloadLen; i++)
		{
			temp0 = temp1;
			pBuff[payloadOffset] |= temp0 >> 2;// 6bits
			temp1 = pBuff[payloadOffset + 1];
			pBuff[payloadOffset + 1] = temp0<<6;// 2bits
			payloadOffset++;
		}

		
		ulFrameBitLen = 10; // 10bit(cmr_4 + F_1 + FT_4 + Q_1)
		ulFrameBitLen += gRtpAmrDscrpt[FT].Total_number_of_bits;

		if(ulFrameBitLen > payloadLen*8)
		{
			payloadLen++;
		}

		lengthOfAmr = payloadLen;
		
		 
	}
	else if(this->enAlignMode == MTXW_AMR_OA_MODE)
	{
		mtxwMemCopy(pBuff+1, pAmrFrame, payloadLen);
		pBuff[0] = ((UINT8)this->mode) << 4;

		lengthOfAmr = payloadLen + 1;
	}
	else
	{
		return -1;
	}

	return lengthOfAmr;


	
}

INT32 MTXW_AmrRtp::Encode_BE_amrframe(MTXW_AMR_FRAME_STRU *pAmrFrame, UINT8 *pBuff, UINT32 length)
{	
	
       MTXW_FUNC_TRACE();
	UINT32 cntbits = 0;
	UINT32 i, j, block_bits, block_bytes, block_cnt, left_bits;
	const UINT32 toc_bits = 6;
	UINT8 tmp, toc, *pData;

	if (0 == pAmrFrame || 0 == pBuff)
	{
		return -1;
	}

	mtxwMemSet(pBuff, 0, length);

	//-1--encode header----------------------------------------------------
	pBuff[0] = pAmrFrame->header.cmr << 4;
	cntbits += 4;

	//-2--encode toc table-------------------------------------------------
	for (i = 0; i < pAmrFrame->toc_table.size(); i++) 
	{
		//-construct toc = 00xx xxxx
		toc = pAmrFrame->toc_table[i].F << 5 | pAmrFrame->toc_table[i].FT << 1
				| pAmrFrame->toc_table[i].Q;

		left_bits = 8 - cntbits % 8;

		if (left_bits >= toc_bits)
		{
			//当前字节所剩余的bits足够放置toc
			pBuff[cntbits >> 3] |= (toc << (left_bits - toc_bits));
		} else {
			pBuff[cntbits >> 3] |= toc >> (toc_bits - left_bits);
			pBuff[(cntbits >> 3) + 1] |= toc << (8 - (toc_bits - left_bits));

		}

		cntbits += toc_bits;

	}

	//-3--encode frame block table-----------------------------------------
	block_cnt = 0;
	for (i = 0; i < pAmrFrame->toc_table.size(); i++) 
	{
		if (pAmrFrame->toc_table[i].FT > 7) 
		{
			//FT>7，说明只有TOC头，没有speech data block
			continue;
		}

		pData = pAmrFrame->frame_block_table[block_cnt++].data;

		block_bits = gRtpAmrDscrpt[pAmrFrame->toc_table[i].FT].Total_number_of_bits;

		block_bytes = block_bits >> 3;

		for (j = 0; j < block_bytes; j++) 
		{
			//处理每一个字节的speech data
			tmp = pData[j];

			left_bits = 8 - cntbits % 8;
			if (left_bits == 8)
			{           //当前字节所剩余的bits足够放置整个tmp
				pBuff[cntbits >> 3] = tmp;
			} 
			else
			{           //当前字节所剩余的bits不足够放置整个tmp
				pBuff[cntbits >> 3] |= tmp >> (8 - left_bits);
				pBuff[(cntbits >> 3) + 1] |= tmp << left_bits;

			}

			cntbits += 8;

		}

		if (block_bits % 8 > 0) 
		{
			tmp = pData[block_bytes];           //最后一个字节里的数据

			//处理最后不完整字节的数据
			left_bits = 8 - cntbits % 8;
			if (left_bits >= block_bits % 8) 
			{
				//当前字节所剩余的bits足够放置最后未编码的bits数
				pBuff[cntbits >> 3] |= tmp >> (cntbits % 8);
			}
			else 
			{
				pBuff[cntbits >> 3] |= tmp >> (cntbits % 8);
				pBuff[(cntbits >> 3) + 1] = tmp << left_bits;

			}

			cntbits += block_bits % 8;
		}

	}

	return (cntbits + 7) >> 3;
}

INT32 MTXW_AmrRtp::Encode_OA_amrframe(MTXW_AMR_FRAME_STRU * pAmrFrame, UINT8 * pBuff, UINT32 length)
{
	
       MTXW_FUNC_TRACE();
       
	unsigned int cntbytes = 0;
	unsigned int i, j, block_bits, block_bytes, block_cnt;
	UINT8 toc, *pData;

	if (0 == pAmrFrame || 0 == pBuff) 
	{
		return -1;
	}

	mtxwMemSet(pBuff, 0, length);

	//-1--encode header----------------------------------------------------
	pBuff[cntbytes++] = pAmrFrame->header.cmr << 4;

	//-2--encode toc table-------------------------------------------------
	for (i = 0; i < pAmrFrame->toc_table.size(); i++) 
	{
		//-construct toc = xxxx xx00
		toc = pAmrFrame->toc_table[i].F << 7 | pAmrFrame->toc_table[i].FT << 3
				| pAmrFrame->toc_table[i].Q << 2;

		pBuff[cntbytes++] = toc;

	}

	//-3--encode frame block table-----------------------------------------
	block_cnt = 0;
	for (i = 0; i < pAmrFrame->toc_table.size(); i++) 
	{
		if (pAmrFrame->toc_table[i].FT > 7) 
		{
			//FT>7，说明只有TOC头，没有speech data block
			continue;
		}

		pData = pAmrFrame->frame_block_table[block_cnt++].data;

		block_bits = gRtpAmrDscrpt[pAmrFrame->toc_table[i].FT].Total_number_of_bits;

		block_bytes = (block_bits + 7) >> 3;

		for (j = 0; j < block_bytes; j++) 
		{
			//处理每一个字节的speech data
			pBuff[cntbytes++] = pData[j];
		}

	}

	return cntbytes;


}

INT32 MTXW_AmrRtp::decode_BE_amrframe(UINT8 *pData, UINT32 length, MTXW_AMR_FRAME_STRU *pAmrFrame)
{
       MTXW_FUNC_TRACE();
	AMR_TOC_STRU toc;

	SPEECH_BLOCK_STRU block;

	unsigned int cntbits = 0, cntbytes = 0;
	unsigned int i, j, block_bits;
	UINT8 tmp;

	if (0 == pData || 0 == pAmrFrame || 0 == length) 
	{
		return -1;
	}

	//-1--decode header-------------------------------------------------
	pAmrFrame->header.cmr = pData[cntbits >> 3] >> 4;
	pAmrFrame->header.rsv = 0;
	cntbits += 4;
	//cntbytes = cntbits>>3; //cntbytes = cntbits/8

	//-2--decode Toc table----------------------------------------------
	while ((cntbits >> 3) < length) 
	{
		//--2.1-decode F---
		toc.F = (pData[cntbits >> 3] >> (7 - cntbits % 8)) & 0x01;
		cntbits++;

		//--2.2-decode FT--
		toc.FT = (((pData[cntbits >> 3] >> (7 - cntbits % 8)) & 0x01) << 3)
				| (((pData[(cntbits + 1) >> 3] >> (7 - (cntbits + 1) % 8)) & 0x01)
						<< 2)
				| (((pData[(cntbits + 2) >> 3] >> (7 - (cntbits + 2) % 8)) & 0x01)
						<< 1)
				| ((pData[(cntbits + 3) >> 3] >> (7 - (cntbits + 3) % 8)) & 0x01);
		toc.Rsv = 0;
		cntbits += 4;

		//--2.3-decode Q
		toc.Q = (pData[cntbits >> 3] >> (7 - cntbits % 8)) & 0x01;
		cntbits++;

		pAmrFrame->toc_table.push_back(toc);

		if (toc.F == 0) 
		{
			//this is the last frame
			break;
		}
	}

	/*增加保护-20160922*/
	if (cntbits > length * 8) 
	{
		return -1;
	}

	//-3--decode frame block table--------------------------------------
	for (i = 0; i < pAmrFrame->toc_table.size(); i++) 
	{

		/*201609211742-modified by wyf  Begin*/
		/*
		 将插入block数据提前，对于toc.ft>7的情况下，插入一个对应空block，使得toc和block一一对应，便于上层访问数据
		 */
		memset(&block, 0, sizeof(block));
		j = 0; //block里的数据字节数
		pAmrFrame->frame_block_table.push_back(block); //插入一个speech block

		if (pAmrFrame->toc_table[i].FT > 7)
		{
			// not a speech block
			//break;
			continue;
		}
		/*201609211742-modified by wyf  end*/

		//block.clear();
		block_bits = gRtpAmrDscrpt[pAmrFrame->toc_table[i].FT].Total_number_of_bits;
		if (block_bits > 0) 
		{
			/*增加保护-20160922*/
			if ((cntbits + block_bits) > length * 8) 
			{
				//进到这里，说明数据包错误
				return -1;
			}
		}

		while (block_bits > 0) 
		{

			if (block_bits >= 8) 
			{
				if (cntbits % 8 == 0) 
				{
					tmp = pData[cntbits >> 3];
					cntbits += 8;

				}
				else 
				{
					tmp = (pData[cntbits >> 3] << (cntbits % 8)) | //(8-cntbits%8)  --高字节的bit数
							(pData[(cntbits >> 3) + 1] >> (8 - cntbits % 8));
					cntbits += 8;
				}
				//pAmrFrame->frame_block_table[i].push_back(tmp);
				pAmrFrame->frame_block_table[i].data[j++] = tmp;
				block_bits -= 8;
			} 
			else 
			{
				if (block_bits <= (8 - cntbits % 8)) 
				{
					tmp = (pData[cntbits >> 3] << (cntbits % 8));
				} else {
					tmp = (pData[cntbits >> 3] << (cntbits % 8)) | //(8-cntbits%8)  --高字节的bit数
							(pData[(cntbits >> 3) + 1] >> (8 - cntbits % 8));
				}
				tmp = (tmp >> (8 - block_bits)) << (8 - block_bits);
				//pAmrFrame->frame_block_table[i].push_back(tmp);
				pAmrFrame->frame_block_table[i].data[j++] = tmp;
				cntbits += block_bits;

				block_bits = 0;
			}
		}

	}

	if(pAmrFrame->toc_table.size()!=pAmrFrame->toc_table.size())
	{
		return -1;
	}

	return 0;

}

INT32 MTXW_AmrRtp::decode_OA_amrframe(UINT8 * pData, UINT32 length, MTXW_AMR_FRAME_STRU * pAmrFrame)
{	
       MTXW_FUNC_TRACE();
	AMR_TOC_STRU toc;

	SPEECH_BLOCK_STRU block;

	unsigned int cntbytes = 0;
	unsigned int i, block_bytes = 0;
	int BlocksBytes=0;
	UINT8 tmp;

	if (0 == pData || 0 == pAmrFrame || 0 == length)
	{
		//ALOGD("0==pData || 0==pAmrFrame || 0==length -1");
		return -1;
	}

	//--1--decode header--------------------------------------
	pAmrFrame->header.cmr = pData[cntbytes] >> 4;
	pAmrFrame->header.rsv = 0;
	cntbytes += 1;

	//--2--decode Toc Table-----------------------------------
	do {
		tmp = pData[cntbytes];
		toc.F = tmp >> 7;
		toc.FT = (tmp >> 3) & 0x0F;
		toc.Q = (tmp >> 2) & 0x01;
		toc.Rsv = 0;
		pAmrFrame->toc_table.push_back(toc);
		cntbytes++;

		BlocksBytes += (gRtpAmrDscrpt[toc.FT].Total_number_of_bits + 7) >> 3;

	} while (toc.F == 1 && cntbytes < length);

	/*20160922--增加保护*/
	if (cntbytes > length) 
	{
		//ALOGD("invalid packet 1");
		pAmrFrame->toc_table.clear();
		return -1;
	}

	if(cntbytes + BlocksBytes > length)
	{
		//ALOGD("invalid packet 2");
		pAmrFrame->toc_table.clear();
		return -1;
	}

	//-3--decode frame block table--------------------------------------
	for (i = 0; (i < pAmrFrame->toc_table.size() && cntbytes < length); i++) 
	{
		/*201609211742-modified by wyf  Begin*/
		/*
		 将插入block数据提前，对于toc.ft>7的情况下，插入一个对应空block，使得toc和block一一对应，便于上层访问数据
		 */
		mtxwMemSet(&block, 0, sizeof(block));
		pAmrFrame->frame_block_table.push_back(block);           //插入一个speech block

		if (pAmrFrame->toc_table[i].FT > 7) 
		{
			// not a speech block
			//break;
			continue;
		}
		/*201609211742-modified by wyf  end*/

		if ((cntbytes + block_bytes) > length) 
		{
			//ALOGD("invalid packet 3");
			//走到这里，说明数据错误
			pAmrFrame->toc_table.clear();
			pAmrFrame->frame_block_table.clear();
			return -1;
		}

		block_bytes =
				(gRtpAmrDscrpt[pAmrFrame->toc_table[i].FT].Total_number_of_bits + 7)
						>> 3;

		mtxwMemCopy(pAmrFrame->frame_block_table[i].data, &pData[cntbytes], block_bytes);
		cntbytes += block_bytes;

	}

	if(pAmrFrame->frame_block_table.size()!=pAmrFrame->toc_table.size())
	{
		//ALOGD("invalid packet 4");
		pAmrFrame->toc_table.clear();
		pAmrFrame->frame_block_table.clear();

		return -1;
	}

	return 0;

}


