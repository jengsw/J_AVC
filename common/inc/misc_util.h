/* ///////////////////////////////////////////////////////////// */
/*   File: misc_util.h                                           */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec utility routines.          */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/*   Multimedia Embedded Systems Lab.                            */
/*   Department of Computer Science and Information engineering  */
/*   National Chiao Tung University, Hsinchu 300, Taiwan         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _MISC_UTIL_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

xint write_frame(frame_obj *frm, 
            xint width, xint height, 
            xint crop_left, xint crop_right,
            xint crop_top, xint crop_bottom,
            xint chroma_format_idc,
            FILE *fp);

frame_obj  *alloc_frame(xint width, xint height);
void        free_frame(frame_obj *pFrame);
slice_obj  *alloc_slice(xint width);
void        free_slice(slice_obj *pSlice);
backup_obj *alloc_backup(xint width);
void        free_backup(backup_obj *pBackup);
double      calc_psnr(uint8 *orig, uint8 *buf, uint32 width, uint32 height);
uint32      GetTimerCount(void);
xint        dup_frame_to_frame( frame_obj* pDst, frame_obj* pSrc, xint width, xint height );
xint        dup_mb(frame_obj* pDst, frame_obj* pSrc, xint mb_id, xint width);
xint		dup_16x16_to_frame( frame_obj* pDst, int16 *py, int16 *pcb, int16 *pcr, xint mb_id, xint width );
xint        dup_4x4_to_frame( frame_obj* pDst, int16 *pSrc, xint mb_id, xint width, xint block );
xint        reset_slice(slice_obj *pSlice, int width);
xint        binary_format(int8 *bin, int32 value, xint len);
xint        update_MB(slice_obj *pSlice, int width);
xint        swap_frame(frame_obj *frame1, frame_obj *frame2);
xint        gen_ref_frame (h264enc_obj *pEnc);
xint		check_reconstruct_frame(h264enc_obj *pEnc);
xint        init_ref_lists(h264dec_obj *pDec);
xint        reorder_lists(h264dec_obj *pDec);
xint        adjust_ref_lists(h264dec_obj *pDec);
xint        init_picture(h264dec_obj *pDec);
xint        check_frame_gaps(h264dec_obj *pDec);
void        restore_intra_flag( uint8 *dest, uint8 *src, int size);


//@jerry
#define SWAP(A,B,TT) {TT TMP;\
                      TMP=A;\
                      A=B;\
                      B=TMP;}

void         mmes_log(char *format, ...);
void         alert_msg(char *format, ...);

// @jerry
static uint __inline log2bin(uint value)
{
	uint n = 0;
	while (value)
	{
		value >>= 1;
		n++;
	}
	return n;
}

#ifdef __cplusplus
//extern "C"
}
#endif

#define _MISC_UTIL_H_
#endif
