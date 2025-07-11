#ifndef DXVA_H
#define DXVA_H
// This is a minimal version of dxva.h from the Windows SDK
//	It contains only the bare necessary structures for interoping with DirectX Video APIs

#pragma pack(push, BeforeDXVApacking, 1)

// H264 (AVC)

/* H.264/AVC picture entry data structure */
typedef struct _DXVA_PicEntry_H264 {
	union {
		struct {
			UCHAR  Index7Bits : 7;
			UCHAR  AssociatedFlag : 1;
		};
		UCHAR  bPicEntry;
	};
} DXVA_PicEntry_H264, * LPDXVA_PicEntry_H264;  /* 1 byte */

/* H.264/AVC picture parameters structure */
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

/* H.264/AVC quantization weighting matrix data structure */
typedef struct _DXVA_Qmatrix_H264 {
	UCHAR  bScalingLists4x4[6][16];
	UCHAR  bScalingLists8x8[2][64];

} DXVA_Qmatrix_H264, * LPDXVA_Qmatrix_H264;

/* H.264/AVC slice control data structure - short form */
typedef struct _DXVA_Slice_H264_Short {
	UINT   BSNALunitDataLocation; /* type 1..5 */
	UINT   SliceBytesInBuffer; /* for off-host parse */
	USHORT wBadSliceChopping;  /* for off-host parse */
} DXVA_Slice_H264_Short, * LPDXVA_Slice_H264_Short;




// H265 (HEVC)

/* HEVC Picture Entry structure */
typedef struct _DXVA_PicEntry_HEVC
{
	union
	{
		struct
		{
			UCHAR Index7Bits : 7;
			UCHAR AssociatedFlag : 1;
		};
		UCHAR bPicEntry;
	};
} DXVA_PicEntry_HEVC, * LPDXVA_PicEntry_HEVC;

/* HEVC Picture Parameter structure */
typedef struct _DXVA_PicParams_HEVC {
	USHORT      PicWidthInMinCbsY;
	USHORT      PicHeightInMinCbsY;
	union {
		struct {
			USHORT  chroma_format_idc : 2;
			USHORT  separate_colour_plane_flag : 1;
			USHORT  bit_depth_luma_minus8 : 3;
			USHORT  bit_depth_chroma_minus8 : 3;
			USHORT  log2_max_pic_order_cnt_lsb_minus4 : 4;
			USHORT  NoPicReorderingFlag : 1;
			USHORT  NoBiPredFlag : 1;
			USHORT  ReservedBits1 : 1;
		};
		USHORT wFormatAndSequenceInfoFlags;
	};
	DXVA_PicEntry_HEVC  CurrPic;
	UCHAR   sps_max_dec_pic_buffering_minus1;
	UCHAR   log2_min_luma_coding_block_size_minus3;
	UCHAR   log2_diff_max_min_luma_coding_block_size;
	UCHAR   log2_min_transform_block_size_minus2;
	UCHAR   log2_diff_max_min_transform_block_size;
	UCHAR   max_transform_hierarchy_depth_inter;
	UCHAR   max_transform_hierarchy_depth_intra;
	UCHAR   num_short_term_ref_pic_sets;
	UCHAR   num_long_term_ref_pics_sps;
	UCHAR   num_ref_idx_l0_default_active_minus1;
	UCHAR   num_ref_idx_l1_default_active_minus1;
	CHAR    init_qp_minus26;
	UCHAR   ucNumDeltaPocsOfRefRpsIdx;
	USHORT  wNumBitsForShortTermRPSInSlice;
	USHORT  ReservedBits2;

	union {
		struct {
			UINT32  scaling_list_enabled_flag : 1;
			UINT32  amp_enabled_flag : 1;
			UINT32  sample_adaptive_offset_enabled_flag : 1;
			UINT32  pcm_enabled_flag : 1;
			UINT32  pcm_sample_bit_depth_luma_minus1 : 4;
			UINT32  pcm_sample_bit_depth_chroma_minus1 : 4;
			UINT32  log2_min_pcm_luma_coding_block_size_minus3 : 2;
			UINT32  log2_diff_max_min_pcm_luma_coding_block_size : 2;
			UINT32  pcm_loop_filter_disabled_flag : 1;
			UINT32  long_term_ref_pics_present_flag : 1;
			UINT32  sps_temporal_mvp_enabled_flag : 1;
			UINT32  strong_intra_smoothing_enabled_flag : 1;
			UINT32  dependent_slice_segments_enabled_flag : 1;
			UINT32  output_flag_present_flag : 1;
			UINT32  num_extra_slice_header_bits : 3;
			UINT32  sign_data_hiding_enabled_flag : 1;
			UINT32  cabac_init_present_flag : 1;
			UINT32  ReservedBits3 : 5;
		};
		UINT32 dwCodingParamToolFlags;
	};

	union {
		struct {
			UINT32  constrained_intra_pred_flag : 1;
			UINT32  transform_skip_enabled_flag : 1;
			UINT32  cu_qp_delta_enabled_flag : 1;
			UINT32  pps_slice_chroma_qp_offsets_present_flag : 1;
			UINT32  weighted_pred_flag : 1;
			UINT32  weighted_bipred_flag : 1;
			UINT32  transquant_bypass_enabled_flag : 1;
			UINT32  tiles_enabled_flag : 1;
			UINT32  entropy_coding_sync_enabled_flag : 1;
			UINT32  uniform_spacing_flag : 1;
			UINT32  loop_filter_across_tiles_enabled_flag : 1;
			UINT32  pps_loop_filter_across_slices_enabled_flag : 1;
			UINT32  deblocking_filter_override_enabled_flag : 1;
			UINT32  pps_deblocking_filter_disabled_flag : 1;
			UINT32  lists_modification_present_flag : 1;
			UINT32  slice_segment_header_extension_present_flag : 1;
			UINT32  IrapPicFlag : 1;
			UINT32  IdrPicFlag : 1;
			UINT32  IntraPicFlag : 1;
			UINT32  ReservedBits4 : 13;
		};
		UINT32 dwCodingSettingPicturePropertyFlags;
	};
	CHAR    pps_cb_qp_offset;
	CHAR    pps_cr_qp_offset;
	UCHAR   num_tile_columns_minus1;
	UCHAR   num_tile_rows_minus1;
	USHORT  column_width_minus1[19];
	USHORT  row_height_minus1[21];
	UCHAR   diff_cu_qp_delta_depth;
	CHAR    pps_beta_offset_div2;
	CHAR    pps_tc_offset_div2;
	UCHAR   log2_parallel_merge_level_minus2;
	INT     CurrPicOrderCntVal;
	DXVA_PicEntry_HEVC	RefPicList[15];
	UCHAR   ReservedBits5;
	INT     PicOrderCntValList[15];
	UCHAR   RefPicSetStCurrBefore[8];
	UCHAR   RefPicSetStCurrAfter[8];
	UCHAR   RefPicSetLtCurr[8];
	USHORT  ReservedBits6;
	USHORT  ReservedBits7;
	UINT    StatusReportFeedbackNumber;
} DXVA_PicParams_HEVC, * LPDXVA_PicParams_HEVC;

/* HEVC Quantizatiuon Matrix structure */
typedef struct _DXVA_Qmatrix_HEVC
{
	UCHAR ucScalingLists0[6][16];
	UCHAR ucScalingLists1[6][64];
	UCHAR ucScalingLists2[6][64];
	UCHAR ucScalingLists3[2][64];
	UCHAR ucScalingListDCCoefSizeID2[6];
	UCHAR ucScalingListDCCoefSizeID3[2];
} DXVA_Qmatrix_HEVC, * LPDXVA_Qmatrix_HEVC;


/* HEVC Slice Control Structure */
typedef struct _DXVA_Slice_HEVC_Short
{
	UINT    BSNALunitDataLocation;
	UINT    SliceBytesInBuffer;
	USHORT  wBadSliceChopping;
} DXVA_Slice_HEVC_Short, * LPDXVA_Slice_HEVC_Short;

/* H.265/HEVC status reporting data structure */
typedef struct _DXVA_Status_HEVC {
	USHORT StatusReportFeedbackNumber;
	DXVA_PicEntry_HEVC CurrPic;
	UCHAR  bBufType;
	UCHAR  bStatus;
	UCHAR  bReserved8Bits;
	USHORT wNumMbsAffected;
} DXVA_Status_HEVC, * LPDXVA_Status_HEVC;

#pragma pack(pop, BeforeDXVApacking)

#endif // DXVA_H
