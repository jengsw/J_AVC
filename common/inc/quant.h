/* ///////////////////////////////////////////////////////////// */
/*   File: quant.h                                               */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec quantization module.       */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _QUANT_H_

#include "../inc/h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

xint init_quant_matrix(void);

xint quant_16x16(h264enc_obj *pEnc);
xint quant_4x4(h264enc_obj *pEnc, xint block);
xint quant_chroma(h264enc_obj *pEnc);
xint inv_quant_16x16(slice_obj *pSlice);
xint inv_quant_4x4(slice_obj *pSlice, xint block);
xint inv_quant_chroma(slice_obj *pSlice);
xint eliminateLumaDCAC( mb_obj *pMB, xint block );
xint quantLumaDCAC_cost( int16 *coeff );
xint quantChromaAC_cost( int16 *coeff );

xint mb_coding(h264enc_obj *pEnc);
xint mb_decoding(h264enc_obj *pEnc);

#ifdef __cplusplus
extern "C"
}
#endif

#define _QUANT_H_
#endif