#pragma once

typedef struct _DXVA_PicEntry_H264 {
	union {
		struct {
			UCHAR  Index7Bits : 7;
			UCHAR  AssociatedFlag : 1;
		};
		UCHAR  bPicEntry;
	};
} DXVA_PicEntry_H264, * LPDXVA_PicEntry_H264;  /* 1 byte */

typedef struct _DXVA_PicParams_H264 {
	USHORT  wFrameWidthInMbsMinus1;
	USHORT  wFrameHeightInMbsMinus1;
	DXVA_PicEntry_H264  CurrPic; /* flag is bot field flag */
	UCHAR   num_ref_frames;

	union {
		struct {
			USHORT  field_pic_flag : 1;
			USHORT  MbaffFrameFlag : 1;
			USHORT  residual_colour_transform_flag : 1;
			USHORT  sp_for_switch_flag : 1;
			USHORT  chroma_format_idc : 2;
			USHORT  RefPicFlag : 1;
			USHORT  constrained_intra_pred_flag : 1;

			USHORT  weighted_pred_flag : 1;
			USHORT  weighted_bipred_idc : 2;
			USHORT  MbsConsecutiveFlag : 1;
			USHORT  frame_mbs_only_flag : 1;
			USHORT  transform_8x8_mode_flag : 1;
			USHORT  MinLumaBipredSize8x8Flag : 1;
			USHORT  IntraPicFlag : 1;
		};
		USHORT  wBitFields;
	};
	UCHAR  bit_depth_luma_minus8;
	UCHAR  bit_depth_chroma_minus8;

	USHORT Reserved16Bits;
	UINT   StatusReportFeedbackNumber;

	DXVA_PicEntry_H264  RefFrameList[16]; /* flag LT */
	INT    CurrFieldOrderCnt[2];
	INT    FieldOrderCntList[16][2];

	CHAR   pic_init_qs_minus26;
	CHAR   chroma_qp_index_offset;   /* also used for QScb */
	CHAR   second_chroma_qp_index_offset; /* also for QScr */
	UCHAR  ContinuationFlag;

	/* remainder for parsing */
	CHAR   pic_init_qp_minus26;
	UCHAR  num_ref_idx_l0_active_minus1;
	UCHAR  num_ref_idx_l1_active_minus1;
	UCHAR  Reserved8BitsA;

	USHORT FrameNumList[16];
	UINT   UsedForReferenceFlags;
	USHORT NonExistingFrameFlags;
	USHORT frame_num;

	UCHAR  log2_max_frame_num_minus4;
	UCHAR  pic_order_cnt_type;
	UCHAR  log2_max_pic_order_cnt_lsb_minus4;
	UCHAR  delta_pic_order_always_zero_flag;

	UCHAR  direct_8x8_inference_flag;
	UCHAR  entropy_coding_mode_flag;
	UCHAR  pic_order_present_flag;
	UCHAR  num_slice_groups_minus1;

	UCHAR  slice_group_map_type;
	UCHAR  deblocking_filter_control_present_flag;
	UCHAR  redundant_pic_cnt_present_flag;
	UCHAR  Reserved8BitsB;

	USHORT slice_group_change_rate_minus1;

	UCHAR  SliceGroupMap[810]; /* 4b/sgmu, Size BT.601 */

} DXVA_PicParams_H264, * LPDXVA_PicParams_H264;
