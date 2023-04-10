/*
 * h264bitstream - a library for reading and writing H.264 video
 * Copyright (C) 2005-2007 Auroras Entertainment, LLC
 * Copyright (C) 2008-2011 Avail-TVN
 *
 * Written by Alex Izvorski <aizvorski@gmail.com> and Alex Giladi <alex.giladi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _H264_BS_H
#define _H264_BS_H        1

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	uint8_t* start;
	uint8_t* p;
	uint8_t* end;
	int bits_left;
} bs_t;

#define _OPTIMIZE_BS_ 1

#if ( _OPTIMIZE_BS_ > 0 )
#ifndef FAST_U8
#define FAST_U8
#endif
#endif


static bs_t* bs_new(uint8_t* buf, size_t size);
static void bs_free(bs_t* b);
static bs_t* bs_clone(bs_t* dest, const bs_t* src);
static bs_t* bs_init(bs_t* b, uint8_t* buf, size_t size);
static uint32_t bs_byte_aligned(bs_t* b);
static int bs_eof(bs_t* b);
static int bs_overrun(bs_t* b);
static int bs_pos(bs_t* b);

static uint32_t bs_peek_u1(bs_t* b);
static uint32_t bs_read_u1(bs_t* b);
static uint32_t bs_read_u(bs_t* b, int n);
static uint32_t bs_read_f(bs_t* b, int n);
static uint32_t bs_read_u8(bs_t* b);
static uint32_t bs_read_ue(bs_t* b);
static int32_t  bs_read_se(bs_t* b);

static void bs_write_u1(bs_t* b, uint32_t v);
static void bs_write_u(bs_t* b, int n, uint32_t v);
static void bs_write_f(bs_t* b, int n, uint32_t v);
static void bs_write_u8(bs_t* b, uint32_t v);
static void bs_write_ue(bs_t* b, uint32_t v);
static void bs_write_se(bs_t* b, int32_t v);

static int bs_read_bytes(bs_t* b, uint8_t* buf, int len);
static int bs_write_bytes(bs_t* b, uint8_t* buf, int len);
static int bs_skip_bytes(bs_t* b, int len);
static uint32_t bs_next_bits(bs_t* b, int nbits);
// IMPLEMENTATION

static inline bs_t* bs_init(bs_t* b, uint8_t* buf, size_t size)
{
	b->start = buf;
	b->p = buf;
	b->end = buf + size;
	b->bits_left = 8;
	return b;
}

static inline bs_t* bs_new(uint8_t* buf, size_t size)
{
	bs_t* b = (bs_t*)malloc(sizeof(bs_t));
	bs_init(b, buf, size);
	return b;
}

static inline void bs_free(bs_t* b)
{
	free(b);
}

static inline bs_t* bs_clone(bs_t* dest, const bs_t* src)
{
	dest->start = src->p;
	dest->p = src->p;
	dest->end = src->end;
	dest->bits_left = src->bits_left;
	return dest;
}

static inline uint32_t bs_byte_aligned(bs_t* b)
{
	return (b->bits_left == 8);
}

static inline int bs_eof(bs_t* b) { if (b->p >= b->end) { return 1; } else { return 0; } }

static inline int bs_overrun(bs_t* b) { if (b->p > b->end) { return 1; } else { return 0; } }

static inline int bs_pos(bs_t* b) { if (b->p > b->end) { return (b->end - b->start); } else { return (b->p - b->start); } }

static inline int bs_bytes_left(bs_t* b) { return (b->end - b->p); }

static inline uint32_t bs_read_u1(bs_t* b)
{
	uint32_t r = 0;

	b->bits_left--;

	if (!bs_eof(b))
	{
		r = ((*(b->p)) >> b->bits_left) & 0x01;
	}

	if (b->bits_left == 0) { b->p++; b->bits_left = 8; }

	return r;
}

static inline void bs_skip_u1(bs_t* b)
{
	b->bits_left--;
	if (b->bits_left == 0) { b->p++; b->bits_left = 8; }
}

static inline uint32_t bs_peek_u1(bs_t* b)
{
	uint32_t r = 0;

	if (!bs_eof(b))
	{
		r = ((*(b->p)) >> (b->bits_left - 1)) & 0x01;
	}
	return r;
}


static inline uint32_t bs_read_u(bs_t* b, int n)
{
	uint32_t r = 0;
	int i;
	for (i = 0; i < n; i++)
	{
		r |= (bs_read_u1(b) << (n - i - 1));
	}
	return r;
}

static inline void bs_skip_u(bs_t* b, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		bs_skip_u1(b);
	}
}

static inline uint32_t bs_read_f(bs_t* b, int n) { return bs_read_u(b, n); }

static inline uint32_t bs_read_u8(bs_t* b)
{
#ifdef FAST_U8
	if (b->bits_left == 8 && !bs_eof(b)) // can do fast read
	{
		uint32_t r = b->p[0];
		b->p++;
		return r;
	}
#endif
	return bs_read_u(b, 8);
}

static inline uint32_t bs_read_ue(bs_t* b)
{
	int32_t r = 0;
	int i = 0;

	while ((bs_read_u1(b) == 0) && (i < 32) && (!bs_eof(b)))
	{
		i++;
	}
	r = bs_read_u(b, i);
	r += (1 << i) - 1;
	return r;
}

static inline int32_t bs_read_se(bs_t* b)
{
	int32_t r = bs_read_ue(b);
	if (r & 0x01)
	{
		r = (r + 1) / 2;
	}
	else
	{
		r = -(r / 2);
	}
	return r;
}


static inline void bs_write_u1(bs_t* b, uint32_t v)
{
	b->bits_left--;

	if (!bs_eof(b))
	{
		// FIXME this is slow, but we must clear bit first
		// is it better to memset(0) the whole buffer during bs_init() instead? 
		// if we don't do either, we introduce pretty nasty bugs
		(*(b->p)) &= ~(0x01 << b->bits_left);
		(*(b->p)) |= ((v & 0x01) << b->bits_left);
	}

	if (b->bits_left == 0) { b->p++; b->bits_left = 8; }
}

static inline void bs_write_u(bs_t* b, int n, uint32_t v)
{
	int i;
	for (i = 0; i < n; i++)
	{
		bs_write_u1(b, (v >> (n - i - 1)) & 0x01);
	}
}

static inline void bs_write_f(bs_t* b, int n, uint32_t v) { bs_write_u(b, n, v); }

static inline void bs_write_u8(bs_t* b, uint32_t v)
{
#ifdef FAST_U8
	if (b->bits_left == 8 && !bs_eof(b)) // can do fast write
	{
		b->p[0] = v;
		b->p++;
		return;
	}
#endif
	bs_write_u(b, 8, v);
}

static inline void bs_write_ue(bs_t* b, uint32_t v)
{
	static const int len_table[256] =
	{
		1,
		1,
		2,2,
		3,3,3,3,
		4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
		8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	};

	int len;

	if (v == 0)
	{
		bs_write_u1(b, 1);
	}
	else
	{
		v++;

		if (v >= 0x01000000)
		{
			len = 24 + len_table[v >> 24];
		}
		else if (v >= 0x00010000)
		{
			len = 16 + len_table[v >> 16];
		}
		else if (v >= 0x00000100)
		{
			len = 8 + len_table[v >> 8];
		}
		else
		{
			len = len_table[v];
		}

		bs_write_u(b, 2 * len - 1, v);
	}
}

static inline void bs_write_se(bs_t* b, int32_t v)
{
	if (v <= 0)
	{
		bs_write_ue(b, -v * 2);
	}
	else
	{
		bs_write_ue(b, v * 2 - 1);
	}
}

static inline int bs_read_bytes(bs_t* b, uint8_t* buf, int len)
{
	int actual_len = len;
	if (b->end - b->p < actual_len) { actual_len = b->end - b->p; }
	if (actual_len < 0) { actual_len = 0; }
	memcpy(buf, b->p, actual_len);
	if (len < 0) { len = 0; }
	b->p += len;
	return actual_len;
}

static inline int bs_write_bytes(bs_t* b, uint8_t* buf, int len)
{
	int actual_len = len;
	if (b->end - b->p < actual_len) { actual_len = b->end - b->p; }
	if (actual_len < 0) { actual_len = 0; }
	memcpy(b->p, buf, actual_len);
	if (len < 0) { len = 0; }
	b->p += len;
	return actual_len;
}

static inline int bs_skip_bytes(bs_t* b, int len)
{
	int actual_len = len;
	if (b->end - b->p < actual_len) { actual_len = b->end - b->p; }
	if (actual_len < 0) { actual_len = 0; }
	if (len < 0) { len = 0; }
	b->p += len;
	return actual_len;
}

static inline uint32_t bs_next_bits(bs_t* bs, int nbits)
{
	bs_t b;
	bs_clone(&b, bs);
	return bs_read_u(&b, nbits);
}

static inline uint64_t bs_next_bytes(bs_t* bs, int nbytes)
{
	int i = 0;
	uint64_t val = 0;

	if ((nbytes > 8) || (nbytes < 1)) { return 0; }
	if (bs->p + nbytes > bs->end) { return 0; }

	for (i = 0; i < nbytes; i++) { val = (val << 8) | bs->p[i]; }
	return val;
}

#define bs_print_state(b) fprintf( stderr,  "%s:%d@%s: b->p=0x%02hhX, b->left = %d\n", __FILE__, __LINE__, __FUNCTION__, *b->p, b->bits_left )


namespace h264 {
	typedef struct
	{
		int profile_idc;
		int constraint_set0_flag;
		int constraint_set1_flag;
		int constraint_set2_flag;
		int constraint_set3_flag;
		int constraint_set4_flag;
		int constraint_set5_flag;
		int reserved_zero_2bits;
		int level_idc;
		int seq_parameter_set_id;
		int chroma_format_idc;
		int residual_colour_transform_flag;
		int bit_depth_luma_minus8;
		int bit_depth_chroma_minus8;
		int qpprime_y_zero_transform_bypass_flag;
		int seq_scaling_matrix_present_flag;
		int seq_scaling_list_present_flag[8];
		int ScalingList4x4[6][16];
		int UseDefaultScalingMatrix4x4Flag[6];
		int ScalingList8x8[2][64];
		int UseDefaultScalingMatrix8x8Flag[2];
		int log2_max_frame_num_minus4;
		int pic_order_cnt_type;
		int log2_max_pic_order_cnt_lsb_minus4;
		int delta_pic_order_always_zero_flag;
		int offset_for_non_ref_pic;
		int offset_for_top_to_bottom_field;
		int num_ref_frames_in_pic_order_cnt_cycle;
		int offset_for_ref_frame[256];
		int num_ref_frames;
		int gaps_in_frame_num_value_allowed_flag;
		int pic_width_in_mbs_minus1;
		int pic_height_in_map_units_minus1;
		int frame_mbs_only_flag;
		int mb_adaptive_frame_field_flag;
		int direct_8x8_inference_flag;
		int frame_cropping_flag;
		int frame_crop_left_offset;
		int frame_crop_right_offset;
		int frame_crop_top_offset;
		int frame_crop_bottom_offset;
		int vui_parameters_present_flag;

		struct
		{
			int aspect_ratio_info_present_flag;
			int aspect_ratio_idc;
			int sar_width;
			int sar_height;
			int overscan_info_present_flag;
			int overscan_appropriate_flag;
			int video_signal_type_present_flag;
			int video_format;
			int video_full_range_flag;
			int colour_description_present_flag;
			int colour_primaries;
			int transfer_characteristics;
			int matrix_coefficients;
			int chroma_loc_info_present_flag;
			int chroma_sample_loc_type_top_field;
			int chroma_sample_loc_type_bottom_field;
			int timing_info_present_flag;
			int num_units_in_tick;
			int time_scale;
			int fixed_frame_rate_flag;
			int nal_hrd_parameters_present_flag;
			int vcl_hrd_parameters_present_flag;
			int low_delay_hrd_flag;
			int pic_struct_present_flag;
			int bitstream_restriction_flag;
			int motion_vectors_over_pic_boundaries_flag;
			int max_bytes_per_pic_denom;
			int max_bits_per_mb_denom;
			int log2_max_mv_length_horizontal;
			int log2_max_mv_length_vertical;
			int num_reorder_frames;
			int max_dec_frame_buffering;
		} vui;

		struct
		{
			int cpb_cnt_minus1;
			int bit_rate_scale;
			int cpb_size_scale;
			int bit_rate_value_minus1[32]; // up to cpb_cnt_minus1, which is <= 31
			int cpb_size_value_minus1[32];
			int cbr_flag[32];
			int initial_cpb_removal_delay_length_minus1;
			int cpb_removal_delay_length_minus1;
			int dpb_output_delay_length_minus1;
			int time_offset_length;
		} hrd;

	} sps_t;

	typedef struct
	{
		int pic_parameter_set_id;
		int seq_parameter_set_id;
		int entropy_coding_mode_flag;
		int pic_order_present_flag;
		int num_slice_groups_minus1;
		int slice_group_map_type;
		int run_length_minus1[8]; // up to num_slice_groups_minus1, which is <= 7 in Baseline and Extended, 0 otheriwse
		int top_left[8];
		int bottom_right[8];
		int slice_group_change_direction_flag;
		int slice_group_change_rate_minus1;
		int pic_size_in_map_units_minus1;
		int slice_group_id[256]; // FIXME what size?
		int num_ref_idx_l0_active_minus1;
		int num_ref_idx_l1_active_minus1;
		int weighted_pred_flag;
		int weighted_bipred_idc;
		int pic_init_qp_minus26;
		int pic_init_qs_minus26;
		int chroma_qp_index_offset;
		int deblocking_filter_control_present_flag;
		int constrained_intra_pred_flag;
		int redundant_pic_cnt_present_flag;

		// set iff we carry any of the optional headers
		int _more_rbsp_data_present;

		int transform_8x8_mode_flag;
		int pic_scaling_matrix_present_flag;
		int pic_scaling_list_present_flag[8];
		int ScalingList4x4[6][16];
		int UseDefaultScalingMatrix4x4Flag[6];
		int ScalingList8x8[2][64];
		int UseDefaultScalingMatrix8x8Flag[2];
		int second_chroma_qp_index_offset;
	} pps_t;

#define SAR_Extended      255        // Extended_SAR
	inline void read_scaling_list(bs_t* b, int* scalingList, int sizeOfScalingList, int* useDefaultScalingMatrixFlag)
	{
		// NOTE need to be able to set useDefaultScalingMatrixFlag when reading, hence passing as pointer
		int lastScale = 8;
		int nextScale = 8;
		int delta_scale;
		for (int j = 0; j < sizeOfScalingList; j++)
		{
			if (nextScale != 0)
			{
				if (0)
				{
					nextScale = scalingList[j];
					if (useDefaultScalingMatrixFlag[0]) { nextScale = 0; }
					delta_scale = (nextScale - lastScale) % 256;
				}

				delta_scale = bs_read_se(b);

				if (1)
				{
					nextScale = (lastScale + delta_scale + 256) % 256;
					useDefaultScalingMatrixFlag[0] = (j == 0 && nextScale == 0);
				}
			}
			if (1)
			{
				scalingList[j] = (nextScale == 0) ? lastScale : nextScale;
			}
			lastScale = scalingList[j];
		}
	}
	inline void read_hrd_parameters(sps_t* sps, bs_t* b)
	{
		sps->hrd.cpb_cnt_minus1 = bs_read_ue(b);
		sps->hrd.bit_rate_scale = bs_read_u(b, 4);
		sps->hrd.cpb_size_scale = bs_read_u(b, 4);
		for (int SchedSelIdx = 0; SchedSelIdx <= sps->hrd.cpb_cnt_minus1; SchedSelIdx++)
		{
			sps->hrd.bit_rate_value_minus1[SchedSelIdx] = bs_read_ue(b);
			sps->hrd.cpb_size_value_minus1[SchedSelIdx] = bs_read_ue(b);
			sps->hrd.cbr_flag[SchedSelIdx] = bs_read_u1(b);
		}
		sps->hrd.initial_cpb_removal_delay_length_minus1 = bs_read_u(b, 5);
		sps->hrd.cpb_removal_delay_length_minus1 = bs_read_u(b, 5);
		sps->hrd.dpb_output_delay_length_minus1 = bs_read_u(b, 5);
		sps->hrd.time_offset_length = bs_read_u(b, 5);
	}
	inline void read_rbsp_trailing_bits(bs_t* b)
	{
		/* rbsp_stop_one_bit */ bs_skip_u(b, 1);

		while (!bs_byte_aligned(b))
		{
			/* rbsp_alignment_zero_bit */ bs_skip_u(b, 1);
		}
	}
	inline void read_vui_parameters(sps_t* sps, bs_t* b)
	{
		sps->vui.aspect_ratio_info_present_flag = bs_read_u1(b);
		if (sps->vui.aspect_ratio_info_present_flag)
		{
			sps->vui.aspect_ratio_idc = bs_read_u8(b);
			if (sps->vui.aspect_ratio_idc == SAR_Extended)
			{
				sps->vui.sar_width = bs_read_u(b, 16);
				sps->vui.sar_height = bs_read_u(b, 16);
			}
		}
		sps->vui.overscan_info_present_flag = bs_read_u1(b);
		if (sps->vui.overscan_info_present_flag)
		{
			sps->vui.overscan_appropriate_flag = bs_read_u1(b);
		}
		sps->vui.video_signal_type_present_flag = bs_read_u1(b);
		if (sps->vui.video_signal_type_present_flag)
		{
			sps->vui.video_format = bs_read_u(b, 3);
			sps->vui.video_full_range_flag = bs_read_u1(b);
			sps->vui.colour_description_present_flag = bs_read_u1(b);
			if (sps->vui.colour_description_present_flag)
			{
				sps->vui.colour_primaries = bs_read_u8(b);
				sps->vui.transfer_characteristics = bs_read_u8(b);
				sps->vui.matrix_coefficients = bs_read_u8(b);
			}
		}
		sps->vui.chroma_loc_info_present_flag = bs_read_u1(b);
		if (sps->vui.chroma_loc_info_present_flag)
		{
			sps->vui.chroma_sample_loc_type_top_field = bs_read_ue(b);
			sps->vui.chroma_sample_loc_type_bottom_field = bs_read_ue(b);
		}
		sps->vui.timing_info_present_flag = bs_read_u1(b);
		if (sps->vui.timing_info_present_flag)
		{
			sps->vui.num_units_in_tick = bs_read_u(b, 32);
			sps->vui.time_scale = bs_read_u(b, 32);
			sps->vui.fixed_frame_rate_flag = bs_read_u1(b);
		}
		sps->vui.nal_hrd_parameters_present_flag = bs_read_u1(b);
		if (sps->vui.nal_hrd_parameters_present_flag)
		{
			read_hrd_parameters(sps, b);
		}
		sps->vui.vcl_hrd_parameters_present_flag = bs_read_u1(b);
		if (sps->vui.vcl_hrd_parameters_present_flag)
		{
			read_hrd_parameters(sps, b);
		}
		if (sps->vui.nal_hrd_parameters_present_flag || sps->vui.vcl_hrd_parameters_present_flag)
		{
			sps->vui.low_delay_hrd_flag = bs_read_u1(b);
		}
		sps->vui.pic_struct_present_flag = bs_read_u1(b);
		sps->vui.bitstream_restriction_flag = bs_read_u1(b);
		if (sps->vui.bitstream_restriction_flag)
		{
			sps->vui.motion_vectors_over_pic_boundaries_flag = bs_read_u1(b);
			sps->vui.max_bytes_per_pic_denom = bs_read_ue(b);
			sps->vui.max_bits_per_mb_denom = bs_read_ue(b);
			sps->vui.log2_max_mv_length_horizontal = bs_read_ue(b);
			sps->vui.log2_max_mv_length_vertical = bs_read_ue(b);
			sps->vui.num_reorder_frames = bs_read_ue(b);
			sps->vui.max_dec_frame_buffering = bs_read_ue(b);
		}
	}
	inline int intlog2(int x)
	{
		int log = 0;
		if (x < 0) { x = 0; }
		while ((x >> log) > 0)
		{
			log++;
		}
		if (log > 0 && x == 1 << (log - 1)) { log--; }
		return log;
	}
	inline int more_rbsp_data(bs_t* bs)
	{
		// TODO this version handles reading only. writing version?

		// no more data
		if (bs_eof(bs)) { return 0; }

		// no rbsp_stop_bit yet
		if (bs_peek_u1(bs) == 0) { return 1; }

		// next bit is 1, is it the rsbp_stop_bit? only if the rest of bits are 0
		bs_t bs_tmp;
		bs_clone(&bs_tmp, bs);
		bs_skip_u1(&bs_tmp);
		while (!bs_eof(&bs_tmp))
		{
			// A later bit was 1, it wasn't the rsbp_stop_bit
			if (bs_read_u1(&bs_tmp) == 1) { return 1; }
		}

		// All following bits were 0, it was the rsbp_stop_bit
		return 0;
	}
	inline void read_seq_parameter_set_rbsp(sps_t* sps, bs_t* b)
	{
		sps->profile_idc = bs_read_u8(b);
		sps->constraint_set0_flag = bs_read_u1(b);
		sps->constraint_set1_flag = bs_read_u1(b);
		sps->constraint_set2_flag = bs_read_u1(b);
		sps->constraint_set3_flag = bs_read_u1(b);
		sps->constraint_set4_flag = bs_read_u1(b);
		sps->constraint_set5_flag = bs_read_u1(b);
		/* reserved_zero_2bits */ bs_skip_u(b, 2);
		sps->level_idc = bs_read_u8(b);
		sps->seq_parameter_set_id = bs_read_ue(b);

		if (sps->profile_idc == 100 || sps->profile_idc == 110 ||
			sps->profile_idc == 122 || sps->profile_idc == 144)
		{
			sps->chroma_format_idc = bs_read_ue(b);
			if (sps->chroma_format_idc == 3)
			{
				sps->residual_colour_transform_flag = bs_read_u1(b);
			}
			sps->bit_depth_luma_minus8 = bs_read_ue(b);
			sps->bit_depth_chroma_minus8 = bs_read_ue(b);
			sps->qpprime_y_zero_transform_bypass_flag = bs_read_u1(b);
			sps->seq_scaling_matrix_present_flag = bs_read_u1(b);
			if (sps->seq_scaling_matrix_present_flag)
			{
				for (int i = 0; i < 8; i++)
				{
					sps->seq_scaling_list_present_flag[i] = bs_read_u1(b);
					if (sps->seq_scaling_list_present_flag[i])
					{
						if (i < 6)
						{
							read_scaling_list(b, sps->ScalingList4x4[i], 16,
								&(sps->UseDefaultScalingMatrix4x4Flag[i]));
						}
						else
						{
							read_scaling_list(b, sps->ScalingList8x8[i - 6], 64,
								&(sps->UseDefaultScalingMatrix8x8Flag[i - 6]));
						}
					}
				}
			}
		}
		sps->log2_max_frame_num_minus4 = bs_read_ue(b);
		sps->pic_order_cnt_type = bs_read_ue(b);
		if (sps->pic_order_cnt_type == 0)
		{
			sps->log2_max_pic_order_cnt_lsb_minus4 = bs_read_ue(b);
		}
		else if (sps->pic_order_cnt_type == 1)
		{
			sps->delta_pic_order_always_zero_flag = bs_read_u1(b);
			sps->offset_for_non_ref_pic = bs_read_se(b);
			sps->offset_for_top_to_bottom_field = bs_read_se(b);
			sps->num_ref_frames_in_pic_order_cnt_cycle = bs_read_ue(b);
			for (int i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
			{
				sps->offset_for_ref_frame[i] = bs_read_se(b);
			}
		}
		sps->num_ref_frames = bs_read_ue(b);
		sps->gaps_in_frame_num_value_allowed_flag = bs_read_u1(b);
		sps->pic_width_in_mbs_minus1 = bs_read_ue(b);
		sps->pic_height_in_map_units_minus1 = bs_read_ue(b);
		sps->frame_mbs_only_flag = bs_read_u1(b);
		if (!sps->frame_mbs_only_flag)
		{
			sps->mb_adaptive_frame_field_flag = bs_read_u1(b);
		}
		sps->direct_8x8_inference_flag = bs_read_u1(b);
		sps->frame_cropping_flag = bs_read_u1(b);
		if (sps->frame_cropping_flag)
		{
			sps->frame_crop_left_offset = bs_read_ue(b);
			sps->frame_crop_right_offset = bs_read_ue(b);
			sps->frame_crop_top_offset = bs_read_ue(b);
			sps->frame_crop_bottom_offset = bs_read_ue(b);
		}
		sps->vui_parameters_present_flag = bs_read_u1(b);
		if (sps->vui_parameters_present_flag)
		{
			read_vui_parameters(sps, b);
		}
		read_rbsp_trailing_bits(b);
	}
	inline void read_pic_parameter_set_rbsp(pps_t* pps, bs_t* b)
	{
		pps->pic_parameter_set_id = bs_read_ue(b);
		pps->seq_parameter_set_id = bs_read_ue(b);
		pps->entropy_coding_mode_flag = bs_read_u1(b);
		pps->pic_order_present_flag = bs_read_u1(b);
		pps->num_slice_groups_minus1 = bs_read_ue(b);

		if (pps->num_slice_groups_minus1 > 0)
		{
			pps->slice_group_map_type = bs_read_ue(b);
			if (pps->slice_group_map_type == 0)
			{
				for (int i_group = 0; i_group <= pps->num_slice_groups_minus1; i_group++)
				{
					pps->run_length_minus1[i_group] = bs_read_ue(b);
				}
			}
			else if (pps->slice_group_map_type == 2)
			{
				for (int i_group = 0; i_group < pps->num_slice_groups_minus1; i_group++)
				{
					pps->top_left[i_group] = bs_read_ue(b);
					pps->bottom_right[i_group] = bs_read_ue(b);
				}
			}
			else if (pps->slice_group_map_type == 3 ||
				pps->slice_group_map_type == 4 ||
				pps->slice_group_map_type == 5)
			{
				pps->slice_group_change_direction_flag = bs_read_u1(b);
				pps->slice_group_change_rate_minus1 = bs_read_ue(b);
			}
			else if (pps->slice_group_map_type == 6)
			{
				pps->pic_size_in_map_units_minus1 = bs_read_ue(b);
				for (int i = 0; i <= pps->pic_size_in_map_units_minus1; i++)
				{
					int v = intlog2(pps->num_slice_groups_minus1 + 1);
					pps->slice_group_id[i] = bs_read_u(b, v);
				}
			}
		}
		pps->num_ref_idx_l0_active_minus1 = bs_read_ue(b);
		pps->num_ref_idx_l1_active_minus1 = bs_read_ue(b);
		pps->weighted_pred_flag = bs_read_u1(b);
		pps->weighted_bipred_idc = bs_read_u(b, 2);
		pps->pic_init_qp_minus26 = bs_read_se(b);
		pps->pic_init_qs_minus26 = bs_read_se(b);
		pps->chroma_qp_index_offset = bs_read_se(b);
		pps->deblocking_filter_control_present_flag = bs_read_u1(b);
		pps->constrained_intra_pred_flag = bs_read_u1(b);
		pps->redundant_pic_cnt_present_flag = bs_read_u1(b);

		int have_more_data = 0;
		if (1) { have_more_data = more_rbsp_data(b); }
		if (0)
		{
			have_more_data = pps->transform_8x8_mode_flag | pps->pic_scaling_matrix_present_flag | pps->second_chroma_qp_index_offset != 0;
		}

		if (have_more_data)
		{
			pps->transform_8x8_mode_flag = bs_read_u1(b);
			pps->pic_scaling_matrix_present_flag = bs_read_u1(b);
			if (pps->pic_scaling_matrix_present_flag)
			{
				for (int i = 0; i < 6 + 2 * pps->transform_8x8_mode_flag; i++)
				{
					pps->pic_scaling_list_present_flag[i] = bs_read_u1(b);
					if (pps->pic_scaling_list_present_flag[i])
					{
						if (i < 6)
						{
							read_scaling_list(b, pps->ScalingList4x4[i], 16,
								&(pps->UseDefaultScalingMatrix4x4Flag[i]));
						}
						else
						{
							read_scaling_list(b, pps->ScalingList8x8[i - 6], 64,
								&(pps->UseDefaultScalingMatrix8x8Flag[i - 6]));
						}
					}
				}
			}
			pps->second_chroma_qp_index_offset = bs_read_se(b);
		}
		read_rbsp_trailing_bits(b);
	}

	enum NAL_REF_IDC
	{
		NAL_REF_IDC_PRIORITY_HIGHEST = 3,
		NAL_REF_IDC_PRIORITY_HIGH = 2,
		NAL_REF_IDC_PRIORITY_LOW = 1,
		NAL_REF_IDC_PRIORITY_DISPOSABLE = 0,
	};

	enum NAL_UNIT_TYPE
	{
		NAL_UNIT_TYPE_UNSPECIFIED = 0,	// Unspecified
		NAL_UNIT_TYPE_CODED_SLICE_NON_IDR = 1,	// Coded slice of a non-IDR picture
		NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_A = 2,	// Coded slice data partition A
		NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_B = 3,	// Coded slice data partition B
		NAL_UNIT_TYPE_CODED_SLICE_DATA_PARTITION_C = 4,	// Coded slice data partition C
		NAL_UNIT_TYPE_CODED_SLICE_IDR = 5,	// Coded slice of an IDR picture
		NAL_UNIT_TYPE_SEI = 6,	// Supplemental enhancement information (SEI)
		NAL_UNIT_TYPE_SPS = 7,	// Sequence parameter set
		NAL_UNIT_TYPE_PPS = 8,	// Picture parameter set
		NAL_UNIT_TYPE_AUD = 9,	// Access unit delimiter
		NAL_UNIT_TYPE_END_OF_SEQUENCE = 10,	// End of sequence
		NAL_UNIT_TYPE_END_OF_STREAM = 11,	// End of stream
		NAL_UNIT_TYPE_FILLER = 12,	// Filler data
		NAL_UNIT_TYPE_SPS_EXT = 13,	// Sequence parameter set extension
		NAL_UNIT_TYPE_CODED_SLICE_AUX = 19,	// Coded slice of an auxiliary coded picture without partitioning
	};

	struct nal_header
	{
		NAL_REF_IDC idc;
		NAL_UNIT_TYPE type;
	};
	inline void read_nal_header(nal_header* nal, bs_t* bs)
	{
		*nal = {};
		uint32_t forbidden_zero_bit = bs_read_u(bs, 1);
		assert(forbidden_zero_bit == 0);
		nal->idc = (h264::NAL_REF_IDC)bs_read_u(bs, 2);
		nal->type = (h264::NAL_UNIT_TYPE)bs_read_u(bs, 5);
	}

}

#endif
