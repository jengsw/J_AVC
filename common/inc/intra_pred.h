/* ///////////////////////////////////////////////////////////// */
/*   File: intra_pred.h                                          */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 intra prediction module.         */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _INTRA_PRED_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* encoder routines */
xint enc_intra_16x16_prediction(h264enc_obj *pEnc);
xint enc_intra_4x4_prediction(h264enc_obj *pEnc, xint block);
xint enc_intra_chroma_prediction(h264enc_obj *pEnc);

/* encoder Intra 16x16 prediction mode routines */
xint enc_intra_16x16_DC_pred(h264enc_obj *pEnc);
xint enc_intra_16x16_H_pred(h264enc_obj *pEnc);
xint enc_intra_16x16_V_pred(h264enc_obj *pEnc);
xint enc_intra_16x16_PL_pred(h264enc_obj *pEnc);

/* encoder Intra 4x4 prediction mode routines */
xint enc_intra_4x4_V_pred(h264enc_obj *pEnc, xint block);
xint enc_intra_4x4_H_pred(h264enc_obj *pEnc, xint block);
xint enc_intra_4x4_DC_pred(h264enc_obj *pEnc, xint block);
xint enc_intra_4x4_DDL_pred(h264enc_obj *pEnc, xint block);
xint enc_intra_4x4_DDR_pred(h264enc_obj *pEnc, xint block);
xint enc_intra_4x4_VR_pred(h264enc_obj *pEnc, xint block);
xint enc_intra_4x4_HD_pred(h264enc_obj *pEnc, xint block);
xint enc_intra_4x4_VL_pred(h264enc_obj *pEnc, xint block);
xint enc_intra_4x4_HU_pred(h264enc_obj *pEnc, xint block);

/* encoder Intra chroma prediction mode routines */
xint enc_intra_chroma_DC_pred(h264enc_obj *pEnc);
xint enc_intra_chroma_H_pred(h264enc_obj *pEnc);
xint enc_intra_chroma_V_pred(h264enc_obj *pEnc);
xint enc_intra_chroma_PL_pred(h264enc_obj *pEnc);

/* decoder routines */
xint dec_intra_16x16_prediction(h264dec_obj *pDec);
xint dec_intra_4x4_prediction(h264dec_obj *pDec, xint block);
xint dec_intra_chroma_prediction(h264dec_obj *pDec);

/* decoder Intra 16x16 prediction mode routines */
xint dec_intra_16x16_DC_pred(h264dec_obj *pDec);
xint dec_intra_16x16_H_pred(h264dec_obj *pDec);
xint dec_intra_16x16_V_pred(h264dec_obj *pDec);
xint dec_intra_16x16_PL_pred(h264dec_obj *pDec);

/* decoder Intra 4x4 prediction mode routines */
xint dec_intra_4x4_V_pred(h264dec_obj *pDec, xint block);
xint dec_intra_4x4_H_pred(h264dec_obj *pDec, xint block);
xint dec_intra_4x4_DC_pred(h264dec_obj *pDec, xint block);
xint dec_intra_4x4_DDL_pred(h264dec_obj *pDec, xint block);
xint dec_intra_4x4_DDR_pred(h264dec_obj *pDec, xint block);
xint dec_intra_4x4_VR_pred(h264dec_obj *pDec, xint block);
xint dec_intra_4x4_HD_pred(h264dec_obj *pDec, xint block);
xint dec_intra_4x4_VL_pred(h264dec_obj *pDec, xint block);
xint dec_intra_4x4_HU_pred(h264dec_obj *pDec, xint block);

/* decoder Intra chroma prediction mode routines */
xint dec_intra_chroma_DC_pred(h264dec_obj *pDec);
xint dec_intra_chroma_H_pred(h264dec_obj *pDec);
xint dec_intra_chroma_V_pred(h264dec_obj *pDec);
xint dec_intra_chroma_PL_pred(h264dec_obj *pDec);

/* Intra chroma prediction mode routines shared in encoder & decoder */
xint intra_chroma_DC_pred(slice_obj *pSlice, uint8  *predictor_cb, uint8  *predictor_cr, int nx_mb, int constrained_intra_pred_flag );



xint dec_intra_prediction(h264dec_obj *pDec);

#ifdef __cplusplus
extern "C"
}
#endif

#define _INTRA_PRED_H_
#endif
