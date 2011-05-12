/* ///////////////////////////////////////////////////////////// */
/*   File: vlc.h                                                 */
/*   Author: JM Authors                                          */
/*   Date: Jan/??/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec CAVLC routines.            */
/*   extracted from JM                                           */
/*                                                               */
/*   See JM Copyright Notice.                                    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   @Jerry Peng Modify the mechanism                            */
/* ///////////////////////////////////////////////////////////// */



#ifndef _VLC_H_

#include "metypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

xint write_block_cavlc(h264enc_obj *pEnc, int16 *pLevel, int16 *pRun, xint residual_type, xint block, xint YCbCr, xint coded);

#ifdef __cplusplus
extern "C"
}
#endif

#define _VLC_H_
#endif
