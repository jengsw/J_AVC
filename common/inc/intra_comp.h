/* ///////////////////////////////////////////////////////////// */
/*   File: intra_comp.h                                          */
/*   Author: Jerry Peng                                          */
/*   Date: Feb/15/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 intra prediction module.         */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _INTRA_COMP_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* encoder routines */
xint enc_intra_16x16_compensation(h264enc_obj *pEnc);
xint enc_intra_4x4_compensation(h264enc_obj *pEnc, xint block);
xint enc_intra_chroma_compensation(h264enc_obj *pEnc);

/* decoder routines */
xint dec_intra_16x16_compensation(h264dec_obj *pDec);
xint dec_intra_4x4_compensation(h264dec_obj *pDec, xint block);
xint dec_intra_chroma_compensation(h264dec_obj *pDec);

xint dec_inter_4x4_compensation(h264dec_obj *pDec, xint block);
xint dec_inter_chroma_compensation(h264dec_obj *pDec);

#ifdef __cplusplus
}
#endif

#define _INTRA_COMP_H_
#endif
