/* ///////////////////////////////////////////////////////////// */
/*   File: mv_pred.h                                             */
/*   Author: Jerry Peng                                          */
/*   Date: Aug/23/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 Motion Vector Prediction Module. */
/*                                                               */
/*   Copyright, 2005.                                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _MV_PRED_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

xint mv_pred_inter( slice_obj *pSlice, backup_obj *pBackup, xint width, xint encode );
xint mv_pred_backup( backup_obj *pBackup, slice_obj *pSlice, int width );
xint mv_pred_restore( backup_obj *pBackup, slice_obj *pSlice, int width );
xint mv_pred_16x16( slice_obj *pSlice, xint width );
xint mv_pred_16x8( slice_obj *pSlice, xint width, xint encode );
xint mv_pred_8x16( slice_obj *pSlice, xint width, xint encode );
xint mv_pred_P8x8( slice_obj *pSlice, xint width, xint encode );
xint mv_pred_PSKIP( slice_obj *pSlice, xint width );
xint mv_pred_get_neighbor( slice_obj *pSlice, xint width, xint part, xint sub_part );
xint mv_pred_update_P8x8( slice_obj *pSlice, xint width, xint part, xint sub_part, xint predictor_x, xint predictor_y, xint encode );
xint mv_pred_set_invalid( pred_obj *pred );
xint mv_pred_get_prd_mv( slice_obj *pSlice, xint part, xint *pred_mvx, xint *pred_mvy );
xint mv_pred_set_pred_X( slice_obj *pSlice, xint width, xint part, xint sub_part );
xint mv_pred_add_mvd( slice_obj *pSlice );

#ifdef __cplusplus
extern "C"
}
#endif

#define _MV_PRED_H_
#endif