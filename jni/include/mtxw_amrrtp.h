#ifndef __MTXW_AMR_RTP_H__
#define __MTXW_AMR_RTP_H__

#include "mtxw.h"
#include "mtxw_rtp.h"
#include "mtxw_amrcodec.h"
#include <vector>


typedef struct
{
  UINT8 frame_type;
  UINT8 mode_indication;
  UINT8 mode_request;
  UINT32 Total_number_of_bits;
  UINT32 classA_bits;
  UINT32 classB_bits;
  UINT32 classC_bits;

  const char *strFrameContent;
}AMR_DECRIPTION;


typedef struct
{
    UINT8 cmr :4;
    UINT8 rsv  :4;
}AMR_HEADER_STRU;

typedef struct
{
    UINT8 F:1;   
    UINT8 FT:4;
    UINT8 Q:1;
    UINT8 Rsv:2;
}AMR_TOC_STRU;

typedef struct
{
    UINT8 data[33];
}SPEECH_BLOCK_STRU;

typedef struct MTXW_AMR_FRAME
{
    AMR_HEADER_STRU header;
    std::vector <AMR_TOC_STRU> toc_table;
    std::vector <SPEECH_BLOCK_STRU> frame_block_table;
    
}MTXW_AMR_FRAME_STRU;


class MTXW_AmrRtp : public MTXW_Rtp
{
private:
    MTXW_ENUM_AMR_ALIGN_MODE enAlignMode;
    Mode  mode;
    
public:
    MTXW_AmrRtp();
    virtual ~MTXW_AmrRtp();

    void SetAMR_ALIGN_MODE(MTXW_ENUM_AMR_ALIGN_MODE enMode){   this->enAlignMode = enMode; }
    void SetAMR_BitRateMode(Mode bitRateMode){  this->mode = bitRateMode; }

    INT32 PackRtpPayload(UINT8 *pFrame, UINT32 length, UINT8 *pBuff);

    INT32 UnpackRtpPayload_BE(UINT8* pInputData, UINT32 ulength,  MTXW_AMR_FRAME_STRU *pAmrFrame);
    INT32 UnpackRtpPayload_OA(UINT8* pInputData, UINT32 ulength,  MTXW_AMR_FRAME_STRU *pAmrFrame);

    INT32 Encode_BE_amrframe(MTXW_AMR_FRAME_STRU *pAmrFrame, UINT8 *pBuff, UINT32 length);
    INT32 Encode_OA_amrframe(MTXW_AMR_FRAME_STRU *pAmrFrame, UINT8 *pBuff, UINT32 length);
    INT32 decode_BE_amrframe(UINT8 *pData, UINT32 length, MTXW_AMR_FRAME_STRU *pAmrFrame);
    INT32 decode_OA_amrframe(UINT8 *pData, UINT32 length, MTXW_AMR_FRAME_STRU *pAmrFrame);

};


#endif //__MTXW_AMR_RTP_H__