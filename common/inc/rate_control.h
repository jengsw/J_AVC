/* ///////////////////////////////////////////////////////////// */
/*   File: rate_control.c                                        */
/*   Author: Jerry Peng                                          */
/*   Date: Jun/03/2006                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 Rate Control Module.             */
/*                                                               */
/*   Copyright, 2006.                                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */



#ifndef _RATE_CONTROL_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

xint  rc_init(h264enc_obj *pEnc, xint target_rate);
xint  rc_update_buf(rc_obj *pRC, xint size);
xint  rc_get_frame_QP(rc_obj *pRC, xint type);
xint  rc_update_data(rc_obj *pRC, xint type, xint qp, xint size);
float rc_get_p_coeff(rc_obj *pRC);
float rc_get_i_coeff(rc_obj *pRC);
xint  rc_constrain_i_qp(rc_obj *pRC, xint target, xint frame_QP);
xint  rc_constrain_p_qp(rc_obj *pRC, xint target, xint frame_QP);
xint  rc_search_step(float step);

#ifdef __cplusplus
extern "C"
}
#endif

#define _RATE_CONTROL_H_
#endif