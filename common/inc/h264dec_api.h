/* ///////////////////////////////////////////////////////////// */
/*   File: h264enc_api.h                                         */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 decoder API.                     */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/*   Multimedia Embedded Systems Lab.                            */
/*   Department of Computer Science and Information engineering  */
/*   National Chiao Tung University, Hsinchu 300, Taiwan         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _H264_DEC_H_
#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

xint h264_init_decoder(h264dec_obj *pDec, xint realloc);
xint h264_free_decoder(h264dec_obj *pDec);
xint h264_decode_header(h264dec_obj *pDec, FILE *fp_bit);
xint h264_decode_slice(h264dec_obj *pDec, uint8 *pBuf);

#ifdef __cplusplus
}
#endif

#define _H264_DEC_H_
#endif
