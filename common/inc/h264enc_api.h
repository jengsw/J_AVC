/* ///////////////////////////////////////////////////////////// */
/*   File: h264enc_api.h                                         */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 encoder API.                     */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _H264_ENC_H_
#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* H.264 specific encoder routines */
xint h264_init_encoder(h264enc_obj *pEnc, enc_cfg *pCtrl, xint width, xint height);
xint h264_free_encoder(h264enc_obj *pEnc);
xint h264_init_bitstream(h264enc_obj *pEnc, uint8 *pBuf, xint size);
xint h264_init_slice(h264enc_obj *pEnc, RBSP_TYPE type, xint no_mb);
xint h264_encode_video_header(h264enc_obj *pEnc);
xint h264_encode_slice_header(h264enc_obj *pEnc);
xint h264_encode_slice(h264enc_obj *pEnc, xint *pDone);

/* Gneral video encoder routines */
RBSP_TYPE slice_mode_select(h264enc_obj *pEnc);

#ifdef __cplusplus
}
#endif

#define _H264_ENC_H_
#endif
