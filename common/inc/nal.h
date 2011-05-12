/* ///////////////////////////////////////////////////////////// */
/*   File: nal.h                                                 */
/*   Author: Jerry Peng                                          */
/*   Date: Mar/06/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 NAL encode/decode module.        */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _NAL_H_
#define _NAL_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define NALU_TYPE_SLICE    1
#define NALU_TYPE_DPA      2
#define NALU_TYPE_DPB      3
#define NALU_TYPE_DPC      4
#define NALU_TYPE_IDR      5
#define NALU_TYPE_SEI      6
#define NALU_TYPE_SPS      7
#define NALU_TYPE_PPS      8
#define NALU_TYPE_AUD      9
#define NALU_TYPE_EOSEQ    10
#define NALU_TYPE_EOSTREAM 11
#define NALU_TYPE_FILL     12

#define NALU_PRIORITY_HIGHEST     3
#define NALU_PRIORITY_HIGH        2
#define NALU_PRIRITY_LOW          1
#define NALU_PRIORITY_DISPOSABLE  0

xint write_uvlc_codeword(h264enc_obj *pEnc, uint value);
xint write_signed_uvlc_codeword(h264enc_obj *pEnc, xint value);
xint write_nal_header(h264enc_obj *pEnc, uint ref_idc, NAL_TYPE nal_uint_type);
xint write_seq_paraset_rbsp(h264enc_obj *pEnc);
xint write_pic_paraset_rbsp(h264enc_obj *pEnc);

uint32 read_uvlc_codeword(h264dec_obj *pDec);
int32  read_signed_uvlc_codeword(h264dec_obj *pDec);

xint parse_slice_header(h264dec_obj *pDec);
xint parse_slice_data(h264dec_obj *pDec, xint *pEndOFFrame);
xint parse_pic_paraset_rbsp(h264dec_obj *pDec);
xint parse_seq_paraset_rbsp(h264dec_obj *pDec);

xint init_nalu(nal_obj *nalu);
void destroy_nalu(nal_obj *nalu);
xint get_nalu(h264dec_obj *pDec, FILE *fp_bit);
void ebsptorbsp(nal_obj *nalu);

#ifdef __cplusplus
}
#endif

#define _NAL_H_
#endif /* _NAL_H_ */
