/* ///////////////////////////////////////////////////////////// */
/*   File: macroblock.c                                          */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 macroblock coding module.        */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "../../common/inc/bitstream.h"
#include "../../common/inc/cavlc.h"
#include "../../common/inc/deblock.h"
#include "../../common/inc/h264enc_api.h"
#include "../../common/inc/inter_comp.h"
#include "../../common/inc/intra_comp.h"
#include "../../common/inc/intra_pred.h"
#include "../../common/inc/macroblock.h"
#include "../../common/inc/misc_util.h"
#include "../../common/inc/mv_pred.h"
#include "../../common/inc/mv_search.h"
#include "../../common/inc/nal.h"
#include "../../common/inc/quant.h"
#include "../../common/inc/rate_control.h"
#include "../../common/inc/scan.h"
#include "../../common/inc/trans.h"
#include "../../common/inc/vlc.h"
#include "../../common/inc/vld.h"

extern uint32 prefix_mask_rev[33];
extern uint32 prefix_mask[33];
extern uint8  postfix_mask[9];

const xint  db_z[4][4] = {{0, 1, 4, 5}, {2, 3, 6, 7}, {8, 9, 12, 13}, {10, 11, 14, 15}};

/* convert from H.263 QP to H.264 quant given by: quant=pow(2,QP/6) */
const int QP2QUANT[40]=
{
   1, 1, 1, 1, 2, 2, 2, 2,
   3, 3, 3, 4, 4, 4, 5, 6,
   6, 7, 8, 9,10,11,13,14,
  16,18,20,23,25,29,32,36,
  40,45,51,57,64,72,81,91
};

/* gives codeword number from CBP value, both for intra and inter */
const uint8 ECBP[48][2]=
{
    { 3, 0},{29, 2},{30, 3},{17, 7},{31, 4},{18, 8},{37,17},{ 8,13},{32, 5},{38,18},{19, 9},{ 9,14},
    {20,10},{10,15},{11,16},{ 2,11},{16, 1},{33,32},{34,33},{21,36},{35,34},{22,37},{39,44},{ 4,40},
    {36,35},{40,45},{23,38},{ 5,41},{24,39},{ 6,42},{ 7,43},{ 1,19},{41, 6},{42,24},{43,25},{25,20},
    {44,26},{26,21},{46,46},{12,28},{45,27},{47,47},{27,22},{13,29},{28,23},{14,30},{15,31},{ 0,12}
};

/* gives CBP value from codeword number, both for intra and inter */
const uint8 DCBP[48][2]=
{
    {47, 0},{31,16},{15, 1},{ 0, 2},{23, 4},{27, 8},{29,32},{30, 3},{ 7, 5},{11,10},{13,12},{14,15},
    {39,47},{43, 7},{45,11},{46,13},{16,14},{ 3, 6},{ 5, 9},{10,31},{12,35},{19,37},{21,42},{26,44},
    {28,33},{35,34},{37,36},{42,40},{44,39},{ 1,43},{ 2,45},{ 4,46},{ 8,17},{17,18},{18,20},{20,24},
    {24,19},{ 6,21},{ 9,26},{22,28},{25,23},{32,27},{33,29},{34,30},{36,22},{40,25},{38,38},{41,41}
};

const char *intra_mode[] =
{
    "INTRA_16x16_V",
    "INTRA_16x16_H",
    "INTRA_16x16_DC",
    "INTRA_16x16_PL",

    "INTRA_CHROMA_DC",
    "INTRA_CHROMA_H",
    "INTRA_CHROMA_V",
    "INTRA_CHROMA_PL",

    "INTRA_4x4_V",
    "INTRA_4x4_H",
    "INTRA_4x4_DC",
    "INTRA_4x4_DDL",
    "INTRA_4x4_DDR",
    "INTRA_4x4_VR",
    "INTRA_4x4_HD",
    "INTRA_4x4_VL",
    "INTRA_4x4_HU"
};

/* The following MACRO definitions are for debugging only */
#define USE_ALL_INTRA_MODES 1
#define USE_ALL_INTRA_4x4_MODES 0

mb_obj debug_mb;

xint display_4x4_residual( slice_obj *pSlice, xint block )
{
    mb_obj *pMB = &(pSlice->cmb);
    int16 *b4x4_start;
    xint mb_x_4_offset, mb_y_4_offset;
    xint x_coord, y_coord;

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];
    b4x4_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            printf( "%3d ", *b4x4_start );
            b4x4_start ++;
        }
        b4x4_start += (MB_SIZE-B_SIZE);
        printf( "\n" );
    }

    return MMES_NO_ERROR;
}

xint dec_display_uv_residual( h264dec_obj *pDec )
{
    xint i, j;
    mb_obj *pMB = &(pDec->curr_slice->cmb);

    printf( "U \n" );
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++ )
        {

            printf( " %3d", pMB->cb_residual[j*(MB_SIZE/2)+i] );
        }
        printf("\n");
    }

    printf( "V \n" );
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++ )
        {
            printf( " %3d", pMB->cr_residual[j*(MB_SIZE/2)+i] );
        }
        printf("\n");
    }

    return MMES_NO_ERROR;
}

xint enc_display_uv_residual( h264enc_obj *pEnc )
{
    xint i, j;
    mb_obj *pMB = &(pEnc->curr_slice->cmb);

    printf( "U \n" );
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++ )
        {

            printf( " %3d", pMB->cb_residual[j*(MB_SIZE/2)+i] );
        }
        printf("\n");
    }

    printf( "V \n" );
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++ )
        {
            printf( " %3d", pMB->cr_residual[j*(MB_SIZE/2)+i] );
        }
        printf("\n");
    }

    return MMES_NO_ERROR;
}

xint enc_display_y_residual( h264enc_obj *pEnc )
{
    xint i, j;
    mb_obj *pMB = &(pEnc->curr_slice->cmb);

    printf( "Y \n" );
    for( j=0; j<MB_SIZE; j++ )
    {
        for( i=0; i<MB_SIZE; i++ )
        {
            printf( " %3d", pMB->best_I16x16_residual[j*(MB_SIZE)+i] );
        }
        printf("\n");
    }

    return MMES_NO_ERROR;
}

xint dec_display_y_residual( h264dec_obj *pDec )
{
    xint i, j;
    mb_obj *pMB = &(pDec->curr_slice->cmb);

    printf( "Y \n" );
    for( j=0; j<MB_SIZE; j++ )
    {
        for( i=0; i<MB_SIZE; i++ )
        {
            printf( " %3d", pMB->best_I16x16_residual[j*(MB_SIZE)+i] );
        }
        printf("\n");
    }

    return MMES_NO_ERROR;
}

xint enc_display_mb( h264enc_obj *pEnc )
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj    *pMB;
    xint      frame_x_16_offset, frame_y_16_offset;
    xint      i, j;
    xint      width, height;
    xint      chroma_mb_size, chroma_frame_width;
    uint8     *y, *cb, *cr;

    pCurr_frame = pEnc->curf;
    width       = pEnc->width;
    height      = pEnc->height;

    pSlice      = pEnc->curr_slice;

    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

    chroma_frame_width = width / 2;
    chroma_mb_size = MB_SIZE / 2;

    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width + frame_x_16_offset * MB_SIZE;
    printf( "Y\n");
    for( j=0; j<MB_SIZE; j++ )
    {
        for( i=0; i<MB_SIZE; i++, y++ )
        {
#if 1
            printf( " %02X", *y);
#else
            printf( " %3d", *y );
#endif
        }
        printf("\n");
        y += width-MB_SIZE;
    }

    cb = pCurr_frame->cb +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;

    printf( "U\n");
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++, cb++ )
        {
#if 1
            printf( " %02X", *cb);
#else
            printf( " %3d", *cb );
#endif
        }
        printf("\n");
        cb += (chroma_frame_width - chroma_mb_size);
    }

    cr = pCurr_frame->cr +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;

    printf( "V\n");
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++, cr++ )
        {
#if 1
            printf( " %02X", *cr);
#else
            printf( " %3d", *cr );
#endif
        }
        printf("\n");
        cr += (chroma_frame_width - chroma_mb_size);
    }

    return MMES_NO_ERROR;
}

xint dec_display_rec_mb( h264dec_obj *pDec )
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint    frame_x_16_offset, frame_y_16_offset;
    xint    i, j;
    xint    width, height;
    xint    chroma_mb_size, chroma_frame_width;
    uint8  *y, *cb, *cr;

    pCurr_frame = pDec->recf;
    width = pDec->width;
    height = pDec->height;

    pSlice = pDec->curr_slice;

    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    chroma_frame_width = width / 2;
    chroma_mb_size = MB_SIZE / 2;

    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
              frame_x_16_offset * MB_SIZE;

    printf( "Y\n");
    for( j=0; j<MB_SIZE; j++ )
    {
        for( i=0; i<MB_SIZE; i++, y++ )
        {
            printf( " %3d", *y );
        }
        printf("\n");
        y += width-MB_SIZE;
    }

    cb = pCurr_frame->cb +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;

    printf( "U\n");
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++, cb++ )
        {
            printf( " %3d", *cb );
        }
        printf("\n");
        cb += (chroma_frame_width - chroma_mb_size);
    }

    cr = pCurr_frame->cr +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;

    printf( "V\n");
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++, cr++ )
        {
            printf( " %3d", *cr );
        }
        printf("\n");
        cr += (chroma_frame_width - chroma_mb_size);
    }

    return MMES_NO_ERROR;
}

xint dec_display_mb( h264dec_obj *pDec )
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint    frame_x_16_offset, frame_y_16_offset;
    xint    i, j;
    xint    width, height;
    xint    chroma_mb_size, chroma_frame_width;
    uint8  *y, *cb, *cr;

    pCurr_frame = pDec->curf;
    width = pDec->width;
    height = pDec->height;

    pSlice = pDec->curr_slice;

    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    chroma_frame_width = width / 2;
    chroma_mb_size = MB_SIZE / 2;

    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
              frame_x_16_offset * MB_SIZE;

    printf( "Y\n");
    for( j=0; j<MB_SIZE; j++ )
    {
        for( i=0; i<MB_SIZE; i++, y++ )
        {
            printf( " %3d", *y );
        }
        printf("\n");
        y += width-MB_SIZE;
    }

    cb = pCurr_frame->cb +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;

    printf( "U\n");
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++, cb++ )
        {
            printf( " %3d", *cb );
        }
        printf("\n");
        cb += (chroma_frame_width - chroma_mb_size);
    }

    cr = pCurr_frame->cr +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;

    printf( "V\n");
    for( j=0; j<MB_SIZE/2; j++ )
    {
        for( i=0; i<MB_SIZE/2; i++, cr++ )
        {
            printf( " %3d", *cr );
        }
        printf("\n");
        cr += (chroma_frame_width - chroma_mb_size);
    }

    return MMES_NO_ERROR;
}

xint encode_init_RDParams( h264enc_obj *pEnc )
{
    double lambda;

    xint qp = pEnc->curr_slice->cmb.QP;

    pEnc->lambda_md = QP2QUANT[ MAX(0, qp-SHIFT_QP) ];

    lambda = pEnc->lambda_md;

    pEnc->lambda_me = LAMBDA_FACTOR(lambda);
    pEnc->lambda_skip = (xint)floor(8*lambda+0.499);

    return MMES_NO_ERROR;
}

xint
calc_cbp(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : 6/30/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Calculate Coded Block Pattern                               */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice;
    mb_obj    *pMB;
    xint      idx, idx2;
    xint      chroma_dc_nonzero = 0, chroma_ac_nonzero = 0;
    int16     zero[16] = {0};
    //int16 lumalevel[MB_SIZE*MB_SIZE];


    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    // initialize cbp
    pMB->cbp     = 0;
    pMB->cbp_blk = 0;

    if(pMB->best_MB_mode == 0 && I16MB(pMB->best_I16x16_mode))
    {
        for(idx = 0 ; idx < MB_SIZE*MB_SIZE ; idx++)
        {
            if(idx % 16 == 0)
                continue;

            if(pMB->LumaACLevel[idx])
            {
                pMB->cbp |= 15;
                break;
            }
        }
        // don't need cbp_blk
        pMB->cbp_blk = 0;
    }
    else
    {
        // cbp_luma(4) : b3 b2 b1 b0
        for(idx = 3 ; idx >= 0 ; idx--)
        {
            pMB->cbp <<= 1;
            for(idx2 = 0 ; idx2 < 64 ; idx2++)
            {
                if(pMB->LumaACLevel[idx * 64 + idx2])
                {
                    pMB->cbp |= 1;
                    break;
                }
            }

            for(idx2 = 3 ; idx2 >= 0 ; idx2--)
                if(memcmp(pMB->LumaACLevel+(idx * 64 + idx2 * 16), zero, sizeof(int16)*16))
                    pMB->cbp_blk |= (1<<db_z[idx][idx2]);
        }
    }

    for(idx = 0 ; idx < 4 ; idx++)
    {
        if(pMB->CbDCLevel[idx] || pMB->CrDCLevel[idx])
        {
            chroma_dc_nonzero = 1;
            break;
        }
    }

    for(idx = 0; idx < MB_SIZE/2 * MB_SIZE/2; idx++)
    {
        if(idx % 16 == 0)
            continue;

        if(pMB->CbACLevel[idx] || pMB->CrACLevel[idx])
        {
            chroma_ac_nonzero = 1;
            break;
        }
    }

    for(idx = 3 ; idx >= 0 ; idx--)
    {
        if(memcmp(pMB->CbACLevel+idx, zero, sizeof(int16)*16))
            pMB->cbp_blk |= (1<<(16+idx));
        if(memcmp(pMB->CrACLevel+idx, zero, sizeof(int16)*16))
            pMB->cbp_blk |= (1<<(20+idx));
    }

    if(chroma_dc_nonzero == 0 && chroma_ac_nonzero == 0)
        pMB->cbp |= (0 << 4);
    else if(chroma_dc_nonzero == 1 && chroma_ac_nonzero == 0)
        pMB->cbp |= (1 << 4);
    else pMB->cbp |= (2 << 4);

    return MMES_NO_ERROR;
}

xint
encode_mb_info(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : 7/01/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Encode MB Information, inclusive of mb_type, mb_pred,       */
/*   cbp, and mb_qp_delta INTO BITSTREAM                         */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice;
    mb_obj    *pMB;
    xint      i16offset;
    xint      mb_type = 0;
    xint      mb_mode;
    xint      idx;
    xint      intra;
    xint      enc_mv = 0;

    pSlice    = pEnc->curr_slice;
    pMB       = &(pSlice->cmb);

    /* best_MB_mode => 1:inter 0:intra */
    intra     = !pMB->best_MB_mode;

    mb_mode = pMB->best_MB_mode ? pMB->best_Inter_mode : pMB->best_I16x16_mode;

    /* write mb_type */
    if(pSlice->type != B_SLICE)
    {
        if(pMB->best_MB_mode == I_PCM)
            mb_type = (pSlice->type == I_SLICE ? 25 : 31);
        else if(mb_mode == INTRA_4x4)
            mb_type = (pSlice->type == I_SLICE ? 0 : 6);
        else if(I16MB(mb_mode))
        {
            i16offset = ((pMB->cbp & 15) ? 13 : 1) + pMB->best_I16x16_mode + ((pMB->cbp & 0x30)>>2);
            mb_type = (pSlice->type == I_SLICE ? 0 : 6) + i16offset;
        }
        else if(mb_mode == INTER_P8x8)
        {
            /* all 8x8 blks use the first reference frame */
            xint all_zero_ref = 1;
            for(idx = 0 ; idx < 4 ; idx++)
            {
                if(pMB->ref_idx[idx] != 0)
                {
                    all_zero_ref = 0;
                    break;
                }
            }
            if(all_zero_ref)
                mb_type = 5;
            else
                mb_type = 4;
        }
        else mb_type = (mb_mode - INTER_16x16 + 1);
    }
    else
    {
        //TODO
        //Get mb_type of B SLICE
        ;
    }

    if(pSlice->type != I_SLICE && pSlice->type != SI_SLICE)
    {
        mb_type -= 1;

        if(mb_mode != INTER_PSKIP)
        {
            write_uvlc_codeword(pEnc, pSlice->mb_skip_run);
            pSlice->mb_skip_run = 0;
        }
        else
        {
            pSlice->mb_skip_run++;

            if(pSlice->no_mbs+1 == pSlice->max_no_mbs || pEnc->cur_mb_no+1 == pEnc->total_mb)
            {
                write_uvlc_codeword(pEnc, pSlice->mb_skip_run);
                pSlice->mb_skip_run = 0;
            }

            return MMES_NO_ERROR;
        }
    }

    if(pMB->best_MB_mode == I_PCM)
    {
        xint  x, y;
        xint  luma_width        = pEnc->width;
        xint  chroma_width      = luma_width >> 1;
        xint  B8x8_SIZE         = MB_SIZE/2;
        xint  frame_x_16_offset = pMB->id % pEnc->nx_mb;
        xint  frame_y_16_offset = pMB->id / pEnc->nx_mb;
        xint  frame_x_4_offset;
        uint8 *res_y  = pEnc->recf->y + frame_y_16_offset * MB_SIZE   * luma_width  + frame_x_16_offset * MB_SIZE;
        uint8 *res_cb = pEnc->recf->cb+ frame_y_16_offset * B8x8_SIZE * chroma_width+ frame_x_16_offset * B8x8_SIZE;
        uint8 *res_cr = pEnc->recf->cr+ frame_y_16_offset * B8x8_SIZE * chroma_width+ frame_x_16_offset * B8x8_SIZE;

        // mb_type
        write_uvlc_codeword(pEnc, mb_type);

        // Set nZCoeff
        // for Y_Block
        frame_x_4_offset = (frame_x_16_offset<<2);
        memset(pSlice->top_y_nzc+frame_x_4_offset,  16, sizeof(int8) * 4);
        // for Cb_Block & Cr_Block
        frame_x_4_offset = (frame_x_16_offset<<1);
        memset(pSlice->top_cb_nzc+frame_x_4_offset, 16, sizeof(int8) * 2);
        memset(pSlice->top_cr_nzc+frame_x_4_offset, 16, sizeof(int8) * 2);

        if(((pMB->id+1)%pEnc->nx_mb) == 0)
        {
            memset(pSlice->left_y_nzc, -1, sizeof(int8) * 4);
            memset(pSlice->left_cb_nzc,-1, sizeof(int8) * 2);
            memset(pSlice->left_cr_nzc,-1, sizeof(int8) * 2);
        }
        else
        {
            memset(pSlice->left_y_nzc,  16, sizeof(int8) * 4);
            memset(pSlice->left_cb_nzc, 16, sizeof(int8) * 2);
            memset(pSlice->left_cr_nzc, 16, sizeof(int8) * 2);
        }

        pMB->cbp       = 0x3F;
        pMB->cbp_blk   = 0xFFFF;
        pMB->QP        = 0;

        while(!byte_align(pEnc, 1))
        {
            put_one_bit(pEnc, 0);
        }

        // Y
        for(y = 0 ; y < MB_SIZE ; y++)
        {
            for(x = 0 ; x < MB_SIZE ; x++)
            {
                put_bits(pEnc, res_y[y*luma_width+x], 8);
            }
        }

        // Cb
        for(y = 0 ; y < B8x8_SIZE ; y++)
        {
            for(x = 0 ; x < B8x8_SIZE ; x++)
            {
                put_bits(pEnc, res_cb[y*chroma_width+x], 8);
            }
        }

        // Cr
        for(y = 0 ; y < B8x8_SIZE ; y++)
        {
            for(x = 0 ; x < B8x8_SIZE ; x++)
            {
                put_bits(pEnc, res_cr[y*chroma_width+x], 8);
            }
        }

        return MMES_NO_ERROR;
    }
    else
        write_uvlc_codeword(pEnc, mb_type);



    /* write mb_pred */

    if((mb_mode != INTRA_4x4 && !I16MB(mb_mode)) && mb_mode == INTER_P8x8)
    {
        xint sub_mb_type;
        /* sub_mb_pred */
        if(mb_mode == INTER_P8x8)
        {
            for(idx = 0 ; idx < 4 ; idx++)
            {
                sub_mb_type = pMB->best_8x8_blk_mode[idx] - INTER_8x8;
                write_uvlc_codeword(pEnc, sub_mb_type);
            }
        }
        enc_mv = 1;
    }
    else
    {
        if(mb_mode == INTRA_4x4 || I16MB(mb_mode))
        {
            if(mb_mode == INTRA_4x4)
            {
                for(idx = 0 ; idx < NBLOCKS ; idx++)
                {
                    if(pMB->I4x4_pred_mode[idx] == -1)
                    {
                        put_one_bit(pEnc, 1);
                    }
                    else
                    {
                        put_bits(pEnc, pMB->I4x4_pred_mode[idx], 4);
                    }
                }
            }
            write_uvlc_codeword(pEnc, pMB->best_chroma_mode - 4);

            enc_mv = 0;
        }
        else
            enc_mv = 1;
    }

    if(enc_mv)
    {
        /* If multiple ref. frames, write reference frame for the MB */

        /* write forward motion vectors */
        switch(mb_mode)
        {
        case INTER_16x16:
            write_signed_uvlc_codeword(pEnc, (pMB->mv.x[0] - pMB->pred_mv.x[0]));
            write_signed_uvlc_codeword(pEnc, (pMB->mv.y[0] - pMB->pred_mv.y[0]));
            break;
        case INTER_16x8:
            // top
            write_signed_uvlc_codeword(pEnc, (pMB->mv.x[0] - pMB->pred_mv.x[0]));
            write_signed_uvlc_codeword(pEnc, (pMB->mv.y[0] - pMB->pred_mv.y[0]));
            // bottom
            write_signed_uvlc_codeword(pEnc, (pMB->mv.x[8] - pMB->pred_mv.x[8]));
            write_signed_uvlc_codeword(pEnc, (pMB->mv.y[8] - pMB->pred_mv.y[8]));
            break;
        case INTER_8x16:
            // left
            write_signed_uvlc_codeword(pEnc, (pMB->mv.x[0] - pMB->pred_mv.x[0]));
            write_signed_uvlc_codeword(pEnc, (pMB->mv.y[0] - pMB->pred_mv.y[0]));
            // right
            write_signed_uvlc_codeword(pEnc, (pMB->mv.x[2] - pMB->pred_mv.x[2]));
            write_signed_uvlc_codeword(pEnc, (pMB->mv.y[2] - pMB->pred_mv.y[2]));
            break;
        case INTER_P8x8:
        case INTER_8x8:
            // top-left
            write_signed_uvlc_codeword(pEnc, (pMB->mv.x[0] - pMB->pred_mv.x[0]));
            write_signed_uvlc_codeword(pEnc, (pMB->mv.y[0] - pMB->pred_mv.y[0]));
            // top-right
            write_signed_uvlc_codeword(pEnc, (pMB->mv.x[2] - pMB->pred_mv.x[2]));
            write_signed_uvlc_codeword(pEnc, (pMB->mv.y[2] - pMB->pred_mv.y[2]));
            // bottom-left
            write_signed_uvlc_codeword(pEnc, (pMB->mv.x[8] - pMB->pred_mv.x[8]));
            write_signed_uvlc_codeword(pEnc, (pMB->mv.y[8] - pMB->pred_mv.y[8]));
            // bottom-right
            write_signed_uvlc_codeword(pEnc, (pMB->mv.x[10] - pMB->pred_mv.x[10]));
            write_signed_uvlc_codeword(pEnc, (pMB->mv.y[10] - pMB->pred_mv.y[10]));
            break;
        default:
            break;
        }
    }

    /* write cbp */
    if(!I16MB(mb_mode))
        write_uvlc_codeword(pEnc, ECBP[pMB->cbp][!intra]);

    /* write mb_qp_delta */
    if(pMB->cbp || I16MB(mb_mode))
    {
        write_signed_uvlc_codeword(pEnc, pMB->qp_delta);
    }
    return MMES_NO_ERROR;
}

xint
encode_intra_mb(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/13/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Encode an Intra MB.                                         */
/*                                                               */
/*   RETURN                                                      */
/*   The size (in bits) of the compressed macroblock data.       */
/*   There is something wrong if the size is negative.           */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    int mode;
    xint nbits, block;
    mb_obj  *pMB = &pEnc->curr_slice->cmb;
	rc_obj  *pRC = &pEnc->pRC;
    pEnc->curr_slice->cmb.best_MB_mode = 0;

    /* record the bit position */
    nbits = get_bitpos(pEnc);

    /* Test IPCM mode */
    if( pEnc->pCtrl->interval_ipcm && ( (pEnc->curr_slice->cmb.id+pEnc->cur_frame_no)%pEnc->pCtrl->interval_ipcm == 0 ) )
    {
        pEnc->curr_slice->cmb.best_MB_mode = I_PCM;
        set_ipcm_info( pEnc->curr_slice, pEnc->recf, pEnc->curf, pEnc->width );

        /* calculate cbp */
        calc_cbp(pEnc);

        /* encode mb_info, inclusive of mb_type, mb_pred, cbp, and mb_qp_delta */
        encode_mb_info(pEnc);

        /* entropy coding of the modes and coefficients of one MB */
        entropy_coding(pEnc);

        /* apply deblocking filter */
        deblock(pEnc->curr_slice, pEnc->width, pEnc->recf);

        /* update MB information */
        update_MB( pEnc->curr_slice, pEnc->width );

        /* compute the number of bits written */
        nbits = get_bitpos(pEnc) - nbits;

        return nbits;
    }

    /* Intra prediction mode selection */
#if SPEEDUP
    mode = 1;
    pMB->best_chroma_mode = INTRA_CHROMA_DC;
    pMB->best_I16x16_mode = INTRA_16x16_DC;
#else
    mode = ( intra_RD_mode_select(pEnc) == INTRA_4x4 ) ? 0 : 1;
#endif

	/* MB cost for rate control */
	pRC->i_cpx += pMB->best_Intra_cost;

    pEnc->ICompensate_enable = 1;

    /* Intra chroma prediction */
    enc_intra_chroma_prediction(pEnc);

    /* now, transform/quantize the chroma residual coefficients */
    trans_chroma(pEnc);

    quant_chroma(pEnc);

    /* reconstruct the encoded chroma MB for future references */
    inv_quant_chroma(pEnc->curr_slice);

    inv_trans_chroma(pEnc->curr_slice);

    enc_intra_chroma_compensation(pEnc);

    /* Intra luma prediction */
    if (mode)
    {
        /* use 16x16 mode prediction */
        enc_intra_16x16_prediction(pEnc);

        /* now, transform/quantize the 16x16 residual coefficients */
        trans_16x16(pEnc);

        quant_16x16(pEnc);

        /* reconstruct the encoded MB for future references */
        inv_quant_16x16(pEnc->curr_slice);

        inv_trans_16x16(pEnc->curr_slice);

        enc_intra_16x16_compensation(pEnc);
    }
    else
    {
        /* use 4x4 mode prediction */
        for (block = 0; block < NBLOCKS; block++)
        {
            enc_intra_4x4_prediction(pEnc, block);

            /* now, transform/quantize the residual coefficients */
            trans_4x4(pEnc, block);
            quant_4x4(pEnc, block);

            /* reconstruct the encoded block for future references */
            inv_quant_4x4(pEnc->curr_slice, block);

            inv_trans_4x4(pEnc->curr_slice, block);

            enc_intra_4x4_compensation(pEnc, block);
        }
    }

#if 1//!SPEEDUP
    dup_mb(pEnc->recf, pEnc->curf, pEnc->curr_slice->cmb.id, pEnc->width);
#endif

    /* calculate cbp */
    calc_cbp(pEnc);

    /* encode mb_info, inclusive of mb_type, mb_pred, cbp, and mb_qp_delta */
    encode_mb_info(pEnc);

    /* entropy coding of the modes and coefficients of one MB */
    entropy_coding(pEnc);

    /* apply deblocking filter */
    deblock(pEnc->curr_slice, pEnc->width, pEnc->recf);

    /* update MB information */
    update_MB( pEnc->curr_slice, pEnc->width );

    /* compute the number of bits written */
    nbits = get_bitpos(pEnc) - nbits;

    return nbits;
}

xint intra_RD_backup( slice_obj *pSlice, backup_obj *pBackup, frame_obj *pCurr_frame, xint width, xint height )
{
    mb_obj *pMB;
    xint    frame_x_16_offset, frame_y_16_offset;
    xint    y_coord, nx_mb;
    uint8  *y;

    pMB = &(pSlice->cmb);
    nx_mb = width >> 4;

    frame_x_16_offset = pMB->id % nx_mb;
    frame_y_16_offset = pMB->id / nx_mb;

    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
              frame_x_16_offset * MB_SIZE;

    // backup y
    for (y_coord = 0; y_coord < MB_SIZE; y_coord++)
    {
        memcpy( pBackup->y_backup[y_coord], y, MB_SIZE*sizeof(uint8) );
        y += width;
    }

    // backup top_y, top_valid, top_mode
    memcpy( pBackup->top_mode_bup,    pSlice->top_y,        width*sizeof(uint8)            );
    memcpy( pBackup->top_valid_bup,   pSlice->top_valid,    (width/B_SIZE)*sizeof(uint8)   );
    memcpy( pBackup->top_ref_idx_bup, pSlice->top_ref_idx,  ((width/B_SIZE)/2)*sizeof(xint)   );
    memcpy( pBackup->top_mvx_bup,     pSlice->top_mvx,      (width/B_SIZE)*sizeof(xint)   );
    memcpy( pBackup->top_mvy_bup,     pSlice->top_mvy,      (width/B_SIZE)*sizeof(xint)   );

    // backup left_y, left_valid, left_mode
    memcpy( pBackup->left_y_bup,       pSlice->left_y,       MB_SIZE*sizeof(uint8)            );
    memcpy( pBackup->left_valid_bup,   pSlice->left_valid,   (MB_SIZE/B_SIZE)*sizeof(uint8)   );
    memcpy( pBackup->left_mvx_bup,     pSlice->left_mvx,     (MB_SIZE/B_SIZE)*sizeof(xint)    );
    memcpy( pBackup->left_mvy_bup,     pSlice->left_mvy,     (MB_SIZE/B_SIZE)*sizeof(xint)    );
    memcpy( pBackup->left_ref_idx_bup, pSlice->left_ref_idx, ((MB_SIZE/B_SIZE)/2)*sizeof(xint));

    // backup upleft_y, upleft_valid
    memcpy( pBackup->upleft_y_bup    ,   pSlice->upleft_y,       (MB_SIZE/B_SIZE)*sizeof(uint8)   );
    memcpy( pBackup->upleft_valid_bup,   pSlice->upleft_valid,   (MB_SIZE/B_SIZE)*sizeof(uint8)   );
    memcpy( pBackup->upleft_mvx_bup,     pSlice->upleft_mvx,     (MB_SIZE/B_SIZE)*sizeof(xint)    );
    memcpy( pBackup->upleft_mvy_bup,     pSlice->upleft_mvy,     (MB_SIZE/B_SIZE)*sizeof(xint)    );
    memcpy( pBackup->upleft_ref_idx_bup, pSlice->upleft_ref_idx, (MB_SIZE/B_SIZE)*sizeof(xint)    );

    return MMES_NO_ERROR;
}

xint intra_RD_restore_backup( h264enc_obj *pEnc )
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    backup_obj *pBackup;
    mb_obj *pMB;
    xint    frame_x_16_offset, frame_y_16_offset;
    xint    y_coord;
    xint    width, height;
    uint8  *y;

    pCurr_frame = pEnc->curf;
    width = pEnc->width;
    height = pEnc->height;

    pSlice = pEnc->curr_slice;
    pBackup = pEnc->backup;

    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
              frame_x_16_offset * MB_SIZE;

    // backup y
    for (y_coord = 0; y_coord < MB_SIZE; y_coord++)
    {
        memcpy( y, pBackup->y_backup[y_coord], MB_SIZE*sizeof(uint8) );
        y += width;
    }

    // backup top_y, top_valid, top_mode
    memcpy( pSlice->top_y,     pBackup->top_mode_bup,       width*sizeof(uint8)            );
    memcpy( pSlice->top_valid, pBackup->top_valid_bup,      (width/B_SIZE)*sizeof(uint8)   );
//  memcpy( pSlice->top_mode,  pBackup->top_mode_bup,       (width/B_SIZE)*sizeof(int) );

    // backup left_y, left_valid, left_mode
    memcpy( pSlice->left_y,     pBackup->left_y_bup,        MB_SIZE*sizeof(uint8)          );
    memcpy( pSlice->left_valid, pBackup->left_valid_bup,    (MB_SIZE/B_SIZE)*sizeof(uint8) );
//  memcpy( pSlice->left_mode,  pBackup->left_mode_bup,     B_SIZE*sizeof(int)         );

    // backup upleft_y, upleft_valid
    memcpy( pSlice->upleft_y,     pBackup->upleft_y_bup    , (MB_SIZE/B_SIZE)*sizeof(uint8) );
    memcpy( pSlice->upleft_valid, pBackup->upleft_valid_bup, (MB_SIZE/B_SIZE)*sizeof(uint8) );

    return MMES_NO_ERROR;
}

xint
intra_RD_mode_select(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : May/??/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Apply RD optimization algorithm to select Intra MB          */
/*   prediction mode.                                            */
/*                                                               */
/*   RETURN                                                      */
/*   Returns '1' if the decision is to use a 16x16 mode, or '0'  */
/*   if 8x8 modes are used. Note that the best mode for each     */
/*   subblocks (including luma and chroma) are stored in *pEnc.  */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint mode=1, idx, mb_xpos, up_avail, left_avail, upleft_avail, block, cur_chroma;
    xint blk4_xpos, blk4_ypos;

    xint validI16 = 1, validI4, validIChroma = 1;
    xint best_I16cost = MAX_XINT, I16cost, best_I4cost, I4cost, I4x4RDcost = MAX_XINT, best_IChroma_cost = MAX_XINT, IChroma_cost;
#if HW_METHOD
    int chroma_idx [] = { INTRA_CHROMA_V, INTRA_CHROMA_H, INTRA_CHROMA_DC, INTRA_CHROMA_PL };
#else
    int chroma_idx [] = { INTRA_CHROMA_DC, INTRA_CHROMA_H, INTRA_CHROMA_V, INTRA_CHROMA_PL };
#endif

    int best_I16mode = INTRA_16x16_DC, best_I4mode = INTRA_4x4_DC, up_mode, left_mode, probable_mode;
    int best_IChroma_mode = INTRA_CHROMA_DC;

    slice_obj *pSlice = pEnc->curr_slice;
    mb_obj    *pMB = &(pEnc->curr_slice->cmb);

#if SPEEDUP
    validI4 = 0;
#else
    validI4 = 1;
#endif

    pEnc->ICompensate_enable = 0;
    mb_xpos = pMB->id % pEnc->nx_mb;

    // backup prediction data for further usage
    intra_RD_backup( pSlice, pEnc->backup, pEnc->curf, pEnc->width, pEnc->height );

    if( validIChroma )
    {
        up_avail = left_avail = upleft_avail = MMES_INVALID;
        if( mb_xpos == 0 )
        {
            up_avail = pSlice->top_valid[ mb_xpos*(MB_SIZE/B_SIZE) ];
        }
        else
        {
            up_avail     = pSlice->top_valid[ mb_xpos*(MB_SIZE/B_SIZE) ];
            left_avail   = pSlice->left_valid[ 0 ];
            //upleft_avail = pSlice->top_valid[ mb_xpos*(MB_SIZE/B_SIZE)-1 ];
            upleft_avail = pSlice->upleft_valid[ 0 ];
        }

        // Loop for all Intra Chroma mode
        for( cur_chroma = 0; cur_chroma<4; cur_chroma++ )
        {
            idx = chroma_idx[cur_chroma];
            if( (idx==INTRA_CHROMA_V && !up_avail) || (idx==INTRA_CHROMA_H && !left_avail) ||
                (idx==INTRA_CHROMA_PL && (!up_avail||!left_avail||!upleft_avail)) )
                continue;

            pMB->best_chroma_mode = idx;
            enc_intra_chroma_prediction(pEnc);

            IChroma_cost = intra_chroma_RDCost(pEnc);

            if( IChroma_cost < best_IChroma_cost )
            {
                best_IChroma_mode = idx;
                best_IChroma_cost = IChroma_cost;
            }
        }

        pMB->best_chroma_mode = best_IChroma_mode;

    }

    if( validI16 )
    {
        up_avail = left_avail = upleft_avail = MMES_INVALID;
        if( mb_xpos == 0 )
        {
            up_avail = pSlice->top_valid[ mb_xpos*(MB_SIZE/B_SIZE) ];
        }
        else
        {
            up_avail     = pSlice->top_valid[ mb_xpos*(MB_SIZE/B_SIZE) ];
            left_avail   = pSlice->left_valid[ 0 ];
            //upleft_avail = pSlice->top_valid[ mb_xpos*(MB_SIZE/B_SIZE)-1 ];
            upleft_avail = pSlice->upleft_valid[ 0 ];
        }

        // Loop for all Intra16 mode
        for( idx=INTRA_16x16_V; idx<INTRA_16x16_PL+1; idx++ )
        {
            if( (idx==INTRA_16x16_V && !up_avail) || (idx==INTRA_16x16_H && !left_avail) ||
                (idx==INTRA_16x16_PL && (!up_avail||!left_avail||!upleft_avail)) )
                continue;

            pMB->best_I16x16_mode = idx;
            enc_intra_16x16_prediction(pEnc);
            I16cost = intra_RDCost_16x16(pEnc);

            if( I16cost < best_I16cost )
            {
                best_I16mode = idx;
                best_I16cost = I16cost;
            }
        }
        pMB->best_I16x16_mode = best_I16mode;
        best_I16cost >>= 1;
    }

    if( validI4 )
    {
        I4x4RDcost = 0;
        // Loop for all 4x4 blocks
        for (block = 0; block < NBLOCKS; block++)
        {
            best_I4cost = MAX_INT16;
            blk4_xpos = mb_xpos*4 + mb_x_4_idx[ block ];
            blk4_ypos = mb_y_4_idx[ block ];
            up_avail = pSlice->top_valid[ blk4_xpos ];
            if( blk4_xpos == 0 )
            {
                left_avail    = MMES_INVALID;
                upleft_avail  = MMES_INVALID;
            }
            else
            {
                left_avail = pSlice->left_valid[ blk4_ypos ];
                upleft_avail = pSlice->upleft_valid[ blk4_ypos ];
            }

            left_mode = (left_avail) ? pSlice->left_mode[ blk4_ypos ]  : -1;
            up_mode   = (up_avail)   ? pSlice->top_mode[ blk4_xpos ] : -1;
            probable_mode = ( left_mode < 0 || up_mode < 0 ) ? INTRA_4x4_DC : up_mode < left_mode ? up_mode : left_mode;

            pEnc->ICompensate_enable = 0;// disable intra compensate

            // Loop for all Intra4x4 mode
            for( idx=INTRA_4x4_V; idx<INTRA_4x4_HU+1; idx++ )
            {
                int available_mode =  (idx==INTRA_4x4_DC) ||
                      ((idx==INTRA_4x4_V||idx==INTRA_4x4_VL||idx==INTRA_4x4_DDL) && up_avail ) ||
                      ((idx==INTRA_4x4_H||idx==INTRA_4x4_HU) && left_avail ) ||(up_avail&&left_avail&&upleft_avail);

                if( !available_mode )
                    continue;

                pMB->best_I4x4_mode[ block ] = idx;
                enc_intra_4x4_prediction(pEnc, block);

                I4cost = (idx == probable_mode) ? 0 : 4*pEnc->lambda_md;
                I4cost += intra_RDCost_4x4(pEnc, block);
                if( I4cost < best_I4cost )
                {
                    best_I4mode = idx;
                    best_I4cost = I4cost;
                }
            }

            pEnc->ICompensate_enable = 1;// enable intra compensate
            pMB->best_I4x4_mode[ block ]   = best_I4mode;
            pSlice->left_mode[ blk4_ypos ] = best_I4mode;
            pSlice->top_mode[ blk4_xpos ]  = best_I4mode;
            I4x4RDcost += best_I4cost;

            // copy to CAVLC dtat structure
            pMB->I4x4_pred_mode[ block ] = (probable_mode == best_I4mode) ? -1 : (best_I4mode < probable_mode) ? (best_I4mode-8) : (best_I4mode-8)-1;

            enc_intra_4x4_prediction(pEnc, block);

            /* now, transform/quantize the residual coefficients */
            trans_4x4(pEnc, block);
            quant_4x4(pEnc, block);

            /* reconstruct the encoded block for future references */
            inv_quant_4x4(pEnc->curr_slice, block);
            inv_trans_4x4(pEnc->curr_slice, block);
            enc_intra_4x4_compensation(pEnc, block);
        }

        I4x4RDcost += 4*6*pEnc->lambda_md;

        intra_RD_restore_backup( pEnc );
    }

    // mode decision for Intra mode
    if( best_I16cost < I4x4RDcost )
    {
        mode = 1;
        pMB->best_Intra_cost = best_I16cost;
    }
    else
    {
        mode = 0;
        pMB->best_Intra_cost = I4x4RDcost;
        pMB->best_I16x16_mode = INTRA_4x4;
    }

    return pMB->best_I16x16_mode;
}

xint
encode_residual_coding8x8(h264enc_obj * pEnc, xint block8x8)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/??/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Apply DPCM procedure (transform, quantize, dequantize,      */
/*   inverse transfrom). In addition, calculate the cost of      */
/*   coefficients for judging if these coefficients is coded or  */
/*   eliminated.                                                 */
/*                                                               */
/*   RETURN                                                      */
/*   Cost of coefficients for this 8x8 block                     */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/*   block8x8 -> [I] which block of the 4 8x8 blocks to be       */
/*                   encoded                                     */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB = &(pEnc->curr_slice->cmb);
    xint block, blkOffset;
    xint coeff_cost = 0;

    blkOffset = (block8x8<<2);

    for( block = blkOffset; block < (blkOffset+4); block++ )
    {
        /* now, transform/quantize the residual coefficients */
        trans_4x4(pEnc, block);
        coeff_cost += quant_4x4(pEnc, block);

        /* reconstruct the encoded block for future references */
        inv_quant_4x4(pEnc->curr_slice, block);
        inv_trans_4x4(pEnc->curr_slice, block);
        enc_intra_4x4_compensation(pEnc, block);
    }

    if( coeff_cost <= _LUMA_COEFF_COST_ )
    {
        coeff_cost = 0;

        /* clear residual (now, inter residual is already saved in best_I4x4_residual) */
        /* copy compensated data to current frame                                      */
        for( block = blkOffset; block < (blkOffset+4); block++ )
        {
            eliminateLumaDCAC( pMB, block );
            dup_4x4_to_frame( pEnc->curf, pMB->Inter_cmp, pMB->id, pEnc->width, block );
        }
    }

    return coeff_cost;
}

xint
encode_inter_mb(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Encode an uni-directional Inter MB.                         */
/*                                                               */
/*   RETURN                                                      */
/*   The size (in bits) of the compressed macroblock data.       */
/*   There is something wrong if the size is negative.           */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    mb_obj  *pMB = &(pEnc->curr_slice->cmb);
	rc_obj  *pRC = &pEnc->pRC;
    int best_intra_mode, best_inter_mode;
    xint block8x8, block;
    xint MB_coeff_cost = 0;
    xint nbits;

    /* record the bit position */
    nbits = get_bitpos(pEnc);

    /* Test IPCM mode */
    if( pEnc->pCtrl->interval_ipcm && ( (pEnc->curr_slice->cmb.id+pEnc->cur_frame_no)%pEnc->pCtrl->interval_ipcm == 0 ) )
    {
        pEnc->curr_slice->cmb.best_MB_mode = I_PCM;
        set_ipcm_info( pEnc->curr_slice, pEnc->recf, pEnc->curf, pEnc->width );

        /* Inter set preidction information */
        inter_set_pred_info( pEnc->curr_slice, pEnc->width );

        /* calculate cbp */
        calc_cbp(pEnc);

        /* encode mb_info, inclusive of mb_type, mb_pred, cbp, and mb_qp_delta */
        encode_mb_info(pEnc);

        /* entropy coding of the modes and coefficients of one MB */
        entropy_coding(pEnc);

        /* apply deblocking filter */
        deblock(pEnc->curr_slice, pEnc->width, pEnc->recf);

        /* update MB information */
        update_MB( pEnc->curr_slice, pEnc->width );

        /* compute the number of bits written */
        nbits = get_bitpos(pEnc) - nbits;

        return nbits;
    }

    /* Inter prediction mode selection */
    best_inter_mode = inter_RD_mode_select(pEnc);

    /* Intra prediction mode selection */
    best_intra_mode = intra_RD_mode_select(pEnc);

    /* Selection between intra or inter prediction */
#if HW_METHOD
    if( pMB->best_Inter_cost <= pMB->best_Intra_cost )
#else
    if( pMB->best_Inter_cost < pMB->best_Intra_cost || (pMB->best_Inter_cost==pMB->best_Intra_cost && best_intra_mode!=INTRA_4x4) )
#endif
    {
        /* The best MB mode is Inter prediction */
        pMB->best_MB_mode = 1;

		/* MB cost for rate control */
		pRC->p_cpx += pMB->best_Inter_cost;

        //calculate compensation and residuals
        enc_cal_inter_comp_mb(pEnc);
        enc_cal_inter_residual(pEnc);

        /* set inter parameters */
        set_inter_parms( pEnc->curr_slice, pEnc->curf, pEnc->width );

        /* transform/quantize the chroma residual coefficients */
        trans_chroma(pEnc);

        quant_chroma(pEnc);

        inv_quant_chroma(pEnc->curr_slice);

        inv_trans_chroma(pEnc->curr_slice);

        enc_intra_chroma_compensation(pEnc);

        for (block8x8 = 0; block8x8 < 4; block8x8++)
        {
            MB_coeff_cost += encode_residual_coding8x8( pEnc, block8x8 );
        }
#if !HW_METHOD
        if( MB_coeff_cost <= _LUMA_MB_COEFF_COST_ )
        {
            /* clear residual (now, inter residual is already saved in best_I4x4_residual) */
            /* copy compensated data to current frame (cb & cr is not necessary to clear ) */
            memset(pMB->best_I4x4_residual, 0, (MB_SIZE*MB_SIZE)*sizeof(int16));
            memset(pMB->LumaACLevel,        0, (MB_SIZE*MB_SIZE)*sizeof(int16));
            dup_16x16_to_frame( pEnc->curf, pMB->Inter_cmp, NULL, NULL, pMB->id, pEnc->width );
        }
#endif

        /* set for intra prediction because of eliminating coefficients */
        set_intra_luma_prediction(pEnc->curr_slice, pEnc->curf, pEnc->width);
    }
    else
    {
        /* The best MB mode is Intra prediction */
        pMB->best_MB_mode = 0;

        pEnc->ICompensate_enable = 1;

		/* MB cost for rate control */
		pRC->p_cpx += pMB->best_Intra_cost;

        /* Intra chroma prediction */
        enc_intra_chroma_prediction(pEnc);

        /* now, transform/quantize the chroma residual coefficients */
        trans_chroma(pEnc);

        quant_chroma(pEnc);

        /* reconstruct the encoded chroma MB for future references */
        inv_quant_chroma(pEnc->curr_slice);

        inv_trans_chroma(pEnc->curr_slice);

        enc_intra_chroma_compensation(pEnc);

        /* Intra luma prediction */
        if( best_intra_mode != INTRA_4x4 )
        {
            /* use 16x16 mode prediction */
            enc_intra_16x16_prediction(pEnc);
            /* now, transform/quantize the 16x16 residual coefficients */
            trans_16x16(pEnc);

            quant_16x16(pEnc);

            /* reconstruct the encoded MB for future references */
            inv_quant_16x16(pEnc->curr_slice);

            inv_trans_16x16(pEnc->curr_slice);

            enc_intra_16x16_compensation(pEnc);
        }
        else
        {
            /* use 4x4 mode prediction */
            for (block = 0; block < NBLOCKS; block++)
            {
                enc_intra_4x4_prediction(pEnc, block);

                /* now, transform/quantize the residual coefficients */
                trans_4x4(pEnc, block);
                quant_4x4(pEnc, block);

                /* reconstruct the encoded block for future references */
                inv_quant_4x4(pEnc->curr_slice, block);

                inv_trans_4x4(pEnc->curr_slice, block);

                enc_intra_4x4_compensation(pEnc, block);
            }
        }
    }

    //moved here for skip mode
    calc_cbp(pEnc);

    //skip mode select
    enc_skip_mode_select_mb(pEnc);

    /* mv prediction */
    if( pMB->best_MB_mode ) // Inter
    {
        mv_pred_inter( pEnc->curr_slice, pEnc->backup, pEnc->width, 1 );
    }

    /* Inter set preidction information */
    inter_set_pred_info( pEnc->curr_slice, pEnc->width );

    dup_mb(pEnc->recf, pEnc->curf, pEnc->curr_slice->cmb.id, pEnc->width);

    /* calculate cbp */
    //move for skip mode
    //calc_cbp(pEnc);

    /* encode mb_info, inclusive of mb_type, mb_pred, cbp, and mb_qp_delta */
    encode_mb_info(pEnc);

    /* entropy coding of the modes and coefficients of one MB */
    entropy_coding(pEnc);

    /* apply deblocking filter */
    deblock(pEnc->curr_slice, pEnc->width, pEnc->recf);

    /* update MB information */
    update_MB( pEnc->curr_slice, pEnc->width );

    nbits = get_bitpos(pEnc) - nbits;

    return nbits;
}

xint
set_intra_luma_prediction(slice_obj *pSlice, frame_obj *pCurr_frame, xint width)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/15/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Set parameters for further intra prediction. Because        */
/*   elimination of coefficients affects reconstructed data, it  */
/*   is also necessary to modify predition value.                */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB = &(pSlice->cmb);
    xint    frame_x_16_offset, frame_y_16_offset;
    xint    nx_mb;
    xint    x_coord, y_coord;
    uint8  *y;

    nx_mb = width >>4;
    frame_x_16_offset = pMB->id % nx_mb;
    frame_y_16_offset = pMB->id / nx_mb;

    // top_y
    // move to bottom of current MB
    y = pCurr_frame->y + (frame_y_16_offset+1) * MB_SIZE * width +
        frame_x_16_offset * MB_SIZE - width;
    for (x_coord = 0; x_coord < MB_SIZE; x_coord++)
    {
        pSlice->top_y[frame_x_16_offset * MB_SIZE + x_coord] = *y;
        y++;
    }

    // left_y
    // move to rightest column of current MB
    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
        (frame_x_16_offset+1) * MB_SIZE - 1;
    for (y_coord = 0; y_coord < MB_SIZE; y_coord++)
    {
        pSlice->left_y[y_coord] = *(y);
        y+= width;
    }

    // upleft_y
    // move to above row of current MB
//  y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
//        (frame_x_16_offset+1) * MB_SIZE - width - 1;
//  pSlice->upleft_y[0] = *y;

    // move to rightest column of current MB
    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
        (frame_x_16_offset+1) * MB_SIZE - 1;
    for (y_coord = 0; y_coord < MB_SIZE; y_coord++)
    {
        if( y_coord == 3 || y_coord == 7 || y_coord == 11 )
        {
            pSlice->upleft_y[ (y_coord+1)/4 ] = *y;
        }
        y += width;
    }

    return MMES_NO_ERROR;
}

xint
set_inter_parms(slice_obj *pSlice, frame_obj *curf, xint width)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/??/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Set parameters for further inter coding.                    */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice  -> [I/O] pointer to the session parameters         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB = &(pSlice->cmb);
    xint    x_coord;
    xint    frame_x_16_offset;
    xint    chroma_mb_size, nx_mb;

    nx_mb = width >> 4;
    chroma_mb_size = MB_SIZE>>1;       // for YCbCr420
    frame_x_16_offset = pMB->id % nx_mb;

    /* copy residual of inter prediction to best_I4x4_residual for (inverse)transfrom, (de)quantization */
    memcpy( pMB->best_I4x4_residual, pMB->best_Inter_residual, sizeof(int16)*MB_SIZE*MB_SIZE );
    memcpy( pMB->cb_residual, pMB->best_Inter_cb_residual, sizeof(int16)*chroma_mb_size*chroma_mb_size );
    memcpy( pMB->cr_residual, pMB->best_Inter_cr_residual, sizeof(int16)*chroma_mb_size*chroma_mb_size );

    /* copy compensated data to current frame */
    dup_16x16_to_frame( curf, pMB->Inter_cmp, pMB->Inter_cb_cmp, pMB->Inter_cr_cmp, pMB->id, width );

    /* set for further intra prediction */
    for (x_coord = 0; x_coord < (MB_SIZE/B_SIZE); x_coord++)
    {
        pSlice->top_mode[frame_x_16_offset * (MB_SIZE/B_SIZE)+x_coord] = INTRA_4x4_DC;
        pSlice->left_mode[x_coord] = INTRA_4x4_DC;
    }

    return MMES_NO_ERROR;
}

xint
inter_set_pred_info( slice_obj *pSlice, xint width )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/26/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Set parameters for further inter prediction.                */
/*   Up-left information should be updated before deblocking     */
/*   filter is processed, which will update top information.     */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice  -> [I/O] pointer to the session parameters         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB = &(pSlice->cmb);
    xint frame_x_16_offset, frame_x_4_offset, frame_x_8_offset;

    xint nx_mb = width >> 4;

    frame_x_16_offset = pMB->id % nx_mb;
    frame_x_8_offset = (frame_x_16_offset<<1)+1;
    frame_x_4_offset = (frame_x_16_offset<<2)+3;

    // update upleft information
    pSlice->upleft_mvx[0] = pSlice->top_mvx[frame_x_4_offset];
    pSlice->upleft_mvy[0] = pSlice->top_mvy[frame_x_4_offset];
    pSlice->upleft_ref_idx[0] = pSlice->top_ref_idx[frame_x_8_offset];

    return MMES_NO_ERROR;
}

xint
inter_RD_mode_select(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/??/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Apply RD optimization algorithm to select Inter MB          */
/*   prediction mode.                                            */
/*                                                               */
/*   RETURN                                                      */
/*   The best Inter prediction mode.                             */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB = &(pEnc->curr_slice->cmb);

    pMB->best_Inter_cost = MAX_XINT;

    debug_mb = *pMB;

    enc_inter_mode_select_mb(pEnc);

    return pMB->best_Inter_mode;
}

xint
encode_bidir_mb(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Encode an bi-directional Inter MB.                          */
/*                                                               */
/*   RETURN                                                      */
/*   The size (in bits) of the compressed macroblock data.       */
/*   There is something wrong if the size is negative.           */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    /* This is a placeholder */
    return 0;
}

xint
cavlc_encode(h264enc_obj *pEnc, int16 *pInput, xint block_type, xint block, xint YCbCr, xint coded)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Feb/03/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   CAVLC coding of a 4x4 block.                                */
/*                                                               */
/*   RETURN                                                      */
/*   The status code.                                            */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    static int16 Level[16];
    static int16 Run[16];

    memset(Level, 0 , sizeof(int16) * 16);
    memset(Run  , 0 , sizeof(int16) * 16);

    if(coded)
    {
        if (block_type == ChromaDCLevel)
            chromaDC_runlevel(pInput, Run, Level);
        else
            zigzag_runlevel(pInput, Run, Level, block_type);
    }

    write_block_cavlc(pEnc, Level, Run, block_type, block, YCbCr, coded);

    return MMES_NO_ERROR;
}

xint
entropy_coding(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Apr/10/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Entropy coding of a macroblock.                             */
/*                                                               */
/*   RETURN                                                      */
/*   The size (in bits) of the compressed macroblock data.       */
/*   There is something wrong if the size is negative.           */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj      *pMB    = &(pEnc->curr_slice->cmb);
    xint  block;
    xint  b8x8, b4x4;
    xint  nbits;
    xint  LumaACtype;
    xint  YCbCr;      // Y:0, Cb:1, Cr:2
    xint  coded;
    xint  mb_mode = pMB->best_MB_mode ? pMB->best_Inter_mode : pMB->best_I16x16_mode;

    if(pMB->best_MB_mode == I_PCM)
        return MMES_NO_ERROR;

    nbits = get_bitpos(pEnc);

    if(I16MB(mb_mode))         // intra_16x16
    {
        coded = 1;
        block = 0;
        YCbCr = Y_block;
        cavlc_encode(pEnc, pMB->LumaDCLevel, Intra16x16DCLevel, block, YCbCr, coded);
        LumaACtype = Intra16x16ACLevel;
    }
    else                                     // intra_4x4
    {
        LumaACtype = LumaLevel;
    }

    // LumaAC
    YCbCr = Y_block;
    for(b8x8 = 0, block = 0; b8x8 < 4 ; b8x8++)
    {
        coded = pMB->cbp & (1 << b8x8);

        for(b4x4 = 0 ; b4x4 < 4 ; b4x4++, block++)
            cavlc_encode(pEnc, &(pMB->LumaACLevel[block*B_SIZE*B_SIZE]), LumaACtype, block, YCbCr, coded);
    }

    // ChromaDC
    coded = (pMB->cbp >> 4) & 3;

    YCbCr = Cb_block;
    block = 0;
    cavlc_encode(pEnc, pMB->CbDCLevel, ChromaDCLevel, block, YCbCr, coded);

    YCbCr = Cr_block;
    block = 0;
    cavlc_encode(pEnc, pMB->CrDCLevel, ChromaDCLevel, block, YCbCr, coded);

    // ChromaAC
    coded = (pMB->cbp >> 4) & 2;

    YCbCr = Cb_block;

    for(block=0; block<4; block++)      // for YCbCr420
        cavlc_encode(pEnc, &(pMB->CbACLevel[block*B_SIZE*B_SIZE]), ChromaACLevel, block, YCbCr, coded);

    YCbCr = Cr_block;
    for(block=0; block<4; block++)      // for YCbCr420
        cavlc_encode(pEnc, &(pMB->CrACLevel[block*B_SIZE*B_SIZE]), ChromaACLevel, block, YCbCr, coded);

    return get_bitpos(pEnc) - nbits;
}

xint
init_mb_info(slice_obj *pSlice, xint width)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Jun/24/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Set macroblock information for predictions                  */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice  -> [I/O] pointer to the slice session parameter    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB;
    xint    frame_x_16_offset, frame_y_16_offset, nx_mb;

    nx_mb = width / MB_SIZE;
    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % nx_mb;
    frame_y_16_offset = pMB->id / nx_mb;

    pMB->cbp      = 0;
    pMB->qp_delta = 0;

#if !SPEEDUP
    // clear All coeff level
    memset(pMB->LumaDCLevel, 0, sizeof(int16) *     B_SIZE *  B_SIZE);
    memset(pMB->LumaACLevel, 0, sizeof(int16) *    MB_SIZE * MB_SIZE);
    memset(pMB->CbDCLevel  , 0, sizeof(int16) *          2 *       2);
    memset(pMB->CrDCLevel  , 0, sizeof(int16) *          2 *       2);
    memset(pMB->CbACLevel  , 0, sizeof(int16) * (MB_SIZE/2)*(MB_SIZE/2));
    memset(pMB->CrACLevel  , 0, sizeof(int16) * (MB_SIZE/2)*(MB_SIZE/2));

    // clear reference indices
    memset(pMB->ref_idx    , 0, sizeof(xint)  * NBLOCKS);
#endif

    // top_valid
    if( pSlice->no_mbs >= nx_mb )
    {
        memset(pSlice->top_valid, MMES_VALID, (width/B_SIZE));
    }

    // left_valid
    if( pSlice->no_mbs == 0 || frame_x_16_offset == 0 )// ( first MB in slice || first MB in column )
    {
        memset(pSlice->left_valid, MMES_INVALID, (MB_SIZE/B_SIZE)*sizeof(uint8));
    }
    else
    {
        memset(pSlice->left_valid, MMES_VALID, (MB_SIZE/B_SIZE)*sizeof(uint8));
    }

    // upleft_valid
    if( pSlice->no_mbs == 0 || frame_x_16_offset == 0 )// ( first MB in slice || first MB in column )
    {
        memset(pSlice->upleft_valid, MMES_INVALID, (MB_SIZE/B_SIZE)*sizeof(uint8));
    }
    else if( pSlice->no_mbs < nx_mb || frame_y_16_offset == 0 || pSlice->no_mbs == nx_mb )// ( first nx_mb MBs in slice || first row || the nx_mb-th MB )
    {
        pSlice->upleft_valid[0] = MMES_INVALID;
        pSlice->upleft_valid[1] = pSlice->upleft_valid[2] = pSlice->upleft_valid[3] = MMES_VALID;
    }
    else
    {
        memset(pSlice->upleft_valid, MMES_VALID, (MB_SIZE/B_SIZE)*sizeof(uint8));
    }

    // deblock_top_valid
    if( pSlice->no_mbs >= nx_mb )
    {
        memset(pSlice->deblock_top_valid, MMES_VALID, (width/MB_SIZE));
    }

    // deblock_left_valid
    if( pSlice->no_mbs == 0 || frame_x_16_offset == 0 )// ( first MB in slice || first MB in row )
    {
        pSlice->deblock_left_valid = MMES_INVALID;
    }
    else
    {
        pSlice->deblock_left_valid = MMES_VALID;
    }

    return MMES_NO_ERROR;
}

__inline xint
read_ref_idx(h264dec_obj * pDec, xint mb_mode, xint num_ref_idx_l0_active, xint ref0)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Sep/05/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   read reference index                                        */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    xint use_te = (num_ref_idx_l0_active == 2);
    xint idx;
    mb_obj *pMB = &pDec->curr_slice->cmb;

    if(num_ref_idx_l0_active > 1)
    {
        switch(mb_mode)
        {
        case INTER_16x16:
            if(use_te)
                pMB->ref_idx[0] = (1 - get_bits(pDec, 1));
            else
                pMB->ref_idx[0] = read_uvlc_codeword(pDec);

            pMB->ref_idx[1] = pMB->ref_idx[2] = pMB->ref_idx[3] = pMB->ref_idx[0];
            break;
        case INTER_16x8:
            for(idx = 0 ; idx < 2 ; idx++)
            {
                if(use_te)
                    pMB->ref_idx[idx<<1] = (1 - get_bits(pDec, 1));
                else
                    pMB->ref_idx[idx<<1] = read_uvlc_codeword(pDec);
            }
            pMB->ref_idx[1] = pMB->ref_idx[0];
            pMB->ref_idx[3] = pMB->ref_idx[2];
            break;
        case INTER_8x16:
            for(idx = 0 ; idx < 2 ; idx++)
            {
                if(use_te)
                    pMB->ref_idx[idx] = (1 - get_bits(pDec, 1));
                else
                    pMB->ref_idx[idx] = read_uvlc_codeword(pDec);
            }
            pMB->ref_idx[2] = pMB->ref_idx[0];
            pMB->ref_idx[3] = pMB->ref_idx[1];
            break;
        case INTER_P8x8:
            if(ref0 != 1)
            {
                for(idx = 0 ; idx < 4 ; idx++)
                {
                    if(use_te)
                        pMB->ref_idx[idx] = (1 - get_bits(pDec, 1));
                    else
                        pMB->ref_idx[idx] = read_uvlc_codeword(pDec);
                }
            }
            break;
        default:
            break;
        }
    }
    return MMES_NO_ERROR;
}

xint
decode_mb_info(h264dec_obj * pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/09/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Decode MB Information, inclusive of mb_type, mb_pred,       */
/*   cbp, and mb_qp_delta INTO BITSTREAM                         */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    slice_obj  *pSlice = pDec->curr_slice;
    mb_obj     *pMB    = &(pDec->curr_slice->cmb);
    xint       mb_mode;
    const xint ICBPTAB[6] = {0,16,32,15,31,47};
    xint       idx;
    xint       intra;
    int        mode;
    xint       dec_mv = 0;
    xint       frame_x_16_offset = pMB->id % pDec->nx_mb;

    pMB->cbp_blk         = 0;

    if(pSlice->type != I_SLICE && pSlice->type != SI_SLICE)
    {
        pSlice->mb_skip_run--;
        if(pSlice->mb_skip_run < 0 && (pSlice->mb_skip_run = read_uvlc_codeword(pDec)) > 0)
        {
            // clear nZCoeff
            for(idx = pMB->id ; idx < pMB->id+pSlice->mb_skip_run ; idx++)
            {
                xint frame_x_4_offset;
                // for Y_Block
                frame_x_4_offset = ((idx % pDec->nx_mb)<<2);
                memset(pSlice->top_y_nzc+frame_x_4_offset,  0, sizeof(int8) * 4);
                // for Cb_Block & Cr_Block
                frame_x_4_offset = ((idx % pDec->nx_mb)<<1);
                memset(pSlice->top_cb_nzc+frame_x_4_offset, 0, sizeof(int8) * 2);
                memset(pSlice->top_cr_nzc+frame_x_4_offset, 0, sizeof(int8) * 2);

                if(((idx+1)%pDec->nx_mb) == 0)
                {
                    memset(pSlice->left_y_nzc, -1, sizeof(int8) * 4);
                    memset(pSlice->left_cb_nzc,-1, sizeof(int8) * 2);
                    memset(pSlice->left_cr_nzc,-1, sizeof(int8) * 2);
                }
                else
                {
                    memset(pSlice->left_y_nzc,  0, sizeof(int8) * 4);
                    memset(pSlice->left_cb_nzc, 0, sizeof(int8) * 2);
                    memset(pSlice->left_cr_nzc, 0, sizeof(int8) * 2);
                }
            }
        }

        if(pSlice->mb_skip_run > 0)
        {
            pMB->best_MB_mode    = 1;
            pMB->best_Inter_mode = INTER_PSKIP;
            memset(pMB->best_Inter_cb_residual, 0, sizeof(int16)*((MB_SIZE*MB_SIZE)>>2));
            memset(pMB->best_Inter_cr_residual, 0, sizeof(int16)*((MB_SIZE*MB_SIZE)>>2));
            memset(pMB->best_Inter_residual, 0, sizeof(int16)*(MB_SIZE*MB_SIZE) );
            memset(pMB->ref_idx, 0, sizeof(xint)*4);
            memset( &pMB->mv.x, 0, 16*sizeof(int16));
            memset( &pMB->mv.y, 0, 16*sizeof(int16));

            return MMES_NO_ERROR;
        }


        if(pMB->id >= pDec->total_mb)
            return MMES_NO_ERROR;
    }

    /* read mb_type */
    mb_mode = read_uvlc_codeword(pDec);

    if(pSlice->type != I_SLICE && pSlice->type != SI_SLICE)
        mb_mode += 1;

    if(pSlice->type == I_SLICE || pSlice->type == ALL_I_SLICE)
    {
        intra = 1;
        if(mb_mode == 0)
            pMB->best_I16x16_mode = INTRA_4x4;
        else if(mb_mode == 25)
            pMB->best_I16x16_mode = I_PCM;
        else
        {
            pMB->best_I16x16_mode = (mb_mode-1) & 0x03;
            pMB->cbp              = ICBPTAB[(mb_mode-1)>>2];
            pMB->cbp_blk          = 0;
        }

        /* for constraint intra-prediction */
        memset(&pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)], 1, (MB_SIZE/B_SIZE));
        memset(pSlice->left_mb_intra, 1, (MB_SIZE/B_SIZE));
    }
    else
    {
        // mb_type(mb_mode-1)     Name of mb_type
        //       0                P_L0_16x16
        //       1                P_L0_L0_16x8
        //       2                P_L0_L0_8x16
        //       3                P_8x8
        //       4                P_8x8ref0
        intra = 0;
        if(mb_mode < 4)
        {
            pMB->best_Inter_mode = INTER_16x16 + mb_mode - 1;
            pMB->best_MB_mode    = 1;
        }
        else if(mb_mode == 4 || mb_mode == 5)
        {
            //INTERP8x8     when mb_mode == 4
            //INTERP8x8ref0 when mb_mode == 5
            pMB->best_Inter_mode = INTER_P8x8;
            pMB->best_MB_mode    = 1;
        }
        else if(mb_mode == 6)
        {
            pMB->best_I16x16_mode= INTRA_4x4;
            pMB->best_MB_mode    = 0;
            intra                = 1;
        }
        else if(mb_mode == 31)
        {
            pMB->best_Inter_mode = I_PCM;
            pMB->best_MB_mode    = 0;
        }
        else
        {
            pMB->cbp             = ICBPTAB[(mb_mode-7)>>2];
            pMB->cbp_blk         = 0;
            pMB->best_I16x16_mode= (mb_mode-7) & 0x03;
            pMB->best_MB_mode    = 0;
            intra                = 1;
        }
    }

    mode = (intra) ? pMB->best_I16x16_mode : pMB->best_Inter_mode;

    if(mode == I_PCM)
    {
        int16 *res_y    = pMB->best_I16x16_residual;
        int16 *res_cb   = pMB->cb_residual;
        int16 *res_cr   = pMB->cr_residual;
        xint  B8x8_SIZE = MB_SIZE / 2;
        xint  frame_x_4_offset;

        pMB->best_MB_mode = I_PCM;

        // Set nZCoeff
        // for Y_Block
        frame_x_4_offset = (frame_x_16_offset<<2);
        memset(pSlice->top_y_nzc+frame_x_4_offset,  16, sizeof(int8) * 4);
        // for Cb_Block & Cr_Block
        frame_x_4_offset = (frame_x_16_offset<<1);
        memset(pSlice->top_cb_nzc+frame_x_4_offset, 16, sizeof(int8) * 2);
        memset(pSlice->top_cr_nzc+frame_x_4_offset, 16, sizeof(int8) * 2);

        if(((pMB->id+1)%pDec->nx_mb) == 0)
        {
            memset(pSlice->left_y_nzc, -1, sizeof(int8) * 4);
            memset(pSlice->left_cb_nzc,-1, sizeof(int8) * 2);
            memset(pSlice->left_cr_nzc,-1, sizeof(int8) * 2);
        }
        else
        {
            memset(pSlice->left_y_nzc,  16, sizeof(int8) * 4);
            memset(pSlice->left_cb_nzc, 16, sizeof(int8) * 2);
            memset(pSlice->left_cr_nzc, 16, sizeof(int8) * 2);
        }

        pMB->cbp       = 0x3F;
        pMB->cbp_blk   = 0xFFFF;

        while(!byte_align(pDec, 0))
            get_bits(pDec, 1);

        // Y
        for(idx = 0 ; idx < MB_SIZE * MB_SIZE ; idx++)
        {
            *res_y++ = (int16)get_bits(pDec, 8);
        }
        // Cb
        for(idx = 0 ; idx < B8x8_SIZE * B8x8_SIZE ; idx++)
        {
            *res_cb++ = (int16)get_bits(pDec, 8);
        }
        // Cr
        for(idx = 0 ; idx < B8x8_SIZE * B8x8_SIZE ; idx++)
        {
            *res_cr++ = (int16)get_bits(pDec, 8);
        }
  
        return MMES_NO_ERROR;
    }

    /* read mb_pred */

    if((mode != INTRA_4x4 && !I16MB(mode)) && mode == INTER_P8x8)
    {
        /* sub_mb_pred */
        for(idx = 0 ; idx < 4 ; idx++)
            pMB->best_8x8_blk_mode[idx] = read_uvlc_codeword(pDec) + INTER_8x8;
        dec_mv = 1;
    }
    else
    {
        if(mode == INTRA_4x4 || I16MB(mode))
        {
            if(mode == INTRA_4x4)
            {
                for(idx = 0 ; idx < NBLOCKS ; idx++)
                {
                    if(get_bits(pDec, 1))
                    {
                        pMB->best_I4x4_mode[idx] = -1;
                    }
                    else
                    {
                        pMB->best_I4x4_mode[idx] = (int8) get_bits(pDec, 3);
                    }
                }
            }
            pMB->best_chroma_mode = read_uvlc_codeword(pDec) + 4;

            dec_mv = 0;
        }
        else
            dec_mv = 1;
    }

    if(dec_mv)
    {
        const xint order[4] = {0, 2, 8, 10};

        /* if mb_mode is 5 => mb_type is P_8x8ref0 */
        xint ref0 = (mode == INTER_P8x8) ? (mb_mode == 5) : 0;

        /* ref_idx */
        read_ref_idx(pDec, mode, pSlice->num_ref_idx_l0_active, ref0);
        /* read forward motion vectors */
        switch(mode)
        {
        case INTER_16x16:
            pMB->mvd.x[0] = (int16)read_signed_uvlc_codeword(pDec);
            pMB->mvd.y[0] = (int16)read_signed_uvlc_codeword(pDec);
            for( idx=0; idx<NBLOCKS; idx++ )
            {
                pMB->mv.x[idx] = pMB->mvd.x[0];
                pMB->mv.y[idx] = pMB->mvd.y[0];
            }
            break;
        case INTER_16x8:
            // top
            pMB->mvd.x[0] = (int16)read_signed_uvlc_codeword(pDec);
            pMB->mvd.y[0] = (int16)read_signed_uvlc_codeword(pDec);
            for( idx=0; idx<8; idx++ )
            {
                pMB->mv.x[idx] = pMB->mvd.x[0];
                pMB->mv.y[idx] = pMB->mvd.y[0];
            }
            // bottom
            pMB->mvd.x[8] = (int16)read_signed_uvlc_codeword(pDec);
            pMB->mvd.y[8] = (int16)read_signed_uvlc_codeword(pDec);
            for( idx=8; idx<NBLOCKS; idx++ )
            {
                pMB->mv.x[idx] = pMB->mvd.x[8];
                pMB->mv.y[idx] = pMB->mvd.y[8];
            }
            break;
        case INTER_8x16:
            // left
            pMB->mvd.x[0] = (int16)read_signed_uvlc_codeword(pDec);
            pMB->mvd.y[0] = (int16)read_signed_uvlc_codeword(pDec);
            for( idx=0; idx<4; idx++ )
            {
                pMB->mv.x[4*idx]   = pMB->mvd.x[0];
                pMB->mv.y[4*idx]   = pMB->mvd.y[0];
                pMB->mv.x[4*idx+1] = pMB->mvd.x[0];
                pMB->mv.y[4*idx+1] = pMB->mvd.y[0];
            }
            // right
            pMB->mvd.x[2] = (int16)read_signed_uvlc_codeword(pDec);
            pMB->mvd.y[2] = (int16)read_signed_uvlc_codeword(pDec);
            for( idx=0; idx<4; idx++ )
            {
                pMB->mv.x[4*idx+2] = pMB->mvd.x[2];
                pMB->mv.y[4*idx+2] = pMB->mvd.y[2];
                pMB->mv.x[4*idx+3] = pMB->mvd.x[2];
                pMB->mv.y[4*idx+3] = pMB->mvd.y[2];
            }
            break;

        case INTER_8x8:
            fprintf(stderr, "Line %d => Exception:INTER_8x8.\n", __LINE__);
            break;
        case INTER_P8x8:
            for(idx = 0 ; idx < 4 ; idx++)
            {
                xint i;
                switch(pMB->best_8x8_blk_mode[idx])
                {
                case INTER_8x8:
                    pMB->mvd.x[order[idx]]   = (int16)read_signed_uvlc_codeword(pDec);
                    pMB->mvd.y[order[idx]]   = (int16)read_signed_uvlc_codeword(pDec);
                    for( i=0; i<4; i++ )
                    {
                        pMB->mv.x[db_z[idx][i]] = pMB->mvd.x[order[idx]];
                        pMB->mv.y[db_z[idx][i]] = pMB->mvd.y[order[idx]];
                    }
                    break;
                case INTER_8x4:
                    pMB->mvd.x[order[idx]]   = (int16)read_signed_uvlc_codeword(pDec);
                    pMB->mvd.y[order[idx]]   = (int16)read_signed_uvlc_codeword(pDec);
                    pMB->mvd.x[order[idx]+4] = (int16)read_signed_uvlc_codeword(pDec);
                    pMB->mvd.y[order[idx]+4] = (int16)read_signed_uvlc_codeword(pDec);
                    for( i=0; i<2; i++ )
                    {
                        pMB->mv.x[order[idx]+4*i]   = pMB->mvd.x[order[idx]+4*i];
                        pMB->mv.y[order[idx]+4*i]   = pMB->mvd.y[order[idx]+4*i];
                        pMB->mv.x[order[idx]+4*i+1] = pMB->mvd.x[order[idx]+4*i];
                        pMB->mv.y[order[idx]+4*i+1] = pMB->mvd.y[order[idx]+4*i];
                    }
                    break;
                case INTER_4x8:
                    pMB->mvd.x[order[idx]]   = (int16)read_signed_uvlc_codeword(pDec);
                    pMB->mvd.y[order[idx]]   = (int16)read_signed_uvlc_codeword(pDec);
                    pMB->mvd.x[order[idx]+1] = (int16)read_signed_uvlc_codeword(pDec);
                    pMB->mvd.y[order[idx]+1] = (int16)read_signed_uvlc_codeword(pDec);
                    for( i=0; i<2; i++ )
                    {
                        pMB->mv.x[order[idx]+i]   = pMB->mvd.x[order[idx]+i];
                        pMB->mv.y[order[idx]+i]   = pMB->mvd.y[order[idx]+i];
                        pMB->mv.x[order[idx]+i+4] = pMB->mvd.x[order[idx]+i];
                        pMB->mv.y[order[idx]+i+4] = pMB->mvd.y[order[idx]+i];
                    }
                    break;
                case INTER_4x4:
                    for( i=0; i<4; i++ )
                    {
                        pMB->mvd.x[db_z[idx][i]]  = (int16)read_signed_uvlc_codeword(pDec);
                        pMB->mvd.y[db_z[idx][i]]  = (int16)read_signed_uvlc_codeword(pDec);
                        pMB->mv.x[db_z[idx][i]]   = pMB->mvd.x[db_z[idx][i]];
                        pMB->mv.y[db_z[idx][i]]   = pMB->mvd.y[db_z[idx][i]];
                    }
                    break;
                }
            }
        default:
            break;
        }
    }
    /* read cbp */
    if(!I16MB(mode))
        pMB->cbp = DCBP[read_uvlc_codeword(pDec)][!intra];

    // QPy = (QPy,prev+mb_qp_delta+52)%52.
    // For the first mb in the slice QPy,prev is initially set equal to SliceQPy.

    /* read mb_qp_delta */
    if(pMB->cbp || I16MB(mode))
    {
        pMB->qp_delta = read_signed_uvlc_codeword(pDec);
        pMB->QP = (pMB->QP +  pMB->qp_delta + 52) % 52;
    }

    return MMES_NO_ERROR;
}

xint
cavlc_decode(h264dec_obj *pDec, int16 * coeffLevel, xint block_type, xint block, xint YCbCr, xint coded)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/10/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   CAVLC decoding of a 4x4 block.                              */
/*                                                               */
/*   RETURN                                                      */
/*   The status code.                                            */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    static int16 Level[16];
    xint   maxnumcoeff, start;

    memset(Level, 0 , sizeof(int16) * 16);

    residual_block_cavlc(pDec, Level, block_type, block, YCbCr, coded);

    if(coded)
    {
        switch(block_type)
        {
        case Intra16x16DCLevel:
        case LumaLevel:
            maxnumcoeff = 16;
            start       = 0;
            break;
        case Intra16x16ACLevel:
        case ChromaACLevel:
            maxnumcoeff = 15;
            start       = 1;
            break;
        case ChromaDCLevel:
            maxnumcoeff = 4;
            start       = 0;
            break;
        default:
            return MMES_ERROR;
            break;
        }
        inv_zigzag(Level, coeffLevel, maxnumcoeff, start);
    }

    return MMES_NO_ERROR;
}

xint
entropy_decoding(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/9/2005                                          */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Entropy decoding of a macroblock.                           */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj      *pMB    = &(pDec->curr_slice->cmb);
    slice_obj   *pSlice = pDec->curr_slice;
    xint        i, j, idx;
    xint        coded, block, YCbCr, LumaACtype;
    xint        b8x8, b4x4;
    const xint  chroma_x[4] = {0, 1, 0, 1};
    const xint  chroma_y[4] = {0, 0, 1, 1};
    int16       zero[16] = {0};
    xint        mode = (pMB->best_MB_mode) ? pMB->best_Inter_mode : pMB->best_I16x16_mode;
    uint        v;

    if(pMB->best_MB_mode == I_PCM || mode == INTER_PSKIP)
        return MMES_NO_ERROR;

    if(I16MB(mode))         // intra_16x16
    {
        coded = 1;
        block = 0;
        YCbCr = Y_block;
        cavlc_decode(pDec, pMB->LumaDCLevel, Intra16x16DCLevel, block, YCbCr, coded);
        LumaACtype = Intra16x16ACLevel;
    }
    else                                     // intra_4x4
    {
        LumaACtype = LumaLevel;
    }

    // LumaAC
    YCbCr = Y_block;

    for(b8x8 = 0, block = 0; b8x8 < 4 ; b8x8++)
    {
        coded = pMB->cbp & (1 << b8x8);

        for(b4x4 = 0 ; b4x4 < 4 ; b4x4++, block++)
        {
            cavlc_decode(pDec, &(pMB->LumaACLevel[block*B_SIZE*B_SIZE]), LumaACtype, block, YCbCr, coded);
            v = show_bits(pDec, 32);
        }
    }

    // ChromaDC
    coded = (pMB->cbp >> 4) & 3;

    YCbCr = Cb_block;
    block = 0;
    cavlc_decode(pDec, pMB->CbDCLevel, ChromaDCLevel, block, YCbCr, coded);
    v = show_bits(pDec, 32);

    YCbCr = Cr_block;
    block = 0;
    cavlc_decode(pDec, pMB->CrDCLevel, ChromaDCLevel, block, YCbCr, coded);
    v = show_bits(pDec, 32);

    // ChromaAC
    coded = (pMB->cbp >> 4) & 2;

    YCbCr = Cb_block;
    for(block=0; block<4; block++)      // for YCbCr420
    {
        cavlc_decode(pDec, &(pMB->CbACLevel[block*B_SIZE*B_SIZE]), ChromaACLevel, block, YCbCr, coded);
        v = show_bits(pDec, 32);
    }

    YCbCr = Cr_block;
    for(block=0; block<4; block++)      // for YCbCr420
    {
        cavlc_decode(pDec, &(pMB->CrACLevel[block*B_SIZE*B_SIZE]), ChromaACLevel, block, YCbCr, coded);
        v = show_bits(pDec, 32);
    }

    /* reconstruct Y, Cb, Cr MB coeffs */
    if(pSlice->type == P_SLICE && pMB->best_MB_mode)
    {
        // Luma
        for(i = 0, idx = 0 ; i < MB_SIZE ; i++)
        {
            xint  z_x        = mb_x_4_idx[i];
            xint  z_y        = mb_y_4_idx[i];
            int16 *res_start = pMB->best_Inter_residual + z_x * B_SIZE + z_y * B_SIZE * MB_SIZE;
            for(j = 0 ; j < B_SIZE ; j++, idx++)
                memcpy( res_start + j * MB_SIZE, pMB->LumaACLevel + idx * B_SIZE, sizeof(int16)*B_SIZE);
        }
        // calc cbp_blk
        for(i = 3 ; i >= 0 ; i--)
            for(j = 3 ; j >= 0 ; j--)
                if(memcmp(pMB->LumaACLevel+(i * 64 + j * 16), zero, sizeof(int16)*16))
                    pMB->cbp_blk |= (1<<db_z[i][j]);

        // Chroma
        for(i = 0 ; i < B_SIZE ; i++)
        {
            pMB->CbACLevel[i * MB_SIZE] = pMB->CbDCLevel[i];
            pMB->CrACLevel[i * MB_SIZE] = pMB->CrDCLevel[i];
        }
        for(i = 0, idx = 0 ; i < B_SIZE ; i++)
        {
            xint  z_x        = chroma_x[i];
            xint  z_y        = chroma_y[i];
            int16 *cb_start  = pMB->best_Inter_cb_residual + z_x * B_SIZE + z_y * B_SIZE * MB_SIZE/2;
            int16 *cr_start  = pMB->best_Inter_cr_residual + z_x * B_SIZE + z_y * B_SIZE * MB_SIZE/2;
            for(j = 0 ; j < B_SIZE ; j++, idx++)
            {
                memcpy(cb_start + j * MB_SIZE/2, pMB->CbACLevel + idx * B_SIZE, sizeof(int16) * B_SIZE);
                memcpy(cr_start + j * MB_SIZE/2, pMB->CrACLevel + idx * B_SIZE, sizeof(int16) * B_SIZE);
            }
        }
        // calc cbp_blk
        for(idx = 3 ; idx >= 0 ; idx--)
        {
            if(memcmp(pMB->CbACLevel+idx, zero, sizeof(int16)*16))
                pMB->cbp_blk |= (1<<(16+idx));
            if(memcmp(pMB->CrACLevel+idx, zero, sizeof(int16)*16))
                pMB->cbp_blk |= (1<<(20+idx));
        }
    }
    else
    {
        // Luma
        if(I16MB(pMB->best_I16x16_mode))
        {
            for(i = 0, idx = 0 ; i < MB_SIZE ; i++)
            {
                xint  z_x        = mb_x_4_idx[i];
                xint  z_y        = mb_y_4_idx[i];
                int16 *res_start = pMB->best_I16x16_residual + z_x * B_SIZE + z_y * B_SIZE * MB_SIZE;
                for(j = 0 ; j < B_SIZE ; j++, idx++)
                     memcpy( res_start + j * MB_SIZE, pMB->LumaACLevel + idx * B_SIZE, sizeof(int16)*B_SIZE);
            }

            for(j = 0, idx = 0 ; j < B_SIZE ; j++)
                for(i = 0 ; i < B_SIZE ; i++, idx++)
                    pMB->best_I16x16_residual[ i * B_SIZE + j * B_SIZE * MB_SIZE] = pMB->LumaDCLevel[idx];
        }
        else if(pMB->best_I16x16_mode == INTRA_4x4)
        {
            for(i = 0, idx = 0 ; i < MB_SIZE ; i++)
            {
                xint  z_x        = mb_x_4_idx[i];
                xint  z_y        = mb_y_4_idx[i];
                int16 *res_start = pMB->best_I4x4_residual + z_x * B_SIZE + z_y * B_SIZE * MB_SIZE;
                for(j = 0 ; j < B_SIZE ; j++, idx++)
                     memcpy( res_start + j * MB_SIZE, pMB->LumaACLevel + idx * B_SIZE, sizeof(int16)*B_SIZE);
            }
        }
        // Chroma

        for(i = 0, idx = 0 ; i < B_SIZE ; i++)
        {
            xint  z_x        = chroma_x[i];
            xint  z_y        = chroma_y[i];
            int16 *cb_start  = pMB->cb_residual + z_x * B_SIZE + z_y * B_SIZE * MB_SIZE/2;
            int16 *cr_start  = pMB->cr_residual + z_x * B_SIZE + z_y * B_SIZE * MB_SIZE/2;
            for(j = 0 ; j < B_SIZE ; j++, idx++)
            {
                memcpy(cb_start + j * MB_SIZE/2, pMB->CbACLevel + idx * B_SIZE, sizeof(int16) * B_SIZE);
                memcpy(cr_start + j * MB_SIZE/2, pMB->CrACLevel + idx * B_SIZE, sizeof(int16) * B_SIZE);
            }
        }
        for(i = 0 ; i < B_SIZE ; i++)
        {
            xint  z_x        = chroma_x[i];
            xint  z_y        = chroma_y[i];
            pMB->cb_residual[z_x * B_SIZE + z_y * B_SIZE * MB_SIZE/2] = pMB->CbDCLevel[i];
            pMB->cr_residual[z_x * B_SIZE + z_y * B_SIZE * MB_SIZE/2] = pMB->CrDCLevel[i];
        }
    }

    return MMES_NO_ERROR;
}

xint
decode_intra_mb(h264dec_obj * pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Jun/27/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Decode an Intra MB.                                         */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    xint       block;
    slice_obj  *pSlice = pDec->curr_slice;
    mb_obj     *pMB = &(pDec->curr_slice->cmb);
    pic_paraset_obj *pps;

    pMB->best_MB_mode = 0;

    pps = &pDec->pps[pSlice->pic_parameter_set_id];

    /* decode mb information */
    decode_mb_info(pDec);

    {
        uint v = show_bits(pDec, 32);
        printf("");
    }

    /* IPCM mode */
    if( pMB->best_I16x16_mode == I_PCM )
    {
        xint mb_qp = pMB->QP;

        /* decode mb coefficient */
        entropy_decoding(pDec);

        dup_16x16_to_frame( pDec->curf, pMB->best_I16x16_residual, pMB->cb_residual, pMB->cr_residual, pMB->id, pDec->width );
        set_ipcm_info( pDec->curr_slice, pDec->recf, pDec->curf, pDec->width );

        /* apply deblocking filter */
        deblock(pDec->curr_slice, pDec->width, pDec->recf);

        /* update MB information */
        update_MB( pDec->curr_slice, pDec->width );

        pMB->QP = mb_qp;
        return MMES_NO_ERROR;
    }

    /* decode mb coefficient */
    entropy_decoding(pDec);

    /* reconstruct the encoded chroma MB for future references */
    inv_quant_chroma(pDec->curr_slice);

    inv_trans_chroma( pDec->curr_slice );

    dec_intra_chroma_prediction( pDec );
    dec_intra_chroma_compensation( pDec );

    if( pMB->best_I16x16_mode <= INTRA_16x16_PL )
    {
        /* use 16x16 mode prediction */
        inv_quant_16x16(pDec->curr_slice);

        inv_trans_16x16(pDec->curr_slice);

        dec_intra_16x16_prediction(pDec);
        dec_intra_16x16_compensation(pDec);
    }
    else if( pMB->best_I16x16_mode == INTRA_4x4 )
    {
        /* use 4x4 mode prediction */
        for (block = 0; block < NBLOCKS; block++)
        {
            get_I4x4_pred_mode( pSlice, block, pDec->nx_mb, pps->constrained_intra_pred_flag);

            /* reconstruct the encoded block for future references */
            inv_quant_4x4(pDec->curr_slice, block);

            inv_trans_4x4(pDec->curr_slice, block);

            dec_intra_4x4_prediction(pDec, block);
            dec_intra_4x4_compensation(pDec, block);
        }
    }
    else
    {
        printf( "best_I16x16_mode error\n" );
    }

    dup_mb(pDec->recf, pDec->curf, pDec->curr_slice->cmb.id, pDec->width);

    /* apply deblocking filter */
    deblock(pDec->curr_slice, pDec->width, pDec->recf);

    /* update MB information */
    update_MB( pDec->curr_slice, pDec->width );

    return MMES_NO_ERROR;
}

xint
get_I4x4_pred_mode( slice_obj *pSlice, xint block, xint nx_mb, xint constrained_intra_pred_flag )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Jul/21/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   get the intra 4x4 prediction mode                           */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice  -> [I/O] pointer to the slice session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint mb_xpos, blk4_xpos, blk4_ypos;
    xint up_avail, left_avail;
    int  up_mode, left_mode, probable_mode, mode;
    mb_obj *pMB = &(pSlice->cmb);

    mb_xpos = pMB->id % nx_mb;

    blk4_xpos = mb_xpos*4 + mb_x_4_idx[ block ];
    blk4_ypos = mb_y_4_idx[ block ];

    up_avail = pSlice->top_valid[ blk4_xpos ];
    left_avail = pSlice->left_valid[ blk4_ypos ];

    if( constrained_intra_pred_flag )
    {
        up_avail = up_avail ? pSlice->top_mb_intra[blk4_xpos] : 0;
        left_avail = left_avail ? pSlice->left_mb_intra[blk4_ypos] : 0;
    }

    left_mode = (left_avail) ? pSlice->left_mode[ blk4_ypos ]  : -1;
    up_mode   = (up_avail)   ? pSlice->top_mode[ blk4_xpos ] : -1;
    probable_mode = ( left_mode < 0 || up_mode < 0 ) ? INTRA_4x4_DC : up_mode < left_mode ? up_mode : left_mode;

    mode = pMB->best_I4x4_mode[ block ];
    pMB->best_I4x4_mode[ block ] = ( mode<0 ) ? probable_mode : ( mode < (probable_mode-8) ) ? mode + 8 : mode + 8 + 1;
    pSlice->left_mode[ blk4_ypos ] = pMB->best_I4x4_mode[ block ];
    pSlice->top_mode[ blk4_xpos ] = pMB->best_I4x4_mode[ block ];

    return MMES_NO_ERROR;
}

xint
decode_inter_mb(h264dec_obj * pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Sep/02/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Decode an Inter MB.                                         */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice = pDec->curr_slice;
    mb_obj *pMB = &(pDec->curr_slice->cmb);
    xint block;
    pic_paraset_obj *pps;

    pps = &pDec->pps[pSlice->pic_parameter_set_id];

    /* decode mb information */
    decode_mb_info(pDec);

    /* IPCM mode */
    if( pMB->best_MB_mode == I_PCM )
    {
        xint mb_qp = pMB->QP;
        /* decode mb coefficient */
        entropy_decoding(pDec);

        dup_16x16_to_frame( pDec->curf, pMB->best_I16x16_residual, pMB->cb_residual, pMB->cr_residual, pMB->id, pDec->width );
        set_ipcm_info( pDec->curr_slice, pDec->recf, pDec->curf, pDec->width );

        /* Inter set preidction information */
        inter_set_pred_info( pDec->curr_slice, pDec->width );

        /* apply deblocking filter */
        deblock(pDec->curr_slice, pDec->width, pDec->recf);

        /* update MB information */
        update_MB( pDec->curr_slice, pDec->width );

        pMB->QP = mb_qp;
        return MMES_NO_ERROR;
    }

    /* decode mb coefficient */
    entropy_decoding(pDec);

    /* backup prediction data for further usage */
    intra_RD_backup( pSlice, pDec->backup, pDec->curf, pDec->width, pDec->height );
	restore_intra_flag(pDec->backup->left_mb_intra_bup, pSlice->left_mb_intra, sizeof(pSlice->left_mb_intra));

    if( pMB->best_MB_mode )
    {
        /* mv prediction */
        mv_pred_inter( pSlice, pDec->backup, pDec->width, 0 );

        /* mv prediction add mvd */
        mv_pred_add_mvd( pSlice );

        //decoder motion compensation
        dec_cal_inter_comp_mb(pDec);

        /* set inter parameters */
        set_inter_parms( pSlice, pDec->curf, pDec->width );

        inv_quant_chroma(pSlice);

        inv_trans_chroma(pSlice);

        dec_inter_chroma_compensation(pDec);

        for (block = 0; block < NBLOCKS; block++)
        {
            /* reconstruct the encoded block for future references */
            inv_quant_4x4( pSlice, block );
            inv_trans_4x4( pSlice, block );
            dec_inter_4x4_compensation(pDec, block);
        }
    }
    else
    {
        /* reconstruct the encoded chroma MB for future references */
        inv_quant_chroma(pDec->curr_slice);

        inv_trans_chroma( pDec->curr_slice );

        dec_intra_chroma_prediction( pDec );
        dec_intra_chroma_compensation( pDec );

        if( pMB->best_I16x16_mode <= INTRA_16x16_PL )
        {
            /* use 16x16 mode prediction */
            inv_quant_16x16(pDec->curr_slice);

            inv_trans_16x16(pDec->curr_slice);

            dec_intra_16x16_prediction(pDec);
            dec_intra_16x16_compensation(pDec);
        }
        else if( pMB->best_I16x16_mode == INTRA_4x4 )
        {
            /* use 4x4 mode prediction */
            for (block = 0; block < NBLOCKS; block++)
            {
                get_I4x4_pred_mode( pSlice, block, pDec->nx_mb, pps->constrained_intra_pred_flag);

                /* reconstruct the encoded block for future references */
                inv_quant_4x4(pDec->curr_slice, block);

                inv_trans_4x4(pDec->curr_slice, block);

                dec_intra_4x4_prediction(pDec, block);
                dec_intra_4x4_compensation(pDec, block);
            }
        }
        else
        {
            printf( "best_I16x16_mode error\n" );
        }
    }

	restore_intra_flag(pSlice->left_mb_intra, pDec->backup->left_mb_intra_bup, sizeof(pSlice->left_mb_intra));

    /* Inter set preidction information */
    inter_set_pred_info( pDec->curr_slice, pDec->width );

    dup_mb(pDec->recf, pDec->curf, pDec->curr_slice->cmb.id, pDec->width);

    /* apply deblocking filter */
    deblock(pDec->curr_slice, pDec->width, pDec->recf);

    /* update MB information */
    update_MB( pDec->curr_slice, pDec->width );

    return MMES_NO_ERROR;
}

xint
enc_cal_inter_residual(h264enc_obj *pEnc)
{
    mb_obj  *pMB;
    xint    mbx, mby, width, i, j;
    uint8   *pY, *pCb, *pCr;

    pMB = &(pEnc->curr_slice->cmb);
    mbx = (pMB->id) % (pEnc->nx_mb);
    mby = (pMB->id) / (pEnc->nx_mb);
    width = pEnc->width;
    pY  = pEnc->curf->y;
    pCb = pEnc->curf->cb;
    pCr = pEnc->curf->cr;

    for (j=0; j<MB_SIZE; j++) {
        for (i=0; i<MB_SIZE; i++) {
            pMB->best_Inter_residual[j*MB_SIZE+i] = pY[((mby<<4)+j)*width+((mbx<<4)+i)] - pMB->Inter_cmp[j*MB_SIZE+i];
        }
    }

    for (j=0; j<MB_SIZE/2; j++) {
        for (i=0; i<MB_SIZE/2; i++) {
            pMB->best_Inter_cb_residual[j*MB_SIZE/2+i] = pCb[((mby<<3)+j)*width/2+((mbx<<3)+i)] - pMB->Inter_cb_cmp[j*MB_SIZE/2+i];
            pMB->best_Inter_cr_residual[j*MB_SIZE/2+i] = pCr[((mby<<3)+j)*width/2+((mbx<<3)+i)] - pMB->Inter_cr_cmp[j*MB_SIZE/2+i];
        }
    }

    return MMES_NO_ERROR;
}


xint
set_ipcm_info( slice_obj *pSlice, frame_obj* pDst, frame_obj* pSrc, xint width )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Oct/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Set information for IPCM mode                               */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*   *pSrc -> [I] pointer to the source yuv frame                */
/*   *pDst -> [I] pointer to the reconstructed yuv frame         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB = &(pSlice->cmb);
    xint    x_coord;
    xint    frame_x_16_offset, frame_x_4_offset, nx_mb, chroma_mb_size ;

    chroma_mb_size = MB_SIZE / 2;
    nx_mb = width >> 4;
    frame_x_16_offset = pMB->id % nx_mb;
    frame_x_4_offset = frame_x_16_offset<<2;

    /* copy yuv data */
    dup_mb( pDst, pSrc, pSlice->cmb.id, width );

    pSlice->upleft_y[0] = pSlice->top_y[ (frame_x_16_offset+1)*MB_SIZE-1 ];
    pSlice->upleft_cb = pSlice->top_cb[frame_x_16_offset*chroma_mb_size + chroma_mb_size-1 ];
    pSlice->upleft_cr = pSlice->top_cr[frame_x_16_offset*chroma_mb_size + chroma_mb_size-1 ];

    /* for further intra prediction */
    set_intra_luma_prediction(pSlice, pDst, width);
    set_intra_chroma_prediction(pSlice, pDst, width);

    for (x_coord = 0; x_coord < (MB_SIZE/B_SIZE); x_coord++)
    {
        pSlice->top_mode[frame_x_16_offset * (MB_SIZE/B_SIZE)+x_coord] = INTRA_4x4_DC;
        pSlice->left_mode[x_coord] = INTRA_4x4_DC;
    }

    /* for loop filter */
    pMB->QP = 0;

    return MMES_NO_ERROR;
}

xint
set_intra_chroma_prediction(slice_obj *pSlice, frame_obj *pCurr_frame, xint width)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Oct/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Set parameters for further intra prediction.                */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice  -> [I/O] pointer to the session parameters         */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB = &(pSlice->cmb);
    xint    frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint    nx_mb;
    xint    x_coord, y_coord;
    uint8  *cb, *cr;

    nx_mb = width >>4;
    frame_x_16_offset = pMB->id % nx_mb;
    frame_y_16_offset = pMB->id / nx_mb;
    chroma_frame_width = width / 2;
    chroma_mb_size = MB_SIZE / 2;

    // top_cb
    // move to bottom of current MB
    cb = pCurr_frame->cb + (frame_y_16_offset+1) * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size - chroma_frame_width;
    for (x_coord = 0; x_coord < chroma_mb_size; x_coord++)
    {
        pSlice->top_cb[frame_x_16_offset * chroma_mb_size + x_coord] = *cb;
        cb++;
    }

    // left_cb
    // move to rightest column of current MB
    cb = pCurr_frame->cb + frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        (frame_x_16_offset+1) * chroma_mb_size - 1;
    for (y_coord = 0; y_coord < chroma_mb_size; y_coord++)
    {
        pSlice->left_cb[y_coord] = *(cb);
        cb+= chroma_frame_width;
    }

    // top_cr
    // move to bottom of current MB
    cr = pCurr_frame->cr + (frame_y_16_offset+1) * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size - chroma_frame_width;
    for (x_coord = 0; x_coord < chroma_mb_size; x_coord++)
    {
        pSlice->top_cr[frame_x_16_offset * chroma_mb_size + x_coord] = *cr;
        cr++;
    }

    // left_cr
    // move to rightest column of current MB
    cr = pCurr_frame->cr + frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        (frame_x_16_offset+1) * chroma_mb_size - 1;
    for (y_coord = 0; y_coord < chroma_mb_size; y_coord++)
    {
        pSlice->left_cr[y_coord] = *(cr);
        cr+= chroma_frame_width;
    }
    return MMES_NO_ERROR;
}
