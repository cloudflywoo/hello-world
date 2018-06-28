#ifndef __MTXW_G729_RTP_H__
#define __MTXW_G729_RTP_H__

#include "mtxw.h"
#include "mtxw_rtp.h"
class MTXW_G729Rtp : public MTXW_Rtp
{
private:

public:
    MTXW_G729Rtp();
    virtual ~MTXW_G729Rtp();
    //INT32 Pack(UINT8 *pFrame, UINT32 length, UINT8 *pBuff);
    //INT32 Unpack(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_Rtp *pRtp){return 0;}

    INT32 Pack_G729_Payload();
    INT32 UnPack_G729_Payload();
};


#endif //__MTXW_G729_RTP_H__