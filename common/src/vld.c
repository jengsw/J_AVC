/* ///////////////////////////////////////////////////////////// */
/*   File: vld.c                                                 */
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

#include "../../common/inc/misc_util.h"
#include "../../common/inc/cavlc.h"
#include "../../common/inc/vld.h"
#include "../../common/inc/nal.h"
#include "../../common/inc/h264_def.h"
#include "../../common/inc/bitstream.h"
#include <string.h>

/* Table 9-6 */
uint32
parse_level_prefix(h264dec_obj * pDec)
{
    uint32  level_prefix;
    uint32  b;

    for (level_prefix = 0; level_prefix < 17; level_prefix++)
    {
        if(b = get_bits(pDec, 1))
            break;
    }

    if (level_prefix == 16)
        alert_msg("parse level_prefix error !\n");

    return level_prefix;

}

/* 9.2.2 */
uint16
parse_level_suffix(h264dec_obj * pDec, uint32 level_prefix,
                   xint suffixLength)
{
    uint32  levelSuffixSize;
    xint    level_suffix;

    if ((level_prefix == 14) && (suffixLength == 0))
        levelSuffixSize = 4;
    else if (level_prefix == 15)
        levelSuffixSize = 12;
    else
        levelSuffixSize = suffixLength;

    if (levelSuffixSize > 0)
        level_suffix = get_bits(pDec, levelSuffixSize);
    else if (levelSuffixSize == 0)
        level_suffix = 0;
    else
    {
        alert_msg("parse level_suffix error !\n");
        fprintf(stderr, "parse level_suffix error !\n");
    }

    return level_suffix;

}

uint16
parse_total_zeros(h264dec_obj *pDec, xint total_coeff,
                  xint maxNumCoeff)
{
    uint16  total_zeros;
    uint32  length;
    uint    pattern;

    xint    lentab[15][16] = {
        /*  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  */
        {1, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 9}
        ,
        {3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6, 6, 6}
        ,
        {4, 3, 3, 3, 4, 4, 3, 3, 4, 5, 5, 6, 5, 6}
        ,
        {5, 3, 4, 4, 3, 3, 3, 4, 3, 4, 5, 5, 5}
        ,
        {4, 4, 4, 3, 3, 3, 3, 3, 4, 5, 4, 5}
        ,
        {6, 5, 3, 3, 3, 3, 3, 3, 4, 3, 6}
        ,
        {6, 5, 3, 3, 3, 2, 3, 4, 3, 6}
        ,
        {6, 4, 5, 3, 2, 2, 3, 3, 6}
        ,
        {6, 6, 4, 2, 2, 3, 2, 5}
        ,
        {5, 5, 3, 2, 2, 2, 4}
        ,
        {4, 4, 3, 3, 1, 3}
        ,
        {4, 4, 2, 1, 3}
        ,
        {3, 3, 1, 2}
        ,
        {2, 2, 1}
        ,
        {1, 1}
        ,
    };

    uint16  codtab[15][16] = {
        /*  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  */
        {1, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1}
        ,
        {7, 6, 5, 4, 3, 5, 4, 3, 2, 3, 2, 3, 2, 1, 0}
        ,
        {5, 7, 6, 5, 4, 3, 4, 3, 2, 3, 2, 1, 1, 0}
        ,
        {3, 7, 5, 4, 6, 5, 4, 3, 3, 2, 2, 1, 0}
        ,
        {5, 4, 3, 7, 6, 5, 4, 3, 2, 1, 1, 0}
        ,
        {1, 1, 7, 6, 5, 4, 3, 2, 1, 1, 0}
        ,
        {1, 1, 5, 4, 3, 3, 2, 1, 1, 0}
        ,
        {1, 1, 1, 3, 3, 2, 2, 1, 0}
        ,
        {1, 0, 1, 3, 2, 1, 1, 1,}
        ,
        {1, 0, 1, 3, 2, 1, 1,}
        ,
        {0, 1, 1, 2, 1, 3}
        ,
        {0, 1, 1, 1, 1}
        ,
        {0, 1, 1, 1}
        ,
        {0, 1, 1}
        ,
        {0, 1}
        ,
    };

    xint    chromaDC_lentab[3][4] = {
        {1, 2, 3, 3}
        ,
        {1, 2, 2}
        ,
        {1, 1}
        ,
    };

    uint16  chromaDC_codtab[3][4] = {
        {1, 1, 1, 0}
        ,
        {1, 1, 0}
        ,
        {1, 0}
        ,
    };

    if (maxNumCoeff != 4)
    {
        for (total_zeros = 0; total_zeros < 16; total_zeros++)
        {
            length = lentab[total_coeff - 1][total_zeros];
            if (length == 0)
                continue;

            pattern = show_bits(pDec, length);

            if (pattern == codtab[total_coeff - 1][total_zeros])
            {
                get_bits(pDec, length);
                break;
            }
        }

    }
    else
    {
        //find the chroma DC 2x2 blocks, Table 9-9
        for (total_zeros = 0; total_zeros < maxNumCoeff; total_zeros++)
        {
            length = chromaDC_lentab[total_coeff - 1][total_zeros];
            if (length == 0 || residual_bits(pDec) < length)
                continue;

            pattern = show_bits(pDec, length);

            if (pattern == chromaDC_codtab[total_coeff - 1][total_zeros])
            {
                get_bits(pDec, length);
                break;
            }
        }
    }

    return total_zeros;

}

uint32
parse_run_before(h264dec_obj * pDec, xint zerosLeft)
{
    uint32  run_before;
    uint32  pattern;
    uint32  length;

    xint    lentab[7][16] = {
        {1, 1}
        ,
        {1, 2, 2}
        ,
        {2, 2, 2, 2}
        ,
        {2, 2, 2, 3, 3}
        ,
        {2, 2, 3, 3, 3, 3}
        ,
        {2, 3, 3, 3, 3, 3, 3}
        ,
        {3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11}
        ,
    };

    uint16  codtab[7][16] = {
        {1, 0}
        ,
        {1, 1, 0}
        ,
        {3, 2, 1, 0}
        ,
        {3, 2, 1, 1, 0}
        ,
        {3, 2, 3, 2, 1, 0}
        ,
        {3, 0, 1, 3, 2, 5, 4}
        ,
        {7, 6, 5, 4, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1}
        ,
    };

    if (zerosLeft > 7)
        zerosLeft = 7;

    for (run_before = 0; run_before < 15; run_before++)
    {
        length = lentab[zerosLeft - 1][run_before];
        if (length == 0 || residual_bits(pDec) < length)
            continue;

        pattern = show_bits(pDec, length);

        if (pattern == codtab[zerosLeft - 1][run_before])
        {
            get_bits(pDec, length);
            break;
        }
    }

    return run_before;

}

/* Table 9-5 */
xint
parse_coeff_token(h264dec_obj *pDec, xint * total_coeff,
                  xint * trailing_ones, xint vlcnum)
{
    uint32  pattern;
    xint    ones, total;
    uint32  length;

    const xint lentab[3][4][17] = {
        {                       // 0702
         {1, 6, 8, 9, 10, 11, 13, 13, 13, 14, 14, 15, 15, 16, 16, 16,
          16},
         {0, 2, 6, 8, 9, 10, 11, 13, 13, 14, 14, 15, 15, 15, 16, 16,
          16},
         {0, 0, 3, 7, 8, 9, 10, 11, 13, 13, 14, 14, 15, 15, 16, 16, 16},
         {0, 0, 0, 5, 6, 7, 8, 9, 10, 11, 13, 14, 14, 15, 15, 16, 16},
         },
        {
         {2, 6, 6, 7, 8, 8, 9, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14},
         {0, 2, 5, 6, 6, 7, 8, 9, 11, 11, 12, 12, 13, 13, 14, 14, 14},
         {0, 0, 3, 6, 6, 7, 8, 9, 11, 11, 12, 12, 13, 13, 13, 14, 14},
         {0, 0, 0, 4, 4, 5, 6, 6, 7, 9, 11, 11, 12, 13, 13, 13, 14},
         },
        {
         {4, 6, 6, 6, 7, 7, 7, 7, 8, 8, 9, 9, 9, 10, 10, 10, 10},
         {0, 4, 5, 5, 5, 5, 6, 6, 7, 8, 8, 9, 9, 9, 10, 10, 10},
         {0, 0, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 10},
         {0, 0, 0, 4, 4, 4, 4, 4, 5, 6, 7, 8, 8, 9, 10, 10, 10},
         },
    };

    const uint codtab[3][4][17] = {
        {
         {1, 5, 7, 7, 7, 7, 15, 11, 8, 15, 11, 15, 11, 15, 11, 7, 4},
         {0, 1, 4, 6, 6, 6, 6, 14, 10, 14, 10, 14, 10, 1, 14, 10, 6},
         {0, 0, 1, 5, 5, 5, 5, 5, 13, 9, 13, 9, 13, 9, 13, 9, 5},
         {0, 0, 0, 3, 3, 4, 4, 4, 4, 4, 12, 12, 8, 12, 8, 12, 8},
         },
        {
         {3, 11, 7, 7, 7, 4, 7, 15, 11, 15, 11, 8, 15, 11, 7, 9, 7},
         {0, 2, 7, 10, 6, 6, 6, 6, 14, 10, 14, 10, 14, 10, 11, 8, 6},
         {0, 0, 3, 9, 5, 5, 5, 5, 13, 9, 13, 9, 13, 9, 6, 10, 5},
         {0, 0, 0, 5, 4, 6, 8, 4, 4, 4, 12, 8, 12, 12, 8, 1, 4},
         },
        {
         {15, 15, 11, 8, 15, 11, 9, 8, 15, 11, 15, 11, 8, 13, 9, 5, 1},
         {0, 14, 15, 12, 10, 8, 14, 10, 14, 14, 10, 14, 10, 7, 12, 8,
          4},
         {0, 0, 13, 14, 11, 9, 13, 9, 13, 10, 13, 9, 13, 9, 11, 7, 3},
         {0, 0, 0, 12, 11, 10, 9, 8, 13, 12, 12, 12, 8, 12, 10, 6, 2},
         },

    };

    const xint ChromaDC_lentab[4][5] = {
        {2, 6, 6, 6, 6},
        {0, 1, 6, 7, 8},
        {0, 0, 3, 7, 8},
        {0, 0, 0, 6, 7},
    };

    const uint16 ChromaDC_codtab[4][5] = {
        {1, 7, 4, 3, 2},
        {0, 1, 6, 3, 3},
        {0, 0, 1, 2, 2},
        {0, 0, 0, 5, 0},
    };

    if (vlcnum == 3)
    {
        pattern = get_bits(pDec, 6);
        *total_coeff   = pattern >> 2;
        *trailing_ones = pattern & 3;

        if ((*total_coeff) == 0 && (*trailing_ones) == 3)
            *trailing_ones = 0;
        else
            *total_coeff += 1;

        return MMES_NO_ERROR;
    }
    else if (vlcnum < 3)
    {
        for (ones = 0; ones < 4; ones++)
        {
            for (total = 0; total < 17; total++)
            {
                length = lentab[vlcnum][ones][total];
                if (length == 0 || residual_bits(pDec) < length)
                    continue;

                pattern = show_bits(pDec, length);

                if (pattern == codtab[vlcnum][ones][total])
                {
                    get_bits(pDec, length);
                    *total_coeff = total;
                    *trailing_ones = ones;
                    return MMES_NO_ERROR;
                }

            }
        }
    }
    else if (vlcnum == 4)
    {
        // for Chroma DC block
        for (ones = 0; ones < 4; ones++)
        {
            for (total = 0; total < 5; total++)
            {
                length = ChromaDC_lentab[ones][total];
                if (length == 0 || residual_bits(pDec) < length)
                    continue;

                pattern = show_bits(pDec, length);

                if (pattern == ChromaDC_codtab[ones][total])
                {
                    get_bits(pDec, length);
                    *total_coeff = total;
                    *trailing_ones = ones;
                    return MMES_NO_ERROR;
                }
            }
        }
    }
    else
    {
        alert_msg("parse coeff_token vlcnum error !\n");
        fprintf(stderr, "parse coeff_token vlcnum error !\n");
    }

    alert_msg("parse coeff_token error !\n");
    fprintf(stderr, "parse coeff_token error !\n");

    return TABLE_ERROR;
}

xint
residual_block_cavlc(h264dec_obj *pDec, int16 * coeffLevel, xint block_type, xint block, xint YCbCr, xint coded)
{
    xint        i = 0;
    xint        level[16], run[16];
    xint        suffixLength = 0;
    uint32      trailing_ones_sign_flag = 0;
    uint32      level_prefix = 0;
    int32       levelCode = 0;
    uint32      level_suffix = 0;
    uint32      total_zeros = 0;
    uint32      zerosLeft = 0;
    uint32      run_before = 0;
    int32       coeffNum = 0;
    xint        total_coeff = 0;
    xint        trailing_ones = 0;
    xint        nnz = 0;
    xint        table_idx = 0;
    xint        maxNumCoeff = 0;
    xint        frame_x_16_offset, frame_x_4_offset;
    xint        mb_y_4_offset, mb_x_4_offset;
    xint        nA, nB, nA_valid, nB_valid;
   
    slice_obj *pSlice = pDec->curr_slice;
   
    switch(block_type)
    {
    case LumaLevel:
        maxNumCoeff = 16;
        break;
    case Intra16x16DCLevel:
        maxNumCoeff = 16;
        break;
    case Intra16x16ACLevel:
        maxNumCoeff = 15;
        break;
    case ChromaDCLevel:
        maxNumCoeff = 4;
        break;
    case ChromaACLevel:
        maxNumCoeff = 15;
        break;
    default:
        alert_msg("residual_block_cavlc error !\n");
        return MMES_ERROR;
    }
   
    memset(coeffLevel, 0, sizeof(int16) * maxNumCoeff);
    memset(level     , 0, sizeof(xint)  * 16);
    memset(run       , 0, sizeof(xint)  * 16);

    mb_x_4_offset     = mb_x_4_idx[block];
    mb_y_4_offset     = mb_y_4_idx[block];
    frame_x_16_offset = pDec->curr_slice->cmb.id % pDec->nx_mb;

    if (block_type != ChromaDCLevel)
    {
        nA_valid = nB_valid = MMES_VALID;
        switch(YCbCr)
        {
        case Y_block:
            frame_x_4_offset = frame_x_16_offset * 4 + mb_x_4_offset;
            nA = pSlice->top_y_nzc[frame_x_4_offset];
            if(nA<0)
            {
                nA_valid = MMES_INVALID;
                nA = 0;
            }
            nB = pSlice->left_y_nzc[mb_y_4_offset];
            if(nB<0)
            {
                nB_valid = MMES_INVALID;
                nB = 0;
            }
            break;
        case Cb_block:
            frame_x_4_offset = frame_x_16_offset * 2 + mb_x_4_offset;
            nA = pSlice->top_cb_nzc[frame_x_4_offset];     // for YCbCr420
            if(nA<0)
            {
                nA_valid = MMES_INVALID;
                nA = 0;
            }
            nB = pSlice->left_cb_nzc[mb_y_4_offset];
            if(nB<0)
            {
                nB_valid = MMES_INVALID;
                nB = 0;
            }
            break;
        case Cr_block:
            frame_x_4_offset = frame_x_16_offset*2+mb_x_4_offset;
            nA = pSlice->top_cr_nzc[frame_x_4_offset];     // for YCbCr420
            if(nA<0)
            {
                nA_valid = MMES_INVALID;
                nA = 0;
            }
            nB = pSlice->left_cr_nzc[mb_y_4_offset];
            if(nB<0)
            {
                nB_valid = MMES_INVALID;
                nB = 0;
            }
            break;
        default:
            break;
        }
        
        if(nA_valid && nB_valid)
        {
            nnz = (nA + nB + 1)>>1;
        }
        else
            nnz = nA + nB;

        if (nnz < 2)
            table_idx = 0;
        else if (nnz < 4)
            table_idx = 1;
        else if (nnz < 8)
            table_idx = 2;
        else
            table_idx = 3;
    }
    else
        table_idx = 4;

    if(coded)
    {
        if(parse_coeff_token(pDec, &total_coeff, &trailing_ones, table_idx) == TABLE_ERROR)
            return MMES_ERROR;
    }

    alert_msg("number of coeff = %d\n", total_coeff);
    alert_msg("number of T1    = %d\n", trailing_ones);
    alert_msg("T1 table num    = %d\n", table_idx);

    if(coded == 0)
    {
        total_coeff = trailing_ones = 0;
    }

    // update the number of non-zero coeff info
    if (block_type != ChromaDCLevel)
    {        
        switch(YCbCr)
        {
        case Y_block:
            frame_x_4_offset = frame_x_16_offset * 4 + mb_x_4_offset;
            if(block_type != Intra16x16DCLevel)
            {
                pSlice->top_y_nzc[frame_x_4_offset] = total_coeff;
                
                if(frame_x_4_offset == ((pDec->nx_mb*4)-1))     // leftest block
                    pSlice->left_y_nzc[mb_y_4_offset] = -1;
                else
                    pSlice->left_y_nzc[mb_y_4_offset] = total_coeff;
            }
            break;
        case Cb_block:
            frame_x_4_offset = frame_x_16_offset * 2 + mb_x_4_offset;
            pSlice->top_cb_nzc[frame_x_4_offset] = total_coeff;
            if(frame_x_4_offset == ((pDec->nx_mb*2)-1))
            {
                pSlice->left_cb_nzc[mb_y_4_offset] = -1;
            }
            else
                pSlice->left_cb_nzc[mb_y_4_offset] = total_coeff;
            break;
        case Cr_block:
            frame_x_4_offset = frame_x_16_offset * 2 + mb_x_4_offset;
            pSlice->top_cr_nzc[frame_x_4_offset] = total_coeff;
            if(frame_x_4_offset == ((pDec->nx_mb*2)-1))
            {
                pSlice->left_cr_nzc[mb_y_4_offset] = -1;
            }
            else
                pSlice->left_cr_nzc[mb_y_4_offset] = total_coeff;
            break;
        }
        
    }

    if(coded == 0)
        return MMES_NO_ERROR;

    if (total_coeff > 0)
    {
        if (total_coeff > 10 && trailing_ones < 3)
            suffixLength = 1;
        else
            suffixLength = 0;

        for (i = 0; i < total_coeff; i++)
        {
            if (i < trailing_ones)
            {
                // get trailing_ones_sign_flag
                trailing_ones_sign_flag = get_bits(pDec, 1);

                level[i] = 1 - 2 * trailing_ones_sign_flag;

                alert_msg("%2d T1           = %2d\n", i,
                        level[i]);
            }
            else
            {
                // get level_prefix
                level_prefix = parse_level_prefix(pDec);

                levelCode = (level_prefix << suffixLength);

                if (suffixLength > 0 || level_prefix >= 14)
                {
                    // get level_suffix
                    level_suffix = parse_level_suffix(pDec, level_prefix, suffixLength);

                    levelCode += level_suffix;
                }

                if (level_prefix == 15 && suffixLength == 0)
                    levelCode += 15;

                if (i == trailing_ones && trailing_ones < 3)
                    levelCode += 2;

                if ((levelCode % 2) == 0)
                    level[i] = (levelCode + 2) >> 1;
                else
                    level[i] = (-levelCode - 1) >> 1;

                alert_msg("%2d level       = %2d \n", i,
                        level[i]);

                if (suffixLength == 0)
                    suffixLength = 1;

                if (ABS(level[i]) > (3 << (suffixLength - 1))
                    && suffixLength < 6)
                    suffixLength++;
            }
        }

        if (total_coeff < maxNumCoeff)
        {
            // get total_zero;
            total_zeros =
                parse_total_zeros(pDec, total_coeff, maxNumCoeff);

            alert_msg("total zeros     = %d\n", total_zeros);
            zerosLeft = total_zeros;
        }
        else
        {
            zerosLeft = 0;
        }

        for (i = 0; i < total_coeff - 1; i++)
        {
            if (zerosLeft > 0)
            {
                // get run_before
                run_before = parse_run_before(pDec, zerosLeft);
                run[i] = run_before;
                alert_msg("%2d Run before  = %2d\n", i, run[i]);
            }
            else
            {
                run[i] = 0;
            }
            zerosLeft = zerosLeft - run[i];
        }

        run[total_coeff - 1] = zerosLeft;
        coeffNum = -1;
        for (i = total_coeff - 1; i >= 0; i--)
        {
            coeffNum += run[i] + 1;
            coeffLevel[coeffNum] = level[i];
        }
    }

    return MMES_NO_ERROR;
}
