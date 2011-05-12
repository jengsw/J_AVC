/* ///////////////////////////////////////////////////////////// */
/*   File: macroblock.h                                          */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 macroblock coding module.        */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _MACROBLOCK_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

xint encode_intra_mb(h264enc_obj *pEnc);
xint encode_residual_coding8x8(h264enc_obj * pEnc, xint block8x8);
xint encode_inter_mb(h264enc_obj *pEnc);
xint inter_RD_mode_select(h264enc_obj *pEnc);
xint encode_bidir_mb(h264enc_obj *pEnc);
xint entropy_coding(h264enc_obj *pEnc);
xint intra_RD_mode_select(h264enc_obj *pEnc);
xint intra_RD_backup( slice_obj *pSlice, backup_obj *pBackup, frame_obj *pCurr_frame, xint width, xint height );
xint intra_RD_restore_backup( h264enc_obj *pEnc );
xint encode_init_RDParams( h264enc_obj *pEnc );
xint init_mb_info(slice_obj *pSlice, xint width);
xint enc_display_mb( h264enc_obj *pEnc );
xint enc_display_y_residual( h264enc_obj *pEnc );
xint enc_display_uv_residual( h264enc_obj *pEnc );
xint set_intra_luma_prediction(slice_obj *pSlice, frame_obj *pCurr_frame, xint width);
xint set_intra_chroma_prediction(slice_obj *pSlice, frame_obj *pCurr_frame, xint width);
xint set_inter_parms(slice_obj *pSlice, frame_obj *curf, xint width);
xint inter_set_pred_info( slice_obj *pSlice, xint width );
xint decode_intra_mb(h264dec_obj *pDec);
xint dec_display_uv_residual( h264dec_obj *pDec );
xint get_I4x4_pred_mode( slice_obj *pSlice, xint block, xint nx_mb, xint constrained_intra_pred_flag );
xint decode_inter_mb(h264dec_obj * pDec);
xint enc_cal_inter_residual(h264enc_obj *pEnc);
xint set_ipcm_info( slice_obj *pSlice, frame_obj* pDst, frame_obj* pSrc, xint width );

xint entropy_decoding(h264dec_obj *pDec);

#ifdef __cplusplus
extern "C"
}
#endif

#define _MACROBLOCK_H_
#endif
