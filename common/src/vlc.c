/* ///////////////////////////////////////////////////////////// */
/*   File: vlc.c                                                 */
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
#include "../../common/inc/bitstream.h"
#include "../../common/inc/cavlc.h"

xint
write_coeff_token(h264enc_obj *pEnc, xint numcoeff, xint numtrailingones, xint table_idx)
{
    uint32  pattern = 0;
    xint    pattern_len = 0;

    static const uint16 lentab[3][4][17] = {
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

    static const xint codtab[3][4][17] = {
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

    static const uint16 cdc_lentab[4][17] = {
        //YUV420
        {2, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 1, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 3, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

    };

    static const xint cdc_codtab[4][17] = {
        //YUV420
        {1, 7, 4, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 1, 6, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

    };

    if (table_idx < 3)
    {
        pattern = codtab[table_idx][numtrailingones][numcoeff];
        pattern_len = lentab[table_idx][numtrailingones][numcoeff];
    }
    else if (table_idx == 3)
    {
        pattern_len = 6;        // 4 + 2 bit FLC

        if (numcoeff > 0)
        {
            pattern = ((numcoeff - 1) << 2) | numtrailingones;
        }
        else
            pattern = 3;
    }
    else                        // ChromaDCLevel
    {
        pattern = cdc_codtab[numtrailingones][numcoeff];
        pattern_len = cdc_lentab[numtrailingones][numcoeff];
    }

    if (pattern_len == 0)
    {
        alert_msg("ERROR: write_coeff_token() !\n");
        exit(-1);
    }

    put_bits(pEnc, pattern, pattern_len);

    return pattern_len;
}

xint
write_trailingones_sign_flag(h264enc_obj *pEnc, uint32 flag_pattern, xint length)
{
    put_bits(pEnc, flag_pattern, length);
    
    return length;
}


/* ----------------------------------------------------------------------- */
/* brief: lev-VLC0                                                         */
/* Code                             Level                                  */
/* 1                                1                                      */
/* 01                               -1                                     */
/* 001                              2                                      */
/* 0001                             -2                                     */
/* ...                              ...                                    */
/* 00000000000001                   -7                                     */
/* 000000000000001xxxs              +-8 ~ +-15                             */
/* 0000000000000001xxxxxxxxxxxs     +-16 ~                                 */
/* ----------------------------------------------------------------------- */      

xint 
write_level_vlc0(h264enc_obj *pEnc, xint level)
{
    xint levabs, sign, pattern_len;
    uint32 pattern;

    levabs = ABS(level);
    sign = (level<0 ? 1 : 0);

    if (levabs < 8)
    {
        pattern_len = levabs * 2 + sign - 1;
        pattern     = 1;
    }
    else if (levabs < 8+8)
    {
        //escape code1 
        pattern_len = 14 + 1 + 4;
        pattern = (1<<4) | ((levabs - 8) << 1) | sign;
    }
    else
    {
        //escape code2
        pattern_len = 14 + 2 + 12;
        pattern = (0x1<<12) | ((levabs - 16) << 1) | sign;
    }

    put_bits(pEnc, pattern, pattern_len);
    
    return pattern_len;

}

/* ----------------------------------------------------------------------- */
/* brief: lev-VLCN, N=1 to 6                                               */
/* if( |level-1| < (15<<(N-1)) )                                           */ 
/*    Code:            0...01x...xs                                        */
/*          where      number of 0's = |level-1|>>(N-1)                    */
/*                     number of x's = N-1                                 */
/*                     value of x's  = |level-1| % 2^(N-1)                 */
/*                     s             = sign bit                            */
/* else                                                                    */ 
/* 28-bit escape code: 0000 0000 0000 0001 xxxx xxxx xxxs                  */
/*          where      value of x's  = |level-1| - (15<<(N-1))             */
/*                     s             = sign bit                            */
/* ----------------------------------------------------------------------- */          

xint
write_level_vlcN(h264enc_obj *pEnc, xint level, xint level_vlcnum)
{
    xint levabs, sign, pattern_len;
    uint32 pattern;
    xint shift;     //N-1
    xint escape;
    xint numPrefix, suffix;

    levabs = ABS(level);
    sign = (level<0 ? 1 : 0); 

    shift = level_vlcnum - 1;
    escape = (15<<shift)+1;

    numPrefix = (levabs-1)>>shift;
    suffix = (levabs-1) - (numPrefix<<shift);

    if(levabs < escape)
    {
        pattern_len = numPrefix + 1 + level_vlcnum;
        pattern = (1<<(shift+1)) | (suffix<<1) | sign;
    }
    else
    {
        pattern_len = 28;
        pattern = (1<<12) | ((levabs-escape)<<1) | sign;
    }

    put_bits(pEnc, pattern, pattern_len);

    return pattern_len;
}


xint
write_total_zeros(h264enc_obj *pEnc, xint total_zeros, xint total_coeff,
                  xint residual_type)
{

    uint32  pattern;
    xint    pattern_len;
    static const xint lentab[15][16] = {
        {1, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 9},
        {3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 6, 6, 6, 6},
        {4, 3, 3, 3, 4, 4, 3, 3, 4, 5, 5, 6, 5, 6},
        {5, 3, 4, 4, 3, 3, 3, 4, 3, 4, 5, 5, 5},
        {4, 4, 4, 3, 3, 3, 3, 3, 4, 5, 4, 5},
        {6, 5, 3, 3, 3, 3, 3, 3, 4, 3, 6},
        {6, 5, 3, 3, 3, 2, 3, 4, 3, 6},
        {6, 4, 5, 3, 2, 2, 3, 3, 6},
        {6, 6, 4, 2, 2, 3, 2, 5},
        {5, 5, 3, 2, 2, 2, 4},
        {4, 4, 3, 3, 1, 3},
        {4, 4, 2, 1, 3},
        {3, 3, 1, 2},
        {2, 2, 1},
        {1, 1},
    };

    static const uint16 codtab[15][16] = {
        {1, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 1},
        {7, 6, 5, 4, 3, 5, 4, 3, 2, 3, 2, 3, 2, 1, 0},
        {5, 7, 6, 5, 4, 3, 4, 3, 2, 3, 2, 1, 1, 0},
        {3, 7, 5, 4, 6, 5, 4, 3, 3, 2, 2, 1, 0},
        {5, 4, 3, 7, 6, 5, 4, 3, 2, 1, 1, 0},
        {1, 1, 7, 6, 5, 4, 3, 2, 1, 1, 0},
        {1, 1, 5, 4, 3, 3, 2, 1, 1, 0},
        {1, 1, 1, 3, 3, 2, 2, 1, 0},
        {1, 0, 1, 3, 2, 1, 1, 1,},
        {1, 0, 1, 3, 2, 1, 1,},
        {0, 1, 1, 2, 1, 3},
        {0, 1, 1, 1, 1},
        {0, 1, 1, 1},
        {0, 1, 1},
        {0, 1},
    };

    static const xint cdc_lentab[3][4] = {
        //YUV420
        {1, 2, 3, 3},
        {1, 2, 2},
        {1, 1},
    };

    static const uint16 cdc_codtab[3][4] = {
        //YUV420
        {1, 1, 1, 0},
        {1, 1, 0},
        {1, 0},

    };

    if (residual_type != ChromaDCLevel)
    {
        pattern_len = lentab[total_coeff - 1][total_zeros];
        pattern = codtab[total_coeff - 1][total_zeros];
    }
    else
    {
        pattern_len = cdc_lentab[total_coeff - 1][total_zeros];
        pattern = cdc_codtab[total_coeff - 1][total_zeros];
    }

    if (pattern_len == 0)
    {
        alert_msg("ERROR: write_total_zeros() !\n");
        exit(-1);
    }

    put_bits(pEnc, pattern, pattern_len);

    return pattern_len;
}

xint
write_run_before(h264enc_obj *pEnc, xint run, xint zeroleft)
{
    xint    table_idx;
    uint32  pattern;
    xint    pattern_len;
    static const xint lentab[7][16] = {
        {1, 1},
        {1, 2, 2},
        {2, 2, 2, 2},
        {2, 2, 2, 3, 3},
        {2, 2, 3, 3, 3, 3},
        {2, 3, 3, 3, 3, 3, 3},
        {3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 10, 11},
    };

    static const uint16 codtab[7][16] = {
        {1, 0},
        {1, 1, 0},
        {3, 2, 1, 0},
        {3, 2, 1, 1, 0},
        {3, 2, 3, 2, 1, 0},
        {3, 0, 1, 3, 2, 5, 4},
        {7, 6, 5, 4, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

    table_idx = zeroleft - 1;
    if (table_idx > 6)
        table_idx = 6;

    pattern = codtab[table_idx][run];
    pattern_len = lentab[table_idx][run];

    if (pattern_len == 0)
    {
        alert_msg("ERROR: write_run_before() !\n");
        exit(-1);
    }

    put_bits(pEnc, pattern, pattern_len);
    
    return pattern_len;
}

xint
write_block_cavlc(h264enc_obj *pEnc, int16 * pLevel, int16 * pRun, xint residual_type, xint block, xint YCbCr, xint coded)
{
    slice_obj *pSlice;
    mb_obj    *pMB;
    xint    level, run;
    xint    max_coeff_num = 0;
    xint    total_zeros = 0;
    xint    numtrailingones = 0, numcoeff = 0, lastcoeff_idx = 0;
    xint    idx;
    xint    table_idx;
    xint    pred_nzc;
    uint32  flag_pattern;
    xint    num_bits = 0;
    xint    zeroleft, reverse_numcoeff;
    xint    level_vlcnum;
    xint    two_level_or_higher;
    xint    frame_x_16_offset, frame_x_4_offset;
    xint    mb_y_4_offset, mb_x_4_offset;
    xint    nA, nB, nA_valid, nB_valid;
    xint    mb_skip_flag;

    pSlice = pEnc->curr_slice;
    pMB    = &pSlice->cmb;
    mb_x_4_offset     = mb_x_4_idx[block];
    mb_y_4_offset     = mb_y_4_idx[block];
    frame_x_16_offset = pEnc->curr_slice->cmb.id % pEnc->nx_mb;
    mb_skip_flag      = (pMB->best_MB_mode && pMB->best_Inter_mode == INTER_PSKIP);

    /* bitstream log */

    switch(residual_type)
    {
    case LumaLevel:
        max_coeff_num = 16;
        break;
    case Intra16x16DCLevel:
        max_coeff_num = 16;
        break;
    case Intra16x16ACLevel:
        max_coeff_num = 15;
        break;
    case ChromaDCLevel:
        max_coeff_num = 4;
        break;
    case ChromaACLevel:
        max_coeff_num = 15;
        break;
    default:
        alert_msg("cavlc luma intra error !\n");
        exit(-1);
    }

    if(!mb_skip_flag)
    {
        for (idx = 0; idx < max_coeff_num; idx++)
        {
            level = pLevel[idx];
            run = pRun[idx];

            if (level)
            {
                if (run)
                    total_zeros += run;

                if (ABS(level) == 1)
                {
                    numtrailingones++;

                    if (numtrailingones > 3)
                        numtrailingones = 3;
                }
                else
                    numtrailingones = 0;

                numcoeff++;
                lastcoeff_idx = idx;
            }
            else
                break;

        }
    }    
    else
        numcoeff = 0;

    alert_msg("number of coeff = %d\n", numcoeff);
    alert_msg("number of T1    = %d\n", numtrailingones);

    if (residual_type != ChromaDCLevel)
    {
        // prediction number of non zero
        nA_valid = nB_valid = MMES_VALID;
        if(YCbCr == Y_block)
        {
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

            // update the number of non-zero coeff info
            if(residual_type != Intra16x16DCLevel)
            {
                pSlice->top_y_nzc[frame_x_4_offset] = numcoeff;
                
                if(frame_x_4_offset == ((pEnc->nx_mb*4)-1))     // leftest block
                {
                    pSlice->left_y_nzc[mb_y_4_offset] = -1;
                }
                else
                    pSlice->left_y_nzc[mb_y_4_offset] = numcoeff;
            }
        }
        else if(YCbCr == Cb_block)
        {
            frame_x_4_offset = frame_x_16_offset*2+mb_x_4_offset;
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
            // update the number of non-zero coeff info
            pSlice->top_cb_nzc[frame_x_4_offset] = numcoeff;
            if(frame_x_4_offset == ((pEnc->nx_mb*2)-1))
            {
                pSlice->left_cb_nzc[mb_y_4_offset] = -1;
            }
            else
                pSlice->left_cb_nzc[mb_y_4_offset] = numcoeff;
        }
        else        // Cr_block
        {
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

            // update the number of non-zero coeff info
            pSlice->top_cr_nzc[frame_x_4_offset] = numcoeff;
            if(frame_x_4_offset == ((pEnc->nx_mb*2)-1))
            {
                pSlice->left_cr_nzc[mb_y_4_offset] = -1;
            }
            else
                pSlice->left_cr_nzc[mb_y_4_offset] = numcoeff;
        }

        if(nA_valid && nB_valid)
        {
            pred_nzc = (nA + nB + 1)>>1;
        }
        else
            pred_nzc = nA + nB;


        // use pred_nnz to select the vlc table
        if (pred_nzc < 2)
            table_idx = 0;
        else if (pred_nzc < 4)
            table_idx = 1;
        else if (pred_nzc < 8)
            table_idx = 2;
        else
            table_idx = 3;
    }
    else
        table_idx = 4;

    if(coded == 0 || mb_skip_flag)
        return 0;

    alert_msg("T1 table num    = %d\n", table_idx);

    num_bits += write_coeff_token(pEnc, numcoeff, numtrailingones, table_idx);

    if (!numcoeff)
    {
        return num_bits;
    }
    else
    {
        // encode trailing ones sign flag
        flag_pattern = 0;
        for (idx = lastcoeff_idx; idx > (lastcoeff_idx - numtrailingones); idx--)       //reverse order
        {
            level = pLevel[idx];

            alert_msg("%2d T1           = %2d\n", idx, level);

            if (ABS(level) > 1)
            {
                alert_msg("ERROR: level > 1 !\n");
                exit(-1);
            }

            flag_pattern <<= 1;

            if (level < 0)
                flag_pattern |= 0x1;
        }

        if (numtrailingones)
            num_bits += write_trailingones_sign_flag(pEnc, flag_pattern, numtrailingones);

        // encode level
        two_level_or_higher = 1;
        if(numcoeff>3 && numtrailingones==3)
            two_level_or_higher = 0;

        if ((numcoeff > 10) && (numtrailingones < 3))
            level_vlcnum = 1;
        else
            level_vlcnum = 0;

        for (idx = (lastcoeff_idx - numtrailingones); idx >= 0; idx--)
        {
            xint levelcode = level = pLevel[idx];

            alert_msg("%2d level       = %2d \n", idx, level);

            if(two_level_or_higher)
            {
                if(levelcode > 0)
                    levelcode--;
                else 
                    levelcode++;
                
                two_level_or_higher = 0;
            }

            if(level_vlcnum == 0)
                num_bits += write_level_vlc0(pEnc, levelcode);
            else 
                num_bits += write_level_vlcN(pEnc, levelcode, level_vlcnum);

            if (level_vlcnum == 0)
                level_vlcnum = 1;

            if ((ABS(level) > (3 << (level_vlcnum - 1)))
                && (level_vlcnum < 6))
                level_vlcnum += 1;

        }

        // encode totoal zeros
        if (numcoeff < max_coeff_num)
        {
            alert_msg("total zeros     = %d\n", total_zeros);
            num_bits +=
                write_total_zeros(pEnc, total_zeros, numcoeff, residual_type);
        }

        // encode run before ecah coefficient
        zeroleft = total_zeros;
        reverse_numcoeff = numcoeff;

        for (idx = lastcoeff_idx; idx >= 0; idx--)
        {
            run = pRun[idx];

            if (numcoeff <= 1 || !zeroleft)
                break;

            if ((reverse_numcoeff > 1) && (zeroleft))
            {
                alert_msg("%2d Run before  = %2d\n", idx, run);
                num_bits += write_run_before(pEnc, run, zeroleft);

                zeroleft -= run;
                reverse_numcoeff--;
            }
        }
    }

    return num_bits;
}
