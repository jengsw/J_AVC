/* ///////////////////////////////////////////////////////////// */
/*   File: scan.c                                                */
/*   Author: JM Author                                           */
/*   Date: Jan/??/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec CAVLC routines.            */
/*   extracted by Jerry Peng                                     */
/*   Copyright, 2004-2005.                                       */
/*   See JM Copyright Notice.                                    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#include "../../common/inc/misc_util.h"
#include "../../common/inc/cavlc.h"

static const uint16 zigzag_tab[16] = {
    0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15
};

xint
zigzag_runlevel(int16 * pInput, int16 * pRun, int16 * pLevel,
                xint block_type)
{
    xint    coeff_ctr;
    int16   run;
    int16   level;
    xint    scan_pos = 0;

    if (block_type == LumaLevel || block_type == Intra16x16DCLevel)
        coeff_ctr = 0;
    else
        coeff_ctr = 1;

    run = -1;

    for (; coeff_ctr < 16; coeff_ctr++)
    {
        level = pInput[zigzag_tab[coeff_ctr]];

        run++;

        if (level != 0)
        {
            pLevel[scan_pos] = level;
            pRun[scan_pos] = run;
            run = -1;
            scan_pos++;
        }

    }
    pLevel[scan_pos] = 0;

    return NO_ERROR;
}

xint
chromaDC_runlevel(int16 * pInput, int16 * pRun, int16 * pLevel)
{
    xint    idx;
    int16   level;
    int16   run;
    xint    scan_pos = 0;

    run = -1;

    for (idx = 0; idx < 4; idx++)
    {
        level = pInput[idx];

        run++;

        if (level != 0)
        {
            pLevel[scan_pos] = level;
            pRun[scan_pos] = run;
            run = -1;
            scan_pos++;
        }
    }
    pLevel[scan_pos] = 0;

    return NO_ERROR;
}

xint
inv_zigzag(int16 * pIn, int16 * pOut, xint maxnumcoef, xint start)
{
    xint    coeff_ctr;
    xint    zigzag_idx;

    for (coeff_ctr = 0; coeff_ctr < maxnumcoef; coeff_ctr++)
    {
        zigzag_idx = (maxnumcoef > 4) ? zigzag_tab[coeff_ctr + start] : coeff_ctr;
        pOut[zigzag_idx] = pIn[coeff_ctr];
    }

    return NO_ERROR;
}
