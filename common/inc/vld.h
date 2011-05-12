/* ///////////////////////////////////////////////////////////// */
/*   File: vld.h                                                 */
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


#ifndef _VLD_H_

#include "metypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

xint residual_block_cavlc(h264dec_obj *pDec, int16 * coeffLevel, xint block_type, xint block, xint YCbCr, xint coded);

#ifdef __cplusplus
extern "C"
}
#endif

#define _VLD_H_
#endif

