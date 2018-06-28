#ifndef __MTXW_H264_RTP_H__
#define __MTXW_H264_RTP_H__

#include "mtxw.h"
#include "mtxw_rtp.h"
#include "mtxw_comm.h"
#include "mtxw_h264payloadheader.h"

typedef struct MTXW_SPS_PPS_BUFFER
{
    unsigned int uLength;
    unsigned char pData[128];	
}MTXW_SPS_PPS_BUFFER_STRU;

class MTXW_H264Rtp:public MTXW_Rtp
{
private:
    MTXW_H264_RTP_PAYLOAD_HEADER_UNION  payloadHeader;
    UINT32 ulH264FrameRate;

    UINT32 uRotatedAngle;

    MTXW_SPS_PPS_BUFFER SPS;
    MTXW_SPS_PPS_BUFFER PPS;

public:
    MTXW_H264Rtp();
    virtual ~MTXW_H264Rtp();

    void SetH264FrameRate(UINT32 FrameRate);
    INT32 Pack_H264_Payload(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_DATA_BUFFER_STRU* pPackedData);
    INT32 UnPack_H264_Payload(const UINT8* pInputData, MTXW_H264_RTP_PAYLOAD_HEADER_UNION * pPayloadHdr, UINT8 *ucLength);

    UINT32 GetNextNalLocation(unsigned char* pStartBuff,UINT32 ulLength,UINT32 *pulStartCodeLength);
    UINT32 GetNextStartCodeLocation(unsigned char* pStartBuff,UINT32 ulLength);

    INT32 PackH264RtpHeader (UINT8 *pData, UINT32 timeStamp, bool maker,UINT32 ulRotatedAngle);

    INT32 SendSPSorPPS (UINT8 * pPackedData,MTXW_SPS_PPS_BUFFER SPS_PPS,UINT32  timeStamp,UINT32  bitMaker,UINT16 uRotatedAngle);
};


#endif //__MTXW_H264_RTP_H__

