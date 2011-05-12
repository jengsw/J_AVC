/* ///////////////////////////////////////////////////////////// */
/*   File: scan.h                                                */
/*   Author: JM Authors                                          */
/*   Date: Jan/??/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec CAVLC routines. This file  */
/*   is extracted from JM 9.2 by Jerry Peng.                     */
/*                                                               */
/*   Copyright, 2004.                                            */
/*   See JM Copyright Notice.                                    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#ifndef _SCAN_H_
#include "metypes.h"

xint zigzag_runlevel(int16 *pInput, int16 *pRun, int16 *pLevel, xint block_type);
xint chromaDC_runlevel(int16 *pInput, int16 *pRun, int16 *pLevel);
xint inv_zigzag(int16 * pIn, int16 * pOut, xint maxnumcoef, xint start);

#define _SCAN_H_
#endif
