/* ///////////////////////////////////////////////////////////// */
/*   File: mv_search.h                                           */
/*   Author: Jerry Peng                                          */
/*   Date: Oct/06/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 Motion Estimation Module.        */
/*                                                               */
/*   Copyright, 2005.                                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _MV_SEARCH_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define LAMBDA_ACCURACY_BITS        16
#define LAMBDA_FACTOR(lambda)       ((int)((double)(1<<LAMBDA_ACCURACY_BITS)*lambda+0.5))

xint me_set_lambda(h264enc_obj *pEnc, double lambda);
xint me_init_encoder(h264enc_obj *pEnc);
xint me_init_frame(h264enc_obj *pEnc);
xint me_init_slice(h264enc_obj *pEnc);
xint me_free_encoder(h264enc_obj *pEnc);
xint enc_inter_mode_select_mb(h264enc_obj *pEnc);
void enc_skip_mode_select_mb(h264enc_obj *pEnc);
xint me_downsample_frame(frame_obj *pFrame, int width, int height);

#ifdef __cplusplus
extern "C"
}
#endif

#define _MV_SEARCH_H_
#endif