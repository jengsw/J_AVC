/* ///////////////////////////////////////////////////////////// */
/*   File: bitstream.h                                           */
/*   Author: Jerry Peng                                          */
/*   Date: Mar/06/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 bitstream read/write module.     */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/*   Multimedia Embedded Systems Lab.                            */
/*   Department of Computer Science and Information engineering  */
/*   National Chiao Tung University, Hsinchu 300, Taiwan         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

void   stream_init(h264dec_obj *pDec, uint8 *bitbuf, uint32 bufsize);
uint32 residual_bits(h264dec_obj *pDec);
uint32 get_bits(h264dec_obj *pDec, uint32 nbits);
uint32 show_bits(h264dec_obj *pDec, uint32 nbits);
void   skip_bits(h264dec_obj *pDec, uint32 nbits);
void   rewind_bits(h264dec_obj *pDec, int32 nbits);
xint   put_bits(h264enc_obj *pEnc, uint32 code, xint nbits);
xint   put_one_bit(h264enc_obj *pEnc, uint32 code);
xint   byte_align(void *pHnd, xint EorD);

void   dectobin(uint8 *buf, uint32 value, uint32 nbits);

static xint __inline get_bitpos(h264enc_obj *pEnc)
{
    return (pEnc->byte_pos<<3) + pEnc->bit_pos;
}

#ifdef __cplusplus
}
#endif

#endif /* _BITSTREAM_H_ */
