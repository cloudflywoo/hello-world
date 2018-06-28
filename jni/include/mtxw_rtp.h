#ifndef __MTXW_RTP_H__
#define __MTXW_RTP_H__

#include "mtxw.h"
#include "mtxw_comm.h"
#include <vector>

#define RTP_HEAD_LENGTH  12


class MTXW_Rtp
{
private:

    UINT32  bitsVersion : 2;
    UINT32  bitsP : 1;              /* padding flag */
    UINT32  bitsX:1;                /* header extension flag */
    UINT32  bitsCC:4;               /*CSRC Count*/
    UINT32  SSRC;
    std::vector<UINT32> vecCsrcId;

    struct 
    {
        UINT16 tag;
        UINT16 usLength;
        UINT32 ulOffset; //À©Õ¹Í·ÄÚÈÝÆ«ÒÆ
        
    }headExtension;
    
public:
    UINT32  bitMaker:1;
    UINT32  payloadType:7;
    UINT32  seq:16;
    UINT32  timeStamp;

    MTXW_Rtp();
    virtual ~MTXW_Rtp();
    void SetCsrcId(UINT32 CsrcId)  { vecCsrcId.push_back(CsrcId); }
    std::vector<UINT32> GetCsrcId() {   return vecCsrcId;   }
    void SetPayloadType(UINT8 pt) { payloadType = pt; }
    UINT8 GetPayloadType() {    return payloadType; }
    UINT8 GetVersion() { return bitsVersion; }
    UINT8 GetPadding() { return bitsP; }
    UINT8 SetPadding(UINT8 p) { return bitsP=p; }
    UINT8 GetCSRCCount() { return bitsCC; }

    bool    HasHeaderExtension()    {   return bitsX;    }
    void    SetHeaderExtension(bool m) {   bitsX = m;   }
    
    bool    IsMaked()       {   return  bitMaker;  }
    void    SetbitMaker(bool m) {   bitMaker = m;   }
    void    SetTimeStamp(UINT32 TS) {  timeStamp = TS;}
    void    SetSSRC(UINT32 ssrcValue)    {   SSRC = ssrcValue;}
    UINT32  GetSSRC()   {   return  SSRC;   }   
    UINT16  GetSeq() { return seq; }
    UINT16 GetHeadExtensionTag(){   return this->headExtension.tag; }
    UINT32 GetHeadExtensionLength(){   return this->headExtension.usLength; }
    UINT32 GetHeadExtensionOffset(){   return this->headExtension.ulOffset; }
    INT32 PackRtpHeader (UINT8 *pData, UINT32 timeStamp, bool maker);
    
    INT32 UnpackRtpHeader(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_Rtp *pRtp);
	
};


#endif //__MTXW_RTP_H__