#ifndef DXVA_H
#define DXVA_H
// This is a minimal version of dxva.h from the Windows SDK
//	It contains only the bare necessary structures for interoping with DirectX Video APIs

/* H.264/AVC picture entry data structure */
#pragma pack(push, BeforeDXVApacking, 1)
typedef struct _DXVA_PicEntry_H264 {
	union {
		struct {
			UCHAR  Index7Bits : 7;
			UCHAR  AssociatedFlag : 1;
		};
		UCHAR  bPicEntry;
	};
} DXVA_PicEntry_H264, * LPDXVA_PicEntry_H264;  /* 1 byte */
#pragma pack(pop, BeforeDXVApacking)

/* H.264/AVC picture parameters structure */
#pragma pack(push, BeforeDXVApacking, 1)
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
#pragma pack(pop, BeforeDXVApacking)

/* H.264/AVC quantization weighting matrix data structure */
#pragma pack(push, BeforeDXVApacking, 1)
typedef struct _DXVA_Qmatrix_H264 {
	UCHAR  bScalingLists4x4[6][16];
	UCHAR  bScalingLists8x8[2][64];

} DXVA_Qmatrix_H264, * LPDXVA_Qmatrix_H264;
#pragma pack(pop, BeforeDXVApacking)

/* H.264/AVC slice control data structure - short form */
#pragma pack(push, BeforeDXVApacking, 1)
typedef struct _DXVA_Slice_H264_Short {
	UINT   BSNALunitDataLocation; /* type 1..5 */
	UINT   SliceBytesInBuffer; /* for off-host parse */
	USHORT wBadSliceChopping;  /* for off-host parse */
} DXVA_Slice_H264_Short, * LPDXVA_Slice_H264_Short;
#pragma pack(pop, BeforeDXVApacking)

/* H.264/AVC picture entry data structure - long form */
#pragma pack(push, BeforeDXVApacking, 1)
typedef struct _DXVA_Slice_H264_Long {
	UINT   BSNALunitDataLocation; /* type 1..5 */
	UINT   SliceBytesInBuffer; /* for off-host parse */
	USHORT wBadSliceChopping;  /* for off-host parse */

	USHORT first_mb_in_slice;
	USHORT NumMbsForSlice;

	USHORT BitOffsetToSliceData; /* after CABAC alignment */

	UCHAR  slice_type;
	UCHAR  luma_log2_weight_denom;
	UCHAR  chroma_log2_weight_denom;
	UCHAR  num_ref_idx_l0_active_minus1;
	UCHAR  num_ref_idx_l1_active_minus1;
	CHAR   slice_alpha_c0_offset_div2;
	CHAR   slice_beta_offset_div2;
	UCHAR  Reserved8Bits;
	DXVA_PicEntry_H264 RefPicList[2][32]; /* L0 & L1 */
	SHORT  Weights[2][32][3][2]; /* L0 & L1; Y, Cb, Cr */
	CHAR   slice_qs_delta;
	/* rest off-host parse */
	CHAR   slice_qp_delta;
	UCHAR  redundant_pic_cnt;
	UCHAR  direct_spatial_mv_pred_flag;
	UCHAR  cabac_init_idc;
	UCHAR  disable_deblocking_filter_idc;
	USHORT slice_id;
} DXVA_Slice_H264_Long, * LPDXVA_Slice_H264_Long;
#pragma pack(pop, BeforeDXVApacking)

#endif // DXVA_H
