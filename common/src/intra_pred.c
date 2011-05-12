/* ///////////////////////////////////////////////////////////// */
/*   File: intra_pred.c                                          */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 intra prediction module.         */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/*   Multimedia Embedded Systems Lab.                            */
/*   Department of Computer Science and Information engineering  */
/*   National Chiao Tung University, Hsinchu 300, Taiwan         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ///////////////////////////////////////////////////////////// */

#include <string.h>
#include "../../common/inc/intra_pred.h"

xint enc_intra_16x16_prediction(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra luma 16x16 prediction residuals.       */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    switch (pEnc->curr_slice->cmb.best_I16x16_mode)
    {
    case INTRA_16x16_DC:
        enc_intra_16x16_DC_pred(pEnc);
        break;
    case INTRA_16x16_H:
        enc_intra_16x16_H_pred(pEnc);
        break;
    case INTRA_16x16_V:
        enc_intra_16x16_V_pred(pEnc);
        break;
    case INTRA_16x16_PL:
        enc_intra_16x16_PL_pred(pEnc);
        break;
    }

    return 0;
}

xint enc_intra_4x4_prediction(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra luma 4x4 prediction residuals.         */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    switch (pEnc->curr_slice->cmb.best_I4x4_mode[block])
    {
    case INTRA_4x4_V:
        enc_intra_4x4_V_pred(pEnc, block);
        break;
    case INTRA_4x4_H:
        enc_intra_4x4_H_pred(pEnc, block);
        break;
    case INTRA_4x4_DC:
        enc_intra_4x4_DC_pred(pEnc, block);
        break;
    case INTRA_4x4_DDL:
        enc_intra_4x4_DDL_pred(pEnc, block);
        break;
    case INTRA_4x4_DDR:
        enc_intra_4x4_DDR_pred(pEnc, block);
        break;
    case INTRA_4x4_VR:
        enc_intra_4x4_VR_pred(pEnc, block);
        break;
    case INTRA_4x4_HD:
        enc_intra_4x4_HD_pred(pEnc, block);
        break;
    case INTRA_4x4_VL:
        enc_intra_4x4_VL_pred(pEnc, block);
        break;
    case INTRA_4x4_HU:
        enc_intra_4x4_HU_pred(pEnc, block);
        break;
    }

    return 0;
}

xint
enc_intra_chroma_prediction(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma prediction residuals.           */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    switch (pEnc->curr_slice->cmb.best_chroma_mode)
    {
    case INTRA_CHROMA_DC:
        enc_intra_chroma_DC_pred(pEnc);
        break;
    case INTRA_CHROMA_H:
        enc_intra_chroma_H_pred(pEnc);
        break;
    case INTRA_CHROMA_V:
        enc_intra_chroma_V_pred(pEnc);
        break;
    case INTRA_CHROMA_PL:
        enc_intra_chroma_PL_pred(pEnc);
        break;
    }

    return 0;
}

xint
enc_intra_16x16_DC_pred(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 16x16 DC predition mode residuals.     */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint    frame_x_16_offset, frame_y_16_offset;
    int16   s1, s2, s, idx;
    uint8   predictor;
    xint    x_coord, y_coord;
    uint8  *tmp_predictor_y;
    uint8   top_valid;
    uint8   left_valid;
    xint    width, height;
    uint8  *y;
	uint8   predictor_y[ MB_SIZE ][ MB_SIZE ];
	
    pCurr_frame = pEnc->curf;
    width = pEnc->width;
    height = pEnc->height;

    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

//    tmp_predictor_y = pEnc->scratch;
	tmp_predictor_y = &predictor_y[0][0];

    top_valid = pSlice->top_valid[frame_x_16_offset * (MB_SIZE/B_SIZE)];
    left_valid = pSlice->left_valid[0];

    // perform luma intra16x16 DC prediction
    s = s1 = s2 = 0;
    if (top_valid == MMES_VALID)
    {
        for (idx = 0; idx < MB_SIZE; idx++)
        {
            s1 += pSlice->top_y[frame_x_16_offset * MB_SIZE + idx];
        }
    }
    if (left_valid == MMES_VALID)
    {
        for (idx = 0; idx < MB_SIZE; idx++)
        {
            s2 += pSlice->left_y[idx];
        }
    }

    if ((top_valid == MMES_VALID) && (left_valid == MMES_VALID))
    {
        s = (s1 + s2 + 16) / (2 * MB_SIZE);
    }
    else if (top_valid == MMES_VALID)
    {
        s = (s1 + 8) / MB_SIZE;
    }
    else if (left_valid)
    {
        s = (s2 + 8) / MB_SIZE;
    }
    else
    {
        s = 128;
    }

    predictor = (uint8) CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s);

    memset(tmp_predictor_y, predictor, MB_SIZE * MB_SIZE);

    //calculate residual and save predictor
    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
        frame_x_16_offset * MB_SIZE;
    idx = 0;
    for (y_coord = 0; y_coord < MB_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < MB_SIZE; x_coord++)
        {
            pMB->best_I16x16_residual[idx] = (*y) - tmp_predictor_y[idx];
            if( pEnc->ICompensate_enable )	*y = tmp_predictor_y[idx];
            y++;
            idx++;
        }
        y += (width - MB_SIZE);
    }

    return MMES_NO_ERROR;
}

xint
enc_intra_16x16_H_pred(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng             */
/*   Date  : Feb/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 16x16 horizontal pred. mode residuals. */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint x_coord, y_coord;
    xint width;
    uint8 *y;


    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    width = pEnc->width;

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

    //calculate the residual and save the predictor
    y = pCurr_frame->y + frame_y_16_offset*MB_SIZE*width + frame_x_16_offset*MB_SIZE;
    for(y_coord=0; y_coord<MB_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<MB_SIZE; x_coord++)
        {
            pMB->best_I16x16_residual[x_coord+y_coord*MB_SIZE] = (*y) - pSlice->left_y[y_coord];

            if( pEnc->ICompensate_enable )	*y = pSlice->left_y[y_coord];
            y++;
        }
        y += (width - MB_SIZE);
    }
    
    return MMES_NO_ERROR;
}

xint
enc_intra_16x16_V_pred(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Feb/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 16x16 vertical pred. mode residuals.   */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint x_coord, y_coord;
    xint width;
    uint8 *y;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    width = pEnc->width;

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

    //calculate the residual and save the predictor
    y = pCurr_frame->y + frame_y_16_offset*MB_SIZE*width + frame_x_16_offset*MB_SIZE;
    for(y_coord=0; y_coord<MB_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<MB_SIZE; x_coord++)
        {
            pMB->best_I16x16_residual[x_coord+y_coord*MB_SIZE] = (*y) - pSlice->top_y[frame_x_16_offset*MB_SIZE+x_coord];

            if( pEnc->ICompensate_enable )	*y = pSlice->top_y[frame_x_16_offset*MB_SIZE+x_coord];
            y++;
        }
        y += (width - MB_SIZE);
    }

    return MMES_NO_ERROR;
}

xint
enc_intra_16x16_PL_pred(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Feb/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 16x16 plane predition mode residuals.  */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jun/01/2005 Jerry Peng, Fix problem in accessing         */
/*               neighboring pixels                              */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint x_coord, y_coord, idx;
    xint width;
    int16 iH, iV, ia, ib, ic, tmp_value;
    uint8 *top_value, *left_value, *y, top[MB_SIZE+1], left[MB_SIZE+1];

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);
    width = pEnc->width;
    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

	top[0] = pSlice->upleft_y[0];
	left[0] = pSlice->upleft_y[0];

	for(idx=0; idx<MB_SIZE; idx++)
	{
		top[idx+1] = pSlice->top_y[frame_x_16_offset*MB_SIZE+idx];
		left[idx+1] = pSlice->left_y[idx];
	}

	top_value = &top[1];
	left_value = &left[1];

    iH=0;
    for(idx=0; idx<8; idx++)
    {
        iH += (idx+1)*(top_value[8+idx] - top_value[6-idx]);
    }

    iV=0;
    for(idx=0; idx<7; idx++)
    {
        iV += (idx+1)*(left_value[8+idx] - left_value[6-idx]);
    }
    iV += 8*(left_value[15] - top_value[-1]);

    ia = 16*(left_value[15]+top_value[15]);
    ib = (5*iH+32)>>6;
    ic = (5*iV+32)>>6;

    // calculate the residual and save the predictor
    y = pCurr_frame->y + frame_y_16_offset*MB_SIZE*width + frame_x_16_offset*MB_SIZE;
    for(y_coord=0; y_coord<MB_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<MB_SIZE; x_coord++)
        {
            tmp_value = (ia+ib*(x_coord-7)+ic*(y_coord-7)+16)>>5;
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            pMB->best_I16x16_residual[x_coord+y_coord*MB_SIZE] = (*y) - tmp_value;

            if( pEnc->ICompensate_enable )	*y = (uint8) tmp_value;
            y++;
        }
        y += (width-MB_SIZE);
    }    
    return MMES_NO_ERROR;
}

xint
enc_intra_4x4_V_pred(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/24/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 vertical predition mode residuals. */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Jerry Peng, Add 'block' to the parameters       */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..3 are marked as "available for Intra_4x4 prediction */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *up_value;
    xint x_coord, y_coord;
    uint8 *block_start;
    int16 *residual_start;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;

    frame_y_16_offset = pMB->id / pEnc->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pEnc->width;

    up_value = &(pSlice->top_y[frame_x_4_offset*4]);

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            residual_start[x_coord+y_coord*MB_SIZE] = (*block_start) - up_value[x_coord];
            if( pEnc->ICompensate_enable )	*block_start = up_value[x_coord];
            block_start++;
        }
        block_start += (frame_width - B_SIZE);

    }
    
    return MMES_NO_ERROR;
}

xint
enc_intra_4x4_H_pred(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/25/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 horizontal pred. mode residuals.   */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Jerry Peng, Add 'block' to the parameters    */
/* ------------------------------------------------------------- */
{
    /* used when p[-1,y] with y=0..3 are marked as "available for Intra_4x4 prediction */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *left_value;
    xint x_coord, y_coord;
    uint8 *block_start;
    int16 *residual_start;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;

    frame_y_16_offset = pMB->id / pEnc->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pEnc->width;

    left_value = &(pSlice->left_y[mb_y_4_offset*4]);

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            residual_start[x_coord+y_coord*MB_SIZE] = (*block_start) - left_value[y_coord];
            if( pEnc->ICompensate_enable )	*block_start = left_value[y_coord];
            block_start++;
        }
        block_start += (frame_width - B_SIZE);
    }
    return MMES_NO_ERROR;
}

xint
enc_intra_4x4_DC_pred(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/24/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 DC predition mode residuals.       */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Jerry Peng, Add 'block' to the parameters    */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    uint8 up_available, left_available;
    uint8 *up_value, *left_value;
    int16 pred_value;
    xint frame_width;
    uint8 *block_start;
    int16 *residual_start;
    xint x_coord, y_coord;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pEnc->width;

    // check available
    up_available = pSlice->top_valid[frame_x_4_offset];
    left_available = pSlice->left_valid[mb_y_4_offset];

    up_value = &(pSlice->top_y[frame_x_4_offset*4]);
    left_value = &(pSlice->left_y[mb_y_4_offset*4]);

    if(up_available && left_available)
    {
        /* 8-47 at w6359 */
        pred_value = up_value[0] + up_value[1] + up_value[2] + up_value[3] +
                     left_value[0] + left_value[1] + left_value[2] + left_value[3];
        pred_value = (pred_value + 4)>>3;
    }
    else if(left_available)
    {
        /* 8-48 at w6359 */
        pred_value = left_value[0] + left_value[1] + left_value[2] + left_value[3];
        pred_value = (pred_value + 2)>>2;
    }
    else if(up_available)
    {
        /* 8-49 at w6359 */
        pred_value = up_value[0] + up_value[1] + up_value[2] + up_value[3];
        pred_value = (pred_value + 2)>>2;
    }
    else 
    {
        /* 8-50 at w6359 */
        pred_value = 128; 
    }

    pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            residual_start[x_coord+y_coord*MB_SIZE] = (*block_start) - pred_value;
            if( pEnc->ICompensate_enable )	*block_start = (uint8) pred_value;
            block_start ++;
        }
        block_start += (frame_width - B_SIZE);
    }

    return MMES_NO_ERROR;
}

xint
enc_intra_4x4_DDL_pred(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/25/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 diag-down-left mode residuals.     */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..7 are marked as "available for Intra_4x4 prediction */
    /* ==> p[x,-1] with x=0..3 will satisfy the above constraint */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 up_value[8];
    uint8 up_right_available;
    xint x_coord, y_coord;
    uint8 *block_start;
    int16 *residual_start;
    int16 pred_value;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;

    frame_y_16_offset = pMB->id / pEnc->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pEnc->width;

    if(frame_x_4_offset == (frame_width/B_SIZE - 1))
        up_right_available = MMES_INVALID;
    else
        up_right_available = pSlice->top_valid[frame_x_4_offset + 1];

    if( (block==3) || (block==11) )
		up_right_available = MMES_INVALID;
	else if( (block==7) || (block==13) || (block==15) )
		up_right_available = MMES_INVALID;

    if(up_right_available)
        memcpy(up_value, &(pSlice->top_y[frame_x_4_offset*4]), 2*B_SIZE*sizeof(uint8));
    else
    {
        /* p104 at w6359 */
        memcpy(up_value, &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));
        memset(&(up_value[4]), up_value[3], B_SIZE*sizeof(uint8));
    }

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            if(x_coord==3 && y_coord==3)
                pred_value = (up_value[6] + up_value[7]*3 + 2)>>2;
            else
                pred_value = (up_value[x_coord+y_coord] + 2*up_value[x_coord+y_coord+1] +
                              up_value[x_coord+y_coord+2] + 2)>>2;

            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);

            residual_start[x_coord+y_coord*MB_SIZE] = (*block_start) - pred_value;
            if( pEnc->ICompensate_enable )	*block_start = (uint8) pred_value;
            block_start ++;
        }
        block_start += (frame_width - B_SIZE);
    }

    return MMES_NO_ERROR;
}

xint
enc_intra_4x4_DDR_pred(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 diag-down-right mode residuals.    */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Jerry Peng, Add 'block' to the parameters    */
/*   May/26/2005 Jerry Peng, Fix bugs in accessing neighboring*/
/*               pixels                                          */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..3 and p[-1,y] with y=-1..3 are marked as "available for Intra_4x4" */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *block_start;
    int16 *residual_start;
    xint x_coord, y_coord;
    uint8 left_value[5];
	uint8 up_value[5];
    uint8 *up, *left;
    int16 pred_value;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    
    frame_y_16_offset = pMB->id / pEnc->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pEnc->width;

    /* p[-1,-1] */
    left_value[0] = pSlice->upleft_y[ mb_y_4_offset ];
	up_value[0] = pSlice->upleft_y[ mb_y_4_offset ];

    /* p[-1,y] with y=0..3 */
    memcpy(&(left_value[1]), &(pSlice->left_y[mb_y_4_offset*4]), B_SIZE*sizeof(uint8));
	/* p[x, -1] with x=0..3 */
	memcpy(&(up_value[1]), &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));    
    
    left = &(left_value[1]);
	up = &(up_value[1]);

    // calculate residual and save prediction pixels */
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            if(x_coord>y_coord)         /* 8-53 at w6359 */
                pred_value = up[x_coord-y_coord-2] + 2*up[x_coord-y_coord-1] + up[x_coord-y_coord];
            else if(x_coord<y_coord)    /* 8-54 at w6359 */
                pred_value = left[y_coord-x_coord-2] + 2*left[y_coord-x_coord-1] + left[y_coord-x_coord];
            else                        /* 8-55 at w6359 */
                pred_value = up[0] + 2*left[-1] + left[0];

            pred_value = (pred_value+2)>>2;

            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);

            residual_start[x_coord + y_coord*MB_SIZE] = (*block_start) - pred_value;
            if( pEnc->ICompensate_enable )	*block_start = (uint8)pred_value;
            block_start++;
        }
        block_start += (frame_width - B_SIZE);
    }
    
    return MMES_NO_ERROR;
}

xint
enc_intra_4x4_VR_pred(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 vert.-right pred. mode residuals.  */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..3 and p[-1,y] with y=-1..3 are marked as "available for Intra_4x4" */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *block_start;
    int16 *residual_start;
    xint x_coord, y_coord;
    uint8 left_value[5];
	uint8 up_value[5];
    uint8 *up, *left;
    int16 pred_value;
    xint zVR;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    
    frame_y_16_offset = pMB->id / pEnc->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pEnc->width;

    /* p[-1,-1] */
    left_value[0] = pSlice->upleft_y[ mb_y_4_offset ];
	up_value[0] = pSlice->upleft_y[ mb_y_4_offset ];

    /* p[-1,y] with y=0..3 */
	memcpy(&(left_value[1]), &(pSlice->left_y[mb_y_4_offset*4]), B_SIZE*sizeof(uint8));
	/* p[x, -1] with x=0..3 */
	memcpy(&(up_value[1]), &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));
        
    left = &(left_value[1]);
	up = &(up_value[1]);

    // calculate residual and save prediction pixels */
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            zVR = 2*x_coord - y_coord;

            switch (zVR)
            {
            case 0:
            case 2: 
            case 4:
            case 6:
                /* 8-56 at w6359 */
                pred_value = up[x_coord - (y_coord>>1) -1] + up[x_coord - (y_coord>>1)];
                pred_value = (pred_value+1)>>1;
                break;
            case 1: 
            case 3:
            case 5:
                /* 8-57 at w6359 */
                pred_value = up[x_coord - (y_coord>>1) -2] + 2*up[x_coord - (y_coord>>1) -1] + up[x_coord - (y_coord>>1)];
                pred_value = (pred_value+2)>>2;
                break;
            case -1:
                /* 8-58 at w6359 */
                pred_value = up[0] + 2*up[-1] + left[0];
                pred_value = (pred_value+2)>>2;
                break;
            case -2:
            case -3:
                /* 8-59 at w6359 */
                pred_value = left[y_coord-1] + 2*left[y_coord-2] + left[y_coord-3];
                pred_value = (pred_value+2)>>2;
            }

            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);

            residual_start[x_coord + y_coord*MB_SIZE] = (*block_start) - pred_value;
            if( pEnc->ICompensate_enable )	*block_start = (uint8)pred_value;
            block_start++;
        }
        block_start += (frame_width - B_SIZE);
    }
    
    return MMES_NO_ERROR;
}

xint
enc_intra_4x4_HD_pred(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 horz.-down pred. mode residuals.   */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..3 and p[-1,y] with y=-1..3 are marked as "available for Intra_4x4" */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *block_start;
    int16 *residual_start;
    xint x_coord, y_coord;
    uint8 left_value[5];
	uint8 up_value[5];
    uint8 *up, *left;
    int16 pred_value;
    xint zHD;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    
    frame_y_16_offset = pMB->id / pEnc->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pEnc->width;

    /* p[-1,-1] */    
    left_value[0] = pSlice->upleft_y[ mb_y_4_offset ];
	up_value[0] = pSlice->upleft_y[ mb_y_4_offset ];

    /* p[-1,y] with y=0..3 */
	memcpy(&(left_value[1]), &(pSlice->left_y[mb_y_4_offset*4]), B_SIZE*sizeof(uint8));
	/* p[x, -1] with x=0..3 */
	memcpy(&(up_value[1]), &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));

    left = &(left_value[1]);
	up = &(up_value[1]);

    // calculate residual and save prediction pixels */
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            zHD = 2*y_coord - x_coord;

            switch (zHD)
            {
            case 0:
            case 2:
            case 4:
            case 6:
                /* 8-60 at w6359 */
                pred_value = left[y_coord - (x_coord>>1) -1] + left[y_coord - (x_coord>>1)];
                pred_value = (pred_value+1)>>1;
                break;
            case 1:
            case 3:
            case 5:
                /* 8-61 at w6359 */
                pred_value = left[y_coord - (x_coord>>1) -2] + 2*left[y_coord - (x_coord>>1)-1] + left[y_coord - (x_coord>>1)];
                pred_value = (pred_value+2)>>2;
                break;
            case -1:
                /* 8-62 at w6359 */
                pred_value = left[0] + 2*left[-1] + up[0];
                pred_value = (pred_value + 2)>>2;
                break;
            case -2: 
            case -3:
                /* 8-63 at w6359 */
                pred_value = up[x_coord-1] + 2*up[x_coord-2]+ up[x_coord-3];
                pred_value = (pred_value + 2)>>2;
            }
            
            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);

            residual_start[x_coord + y_coord*MB_SIZE] = (*block_start) - pred_value;
            if( pEnc->ICompensate_enable )	*block_start = (uint8)pred_value;//  May 25 2005
            block_start++;
        }
        block_start += (frame_width - B_SIZE);
    }
    
    return MMES_NO_ERROR;
}

xint
enc_intra_4x4_VL_pred(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 vert.-left pred. mode residuals.   */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Jerry Peng, Add 'block' to the parameters    */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..7 are marked as "available for Intra_4x4 prediction */
    /* ==> p[x,-1] with x=0..3 will satisfy the above constraint */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 up_value[8];
    uint8 up_right_available;
    xint x_coord, y_coord;
    uint8 *block_start;
    int16 *residual_start;
    int16 pred_value;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;

    frame_y_16_offset = pMB->id / pEnc->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pEnc->width;

    if(frame_x_4_offset == (frame_width/B_SIZE - 1))
        up_right_available = MMES_INVALID;
    else
        up_right_available = pSlice->top_valid[frame_x_4_offset + 1];

    if( (block==3) || (block==11))
        up_right_available = MMES_INVALID;
	else if( (block==7) || (block==13) || (block==15) )//  May 31 2005
		up_right_available = MMES_INVALID;

    if(up_right_available)
        memcpy(up_value, &(pSlice->top_y[frame_x_4_offset*4]), 2*B_SIZE*sizeof(uint8));
    else
    {
        /* p104 at w6359 */
        memcpy(up_value, &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));
        memset(&(up_value[4]), up_value[3], B_SIZE*sizeof(uint8));
    }

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            if((y_coord==0) || (y_coord ==2))
            {
                /* 8-64 at w6359 */
                pred_value = up_value[x_coord+(y_coord>>1)] + up_value[x_coord+(y_coord>>1)+1];
                pred_value = (pred_value+1)>>1;
            }
            else
            {
                /* 8-65 at w6359 */
                pred_value = up_value[x_coord+(y_coord>>1)] + 2*up_value[x_coord+(y_coord>>1)+1] + up_value[x_coord+(y_coord>>1)+2];
                pred_value = (pred_value+2)>>2;
            }
                
            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);

            residual_start[x_coord+y_coord*MB_SIZE] = (*block_start) - pred_value;
            if( pEnc->ICompensate_enable )	*block_start = (uint8) pred_value;//  May 25 2005
            block_start ++;
        }
        block_start += (frame_width - B_SIZE);
    }   
    
    return MMES_NO_ERROR;
}

xint
enc_intra_4x4_HU_pred(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 horz.-up predition mode residuals. */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    /* used when p[-1,y] with y=0..3 are marked as "available for Intra_4x4" */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *block_start;
    int16 *residual_start;
    xint x_coord, y_coord;
    uint8 *left;
    int16 pred_value;
    xint zHU;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    
    frame_y_16_offset = pMB->id / pEnc->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pEnc->width;

    left = &(pSlice->left_y[mb_y_4_offset*4]);

    // calculate residual and save prediction pixels */
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            zHU = x_coord+2*y_coord;
            
            switch (zHU)
            {
            case 0: 
            case 2:
            case 4:
                /* 8-66 at w6359 */
                pred_value = left[y_coord + (x_coord>>1)] + left[y_coord+(x_coord>>1)+1];
                pred_value = (pred_value+1)>>1;
                break;
            case 1:
            case 3:
                /* 8-67 at w6359 */
                pred_value = left[y_coord+(x_coord>>1)] + 2*left[y_coord+(x_coord>>1)+1] + left[y_coord+(x_coord>>1)+2];
                pred_value = (pred_value+2)>>2;
                break;
            case 5:
                /* 8-68 at w6359 */
                pred_value = left[2] + 3*left[3];
                pred_value = (pred_value+2)>>2;
                break;
            default:
                /* zHU is greater than 5, 8-69 at w6359 */
                pred_value = left[3];
            }

            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);
            residual_start[x_coord + y_coord*MB_SIZE] = (*block_start) - pred_value;
            if( pEnc->ICompensate_enable )	*block_start = (uint8)pred_value;//  May 25 2005
            block_start++;
        }
        block_start += (frame_width - B_SIZE);
    }
    
    return MMES_NO_ERROR;
}

xint
enc_intra_chroma_DC_pred(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma DC predition mode residuals.    */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint    frame_x_16_offset, frame_y_16_offset;
    int16   idx;
    xint    x_coord, y_coord;
    xint    width, height;
    uint8  *cb, *cr;
    xint    chroma_mb_size, chroma_frame_width;
	uint8   predictor_cb[ (MB_SIZE*MB_SIZE)>>2 ], predictor_cr[ (MB_SIZE*MB_SIZE)>>2 ];//  06 28 2005

    pCurr_frame = pEnc->curf;
    width = pEnc->width;
    height = pEnc->height;

    // compute choma size for YCbCr 4:2:0 format
    chroma_frame_width = width / 2;
    chroma_mb_size = MB_SIZE / 2;

    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

	//  06 28 2005
	intra_chroma_DC_pred(pSlice, predictor_cb, predictor_cr, pEnc->nx_mb, 0);

    //calculate residual and save predictor
    cb = pCurr_frame->cb +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;
    idx = 0;
    for (y_coord = 0; y_coord < chroma_mb_size; y_coord++)
    {
        for (x_coord = 0; x_coord < chroma_mb_size; x_coord++)
        {
            pMB->cb_residual[idx] = (*cb) - predictor_cb[idx];

            if( pEnc->ICompensate_enable )	*cb = predictor_cb[idx];//  May 26 2005
            cb++;
            idx++;
        }
        cb += (chroma_frame_width - chroma_mb_size);
    }

    //calculate residual and save predictor
    cr = pCurr_frame->cr +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;
    idx = 0;
    for (y_coord = 0; y_coord < chroma_mb_size; y_coord++)
    {
        for (x_coord = 0; x_coord < chroma_mb_size; x_coord++)
        {
            pMB->cr_residual[idx] = (*cr) - predictor_cr[idx];      //idx = y_coord*chroma_mb_size+x_coord

            if( pEnc->ICompensate_enable )	*cr = predictor_cr[idx];//  May 26 2005
            cr++;
            idx++;
        }
        cr += (chroma_frame_width - chroma_mb_size);
    }

    return MMES_NO_ERROR;
}

xint
enc_intra_chroma_H_pred(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Feb/21/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma horizontal pred. mode residuals.*/
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint x_coord, y_coord;
    uint8 *cb, *cr;


    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

    // compute chroma size for YCbCr 4:2:0 format
    chroma_frame_width = pEnc->width / 2;
    chroma_mb_size = MB_SIZE / 2;

    // calculate residual and save predictor
    cb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    cr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
    {
        for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
        {
            // cb component
            pMB->cb_residual[x_coord+y_coord*chroma_mb_size] = (*cb) - pSlice->left_cb[y_coord];
            if( pEnc->ICompensate_enable )	*cb = pSlice->left_cb[y_coord];//  May 26 2005
            cb++;

            // cr component
            pMB->cr_residual[x_coord+y_coord*chroma_mb_size] = (*cr) - pSlice->left_cr[y_coord];
            if( pEnc->ICompensate_enable )	*cr = pSlice->left_cr[y_coord];//  May 26 2005
            cr++;   
        }
        cb += (chroma_frame_width-chroma_mb_size);
        cr += (chroma_frame_width-chroma_mb_size);
    }
    
    return MMES_NO_ERROR;
}

xint
enc_intra_chroma_V_pred(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Feb/21/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma vertical pred. mode residuals.  */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint x_coord, y_coord;
    uint8 *cb, *cr;


    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

    // compute chroma size for YCbCr 4:2:0 format
    chroma_frame_width = pEnc->width / 2;
    chroma_mb_size = MB_SIZE / 2;

    // calculate residual and save predictor
    cb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    cr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
    {
        for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
        {
            // cb component
            pMB->cb_residual[x_coord+y_coord*chroma_mb_size] = (*cb) - pSlice->top_cb[frame_x_16_offset*chroma_mb_size+x_coord];
            if( pEnc->ICompensate_enable )	*cb = pSlice->top_cb[frame_x_16_offset*chroma_mb_size+x_coord];//  May 26 2005
            cb++;

            // cr component
            pMB->cr_residual[x_coord+y_coord*chroma_mb_size] = (*cr) - pSlice->top_cr[frame_x_16_offset*chroma_mb_size+x_coord];
            if( pEnc->ICompensate_enable )	*cr = pSlice->top_cr[frame_x_16_offset*chroma_mb_size+x_coord];//  May 26 2005
            cr++;
        }
        cb += (chroma_frame_width-chroma_mb_size);
        cr += (chroma_frame_width-chroma_mb_size);
    }
    return MMES_NO_ERROR;
}

xint
enc_intra_chroma_PL_pred(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Feb/21/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma plane predition mode residuals. */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint x_coord, y_coord;
    xint idx;
    int16 cb_H, cb_V, cb_a, cb_b, cb_c;
    int16 cr_H, cr_V, cr_a, cr_b, cr_c;
    int16 tmp_value;
    uint8 *left_cb, *left_cr, *top_cb, *top_cr, top_cb_value[9], top_cr_value[9], left_cb_value[9], left_cr_value[9];
    uint8 *cb, *cr;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

    // compute chroma size for YCbCr 4:2:0 format
    chroma_frame_width = pEnc->width>>1;
    chroma_mb_size = MB_SIZE>>1;

//    left_cb = pSlice->left_cb;
//    left_cr = pSlice->left_cr;

	//  June 1 2005
	top_cb_value[0] = pSlice->upleft_cb;
	top_cr_value[0] = pSlice->upleft_cr;
	left_cb_value[0] = pSlice->upleft_cb;
	left_cr_value[0] = pSlice->upleft_cr;

	for( idx=0; idx<MB_SIZE/2; idx++ )
	{
		top_cb_value[idx+1] = pSlice->top_cb[frame_x_16_offset*chroma_mb_size+idx];
		top_cr_value[idx+1] = pSlice->top_cr[frame_x_16_offset*chroma_mb_size+idx];
		left_cb_value[idx+1] = pSlice->left_cb[idx];
		left_cr_value[idx+1] = pSlice->left_cr[idx];
	}

    top_cb = &top_cb_value[1];
    top_cr = &top_cr_value[1];
    left_cb = &left_cb_value[1];
    left_cr = &left_cr_value[1];

    cb_H = cb_V = cr_H = cr_V = 0;
    for(idx=0; idx<4; idx++)
    {
        cb_H += (idx+1)*(top_cb[4+idx] - top_cb[2-idx]);
        cr_H += (idx+1)*(top_cr[4+idx] - top_cr[2-idx]);
    }

    for(idx=0; idx<3; idx++)
    {
        cb_V += (idx+1)*(left_cb[4+idx] - left_cb[2-idx]);
        cr_V += (idx+1)*(left_cr[4+idx] - left_cr[2-idx]);
    }
    cb_V += 4*(left_cb[7] - top_cb[-1]);
    cr_V += 4*(left_cr[7] - top_cr[-1]);
    
    cb_a = 16*(top_cb[7] + left_cb[7]);
    cb_b = (17*cb_H+16)>>5;
    cb_c = (17*cb_V+16)>>5;

    cr_a = 16*(top_cr[7] + left_cr[7]);
    cr_b = (17*cr_H+16)>>5;
    cr_c = (17*cr_V+16)>>5;

    // calculate residual and save predictor
    cb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    cr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;

    for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
    {
        for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
        {
            //cb component
            tmp_value = (cb_a+cb_b*(x_coord-3)+cb_c*(y_coord-3)+16)>>5;
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            pMB->cb_residual[x_coord+y_coord*chroma_mb_size] = (*cb) - tmp_value;

            if( pEnc->ICompensate_enable )	*cb = (uint8)tmp_value;//  May 26 2005
            cb++;

            //cr component
            tmp_value = (cr_a+cr_b*(x_coord-3)+cr_c*(y_coord-3)+16)>>5;
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            pMB->cr_residual[x_coord+y_coord*chroma_mb_size] = (*cr) - tmp_value;
            if( pEnc->ICompensate_enable )	*cr = (uint8)tmp_value;//  May 26 2005
            cr++;
        }
        cb += (chroma_frame_width - chroma_mb_size);
        cr += (chroma_frame_width - chroma_mb_size);
    }
    return MMES_NO_ERROR;
}

xint dec_intra_16x16_prediction(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/09/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra luma 16x16 prediction residuals.       */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    switch (pDec->curr_slice->cmb.best_I16x16_mode)
    {
    case INTRA_16x16_DC:
        dec_intra_16x16_DC_pred(pDec);
        break;
    case INTRA_16x16_H:
        dec_intra_16x16_H_pred(pDec);
        break;
    case INTRA_16x16_V:
        dec_intra_16x16_V_pred(pDec);
        break;
    case INTRA_16x16_PL:
        dec_intra_16x16_PL_pred(pDec);
        break;
    }

    return 0;
}

xint dec_intra_4x4_prediction(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/09/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra luma 4x4 prediction residuals.         */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    switch (pDec->curr_slice->cmb.best_I4x4_mode[block])
    {
    case INTRA_4x4_V:
        dec_intra_4x4_V_pred(pDec, block);
        break;
    case INTRA_4x4_H:
        dec_intra_4x4_H_pred(pDec, block);
        break;
    case INTRA_4x4_DC:
        dec_intra_4x4_DC_pred(pDec, block);
        break;
    case INTRA_4x4_DDL:
        dec_intra_4x4_DDL_pred(pDec, block);
        break;
    case INTRA_4x4_DDR:
        dec_intra_4x4_DDR_pred(pDec, block);
        break;
    case INTRA_4x4_VR:
        dec_intra_4x4_VR_pred(pDec, block);
        break;
    case INTRA_4x4_HD:
        dec_intra_4x4_HD_pred(pDec, block);
        break;
    case INTRA_4x4_VL:
        dec_intra_4x4_VL_pred(pDec, block);
        break;
    case INTRA_4x4_HU:
        dec_intra_4x4_HU_pred(pDec, block);
        break;
    }

    return 0;
}

xint
dec_intra_chroma_prediction(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jun/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma prediction.                     */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    switch (pDec->curr_slice->cmb.best_chroma_mode)
    {
    case INTRA_CHROMA_DC:
        dec_intra_chroma_DC_pred(pDec);
        break;
    case INTRA_CHROMA_H:
        dec_intra_chroma_H_pred(pDec);
        break;
    case INTRA_CHROMA_V:
        dec_intra_chroma_V_pred(pDec);
        break;
    case INTRA_CHROMA_PL:
        dec_intra_chroma_PL_pred(pDec);
        break;
    }

    return 0;
}

xint
dec_intra_16x16_DC_pred(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 16x16 DC predition mode residuals.     */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Nov/08/2005 Jerry Peng, modify intra prediction when     */
/*               constrained_intra_pred_flag is employed.        */
/* ------------------------------------------------------------- */
{
	pic_paraset_obj *pps;
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint    frame_x_16_offset, frame_y_16_offset;
    int16   s1, s2, s, idx;
    uint8   predictor;
    xint    x_coord, y_coord;
    uint8  *tmp_predictor_y;
    uint8   top_valid;
    uint8   left_valid;
    xint    width, height;
    uint8  *y;
	uint8   predictor_y[ MB_SIZE ][ MB_SIZE ];

    pCurr_frame = pDec->curf;
    width = pDec->width;
    height = pDec->height;

    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);
	pps = &pDec->pps[pSlice->pic_parameter_set_id];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

	tmp_predictor_y = &predictor_y[0][0];

    top_valid = pSlice->top_valid[frame_x_16_offset * (MB_SIZE/B_SIZE)];
    left_valid = pSlice->left_valid[0];

	//  11 07 2005
	if( pps->constrained_intra_pred_flag )
	{
		if( !pSlice->top_mb_intra[frame_x_16_offset * (MB_SIZE/B_SIZE)] )
			top_valid = MMES_INVALID;
		if( !pSlice->left_mb_intra[0] )
			left_valid = MMES_INVALID;
	}

    // perform luma intra16x16 DC prediction
    s = s1 = s2 = 0;
    if (top_valid == MMES_VALID)
    {
        for (idx = 0; idx < MB_SIZE; idx++)
        {
            s1 += pSlice->top_y[frame_x_16_offset * MB_SIZE + idx];
        }
    }
    if (left_valid == MMES_VALID)
    {
        for (idx = 0; idx < MB_SIZE; idx++)
        {
            s2 += pSlice->left_y[idx];
        }
    }

    if ((top_valid == MMES_VALID) && (left_valid == MMES_VALID))
    {
        s = (s1 + s2 + 16) / (2 * MB_SIZE);
    }
    else if (top_valid == MMES_VALID)
    {
        s = (s1 + 8) / MB_SIZE;
    }
    else if (left_valid)
    {
        s = (s2 + 8) / MB_SIZE;
    }
    else
    {
        s = 128;
    }

    predictor = (uint8) CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s);

    memset(tmp_predictor_y, predictor, MB_SIZE * MB_SIZE);
    
    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
        frame_x_16_offset * MB_SIZE;
    idx = 0;
    for (y_coord = 0; y_coord < MB_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < MB_SIZE; x_coord++)
        {           
            *y = tmp_predictor_y[idx];
            y++;
            idx++;
        }
        y += (width - MB_SIZE);
    }

    return MMES_NO_ERROR;
}

xint
dec_intra_16x16_H_pred(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 16x16 horizontal pred. mode residuals. */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint x_coord, y_coord;
    xint width;
    uint8 *y;


    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    width = pDec->width;

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    //calculate the residual and save the predictor
    y = pCurr_frame->y + frame_y_16_offset*MB_SIZE*width + frame_x_16_offset*MB_SIZE;
    for(y_coord=0; y_coord<MB_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<MB_SIZE; x_coord++)
        {
            *y = pSlice->left_y[y_coord];//  May 24 2005
            y++;
        }
        y += (width - MB_SIZE);
    }
    
    return MMES_NO_ERROR;
}

xint
dec_intra_16x16_V_pred(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 16x16 vertical pred. mode residuals.   */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint x_coord, y_coord;
    xint width;
    uint8 *y;


    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    width = pDec->width;

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    //calculate the residual and save the predictor
    y = pCurr_frame->y + frame_y_16_offset*MB_SIZE*width + frame_x_16_offset*MB_SIZE;
    for(y_coord=0; y_coord<MB_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<MB_SIZE; x_coord++)
        {

			*y = pSlice->top_y[frame_x_16_offset*MB_SIZE+x_coord];//  May 24 2005
            y++;
        }
        y += (width - MB_SIZE);
    }

    return MMES_NO_ERROR;
}

xint
dec_intra_16x16_PL_pred(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 16x16 plane predition mode residuals.  */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint x_coord, y_coord, idx;
    xint width;
    int16 iH, iV, ia, ib, ic, tmp_value;
    uint8 *top_value, *left_value, *y, top[MB_SIZE+1], left[MB_SIZE+1];

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);
    width = pDec->width;

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

	top[0] = pSlice->upleft_y[0];
	left[0] = pSlice->upleft_y[0];
	for(idx=0; idx<MB_SIZE; idx++)
	{
		top[idx+1] = pSlice->top_y[frame_x_16_offset*MB_SIZE+idx];
		left[idx+1] = pSlice->left_y[idx];
	}

	top_value = &top[1];
	left_value = &left[1];

    iH=0;
    for(idx=0; idx<8; idx++)
    {
        iH += (idx+1)*(top_value[8+idx] - top_value[6-idx]);
    }

    iV=0;
    for(idx=0; idx<7; idx++)
    {
        iV += (idx+1)*(left_value[8+idx] - left_value[6-idx]);
    }
    iV += 8*(left_value[15] - top_value[-1]);

    ia = 16*(left_value[15]+top_value[15]);
    ib = (5*iH+32)>>6;
    ic = (5*iV+32)>>6;

    // calculate the residual and save the predictor
    y = pCurr_frame->y + frame_y_16_offset*MB_SIZE*width + frame_x_16_offset*MB_SIZE;
    for(y_coord=0; y_coord<MB_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<MB_SIZE; x_coord++)
        {
            tmp_value = (ia+ib*(x_coord-7)+ic*(y_coord-7)+16)>>5;
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);

			*y = (uint8) tmp_value;
            y++;
        }
        y += (width-MB_SIZE);
    }    

    return MMES_NO_ERROR;
}

xint
dec_intra_4x4_V_pred(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/09/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 vertical predition mode residuals. */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be decoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..3 are marked as "available for Intra_4x4 prediction */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *up_value;
    xint x_coord, y_coord;
    uint8 *block_start;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;

    frame_y_16_offset = pMB->id / pDec->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pDec->width;

    up_value = &(pSlice->top_y[frame_x_4_offset*4]);

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            *block_start = up_value[x_coord];
            block_start++;

        }
        block_start += (frame_width - B_SIZE);
    }
    
    return MMES_NO_ERROR;
}

xint
dec_intra_4x4_H_pred(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/25/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 horizontal pred. mode residuals.   */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Jerry Peng, Add 'block' to the parameters    */
/* ------------------------------------------------------------- */
{
    /* used when p[-1,y] with y=0..3 are marked as "available for Intra_4x4 prediction */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *left_value;
    xint x_coord, y_coord;
    uint8 *block_start;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;

    frame_y_16_offset = pMB->id / pDec->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pDec->width;

	//  May 25 2005
    left_value = &(pSlice->left_y[mb_y_4_offset*4]);

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            *block_start = left_value[y_coord];
            block_start++;
        }
        block_start += (frame_width - B_SIZE);
    }
    return MMES_NO_ERROR;
}

xint
dec_intra_4x4_DC_pred(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/09/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 DC predition mode residuals.       */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Nov/08/2005 Jerry Peng, modify intra prediction when     */
/*               constrained_intra_pred_flag is employed.        */
/* ------------------------------------------------------------- */
{
	pic_paraset_obj *pps;
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    uint8 up_available, left_available;
    uint8 *up_value, *left_value;
    int16 pred_value;
    xint frame_width;
    uint8 *block_start;
    xint x_coord, y_coord;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);
	pps = &pDec->pps[pSlice->pic_parameter_set_id];

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    frame_y_16_offset = pMB->id / pDec->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pDec->width;

    // check available
    up_available = pSlice->top_valid[frame_x_4_offset];
    left_available = pSlice->left_valid[mb_y_4_offset];

	//  11 07 2005
	if( pps->constrained_intra_pred_flag )
	{
		if( !pSlice->top_mb_intra[frame_x_4_offset] )
			up_available = MMES_INVALID;
		if( !pSlice->left_mb_intra[mb_y_4_offset] )
			left_available = MMES_INVALID;
	}

    up_value = &(pSlice->top_y[frame_x_4_offset*4]);
    left_value = &(pSlice->left_y[mb_y_4_offset*4]);

    if(up_available && left_available)
    {
        /* 8-47 at w6359 */
        pred_value = up_value[0] + up_value[1] + up_value[2] + up_value[3] +
                     left_value[0] + left_value[1] + left_value[2] + left_value[3];
        pred_value = (pred_value + 4)>>3;
    }
    else if(left_available)
    {
        /* 8-48 at w6359 */
        pred_value = left_value[0] + left_value[1] + left_value[2] + left_value[3];
        pred_value = (pred_value + 2)>>2;
    }
    else if(up_available)
    {
        /* 8-49 at w6359 */
        pred_value = up_value[0] + up_value[1] + up_value[2] + up_value[3];
        pred_value = (pred_value + 2)>>2;
    }
    else 
    {
        /* 8-50 at w6359 */
        pred_value = 128; 
    }

    pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
			*block_start = (uint8) pred_value;
            block_start ++;

        }
        block_start += (frame_width - B_SIZE);

    }

    return MMES_NO_ERROR;
}

xint
dec_intra_4x4_DDL_pred(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/09/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 diag-down-left mode residuals.     */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Nov/08/2005 Jerry Peng, modify intra prediction when     */
/*               constrained_intra_pred_flag is employed.        */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..7 are marked as "available for Intra_4x4 prediction */
    /* ==> p[x,-1] with x=0..3 will satisfy the above constraint */
	pic_paraset_obj *pps;
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 up_value[8];
    uint8 up_right_available;
    xint x_coord, y_coord;
    uint8 *block_start;
    int16 pred_value;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);
	pps = &pDec->pps[pSlice->pic_parameter_set_id];

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;

    frame_y_16_offset = pMB->id / pDec->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pDec->width;

    if(frame_x_4_offset == (frame_width/B_SIZE - 1))
        up_right_available = MMES_INVALID;
    else
	{
		up_right_available = pSlice->top_valid[frame_x_4_offset + 1];

		//  11 07 2005
		if( pps->constrained_intra_pred_flag && up_right_available )
		{
			up_right_available = pSlice->top_mb_intra[frame_x_4_offset + 1];
		}
	}
        

    if( (block==3) || (block==11) )
		up_right_available = MMES_INVALID;
	else if( (block==7) || (block==13) || (block==15) )//  May 31 2005
		up_right_available = MMES_INVALID;

    if(up_right_available)
        memcpy(up_value, &(pSlice->top_y[frame_x_4_offset*4]), 2*B_SIZE*sizeof(uint8));
    else
    {
        /* p104 at w6359 */
        memcpy(up_value, &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));
        memset(&(up_value[4]), up_value[3], B_SIZE*sizeof(uint8));
    }

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            if(x_coord==3 && y_coord==3)
                pred_value = (up_value[6] + up_value[7]*3 + 2)>>2;//  May 26 2005
            else
                pred_value = (up_value[x_coord+y_coord] + 2*up_value[x_coord+y_coord+1] +
                              up_value[x_coord+y_coord+2] + 2)>>2;

            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);
            *block_start = (uint8) pred_value;//  May 25 2005
            block_start ++;

        }
        block_start += (frame_width - B_SIZE);
    }

    return MMES_NO_ERROR;
}

xint
dec_intra_4x4_DDR_pred(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/09/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 diag-down-right mode residuals.    */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..3 and p[-1,y] with y=-1..3 are marked as "available for Intra_4x4" */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *block_start;
    xint x_coord, y_coord;
    uint8 left_value[5];
	uint8 up_value[5];//  May 26 2005
    uint8 *up, *left;
    int16 pred_value;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    
    frame_y_16_offset = pMB->id / pDec->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pDec->width;

    /* p[-1,-1] */
	//  May 26 2005
    left_value[0] = pSlice->upleft_y[ mb_y_4_offset ];
	up_value[0] = pSlice->upleft_y[ mb_y_4_offset ];

    /* p[-1,y] with y=0..3 */
    memcpy(&(left_value[1]), &(pSlice->left_y[mb_y_4_offset*4]), B_SIZE*sizeof(uint8));
	/* p[x, -1] with x=0..3 */
	memcpy(&(up_value[1]), &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));    
    
    left = &(left_value[1]);
	up = &(up_value[1]);

    // calculate residual and save prediction pixels */
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            if(x_coord>y_coord)         /* 8-53 at w6359 */
                pred_value = up[x_coord-y_coord-2] + 2*up[x_coord-y_coord-1] + up[x_coord-y_coord];
            else if(x_coord<y_coord)    /* 8-54 at w6359 */
                pred_value = left[y_coord-x_coord-2] + 2*left[y_coord-x_coord-1] + left[y_coord-x_coord];
            else                        /* 8-55 at w6359 */
                pred_value = up[0] + 2*left[-1] + left[0];

            pred_value = (pred_value+2)>>2;

            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);
            *block_start = (uint8)pred_value;
            block_start++;

        }
        block_start += (frame_width - B_SIZE);

    }    
    return MMES_NO_ERROR;
}

xint
dec_intra_4x4_VR_pred(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 vert.-right pred. mode residuals.  */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Jerry Peng, Add 'block' to the parameters    */
/*   May/26/2005 Jerry Peng, Fix bugs in accessing neighboring*/
/*               pixels                                          */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..3 and p[-1,y] with y=-1..3 are marked as "available for Intra_4x4" */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *block_start;
    xint x_coord, y_coord;
    uint8 left_value[5];
	uint8 up_value[5];//  May 26 2005
    uint8 *up, *left;
    int16 pred_value;
    xint zVR;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    
    frame_y_16_offset = pMB->id / pDec->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pDec->width;

    /* p[-1,-1] */
	//  May 26 2005
    left_value[0] = pSlice->upleft_y[ mb_y_4_offset ];
	up_value[0] = pSlice->upleft_y[ mb_y_4_offset ];

    /* p[-1,y] with y=0..3 */
	memcpy(&(left_value[1]), &(pSlice->left_y[mb_y_4_offset*4]), B_SIZE*sizeof(uint8));
	/* p[x, -1] with x=0..3 */
	memcpy(&(up_value[1]), &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));
        
    left = &(left_value[1]);
	up = &(up_value[1]);

    // calculate residual and save prediction pixels */
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            zVR = 2*x_coord - y_coord;

            switch (zVR)
            {
            case 0:
            case 2: 
            case 4:
            case 6:
                /* 8-56 at w6359 */
                pred_value = up[x_coord - (y_coord>>1) -1] + up[x_coord - (y_coord>>1)];
                pred_value = (pred_value+1)>>1;
                break;
            case 1: 
            case 3:
            case 5:
                /* 8-57 at w6359 */
                pred_value = up[x_coord - (y_coord>>1) -2] + 2*up[x_coord - (y_coord>>1) -1] + up[x_coord - (y_coord>>1)];
                pred_value = (pred_value+2)>>2;
                break;
            case -1:
                /* 8-58 at w6359 */
                pred_value = up[0] + 2*up[-1] + left[0];
                pred_value = (pred_value+2)>>2;
                break;
            case -2:
            case -3:
                /* 8-59 at w6359 */
                pred_value = left[y_coord-1] + 2*left[y_coord-2] + left[y_coord-3];
                pred_value = (pred_value+2)>>2;
            }

            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);
            *block_start = (uint8)pred_value;//  May 25 2005
            block_start++;
        }
        block_start += (frame_width - B_SIZE);
    }
    
    return MMES_NO_ERROR;
}

xint
dec_intra_4x4_HD_pred(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 horz.-down pred. mode residuals.   */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Jerry Peng, Add 'block' to the parameters    */
/*   May/26/2005 Jerry Peng, Fix bugs in accessing neighboring*/
/*               pixels                                          */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..3 and p[-1,y] with y=-1..3 are marked as "available for Intra_4x4" */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *block_start;
    xint x_coord, y_coord;
    uint8 left_value[5];
	uint8 up_value[5];//  May 26 2005
    uint8 *up, *left;
    int16 pred_value;
    xint zHD;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    
    frame_y_16_offset = pMB->id / pDec->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pDec->width;

    /* p[-1,-1] */    
	//  May 26 2005
    left_value[0] = pSlice->upleft_y[ mb_y_4_offset ];
	up_value[0] = pSlice->upleft_y[ mb_y_4_offset ];

    /* p[-1,y] with y=0..3 */
	memcpy(&(left_value[1]), &(pSlice->left_y[mb_y_4_offset*4]), B_SIZE*sizeof(uint8));
	/* p[x, -1] with x=0..3 */
	memcpy(&(up_value[1]), &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));

    left = &(left_value[1]);
	up = &(up_value[1]);

    // calculate residual and save prediction pixels */
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            zHD = 2*y_coord - x_coord;

            switch (zHD)
            {
            case 0:
            case 2:
            case 4:
            case 6:
                /* 8-60 at w6359 */
                pred_value = left[y_coord - (x_coord>>1) -1] + left[y_coord - (x_coord>>1)];
                pred_value = (pred_value+1)>>1;
                break;
            case 1:
            case 3:
            case 5:
                /* 8-61 at w6359 */
                pred_value = left[y_coord - (x_coord>>1) -2] + 2*left[y_coord - (x_coord>>1)-1] + left[y_coord - (x_coord>>1)];
                pred_value = (pred_value+2)>>2;
                break;
            case -1:
                /* 8-62 at w6359 */
                pred_value = left[0] + 2*left[-1] + up[0];
                pred_value = (pred_value + 2)>>2;
                break;
            case -2: 
            case -3:
                /* 8-63 at w6359 */
                pred_value = up[x_coord-1] + 2*up[x_coord-2]+ up[x_coord-3];
                pred_value = (pred_value + 2)>>2;
            }
            
            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);

            *block_start = (uint8)pred_value;//  May 25 2005
            block_start++;

        }
        block_start += (frame_width - B_SIZE);

    }
    
    return MMES_NO_ERROR;
}

xint
dec_intra_4x4_VL_pred(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 vert.-left pred. mode residuals.   */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    /* used when p[x,-1] with x=0..7 are marked as "available for Intra_4x4 prediction */
    /* ==> p[x,-1] with x=0..3 will satisfy the above constraint */
	pic_paraset_obj *pps;
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 up_value[8];
    uint8 up_right_available;
    xint x_coord, y_coord;
    uint8 *block_start;
    int16 pred_value;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);
	pps = &pDec->pps[pSlice->pic_parameter_set_id];

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;

    frame_y_16_offset = pMB->id / pDec->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pDec->width;

    if(frame_x_4_offset == (frame_width/B_SIZE - 1))
        up_right_available = MMES_INVALID;
    else
	{
		up_right_available = pSlice->top_valid[frame_x_4_offset + 1];

		//  11 07 2005
		if( pps->constrained_intra_pred_flag && up_right_available )
		{
			up_right_available = pSlice->top_mb_intra[frame_x_4_offset + 1];
		}
	}

    if( (block==3) || (block==11))
        up_right_available = MMES_INVALID;
	else if( (block==7) || (block==13) || (block==15) )//  May 31 2005
		up_right_available = MMES_INVALID;

    if(up_right_available)
        memcpy(up_value, &(pSlice->top_y[frame_x_4_offset*4]), 2*B_SIZE*sizeof(uint8));
    else
    {
        /* p104 at w6359 */
        memcpy(up_value, &(pSlice->top_y[frame_x_4_offset*4]), B_SIZE*sizeof(uint8));
        memset(&(up_value[4]), up_value[3], B_SIZE*sizeof(uint8));
    }

    // calculate residual and save prediction pixel
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            if((y_coord==0) || (y_coord ==2))
            {
                /* 8-64 at w6359 */
                pred_value = up_value[x_coord+(y_coord>>1)] + up_value[x_coord+(y_coord>>1)+1];
                pred_value = (pred_value+1)>>1;
            }
            else
            {
                /* 8-65 at w6359 */
                pred_value = up_value[x_coord+(y_coord>>1)] + 2*up_value[x_coord+(y_coord>>1)+1] + up_value[x_coord+(y_coord>>1)+2];
                pred_value = (pred_value+2)>>2;
            }
                
            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);

            *block_start = (uint8) pred_value;//  May 25 2005
            block_start ++;
        }
        block_start += (frame_width - B_SIZE);
    }   
    
    return MMES_NO_ERROR;
}

xint
dec_intra_4x4_HU_pred(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra 4x4 horz.-up predition mode residuals. */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    /* used when p[-1,y] with y=0..3 are marked as "available for Intra_4x4" */
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width;
    uint8 *block_start;
    int16 *residual_start;
    xint x_coord, y_coord;
    uint8 *left;
    int16 pred_value;
    xint zHU;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    
    frame_y_16_offset = pMB->id / pDec->nx_mb;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;

    frame_width = pDec->width;

    left = &(pSlice->left_y[mb_y_4_offset*4]);

    // calculate residual and save prediction pixels */
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            zHU = x_coord+2*y_coord;
            
            switch (zHU)
            {
            case 0: 
            case 2:
            case 4:
                /* 8-66 at w6359 */
                pred_value = left[y_coord + (x_coord>>1)] + left[y_coord+(x_coord>>1)+1];
                pred_value = (pred_value+1)>>1;
                break;
            case 1:
            case 3:
                /* 8-67 at w6359 */
                pred_value = left[y_coord+(x_coord>>1)] + 2*left[y_coord+(x_coord>>1)+1] + left[y_coord+(x_coord>>1)+2];
                pred_value = (pred_value+2)>>2;
                break;
            case 5:
                /* 8-68 at w6359 */
                pred_value = left[2] + 3*left[3];
                pred_value = (pred_value+2)>>2;
                break;
            default:
                /* zHU is greater than 5, 8-69 at w6359 */
                pred_value = left[3];
            }

            pred_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, pred_value);
            *block_start = (uint8)pred_value;//  May 25 2005
            block_start++;
        }
        block_start += (frame_width - B_SIZE);
    }
    
    return MMES_NO_ERROR;
}

xint
dec_intra_chroma_DC_pred(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jun/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma DC predition mode.              */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	pic_paraset_obj *pps;
	frame_obj *pCurr_frame;
	slice_obj *pSlice;
	mb_obj *pMB;
	xint    frame_x_16_offset, frame_y_16_offset;
	int16   idx;
	xint    x_coord, y_coord;
	xint    width, height;
	uint8  *cb, *cr;
	xint    chroma_mb_size, chroma_frame_width;
	uint8   predictor_cb[ (MB_SIZE*MB_SIZE)>>2 ], predictor_cr[ (MB_SIZE*MB_SIZE)>>2 ];//  06 28 2005
	
	pCurr_frame = pDec->curf;
    width = pDec->width;
    height = pDec->height;

	// compute choma size for YCbCr 4:2:0 format
    chroma_frame_width = width / 2;
    chroma_mb_size = MB_SIZE / 2;

	pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);
	pps = &pDec->pps[pSlice->pic_parameter_set_id];

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

	intra_chroma_DC_pred(pSlice, predictor_cb, predictor_cr, pDec->nx_mb, pps->constrained_intra_pred_flag);

    //calculate residual and save predictor
    cb = pCurr_frame->cb +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;
    idx = 0;
    for (y_coord = 0; y_coord < chroma_mb_size; y_coord++)
    {
        for (x_coord = 0; x_coord < chroma_mb_size; x_coord++)
        {
//            pMB->cb_residual[idx] = (*cb) - predictor_cb[idx];
			*cb = predictor_cb[idx];
            cb++;
            idx++;
        }
        cb += (chroma_frame_width - chroma_mb_size);
    }

    //calculate residual and save predictor
    cr = pCurr_frame->cr +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;
    idx = 0;
    for (y_coord = 0; y_coord < chroma_mb_size; y_coord++)
    {
        for (x_coord = 0; x_coord < chroma_mb_size; x_coord++)
        {
//            pMB->cr_residual[idx] = (*cr) - predictor_cr[idx];      //idx = y_coord*chroma_mb_size+x_coord
			*cr = predictor_cr[idx];
            cr++;
            idx++;
        }
        cr += (chroma_frame_width - chroma_mb_size);
    }

    return MMES_NO_ERROR;
}


xint
dec_intra_chroma_H_pred(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma horizontal pred. mode residuals.*/
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint x_coord, y_coord;
    uint8 *cb, *cr;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    // compute chroma size for YCbCr 4:2:0 format
    chroma_frame_width = pDec->width / 2;
    chroma_mb_size = MB_SIZE / 2;

    // calculate residual and save predictor
    cb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    cr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
    {
        for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
        {
            // cb component            
            *cb = pSlice->left_cb[y_coord];
            cb++;

            // cr component
			*cr = pSlice->left_cr[y_coord];
            cr++;   
        }
        cb += (chroma_frame_width-chroma_mb_size);
        cr += (chroma_frame_width-chroma_mb_size);
    }
    
    return MMES_NO_ERROR;
}

xint
dec_intra_chroma_V_pred(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma vertical pred. mode residuals.  */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint x_coord, y_coord;
    uint8 *cb, *cr;


    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    // compute chroma size for YCbCr 4:2:0 format
    chroma_frame_width = pDec->width / 2;
    chroma_mb_size = MB_SIZE / 2;

    // calculate residual and save predictor
    cb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    cr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
    {
        for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
        {
            // cb component
            *cb = pSlice->top_cb[frame_x_16_offset*chroma_mb_size+x_coord];//  May 26 2005
            cb++;
            // cr component
            *cr = pSlice->top_cr[frame_x_16_offset*chroma_mb_size+x_coord];//  May 26 2005
            cr++;
        }
        cb += (chroma_frame_width-chroma_mb_size);
        cr += (chroma_frame_width-chroma_mb_size);
    }
    return MMES_NO_ERROR;
}

xint
dec_intra_chroma_PL_pred(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma plane predition mode residuals. */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint x_coord, y_coord;
    xint idx;
    int16 cb_H, cb_V, cb_a, cb_b, cb_c;
    int16 cr_H, cr_V, cr_a, cr_b, cr_c;
    int16 tmp_value;
    uint8 *left_cb, *left_cr, *top_cb, *top_cr, top_cb_value[9], top_cr_value[9], left_cb_value[9], left_cr_value[9];
    uint8 *cb, *cr;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    // compute chroma size for YCbCr 4:2:0 format
    chroma_frame_width = pDec->width>>1;
    chroma_mb_size = MB_SIZE>>1;

//    left_cb = pSlice->left_cb;
//    left_cr = pSlice->left_cr;

	//  June 1 2005
	top_cb_value[0] = pSlice->upleft_cb;
	top_cr_value[0] = pSlice->upleft_cr;
	left_cb_value[0] = pSlice->upleft_cb;
	left_cr_value[0] = pSlice->upleft_cr;

	for( idx=0; idx<MB_SIZE/2; idx++ )
	{
		top_cb_value[idx+1] = pSlice->top_cb[frame_x_16_offset*chroma_mb_size+idx];
		top_cr_value[idx+1] = pSlice->top_cr[frame_x_16_offset*chroma_mb_size+idx];
		left_cb_value[idx+1] = pSlice->left_cb[idx];
		left_cr_value[idx+1] = pSlice->left_cr[idx];
	}

    top_cb = &top_cb_value[1];
    top_cr = &top_cr_value[1];
    left_cb = &left_cb_value[1];
    left_cr = &left_cr_value[1];

    cb_H = cb_V = cr_H = cr_V = 0;
    for(idx=0; idx<4; idx++)
    {
        cb_H += (idx+1)*(top_cb[4+idx] - top_cb[2-idx]);
        cr_H += (idx+1)*(top_cr[4+idx] - top_cr[2-idx]);
    }

    for(idx=0; idx<3; idx++)
    {
        cb_V += (idx+1)*(left_cb[4+idx] - left_cb[2-idx]);
        cr_V += (idx+1)*(left_cr[4+idx] - left_cr[2-idx]);
    }
    cb_V += 4*(left_cb[7] - top_cb[-1]);
    cr_V += 4*(left_cr[7] - top_cr[-1]);
    
    cb_a = 16*(top_cb[7] + left_cb[7]);
    cb_b = (17*cb_H+16)>>5;
    cb_c = (17*cb_V+16)>>5;

    cr_a = 16*(top_cr[7] + left_cr[7]);
    cr_b = (17*cr_H+16)>>5;
    cr_c = (17*cr_V+16)>>5;

    // calculate residual and save predictor
    cb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    cr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;

    for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
    {
        for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
        {
            //cb component
            tmp_value = (cb_a+cb_b*(x_coord-3)+cb_c*(y_coord-3)+16)>>5;
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            
            *cb = (uint8)tmp_value;
            cb++;

            //cr component
            tmp_value = (cr_a+cr_b*(x_coord-3)+cr_c*(y_coord-3)+16)>>5;
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            
            *cr = (uint8)tmp_value;
            cr++;
        }
        cb += (chroma_frame_width - chroma_mb_size);
        cr += (chroma_frame_width - chroma_mb_size);
    }
    return MMES_NO_ERROR;
}

xint
intra_chroma_DC_pred(slice_obj *pSlice, uint8  *predictor_cb, uint8  *predictor_cr, int nx_mb, int constrained_intra_pred_flag )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jun/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Rewrite intra chroma prediction for the useage in           */
/*   encoder & decoder                                           */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the slice session parameters    */
/*	 *predictor_cb -> [I/O] intra chroma DC predictors of cb     */
/*	 *predictor_cr -> [I/O] intra chroma DC predictors of cr     */
/*   nx_mb   -> [I] number of macroblocks in width               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Nov/08/2005 Jerry Peng, modify intra prediction when     */
/*               constrained_intra_pred_flag is employed.        */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB;
	uint8   top_valid;
    uint8   left_valid;
	xint    frame_x_16_offset;
    xint    x_coord, y_coord;
    int16   s0, s1, s2, s3, s, idx;
	xint    chroma_mb_size;

    pMB = &(pSlice->cmb);
    frame_x_16_offset = pMB->id % nx_mb;
    chroma_mb_size = MB_SIZE / 2;


    top_valid = pSlice->top_valid[frame_x_16_offset * (MB_SIZE/B_SIZE)];
    left_valid = pSlice->left_valid[0];

	//  11 07 2005
	if( constrained_intra_pred_flag )
	{
		if( !pSlice->top_mb_intra[frame_x_16_offset * (MB_SIZE/B_SIZE)] )
			top_valid = MMES_INVALID;
		if( !pSlice->left_mb_intra[0] )
			left_valid = MMES_INVALID;
	}

    // make chroma (cb) DC predictor
    s = s0 = s1 = s2 = s3 = 0;
    if ((top_valid == MMES_VALID) && (left_valid == MMES_VALID))
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s0 += pSlice->left_cb[idx];
        }

        s0 = (s0 + 4) >> 3;     //top_left

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s1 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s3 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s2 += pSlice->left_cb[idx];
            s3 += pSlice->left_cb[idx];
        }

        s1 = (s1 + 2) >> 2;     //top_right
        s2 = (s2 + 2) >> 2;     //bottom_left
        s3 = (s3 + 4) >> 3;     //bottom_right

    }
    else if (top_valid == MMES_VALID)
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s2 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
        }
        s0 = (s0 + 2) >> 2;     //top_left

        s2 = (s2 + 2) >> 2;     //bottom_left

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s1 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s3 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
        }
        s1 = (s1 + 2) >> 2;     //top_right

        s3 = (s3 + 2) >> 2;     //bottom_right 

    }
    else if (left_valid == MMES_VALID)
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->left_cb[idx];
            s1 += pSlice->left_cb[idx];
        }
        s0 = (s0 + 2) >> 2;     //top_left

        s1 = (s1 + 2) >> 2;     //top_rigth;

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s2 += pSlice->left_cb[idx];
            s3 += pSlice->left_cb[idx];
        }
        s2 = (s2 + 2) >> 2;     //top_left

        s3 = (s3 + 2) >> 2;     //top_rigth;

    }
    else
        s0 = s1 = s2 = s3 = 128;

    s0 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s0);
    s1 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s1);
    s2 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s2);
    s3 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s3);

    for (y_coord = 0; y_coord < 4; y_coord++)
    {
        for (x_coord = 0; x_coord < 4; x_coord++)
        {
            /* top_left */
            predictor_cb[y_coord * chroma_mb_size + x_coord] = (uint8) s0;

            /* top_right */
            predictor_cb[y_coord * chroma_mb_size + x_coord + 4] = (uint8) s1;

            /* bottom_left */
            predictor_cb[(y_coord + 4) * chroma_mb_size + x_coord] = (uint8) s2;

            /* bottom_right */
            predictor_cb[(y_coord + 4) * chroma_mb_size + x_coord + 4] = (uint8) s3;
        }
    }

    // make chroma (cr) DC predictor
    s = s0 = s1 = s2 = s3 = 0;
    if ((top_valid == MMES_VALID) && (left_valid == MMES_VALID))
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s0 += pSlice->left_cr[idx];
        }

        s0 = (s0 + 4) >> 3;     //top_left

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s1 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s3 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s2 += pSlice->left_cr[idx];
            s3 += pSlice->left_cr[idx];
        }

        s1 = (s1 + 2) >> 2;     //top_right
        s2 = (s2 + 2) >> 2;     //bottom_left
        s3 = (s3 + 4) >> 3;     //bottom_right
    }
    else if (top_valid == MMES_VALID)
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s2 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
        }
        s0 = (s0 + 2) >> 2;     //top_left

        s2 = (s2 + 2) >> 2;     //bottom_left

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s1 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s3 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
        }
        s1 = (s1 + 2) >> 2;     //top_right

        s3 = (s3 + 2) >> 2;     //bottom_right 

    }
    else if (left_valid == MMES_VALID)
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->left_cr[idx];
            s1 += pSlice->left_cr[idx];
        }
        s0 = (s0 + 2) >> 2;     //top_left

        s1 = (s1 + 2) >> 2;     //top_rigth;

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s2 += pSlice->left_cr[idx];
            s3 += pSlice->left_cr[idx];
        }
        s2 = (s2 + 2) >> 2;     //top_left

        s3 = (s3 + 2) >> 2;     //top_rigth;

    }
    else
    {
        s0 = s1 = s2 = s3 = 128;
    }

    s0 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s0);
    s1 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s1);
    s2 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s2);
    s3 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s3);

    for (y_coord = 0; y_coord < 4; y_coord++)
    {
        for (x_coord = 0; x_coord < 4; x_coord++)
        {
            /* top_left */
            predictor_cr[y_coord * chroma_mb_size + x_coord] = (uint8) s0;

            /* top_right */
            predictor_cr[y_coord * chroma_mb_size + x_coord + 4] = (uint8) s1;

            /* bottom_left */
            predictor_cr[(y_coord + 4) * chroma_mb_size + x_coord] = (uint8) s2;

            /* bottom_right */
            predictor_cr[(y_coord + 4) * chroma_mb_size + x_coord + 4] = (uint8) s3;
        }
    }

	return MMES_NO_ERROR;
}

/* make predictor and reconstruct frame */
xint
dec_intra_prediction(h264dec_obj * pDec)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   H.264 intra reconstruction module.                          */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj *pMB;
    uint8  *y, *cb, *cr;
    xint    frame_x_16_offset, frame_y_16_offset;
    xint    width;
    xint    idx, x_coord, y_coord;
    int16   tmp_value;
    xint    chroma_mb_size, chroma_frame_width;
    int16   s0, s1, s2, s3, s;
    uint8   top_valid, left_valid;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    width = pDec->width;
    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    chroma_mb_size = MB_SIZE / 2;       // for YCbCr420
    chroma_frame_width = width / 2;     // for YCbCr420

    //default predictor is Intra16x16 DC
    top_valid = pSlice->top_valid[frame_x_16_offset * B_SIZE];
    left_valid = pSlice->left_valid[0];

    s = s1 = s2 = 0;
    if (top_valid == MMES_VALID)
    {
        for (idx = 0; idx < MB_SIZE; idx++)
        {
            s1 += pSlice->top_y[frame_x_16_offset * MB_SIZE + idx];
        }
    }
    if (left_valid == MMES_VALID)
    {
        for (idx = 0; idx < MB_SIZE; idx++)
        {
            s2 += pSlice->left_y[idx];
        }
    }
    if ((top_valid == MMES_VALID) && (left_valid == MMES_VALID))
        s = (s1 + s2 + 16) >> 5;
    else if (top_valid == MMES_VALID)
        s = (s1 + 8) >> 4;
    else if (left_valid == MMES_VALID)
        s = (s2 + 8) >> 4;
    else
        s = 128;

    // reconstruct y component
    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
        frame_x_16_offset * MB_SIZE;
    idx = 0;
    for (y_coord = 0; y_coord < MB_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < MB_SIZE; x_coord++)
        {
            tmp_value = pMB->best_I16x16_residual[idx++] + s;   //idx = y_coord*MB_SIZE+x_coord 

            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *y = (uint8) tmp_value;

            if (y_coord == (MB_SIZE - 1))
                pSlice->top_y[frame_x_16_offset * MB_SIZE + x_coord] = *y;

            y++;
        }
        pSlice->left_y[y_coord] = *(y - 1);
        y += (width - MB_SIZE);
    }

    //default cb predictor is Intra8x8_DC for chroma
    s = s0 = s1 = s2 = s3 = 0;
    if ((top_valid == MMES_VALID) && (left_valid == MMES_VALID))
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s0 += pSlice->left_cb[idx];
        }
        s0 = (s0 + 4) >> 3;     //top_left

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s1 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s3 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s2 += pSlice->left_cb[idx];
            s3 += pSlice->left_cb[idx];
        }
        s1 = (s1 + 2) >> 2;     //top_right

        s2 = (s2 + 2) >> 2;     //bottom_left

        s3 = (s3 + 4) >> 3;     //bottom_right

    }
    else if (top_valid == MMES_VALID)
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s2 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
        }
        s0 = (s0 + 2) >> 2;     //top_left

        s2 = (s1 + 2) >> 2;     //bottom_left

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s1 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
            s3 += pSlice->top_cb[frame_x_16_offset * chroma_mb_size + idx];
        }
        s1 = (s1 + 2) >> 2;     //top_right

        s3 = (s3 + 2) >> 2;     //bottom_right 

    }
    else if (left_valid == MMES_VALID)
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->left_cb[idx];
            s1 += pSlice->left_cb[idx];
        }
        s0 = (s0 + 2) >> 2;     //top_left

        s1 = (s1 + 2) >> 2;     //top_rigth;

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s2 += pSlice->left_cb[idx];
            s3 += pSlice->left_cb[idx];
        }
        s2 = (s2 + 2) >> 2;     //top_left

        s3 = (s3 + 2) >> 2;     //top_rigth;

    }
    else
        s0 = s1 = s2 = s3 = 128;

    s0 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s0);
    s1 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s1);
    s2 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s2);
    s3 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s3);

    for (y_coord = 0; y_coord < 4; y_coord++)
    {
        for (x_coord = 0; x_coord < 4; x_coord++)
        {
            pMB->cb_residual[y_coord * chroma_mb_size + x_coord] += s0; //top_left

            pMB->cb_residual[y_coord * chroma_mb_size + x_coord + 4] += s1;     //top_right

            pMB->cb_residual[(y_coord + 4) * chroma_mb_size + x_coord] += s2;   //bottom_left

            pMB->cb_residual[(y_coord + 4) * chroma_mb_size + x_coord + 4] += s3;       //bottom_right

        }
    }

    // reconstruct cb component
    idx = 0;
    cb = pCurr_frame->cb +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;
    for (y_coord = 0; y_coord < chroma_mb_size; y_coord++)
    {
        for (x_coord = 0; x_coord < chroma_mb_size; x_coord++)
        {
            tmp_value = pMB->cb_residual[idx++];        // idx = y_coord*chroma_mb_size+x_coord

            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *cb = (uint8) tmp_value;

            if (y_coord == (chroma_mb_size - 1))
                pSlice->top_cb[frame_x_16_offset * chroma_mb_size +
                               x_coord] = *cb;

            cb++;
        }
        pSlice->left_cb[y_coord] = *(cb - 1);

        cb += (chroma_frame_width - chroma_mb_size);
    }

    //default cr predictor is Intra8x8_DC for chroma
    s = s0 = s1 = s2 = s3 = 0;
    if ((top_valid == MMES_VALID) && (left_valid == MMES_VALID))
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s0 += pSlice->left_cr[idx];
        }
        s0 = (s0 + 4) >> 3;     //top_left

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s1 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s3 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s2 += pSlice->left_cr[idx];
            s3 += pSlice->left_cr[idx];
        }
        s1 = (s1 + 2) >> 2;     //top_right

        s2 = (s2 + 2) >> 2;     //bottom_left

        s3 = (s3 + 4) >> 3;     //bottom_right

    }
    else if (top_valid == MMES_VALID)
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s2 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
        }
        s0 = (s0 + 2) >> 2;     //top_left

        s2 = (s1 + 2) >> 2;     //bottom_left

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s1 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
            s3 += pSlice->top_cr[frame_x_16_offset * chroma_mb_size + idx];
        }
        s1 = (s1 + 2) >> 2;     //top_right

        s3 = (s3 + 2) >> 2;     //bottom_right 

    }
    else if (left_valid == MMES_VALID)
    {
        for (idx = 0; idx < 4; idx++)
        {
            s0 += pSlice->left_cr[idx];
            s1 += pSlice->left_cr[idx];
        }
        s0 = (s0 + 2) >> 2;     //top_left

        s1 = (s1 + 2) >> 2;     //top_rigth;

        for (idx = 4; idx < chroma_mb_size; idx++)
        {
            s2 += pSlice->left_cr[idx];
            s3 += pSlice->left_cr[idx];
        }
        s2 = (s2 + 2) >> 2;     //top_left

        s3 = (s3 + 2) >> 2;     //top_rigth;

    }
    else
        s0 = s1 = s2 = s3 = 128;

    s0 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s0);
    s1 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s1);
    s2 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s2);
    s3 = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, s3);

    for (y_coord = 0; y_coord < 4; y_coord++)
    {
        for (x_coord = 0; x_coord < 4; x_coord++)
        {
            pMB->cr_residual[y_coord * chroma_mb_size + x_coord] += s0; //top_left

            pMB->cr_residual[y_coord * chroma_mb_size + x_coord + 4] += s1;     //top_right

            pMB->cr_residual[(y_coord + 4) * chroma_mb_size + x_coord] += s2;   //bottom_left

            pMB->cr_residual[(y_coord + 4) * chroma_mb_size + x_coord + 4] += s3;       //bottom_right

        }
    }

    // reconstruct cr component
    idx = 0;
    cr = pCurr_frame->cr +
        frame_y_16_offset * chroma_mb_size * chroma_frame_width +
        frame_x_16_offset * chroma_mb_size;
    for (y_coord = 0; y_coord < chroma_mb_size; y_coord++)
    {
        for (x_coord = 0; x_coord < chroma_mb_size; x_coord++)
        {
            tmp_value = pMB->cr_residual[idx++];        // idx = y_coord*chroma_mb_size+x_coord

            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *cr = (uint8) tmp_value;

            if (y_coord == (chroma_mb_size - 1))
                pSlice->top_cr[frame_x_16_offset * chroma_mb_size +
                               x_coord] = *cr;

            cr++;
        }
        pSlice->left_cr[y_coord] = *(cr - 1);
        cr += (chroma_frame_width - chroma_mb_size);
    }

    return MMES_NO_ERROR;
}
