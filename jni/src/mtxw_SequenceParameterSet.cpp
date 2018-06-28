
#include "mtxw_sequenceparameterset.h"
#include <math.h>

SequenceParameterSet::SequenceParameterSet(unsigned char* sps, unsigned int length)
{
    if(sps == NULL)
    {
        return;
    }
    this->sps = (unsigned char *)mtxwMemAlloc(length);
    if(!this->sps)
    {
        //分配内存失败
        MTXW_LOGE("mtxwMemAlloc fail!");
        return;
    }
    mtxwMemCopy(this->sps, sps, length);
    this->lengthofsps = length;
     
    this->nStartBit = 0;
    this->offset_for_ref_frame = 0;
    this->parse();
}	

void SequenceParameterSet::parse() 
{
	//const int spslength = sps.length;
	const int spslength = this->lengthofsps;
	profile_idc = u(8);
	constraint_set0_flag = u(1);// (buf[1] & 0x80)>>7;
	constraint_set1_flag = u(1);// (buf[1] & 0x40)>>6;
	constraint_set2_flag = u(1);// (buf[1] & 0x20)>>5;
	constraint_set3_flag = u(1);// (buf[1] & 0x10)>>4;
	reserved_zero_4bits = u(4);
	level_idc = u(8);
	seq_parameter_set_id = Ue(spslength);
	
	if(profile_idc == PROFILE_BASE_LINE || profile_idc == PROFILE_MAIN) {//H264基线档次 或 H264主档次到获取高度都是相同
		log2_max_frame_num_minus4 = Ue(spslength);
		pic_order_cnt_type = Ue(spslength);
		if (pic_order_cnt_type == 0) {
			log2_max_pic_order_cnt_lsb_minus4 = Ue(spslength);
		} else if (pic_order_cnt_type == 1) {
			int delta_pic_order_always_zero_flag = u(1);
			int offset_for_non_ref_pic = Se(spslength);
			int offset_for_top_to_bottom_field = Se(spslength);
			int num_ref_frames_in_pic_order_cnt_cycle = Ue(spslength);
			
			//int[] offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				/*offset_for_ref_frame[i] = */Se(spslength);
		}
		num_ref_frames = Ue(spslength);
		gaps_in_frame_num_value_allowed_flag = u(1);
		pic_width_in_mbs_minus1 = Ue(spslength);
		pic_height_in_map_units_minus1 = Ue(spslength);
	}
	
	if (profile_idc == PROFILE_HIGH || profile_idc == PROFILE_HIGH_10
			|| profile_idc == PROFILE_HIGH_422
			|| profile_idc == PROFILE_HIGH_444) {// H264 高,高10,高4:2:2,高4:4:4  档次
		chroma_format_idc = Ue(spslength);
		bit_depth_luma_minus8 = Ue(spslength);
		bit_depth_chroma_minus8 = Ue(spslength);
		qpprime_y_zero_transform_bypass_flag = u(1);
		seq_scaling_matrix_present_flag = u(1);
		if (seq_scaling_matrix_present_flag != 0)
		{//FIXME-ME 缩放未完善.
		}
		log2_max_frame_num_minus4 = Ue(spslength);
		pic_order_cnt_type = Ue(spslength);
		if (pic_order_cnt_type == 0)
		{
			log2_max_pic_order_cnt_lsb_minus4 = Ue(spslength);
		}
		else if (pic_order_cnt_type == 1)
		{
			delta_pic_order_always_zero_flag = u(1);
			offset_for_non_ref_pic = Se(spslength);
			offset_for_top_to_bottom_field = Se(spslength);
			num_ref_frames_in_pic_order_cnt_cycle = Se(spslength);
			//offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
			offset_for_ref_frame = (int*)mtxwMemAlloc(num_ref_frames_in_pic_order_cnt_cycle);
			length_of_offset_for_ref_frame = num_ref_frames_in_pic_order_cnt_cycle;
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
			{
				offset_for_ref_frame[i] = Se(spslength);
			}
		}
		num_ref_frames = Ue(spslength);
		gaps_in_frame_num_value_allowed_flag = u(1);
		pic_width_in_mbs_minus1 = Ue(spslength);
		pic_height_in_map_units_minus1 = Ue(spslength);
		frame_mbs_only_flag = u(1);
		if (frame_mbs_only_flag == 0) 
		{
			mb_adaptive_frame_field_flag = u(1);
		}
		direct_8x8_inference_flag = u(1);
		frame_cropping_flag = u(1);
		if (frame_cropping_flag != 0)
		{
			frame_crop_left_offset = Ue(spslength);
			frame_crop_right_offset = Ue(spslength);
			frame_crop_top_offset = Ue(spslength);
			frame_crop_bottom_offset = Ue(spslength);
		}
		vui_parameters_present_flag = u(1);
	}
	
	width = (pic_width_in_mbs_minus1 + 1) * 16;
	height = (pic_height_in_map_units_minus1 + 1) * 16;
}

// u Just returns the BitCount bits of buf and change it to decimal.
// e.g. BitCount = 4, buf = 01011100, then return 5(0101)
int SequenceParameterSet::u(int bitCount) 
 {
	int dwRet = 0;
	for (int i = 0; i < bitCount; i++) {
		dwRet <<= 1;
		if ((sps[nStartBit / 8] & (0x80 >> (nStartBit % 8))) > 0) {

			dwRet += 1;

		}
		nStartBit++;
	}
	return dwRet;
}

int SequenceParameterSet::Se(int nLen) 
{
	int UeVal = Ue(nLen);
	double k = UeVal;
	int nValue = (int) ceil(k / 2);// ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}

// Ue find the num of zeros and get (num+1) bits from the first 1, and
// change it to decimal
// e.g. 00110 -> return 6(110)
int SequenceParameterSet::Ue(int nLen) 
{
	// 计算0bit的个数
	int nZeroNum = 0;

	while (nStartBit < nLen * 8) {
		if ((sps[nStartBit / 8] & (0x80 >> (nStartBit % 8))) > 0) // &:按位与，%取余
		{

			break;

		}

		nZeroNum++;
		nStartBit++;
	}

	nStartBit++;

	// 计算结果

	int dwRet = 0;

	for (int i = 0; i < nZeroNum; i++) {
		dwRet <<= 1;

		if ((sps[nStartBit / 8] & (0x80 >> (nStartBit % 8))) > 0) {

			dwRet += 1;

		}

		nStartBit++;
	}

	return (1 << nZeroNum) - 1 + dwRet;
}


