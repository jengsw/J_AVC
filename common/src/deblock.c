/* ///////////////////////////////////////////////////////////// */
/*   File: deblock.c                                             */
/*   Author: JM Authors, extracted by Jerry Peng from JM9.2      */
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

#include "../../common/inc/deblock.h"
#include "../../common/inc/misc_util.h"
#include <string.h>

extern const uint8 QP_SCALE_CR[52];
extern xint db_z[4][4];

uint8  ALPHA_TABLE[52]  = {0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,4,4,5,6,  7,8,9,10,12,13,15,17,  20,22,25,28,32,36,40,45,  50,56,63,71,80,90,101,113,  127,144,162,182,203,226,255,255} ;
uint8  BETA_TABLE[52]  = {0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,2,2,2,3,  3,3,3, 4, 4, 4, 6, 6,   7, 7, 8, 8, 9, 9,10,10,  11,11,12,12,13,13, 14, 14,   15, 15, 16, 16, 17, 17, 18, 18} ;
uint8  CLIP_TAB[52][5]  =
{
  { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 1, 1, 1, 1},
  { 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 2, 3, 3},
  { 0, 1, 2, 3, 3},{ 0, 2, 2, 3, 3},{ 0, 2, 2, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 3, 3, 5, 5},{ 0, 3, 4, 6, 6},{ 0, 3, 4, 6, 6},
  { 0, 4, 5, 7, 7},{ 0, 4, 5, 8, 8},{ 0, 4, 6, 9, 9},{ 0, 5, 7,10,10},{ 0, 6, 8,11,11},{ 0, 6, 8,13,13},{ 0, 7,10,14,14},{ 0, 8,11,16,16},
  { 0, 9,12,18,18},{ 0,10,13,20,20},{ 0,11,15,23,23},{ 0,13,17,25,25}
} ;

const xint blk4x4_no[2][4][4] =
{
    {
        {0, 4, 8, 12},{1, 5, 9, 13}, {2, 6, 10, 14}, {3, 7, 11, 15}
    }, 
    {
        {0, 1, 2, 3} ,{4, 5, 6, 7} , {8, 9, 10, 11}, {12, 13, 14, 15}
    }
};

xint GetStrength(slice_obj *pSlice, int width, uint8 strength[4], xint dir, xint edge, xint enable)
{
    mb_obj *pMB;
    xint	nx_mb;
    xint    frame_x_16_offset, frame_x_4_offset;
    xint    idx;
    xint    mvPx, mvPy, mvQx, mvQy;
    xint    cbp_blkP, cbp_blkQ;
    xint    mb_mode_blkP, mb_mode_blkQ;
    xint    ref_idxP, ref_idxQ;

    nx_mb = width / MB_SIZE;
    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % nx_mb;

    cbp_blkP     = (dir) ? pSlice->top_cbp_blk[frame_x_16_offset] : pSlice->left_cbp_blk;
    cbp_blkQ     = pMB->cbp_blk;

    mb_mode_blkP = (dir) ? pSlice->top_mb_mode[frame_x_16_offset] : pSlice->left_mb_mode;
    mb_mode_blkQ = pMB->best_MB_mode;

    for(idx = 0 ; idx < 4 ; idx++)
    {
        frame_x_4_offset  = frame_x_16_offset * 4 + idx;

        ref_idxP = (dir) ? pSlice->top_ref_idx[frame_x_4_offset>>1] : pSlice->left_ref_idx[idx>>1];
        ref_idxQ = (dir) ? pMB->ref_idx[((edge>>1)<<1)+(idx>>1)] : pMB->ref_idx[((idx>>1)<<1)+(edge>>1)];

        mvPx = (dir) ? pSlice->top_mvx[frame_x_4_offset] : pSlice->left_mvx[idx];
        mvPy = (dir) ? pSlice->top_mvy[frame_x_4_offset] : pSlice->left_mvy[idx];
        mvQx = (dir) ? pMB->mv.x[edge*4+idx] : pMB->mv.x[idx*4+edge];
        mvQy = (dir) ? pMB->mv.y[edge*4+idx] : pMB->mv.y[idx*4+edge];

        if(enable)
        {
            if(mb_mode_blkP == 0 || mb_mode_blkQ == 0) /* intra mode */
            {
                strength[idx] = edge ? 3 : 4;
            }
            else                        /* inter mode */
            {
                if(pMB->best_I16x16_mode != I_PCM)
                {
                    if( (cbp_blkP &  (1 << blk4x4_no[dir][(edge+3)%4][idx]) ) != 0 || 
                        (cbp_blkQ &  (1 << blk4x4_no[dir][edge][idx]      ) ) != 0 )
                        strength[idx] = 2;
                    else
                    {
                        if(ref_idxP != ref_idxQ)
                            strength[idx] = 1;
                        else
                            strength[idx] = (ABS(mvPx - mvQx) >= 4 || ABS(mvPy - mvQy) >= 4);
                    }
                }
            }
        }
        
        if(dir)
        {
            /* update top information */
            pSlice->top_cbp_blk[frame_x_16_offset]   = cbp_blkQ;
            pSlice->top_mb_mode[frame_x_16_offset]   = mb_mode_blkQ;
            pSlice->top_mvx[frame_x_4_offset] = mvQx;
            pSlice->top_mvy[frame_x_4_offset] = mvQy;

            if(idx%2) pSlice->top_ref_idx[frame_x_4_offset>>1] = ref_idxQ;
        }
        else
        {
            /* update left information */
            pSlice->left_cbp_blk         = cbp_blkQ;
            pSlice->left_mb_mode         = mb_mode_blkQ;
            pSlice->left_mvx[idx]        = mvQx;
            pSlice->left_mvy[idx]        = mvQy;

            if(idx%2) pSlice->left_ref_idx[idx>>1] = ref_idxQ;
        }
    }

    return MMES_NO_ERROR;
}

/* filters one edge of 16 (luma) or 8 (chroma) pel */
xint EdgeLoop(uint8 *SrcPtr, uint8 strength[4], xint indexA, xint indexB, xint dir, xint width, xint yuv)
{
    xint pel, ap, aq, PtrInc, BS;
    xint inc, inc1, inc2, inc3;
    xint Alpha, Beta, AbsDelta, Delta;
    uint8 *ClipTab;
    xint  L0, L1, L2, L3, R0, R1, R2, R3, RL0;
    xint  Tc0, Tc, small_gap, dif;


    PtrInc  = (dir) ? 1 : width;
    inc     = (dir) ? width : 1;
    inc1    = inc<<1;
    inc2    = inc + inc1;
    inc3    = inc<<2;

    Alpha   = ALPHA_TABLE[indexA];
    Beta    = BETA_TABLE[indexB];
    ClipTab = CLIP_TAB[indexA];

    for(pel=0; pel<16; pel++)
    {
	    BS = strength[pel>>2];
        if(BS)
        {
            L0 = SrcPtr[-inc];
            L1 = SrcPtr[-inc1];
            L2 = SrcPtr[-inc2];
            L3 = SrcPtr[-inc3];

            R0 = SrcPtr[0];
            R1 = SrcPtr[inc];
            R2 = SrcPtr[inc1];
            R3 = SrcPtr[inc2];

            Delta = R0 - L0;
            AbsDelta = ABS(Delta);

            if(AbsDelta < Alpha)
            {
                Tc0 = ClipTab[BS];

                if( ((ABS(R0 - R1) - Beta) & (ABS(L0-L1) - Beta) ) < 0)
                {
                    if(!yuv)
                    {
                        // Luma
                        aq = (ABS(R0 - R2) - Beta) < 0;
                        ap = (ABS(L0 - L2) - Beta) < 0;
                    }
                    else
                    {
                        L2 = 0;
                        R2 = 0;
                        aq = 0;
                        ap = 0;
                    }

                    RL0 = L0 + R0;

                    if(BS == 4) // INTRA strong filtering
                    {
                        if(yuv)     // Chroma
                        {
                            SrcPtr[0   ] = ((R1 << 1) + R0 + L1 + 2) >> 2; 
                            SrcPtr[-inc] = ((L1 << 1) + L0 + R1 + 2) >> 2;       
                        }
                        else        // luma
                        {
                            small_gap = (AbsDelta < ( (Alpha>>2) + 2));
                            aq &= small_gap;
                            ap &= small_gap;

                            SrcPtr[ 0   ] = aq ? ( L1 + ((R1 + RL0) << 1) +  R2 + 4) >> 3 : ((R1 << 1) + R0 + L1 + 2) >> 2 ;
                            SrcPtr[ -inc] = ap ? ( R1 + ((L1 + RL0) << 1) +  L2 + 4) >> 3 : ((L1 << 1) + L0 + R1 + 2) >> 2 ;

                            SrcPtr[ inc ] = aq  ? ( R2 + R0 + R1 + L0 + 2) >> 2 : R1;
                            SrcPtr[-inc1] = ap  ? ( L2 + L1 + L0 + R0 + 2) >> 2 : L1;

                            SrcPtr[ inc1] = aq ? (((R3 + R2) <<1) + R2 + R1 + RL0 + 4) >> 3 : R2;
                            SrcPtr[-inc2] = ap ? (((L3 + L2) <<1) + L2 + L1 + RL0 + 4) >> 3 : L2;
                            
                        }
                    }
                    else
                    {
                        Tc            = (yuv) ? (Tc0 + 1) : (Tc0 + aq + ap);
                        dif           = CLIP3(-Tc, Tc, (( Delta<<2 ) + ( L1 - R1 ) + 4) >> 3);
                        SrcPtr[ -inc] = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, L0 + dif);
                        SrcPtr[ 0   ] = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, R0 - dif);
                        
                        if(!yuv)
                        {
                            if(ap)
                                SrcPtr[-inc1] += CLIP3( -Tc0, Tc0, ( L2 + ((RL0 + 1) >> 1) - (L1<<1)) >> 1 );
                            if(aq)
                                SrcPtr[ inc ] += CLIP3( -Tc0, Tc0, ( R2 + ((RL0 + 1) >> 1) - (R1<<1)) >> 1 ) ;
                        }
                    }
                }
            }

            SrcPtr += PtrInc;       // increment to next set of pixel
            pel    += yuv;
        }
        else
        {
            SrcPtr += PtrInc<<(2-yuv);
            pel += 3;
        }

    }


    return MMES_NO_ERROR;
}

xint db_display_mb( slice_obj *pSlice, frame_obj *pCurr_frame, xint width)
{
    mb_obj    *pMB;
	xint      frame_x_16_offset, frame_y_16_offset;
    xint      i, j;
	xint      chroma_mb_size, chroma_frame_width;
    xint      nx_mb = width / MB_SIZE;
	uint8     *y, *cb, *cr;

    pMB = &(pSlice->cmb);
    
    frame_x_16_offset = pMB->id % nx_mb;
    frame_y_16_offset = pMB->id / nx_mb;

	chroma_frame_width = width / 2;
	chroma_mb_size = MB_SIZE / 2;

    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width + frame_x_16_offset * MB_SIZE;
	printf( "Y\n");
	for( j=0; j<MB_SIZE; j++ )
	{
		for( i=0; i<MB_SIZE; i++, y++ )
		{
            printf( " %02X", *y);
            if((i+1)%4==0) printf(" ");
		}
        if((j+1)%4==0) printf("\n");
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
            printf( " %02X", *cb);
            if((i+1)%4==0) printf(" ");
		}
        if((j+1)%4==0) printf("\n");
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
            printf( " %02X", *cr);
            if((i+1)%4==0) printf(" ");
		}
        if((j+1)%4==0) printf("\n");
		printf("\n");
		cr += (chroma_frame_width - chroma_mb_size);
	}

	return MMES_NO_ERROR;
}


xint deblock(slice_obj *pSlice, int width, frame_obj *recf)
/* --------------------------------------------------------------- */
/*   Author: Jerry Peng                                            */
/*   Date  : Apr/19/2005                                           */
/* --------------------------------------------------------------- */
/*   COMMENTS                                                      */
/*   Deblocking filter for one macroblock                          */
/*                                                                 */
/*   RETURN                                                        */
/*   Status code.                                                  */
/*                                                                 */
/*   PARAMETERS:                                                   */
/*   *pSlice -> [I/O] pointer to the slice session parameters      */
/* --------------------------------------------------------------- */
/*   MODIFICATIONS                                                 */
/* --------------------------------------------------------------- */
{
    mb_obj *pMB;
    frame_obj  *pCurr_frame;
    xint    EdgeCondition;
    xint    dir, edge;
    xint    curr_QP, prev_QP, aver_QP, aver_chroma_qp;
    xint    frame_x_16_offset, frame_y_16_offset;
    xint    filterLeftMbEdgeFlag, filterTopEdgeMbFlag;
    uint8   BS[4];
    uint8   *SrcY, *SrcCb, *SrcCr;
    xint    chroma_frame_width, chroma_mb_size;
    xint    indexA, indexB, FilterOffsetA, FilterOffsetB;
    xint    nx_mb;
    
    nx_mb = width / MB_SIZE;
    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % nx_mb;
    frame_y_16_offset = pMB->id / nx_mb;
    curr_QP = pMB->QP;

    pCurr_frame = recf;

    // for YCbCR420
    chroma_frame_width = width>>1;
    chroma_mb_size = MB_SIZE>>1;

    SrcY  = pCurr_frame->y  + frame_y_16_offset*MB_SIZE*width + frame_x_16_offset*MB_SIZE;
    SrcCb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    SrcCr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;

    filterLeftMbEdgeFlag = (frame_x_16_offset != 0);
    filterTopEdgeMbFlag  = (frame_y_16_offset != 0);

    FilterOffsetA = pSlice->lf_alpha_c0_offset_div2<<1;
    FilterOffsetB = pSlice->lf_bata_offset_div2<<1;

    if(pSlice->lf_disable_idc == 2)
    {
        // don't filter at slice boundary
        filterLeftMbEdgeFlag = pSlice->deblock_left_valid;
        filterTopEdgeMbFlag  = pSlice->deblock_top_valid[frame_x_16_offset];
    }

    if(pSlice->lf_disable_idc != 1)
    {
        for(dir=0; dir<2; dir++)            // direction of edge 0: vertical, 1: horizontal
        {
            // can not filter at picture boundaries
            EdgeCondition = (!dir && filterLeftMbEdgeFlag) || (dir && filterTopEdgeMbFlag);

            for(edge=0; edge<4; edge++)
            {
                // Strength for 4 blks in 1 stripe
                GetStrength(pSlice, width, BS, dir, edge, edge || EdgeCondition);      
                if(edge || EdgeCondition)
                {
                    prev_QP = (edge) ? curr_QP : ((dir) ? pSlice->top_qp[frame_x_16_offset] : pSlice->left_qp);
                    aver_QP = (curr_QP + prev_QP + 1)>>1;
                    indexA = CLIP3(0, 51, aver_QP+FilterOffsetA);
                    indexB = CLIP3(0, 51, aver_QP+FilterOffsetB);

                    if( *((uint32 *)BS) )
                    {
                        // Loop Filter
                        EdgeLoop(SrcY + ((edge<<2) * ((dir) ? width : 1)), BS, indexA, indexB, dir, width, 0);

                        if(!(edge&1))
                        {
                            aver_chroma_qp = (QP_SCALE_CR[curr_QP+pSlice->chroma_qp_index_offset] + QP_SCALE_CR[prev_QP+pSlice->chroma_qp_index_offset] + 1) >> 1;
                            indexA = CLIP3(0, 51, aver_chroma_qp+FilterOffsetA);
                            indexB = CLIP3(0, 51, aver_chroma_qp+FilterOffsetB);

                            EdgeLoop(SrcCb + ((edge<<1) * ((dir) ? chroma_frame_width : 1)), BS, indexA, indexB, dir, chroma_frame_width, 1);

                            EdgeLoop(SrcCr + ((edge<<1) * ((dir) ? chroma_frame_width : 1)), BS, indexA, indexB, dir, chroma_frame_width, 1);
                        }
                    }
                }
            }
        }
    }


//    db_display_mb(pSlice, pCurr_frame, width);

    // update the QP and deblock slide info
    pSlice->deblock_top_valid[frame_x_16_offset] = MMES_VALID;
    if(frame_x_16_offset == (nx_mb - 1))
        pSlice->deblock_left_valid = MMES_INVALID;
    else
        pSlice->deblock_left_valid = MMES_VALID;
    
    pSlice->top_qp[frame_x_16_offset]  = curr_QP;
    pSlice->left_qp = curr_QP;

    return MMES_NO_ERROR;   
}

