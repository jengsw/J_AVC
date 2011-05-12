/* ///////////////////////////////////////////////////////////// */
/*   File: trans.h                                               */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec transform module.          */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _TRANS_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* external functions */
xint trans_16x16(h264enc_obj *pEnc);
xint intra_RDCost_16x16(h264enc_obj * pEnc);
xint trans_4x4(h264enc_obj *pEnc, xint block);
xint intra_RDCost_4x4(h264enc_obj * pEnc, xint block);
xint trans_chroma(h264enc_obj *pEnc);
xint intra_chroma_RDCost(h264enc_obj * pEnc);
xint inv_trans_16x16(slice_obj *pSlice);
xint inv_trans_4x4(slice_obj *pSlice, xint block);
xint inv_trans_chroma(slice_obj *pSlice);

/* internal functions */
xint core_transform(int16 * transform_buffer);
xint hadamard4x4_transform(int16 *transform_buffer);
xint hadamard4x4_transform_orig(int16 * transform_buffer);
xint inv_hadamard_transform(int16 *transform_buffer);

#ifdef __cplusplus
extern "C"
}
#endif

#define _TRANS_H_
#endif
