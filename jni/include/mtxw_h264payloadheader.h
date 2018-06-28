#ifndef __MTXW_H264_PAYLOAD_HEADER_H__
#define __MTXW_H264_PAYLOAD_HEADER_H__

#include "mtxw.h"

#pragma pack(push,1)
enum MTXW_NALU_TYPE_ENUM
{
    /*---H264�淶 Table7-1 NAL unit type codes-----*/
    MTXW_NALU_TYPE_0 = 0,
    MTXW_NALU_TYPE_1_NON_IDR, 
    MTXW_NALU_TYPE_2_PARTTION_A,
    MTXW_NALU_TYPE_3_PARTTION_B,
    MTXW_NALU_TYPE_4_PARTTION_C,
    MTXW_NALU_TYPE_5_IDR, //IDRͼ���slice
    MTXW_NALU_TYPE_6_SEI, //������ǿ��Ϣ��Ԫ
    MTXW_NALU_TYPE_7_SPS,
    MTXW_NALU_TYPE_8_PPS,
    MTXW_NALU_TYPE_9_DELIMITER,//�ֽ��
    MTXW_NALU_TYPE_10_SEQUENCE_END, //���н���
    MTXW_NALU_TYPE_11_STREAM_END,//��������
    MTXW_NALU_TYPE_12_FILLER_DATA, //���
    
    /*---RFC6184 Table 3-------------------------*/
    MTXW_NALU_TYPE_24_STAP_A=24,
    MTXW_NALU_TYPE_25_STAP_B,
    MTXW_NALU_TYPE_26_MTAP16,
    MTXW_NALU_TYPE_27_MTAP24,
    MTXW_NALU_TYPE_28_FU_A,
    MTXW_NALU_TYPE_29_FU_B,
};
    
    

typedef struct MTXW_FU_HEADER
{
    UINT8 bitS : 1;  //the first packet of FU-A picture
    UINT8 bitE : 1;
    UINT8 bitR : 1;
    UINT8 bitsNalu_type:5; //ȡֵ(24-29)��RFC6184 Table 3
}MTXW_FU_HEADER_STRU;

typedef struct MTXW_FU_INDICATOR
{
    UINT8 bitF:1;  //the first packet of FU-A picture
    UINT8 bitsNRI:2;
    UINT8 bitsType:5;// ȡֵ(0-12)����H264�淶 Table7-1 NAL unit type codes
}MTXW_FU_INDICATOR_STRU;

typedef struct MTXW_FU_A
{
    MTXW_FU_INDICATOR_STRU  fu_indicator;
    MTXW_FU_HEADER_STRU fu_header;
	
}MTXW_FU_A_STRU;

typedef MTXW_FU_INDICATOR_STRU MTXW_NALU_HEADER_STRU;

typedef union
{
    MTXW_NALU_HEADER_STRU  nalu_header;
    MTXW_FU_A_STRU fu_A;
	
}MTXW_H264_RTP_PAYLOAD_HEADER_UNION;



#pragma pack(pop)


#endif //__MTXW_H264_PAYLOAD_HEADER_H__

