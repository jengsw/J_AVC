/* ///////////////////////////////////////////////////////////// */
/*   File: inter_comp.h                                          */
/*   Author: Jerry Peng                                          */
/*   Date: Oct/05/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 intra prediction module.         */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _INTER_COMP_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* encoder routines */
xint enc_cal_inter_comp_mb(h264enc_obj *pEnc);
xint dec_cal_inter_comp_mb(h264dec_obj *pDec);

#ifdef __cplusplus
}
#endif

#define _INTER_COMP_H_
#endif
