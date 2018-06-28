#ifndef MTXW_SEQUENCEPARAMETERSET_H__
#define MTXW_SEQUENCEPARAMETERSET_H__

#include "mtxw_comm.h"

class SequenceParameterSet {

private:
	
	static const int PROFILE_BASE_LINE = 66;
	static const int PROFILE_MAIN = 77;
	static const int PROFILE_EXTENDED = 88;
	static const int PROFILE_HIGH = 100;
	static const int PROFILE_HIGH_10 = 110;
	static const int PROFILE_HIGH_422 = 122;
	static const int PROFILE_HIGH_444 = 144;

	int nStartBit;
	unsigned char *sps;
	int lengthofsps;
	int profile_idc;
	int constraint_set0_flag;
	int constraint_set1_flag;
	int constraint_set2_flag;
	int constraint_set3_flag;
	int reserved_zero_4bits;
	int level_idc;
	int seq_parameter_set_id;
	int log2_max_frame_num_minus4;
	int pic_order_cnt_type;
	int log2_max_pic_order_cnt_lsb_minus4;
	int num_ref_frames;
	int gaps_in_frame_num_value_allowed_flag;
	int pic_width_in_mbs_minus1;
	int pic_height_in_map_units_minus1;
	int width;
	int height;
	int chroma_format_idc;//色度采样格式
	int bit_depth_luma_minus8;//亮度矩阵比特深度
	int bit_depth_chroma_minus8;//色度矩阵比特深度
	int qpprime_y_zero_transform_bypass_flag;
	int seq_scaling_matrix_present_flag;
	int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int offset_for_top_to_bottom_field;
	int num_ref_frames_in_pic_order_cnt_cycle;
	int* offset_for_ref_frame;
	int length_of_offset_for_ref_frame;
	int offset_for_ref_frame_len;
	int frame_mbs_only_flag;
	int mb_adaptive_frame_field_flag;
	int direct_8x8_inference_flag;
	int frame_cropping_flag;
	int frame_crop_left_offset;
	int frame_crop_right_offset;
	int frame_crop_top_offset;
	int frame_crop_bottom_offset;
	int vui_parameters_present_flag;	
public:
	
        SequenceParameterSet(unsigned char* sps, unsigned int length);
        
        ~SequenceParameterSet()
        {
            mtxwMemFree(offset_for_ref_frame);
            mtxwMemFree(sps);
        }

        void parse(); 

        // u Just returns the BitCount bits of buf and change it to decimal.
        // e.g. BitCount = 4, buf = 01011100, then return 5(0101)
        int u(int bitCount); 

        int Se(int nLen) ;

        // Ue find the num of zeros and get (num+1) bits from the first 1, and
        // change it to decimal
        // e.g. 00110 -> return 6(110)
        int Ue(int nLen); 

	int getWidth() {    return width;   }

	int getHeight() {return height; }
};

#endif //MTXW_SEQUENCEPARAMETERSET_H__

