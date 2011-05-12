/* ///////////////////////////////////////////////////////////// */
/*   File: deblock.h                                             */
/*   Author: JM Authors                                          */
/*   Date: Jan/??/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec deblock filter routines.   */
/*   This file is extracted from JM 9.2.                         */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/*   See JM Copyright Notice.                                    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _DEBLOCK_H_

#include "h264_def.h"

#ifdef __cplusplus
extern "C"
{
#endif

xint deblock(slice_obj *pSlice, int width, frame_obj *recf);

#ifdef __cplusplus
}
#endif

#define _DEBLOCK_H_
#endif
