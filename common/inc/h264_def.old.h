/* ///////////////////////////////////////////////////////////// */
/*   File: h264_def.h                                            */
/*   Author: Jerry Peng                                          */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec structure.                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#include "../../common/inc/metypes.h"
#include <stdarg.h>
#include <stdio.h>

#ifndef __H264_DEF_H__

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_REF_FRAME 3

/* definition for error codes */
#define MMES_ERROR           -1
#define MMES_NO_ERROR        0
#define MMES_FILE_ERROR      1
#define MMES_ALLOC_ERROR     2
#define MMES_BITSTREAM_ERROR 3
#define MMES_MEM_ERROR       4

#define MMES_INVALID     0
#define MMES_VALID       1

#define MAX_PIXEL_VALUE  255
#define MIN_PIXEL_VALUE  0

// emulation_prevention
#define EMULATION_PREVENTION_ON     1

/* macroblock and block size */
#define MB_SIZE       16
#define  B_SIZE        4
#define NCOEFFS       16
#define NBLOCKS       ((MB_SIZE/B_SIZE)*(MB_SIZE/B_SIZE))
#define LUMA_NBLOCK   16
#define CHROMA_NBLOCK  8
#define SHIFT_QP      12
#define	_CHROMA_COEFF_COST_    4
#define	_LUMA_COEFF_COST_      4
#define	_LUMA_MB_COEFF_COST_   5

#define MAX_INT16     0x7fff
#define MAX_XINT      0x7fffffff
#define CLIP3(LB, UB, val)  (((val)<(LB)) ? (LB) : (((val)>(UB)) ? (UB) : (val)))
#define ABS(X)  ((X)> 0 ? (X) : -(X))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#define SIGN(X,Y) ((Y)>0 ? (X) : -(X))
#define ROUND(X,Y) (((X)+(Y))%(Y))
#define VOP2STR(x) ((x==I_SLICE)?'I':((x==P_SLICE)?'P':((x==B_SLICE)?'B':'U')))
#define I16MB(X) ((X)<4)
#define INTRA(X) ((X)<=18)
#define IS_ISLICE(X) (X == I_SLICE || X == ALL_I_SLICE)
#define IS_PSLICE(X) (X == P_SLICE || X == ALL_P_SLICE)

/* define for nal unit */
#define NALU_MAXSIZE 8000000
/* define for seq & pic parameter set */
#define MAXnum_slice_groups_minus1  8
#define MAXnum_ref_frames_in_pic_order_cnt_cycle  256
#define MAXSPS     32
#define MAXPPS     256
#define MAXREFNO   16
#define MAXLIST    33

//FREXT Profile IDC definitions
#define FREXT_HP        100      //!< YUV 4:2:0/8 "High"
#define FREXT_Hi10P     110      //!< YUV 4:2:0/10 "High 10"
#define FREXT_Hi422     122      //!< YUV 4:2:2/10 "High 4:2:2"
#define FREXT_Hi444     144      //!< YUV 4:4:4/12 "High 4:4:4"

#define	FRAME_RATE	    30.0
#define INT_MAX         2147483647 

static const xint mb_x_4_idx[16] = { 0, 1, 0, 1, 2, 3, 2, 3, 0, 1, 0, 1, 2, 3, 2, 3};
static const xint mb_y_4_idx[16] = { 0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 3, 3, 2, 2, 3, 3};

// macro for if statement of pattern & logging 
#define HW_METHOD 1
#define IFPATTERN(P, FUN) if(h264_pattern  & P){FUN;}
#define IFLOGMODE(P, FUN) if(h264_log_mode & P){FUN;}

typedef enum
{
    UNKNOWN_NAL = 0,
    NON_IDTR_SLICE = 1,
    DATA_PART_A = 2,
    DATA_PART_B = 3,
    DATA_PART_C = 4,
    IDTR_SLICE = 5,
    SEI_INFO = 6,
    SEQ_PARASET = 7,
    PIC_PARASET = 8,
    AU_DELIM = 9,
    END_SEQ = 10,
    END_STREAM = 11,
    FILLER_DATA = 12
} NAL_TYPE;

typedef enum
{
	RBSP_P_SLICE  = 0,
    RBSP_B_SLICE  = 1,
    RBSP_I_SLICE  = 2,
    RBSP_SP_SLICE = 3,
    RBSP_SI_SLICE = 4,
	
    ALL_P_SLICE  = 5,
    ALL_B_SLICE  = 6,
    ALL_I_SLICE  = 7,
    ALL_SP_SLICE = 8,
    ALL_SI_SLICE = 9,
	
	RBSP_AUTO_SLICE = 10
} RBSP_TYPE;

typedef enum
{
    I_FRAME, P_FRAME, B_FRAME, SI_FRAME, SP_FRAME
} FRAME_TYPE;

typedef enum
{
    I_SLICE, P_SLICE, B_SLICE, SI_SLICE, SP_SLICE, AUTO_SLICE
} SLICE_TYPE;

typedef enum
{
	INTRA_16x16_V   = 0,
	INTRA_16x16_H   = 1,
    INTRA_16x16_DC  = 2,
    INTRA_16x16_PL  = 3,

    INTRA_CHROMA_DC = 4,
    INTRA_CHROMA_H  = 5,
    INTRA_CHROMA_V  = 6,
    INTRA_CHROMA_PL = 7,

    INTRA_4x4_V     = 8,
    INTRA_4x4_H     = 9,
    INTRA_4x4_DC    = 10,
    INTRA_4x4_DDL   = 11,
    INTRA_4x4_DDR   = 12,
    INTRA_4x4_VR    = 13,
    INTRA_4x4_HD    = 14,
    INTRA_4x4_VL    = 15,
    INTRA_4x4_HU    = 16,
    INTRA_4x4       = 17,
    
    I_PCM           = 18,

    INTER_16x16		= 19,	//best_inter_mode
	INTER_16x8		= 20,	//best_inter_mode
	INTER_8x16		= 21,	//best_inter_mode
	INTER_8x8		= 22,	//best_inter_8x8_mode
	INTER_8x4		= 23,	//best_inter_8x8_mode
	INTER_4x8		= 24,	//best_inter_8x8_mode
	INTER_4x4		= 25,	//best_inter_8x8_mode
	INTER_P8x8      = 26,	//best_inter_mode
	INTER_PSKIP     = 27	//best_inter_mode

} MB_MODE;

typedef enum
{
    SRC_FILE,
    REC_FILE,
    BS_FILE,
    FRAME_WIDTH,
    FRAME_HEIGHT,
    START_FRAME,
    FRAME_NUM,
    MB_IN_SLICE,
    LUMA_QP,
    LF_CFG,
    LF_DIS_IDC,
    LF_ALPHA,
    LF_BETA,
    PTN_MODE,
    START_PTN_FRAME_NO,
    END_PTN_FRAME_NO,
    LOG_MODE,
    START_LOG_FRAME_NO,
    END_LOG_FRAME_NO,
    
    DEBUG_MODE,
    START_DEBUG_FRAME_NO,
    END_DEBUG_FRAME_NO,
	START_DEBUG_MB_NO,

    OUTPUT_DEBUG_MODE,
    OUTPUT_START_DEBUG_FRAME_NO,
    OUTPUT_END_DEBUG_FRAME_NO,
	OUTPUT_START_DEBUG_MB_NO,

	INTRA_REFRESH,
    LOGMAXFNO,
    INTERVAL_IPCM,
    LOW_BANDWIDTH_ON
    
} CFG_TYPE;

#define MAX_BS_LEN (1280*1024*3)>>1

typedef struct
{
    uint8 *start;
	uint32 offset;
    uint32 length;
} bstream;

/* Encoding Configuration Structure */
typedef struct _config_ini_
{
    FILE *cfg_fp;

    FILE *src_fp;
	FILE *rec_fp;
    FILE *bit_fp;

    /* File Param */
	char src_file[256];		    //[00]  source sequence 
    char rec_file[256];		    //[01]  reconstructed frame
	char bs_file[256];	    	//[02]  bitstream
    xint frame_width;	    	//[03]  frame width
    xint frame_height;   		//[04]  frame height
	xint start_frame;	    	//[05]  starting frame number
    xint frame_num;			    //[06]  number of frames to be encoded

    /* Encoder Param */
    xint mb_in_slice;           //[07]  number of macroblocks in the slice
    xint luma_qp;			    //[08]  luma qp

    /* Loop Filter Param */
    xint config_loop_filter;    //[09]  Configure loop filter (0=parameter below ingored, 1=parameters sent)
    xint dis_loop_filter_idc;   //[10]  Disable loop filter in slice header (0=Filter, 1=No Filter, No Filter at slice boundary)
    xint alpha_c0_offset_div2;  //[11]  Alpha & C0 offset div. 2, {-6, -5, ... 0, +1, .. +6}
    xint beta_offset_div2;      //[12]  Beta offset div. 2, {-6, -5, ... 0, +1, .. +6}

    /* Pattern Config Param */
    xint ptn_mode;              //[13]  pattern mode (0 : No pattern, 1 : INTRAPRED, 2 : CAVLC)
    xint start_ptn_frame_no;    //[14]  start number of generating patterns
    xint end_ptn_frame_no;      //[15]  end number of generating patterns

    /* Logging Config Param */
    xint log_mode;              //[16]  logging mode (0 : No logging, 1 : logging bitstream)
    xint start_log_frame_no;    //[17]  start number of logging
    xint end_log_frame_no;      //[18]  end number of logging

    /* Debugging Config Param */
    xint debug_mode;            //[19]  deubgging mode
    xint start_debug_frame_no;  //[20]  start number of deubgging
    xint end_debug_frame_no;    //[21]  end number of deubgging
	xint start_debug_mb_no;		//[22]  start MB number of debugging for each frame

    /* Output Debugging Config Param */
    xint output_debug_mode;            //[23]  output_deubgging mode
    xint output_start_debug_frame_no;  //[24]  output_start number of deubgging
    xint output_end_debug_frame_no;    //[25]  output_end number of deubgging
	xint output_start_debug_mb_no;	   //[26]  output_start MB number of debugging for each frame

	/* Others Param */
	xint intra_period;		    //[27]  Number of P frames between I frames (0: only first frame)
    xint log2_max_fno;          //[28]  Log2MaxFrameNum
	xint interval_ipcm;		    //[29]  Interval of MB numbers for using IPCM mode

}enc_cfg;

typedef struct _dec_refpicmark
{
  xint mem_manage_ctrl_op;
  xint diff_of_pic_nums;
  xint lt_pic_num;
  xint lt_frm_idx;
  xint max_lt_frm_idx;
} dec_refpicmark;

typedef struct 
{
  xint   startcodeprefix_len;      // 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  xint   len;                      // Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  xint   nal_unit_type;            // NALU_TYPE_xxxx
  xint   nal_reference_idc;        // NALU_PRIORITY_xxxx
  xint   forbidden_bit;            // should be always FALSE
  xint   is_first_byte;            // denotes the first byte in bitstream or not
  uint8  *buf;                     // contains the first byte followed by the EBSP

  //xint forbidden_zero_bit;
  //xint nal_ref_idc;

} nal_obj;

typedef struct _mv_obj_
{
    int16 x[NBLOCKS];
    int16 y[NBLOCKS];
} mv_obj;

typedef struct _frame_obj_
{
    /* video frame dimension */
    xint    width;
    xint    height;

    FRAME_TYPE type;   /* frame coding type     */
    uint    id;        /* absolute frame number */
    uint32  timestamp; /* in milliseconds       */
    xint    coded;     /* skipped frame?        */
    xint    QP;

    uint8   *y;        /* The  Y component      */
    uint8   *cb;       /* The Cb component      */
    uint8   *cr;       /* The Cr component      */
} frame_obj;

typedef struct _mb_obj_
{
    xint    id;        /* MB id                 */
    xint    fref_id;   /* forward reference ID  */
    xint    bref_id;   /* backward reference ID */
    mv_obj  *mv, *bmv; /* motion vector         */
    xint    QP;        /* MB level QP           */

    /* The following variables are used for best inter */
    /*    prediction mode decision.                    */
    MB_MODE inter_mode[NBLOCKS];

    /* The following variables are used for best intra */
    /*    prediction mode decision.                    */
    MB_MODE best_I16x16_mode;
    int16   best_I16x16_residual[MB_SIZE*MB_SIZE];
    uint32  min_I16x16_sad;

    MB_MODE best_chroma_mode;
    int16   cb_residual[(MB_SIZE/2)*(MB_SIZE/2)];
    int16   cr_residual[(MB_SIZE/2)*(MB_SIZE/2)];

    MB_MODE best_I4x4_mode[NBLOCKS];
    int16   best_I4x4_residual[MB_SIZE*MB_SIZE];
    uint32  min_I4x4_sad[NBLOCKS];

    /* The following data structure are used for CAVLC */
    int16   LumaDCLevel[B_SIZE*B_SIZE];
    int16   LumaACLevel[MB_SIZE*MB_SIZE];
    int16   CbDCLevel[2*2];
    int16   CrDCLevel[2*2];
    int16   CbACLevel[(MB_SIZE/2)*(MB_SIZE/2)];
    int16   CrACLevel[(MB_SIZE/2)*(MB_SIZE/2)];
} Macroblock;

typedef struct _slice_obj_
{
    /* slice parameters */
    xint  first_mb_in_slice;    /* ID of the first MB      */
    xint  pic_parameter_set_id; /* ID of the associated PS */
    xint  max_size;             /* max size of a slice     */

    SLICE_TYPE type;   /* slice coding type     */
    xint       QP;     /* slice level QP        */
    xint       qp_delta;
    xint       chroma_qp_index_offset;
    Macroblock cmb;    /* current MB of the slice */

    /* for MV/spatial prediction, store previous row of decoded data */
    xint    *top_mvx;
    xint    *top_mvy;
    uint8   *top_y, *top_cb, *top_cr;
    uint8   *top_valid;
    int8    *top_y_nzc;
    int8    *top_cb_nzc;
    int8    *top_cr_nzc;

    /* for MV/spatial prediction, store left-hand side decoded data */
    xint    left_mvx[MB_SIZE/B_SIZE], left_mvy[MB_SIZE/B_SIZE];
    uint8   left_y[MB_SIZE], left_cb[MB_SIZE/2], left_cr[MB_SIZE/2];
    uint8   left_valid[MB_SIZE/B_SIZE];
    MB_MODE left_mode[B_SIZE];
    int8    left_y_nzc[MB_SIZE/B_SIZE];
    int8    left_cb_nzc[(MB_SIZE/2)/B_SIZE];        // for YCbCr420
    int8    left_cr_nzc[(MB_SIZE/2)/B_SIZE];        // for YCbCr420

    /* for deblock filter, store the up/left -hand side info */
    uint8   *deblock_top_valid;
    uint8   deblock_left_valid;
    xint    *top_qp;
    xint    left_qp;

} slice_obj;

typedef struct _h264enc_obj_
{
    frame_obj *reff;          /* reference frame          */
    frame_obj *curf;          /* current frame            */
    frame_obj *bckf;          /* backward reference frame */
    xint      *slice_no;      /* slice number for each MB */
    xint      nx_mb, ny_mb;   /* # of MB per col & row    */
    xint      total_mb;       /* total number of MB       */

	/* NAL unit header */
	nal_obj nal;

    /* current slice pointer */
    slice_obj *curr_slice;    /* current slice object     */

    /* reference frame buffer */
    frame_obj *ref_frame[MAX_REF_FRAME];

    /* scratch space for intra_prediction */
    uint8 *scratch;

    /* bitstream buffer & pointer */
    uint8     *stream_buf;    /* output bitstream buffer         */
    uint8      curr_byte;     /* current byte under composition  */
    xint       stream_len;    /* bitstream buffer size (in bits) */
    xint       byte_pos;      /* current byte position           */
    xint       bit_pos;       /* current bit position            */

    /* coding/rate-control parameters */

} h264enc_obj;

/* ------------------------------------------------------------- */
/*                   NAL Unit for decoder                        */
/* ------------------------------------------------------------- */

#define MAXIMUMVALUEOFcpb_cnt   32

typedef struct
{
  uint  cpb_cnt_minus1;                                   // ue(v)
  uint  bit_rate_scale;                                   // u(4)
  uint  cpb_size_scale;                                   // u(4)
    uint  bit_rate_value_minus1 [MAXIMUMVALUEOFcpb_cnt];  // ue(v)
    uint  cpb_size_value_minus1 [MAXIMUMVALUEOFcpb_cnt];  // ue(v)
    uint  cbr_flag              [MAXIMUMVALUEOFcpb_cnt];  // u(1)
  uint  initial_cpb_removal_delay_length_minus1;          // u(5)
  uint  cpb_removal_delay_length_minus1;                  // u(5)
  uint  dpb_output_delay_length_minus1;                   // u(5)
  uint  time_offset_length;                               // u(5)
} hrd_parameters_t;

typedef struct
{
  xint      aspect_ratio_info_present_flag;                   // u(1)
    uint  aspect_ratio_idc;                               // u(8)
      uint  sar_width;                                    // u(16)
      uint  sar_height;                                   // u(16)
  xint      overscan_info_present_flag;                       // u(1)
    xint      overscan_appropriate_flag;                      // u(1)
  xint      video_signal_type_present_flag;                   // u(1)
    uint  video_format;                                   // u(3)
    xint      video_full_range_flag;                          // u(1)
    xint      colour_description_present_flag;                // u(1)
      uint  colour_primaries;                             // u(8)
      uint  transfer_characteristics;                     // u(8)
      uint  matrix_coefficients;                          // u(8)
  xint      chroma_location_info_present_flag;                // u(1)
    uint   chroma_sample_loc_type_top_field;               // ue(v)
    uint   chroma_sample_loc_type_bottom_field;            // ue(v)
  xint      timing_info_present_flag;                         // u(1)
    uint  num_units_in_tick;                              // u(32)
    uint  time_scale;                                     // u(32)
    xint      fixed_frame_rate_flag;                          // u(1)
  xint      nal_hrd_parameters_present_flag;                  // u(1)
    hrd_parameters_t nal_hrd_parameters;                      // hrd_paramters_t
  xint      vcl_hrd_parameters_present_flag;                  // u(1)
    hrd_parameters_t vcl_hrd_parameters;                      // hrd_paramters_t
  // if ((nal_hrd_parameters_present_flag || (vcl_hrd_parameters_present_flag))
    xint      low_delay_hrd_flag;                             // u(1)
  xint      pic_struct_present_flag;                        // u(1)
  xint      bitstream_restriction_flag;                       // u(1)
    xint      motion_vectors_over_pic_boundaries_flag;        // u(1)
    uint  max_bytes_per_pic_denom;                        // ue(v)
    uint  max_bits_per_mb_denom;                          // ue(v)
    uint  log2_max_mv_length_vertical;                    // ue(v)
    uint  log2_max_mv_length_horizontal;                  // ue(v)
    uint  num_reorder_frames;                             // ue(v)
    uint  max_dec_frame_buffering;                        // ue(v)
} vui_seq_parameters_t;

typedef struct
{
  xint   Valid;                  // indicates the parameter set is valid
  uint32  pic_parameter_set_id;                               // ue(v)
  uint32  seq_parameter_set_id;                               // ue(v)
  xint   entropy_coding_mode_flag;                            // u(1)

  xint   transform_8x8_mode_flag;                             // u(1)

  xint   pic_scaling_matrix_present_flag;                     // u(1)
  int       pic_scaling_list_present_flag[8];                 // u(1)
  int       ScalingList4x4[6][16];                            // se(v)
  int       ScalingList8x8[2][64];                            // se(v)
  xint   UseDefaultScalingMatrix4x4Flag[6];
  xint   UseDefaultScalingMatrix8x8Flag[2];

  // if( pic_order_cnt_type < 2 )  in the sequence parameter set
  xint      pic_order_present_flag;                           // u(1)
  uint32  num_slice_groups_minus1;                            // ue(v)
    uint32  slice_group_map_type;                             // ue(v)
    // if( slice_group_map_type = = 0 )
      uint32  run_length_minus1[MAXnum_slice_groups_minus1];  // ue(v)
    // else if( slice_group_map_type = = 2 )
      uint32  top_left[MAXnum_slice_groups_minus1];           // ue(v)
      uint32  bottom_right[MAXnum_slice_groups_minus1];       // ue(v)
    // else if( slice_group_map_type = = 3 || 4 || 5
      xint   slice_group_change_direction_flag;               // u(1)
      uint32  slice_group_change_rate_minus1;                 // ue(v)
    // else if( slice_group_map_type = = 6 )
      uint32  num_slice_group_map_units_minus1;               // ue(v)
      uint32  *slice_group_id;                                // complete MBAmap u(v)
  uint32  num_ref_idx_l0_active_minus1;                       // ue(v)
  uint32  num_ref_idx_l1_active_minus1;                       // ue(v)
  xint   weighted_pred_flag;                                  // u(1)
  uint32  weighted_bipred_idc;                                // u(2)
  xint      pic_init_qp_minus26;                              // se(v)
  xint      pic_init_qs_minus26;                              // se(v)
  xint      chroma_qp_index_offset;                           // se(v)

  int       second_chroma_qp_index_offset;                    // se(v)

  xint   deblocking_filter_control_present_flag;           // u(1)
  xint   constrained_intra_pred_flag;                      // u(1)
  xint   redundant_pic_cnt_present_flag;                   // u(1)
} pic_paraset_obj;

typedef struct
{
  xint   Valid;                  // indicates the parameter set is valid

  uint32  profile_idc;                                      // u(8)
  xint   constrained_set0_flag;                             // u(1)
  xint   constrained_set1_flag;                             // u(1)
  xint   constrained_set2_flag;                             // u(1)
  uint32  level_idc;                                        // u(8)
  uint32  seq_parameter_set_id;                             // ue(v)
  uint32  chroma_format_idc;                                // ue(v)

  xint  seq_scaling_matrix_present_flag;                    // u(1)
  int      seq_scaling_list_present_flag[8];                  // u(1)
  int      ScalingList4x4[6][16];                             // se(v)
  int      ScalingList8x8[2][64];                             // se(v)
  xint  UseDefaultScalingMatrix4x4Flag[6];
  xint  UseDefaultScalingMatrix8x8Flag[2];

  uint32  bit_depth_luma_minus8;                            // ue(v)
  uint32  bit_depth_chroma_minus8;                          // ue(v)

  uint32  log2_max_frame_num_minus4;                        // ue(v)
  uint32 pic_order_cnt_type;
  // if( pic_order_cnt_type == 0 ) 
  uint32 log2_max_pic_order_cnt_lsb_minus4;                 // ue(v)
  // else if( pic_order_cnt_type == 1 )
    xint delta_pic_order_always_zero_flag;                  // u(1)
    int     offset_for_non_ref_pic;                         // se(v)
    int     offset_for_top_to_bottom_field;                 // se(v)
    uint32  num_ref_frames_in_pic_order_cnt_cycle;          // ue(v)
    // for( i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
      int   offset_for_ref_frame[MAXnum_ref_frames_in_pic_order_cnt_cycle];   // se(v)
  uint32  num_ref_frames;                                   // ue(v)
  xint   gaps_in_frame_num_value_allowed_flag;              // u(1)
  uint32  pic_width_in_mbs_minus1;                          // ue(v)
  uint32  pic_height_in_map_units_minus1;                   // ue(v)
  xint   frame_mbs_only_flag;                               // u(1)
  // if( !frame_mbs_only_flag ) 
    xint   mb_adaptive_frame_field_flag;                    // u(1)
  xint   direct_8x8_inference_flag;                         // u(1)
  xint   frame_cropping_flag;                               // u(1)
    uint32  frame_cropping_rect_left_offset;                // ue(v)
    uint32  frame_cropping_rect_right_offset;               // ue(v)
    uint32  frame_cropping_rect_top_offset;                 // ue(v)
    uint32  frame_cropping_rect_bottom_offset;              // ue(v)
  xint   vui_parameters_present_flag;                       // u(1)
    vui_seq_parameters_t vui_seq_parameters;                // vui_seq_parameters_t
} seq_paraset_obj;

#ifdef __cplusplus
}
#endif

#define __H264_DEF_H__
#endif /* __H264_DEF_H__ */
