/* ///////////////////////////////////////////////////////////// */
/*   File: intra_comp.c                                          */
/*   Author: Jerry Peng                                       */
/*   Date: Feb/15/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 intra compensation module.       */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ///////////////////////////////////////////////////////////// */

#include <string.h>
#include "../../common/inc/intra_comp.h"

xint enc_intra_16x16_compensation(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Feb/15/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra luma 16x16 compensation.               */
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
    uint8  *y;
    xint    frame_x_16_offset, frame_y_16_offset;
    xint    width;
    xint    x_coord, y_coord;
    int16   tmp_value;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    width = pEnc->width;
    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

	// @chingho 06 02 2005
	pSlice->upleft_y[0] = pSlice->top_y[ (frame_x_16_offset+1)*MB_SIZE-1 ];

    // reconstruct y component
    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
        frame_x_16_offset * MB_SIZE;
    for (y_coord = 0; y_coord < MB_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < MB_SIZE; x_coord++)
        {
            tmp_value =
                pMB->best_I16x16_residual[y_coord * MB_SIZE +
                                          x_coord] + (*y);
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *y = (uint8) tmp_value;

			// @chingho 06 02 2005
			if( y_coord == 3 || y_coord == 7 || y_coord == 11 )
			{
				if( x_coord == (MB_SIZE-1) )
					pSlice->upleft_y[ (y_coord+1)/4 ] = *y;
			}

            if (y_coord == (MB_SIZE - 1))
                pSlice->top_y[frame_x_16_offset * MB_SIZE + x_coord] = *y;

            y++;
        }

        pSlice->left_y[y_coord] = *(y - 1);
        y += (width - MB_SIZE);
    }

	for (x_coord = 0; x_coord < (MB_SIZE/B_SIZE); x_coord++)
	{
		pSlice->top_mode[frame_x_16_offset * (MB_SIZE/B_SIZE)+x_coord] = INTRA_4x4_DC;
		pSlice->left_mode[x_coord] = INTRA_4x4_DC;
	}

    return MMES_NO_ERROR;
}

xint enc_intra_4x4_compensation(h264enc_obj *pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra luma 4x4 compensation.                 */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   block -> [I] which block of the 16 4x4 blocks to be encoded */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Jerry Peng, move the loop of coding 16 4x4   */
/*               blocks one level up (to encode_intra_mb()).     */
/*               Therefore, the parameter 'block' must be passed */
/*               to lower level functions.                       */
/*   Feb/24/2005 Cheng-Nan Chiu, rerwrite the function to allow  */
/*               proper intra compensation for Luma 4x4          */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj    *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width, tmp_value;
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

    // reconstruct luma 4x4 
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
			// @chingho 06 20 2005
			tmp_value = *block_start;
			tmp_value += residual_start[x_coord + y_coord*MB_SIZE];
			tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *block_start = (uint8) tmp_value;
			//*block_start += residual_start[x_coord + y_coord*MB_SIZE];
		
			// for further intra prediction value, p[-1, -1]
			// @chingho May 26 2005
			if( x_coord == 3 && y_coord == 3 )
			{
				pSlice->upleft_y[mb_y_4_offset] = pSlice->top_y[frame_x_4_offset*4+x_coord];
			}

            if(y_coord == (B_SIZE-1) )
            {	
                pSlice->top_y[frame_x_4_offset*4+x_coord] = *block_start;
            }
            
            block_start ++;
        }
        pSlice->left_y[mb_y_4_offset*4+y_coord] = *(block_start - 1);
        block_start += (frame_width - B_SIZE);
    }

	pSlice->top_valid[frame_x_4_offset] = MMES_VALID;// @chingho 06 16 2005

	// for further intra prediction value, p[-1, -1]
	// @chingho 06 24 2005
	if( frame_y_4_offset == 0 )		// top 4x4 block in the picture
		pSlice->upleft_valid[ mb_y_4_offset ] = MMES_INVALID;
	else if( pSlice->no_mbs < pEnc->nx_mb && mb_y_4_offset == 0 )	// top 4x4 block in slice
		pSlice->upleft_valid[ mb_y_4_offset ] = MMES_INVALID;
	else
		pSlice->upleft_valid[ mb_y_4_offset ] = MMES_VALID;

    if(frame_x_4_offset == ( (pEnc->nx_mb)*4 - 1) )     //leftest block4x4 in the picture
        pSlice->left_valid[mb_y_4_offset] = MMES_INVALID;
    else
        pSlice->left_valid[mb_y_4_offset] = MMES_VALID;

    return MMES_NO_ERROR;
    
}

xint
enc_intra_chroma_compensation(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma compensation.                   */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/24/2005 Cheng-Nan Chiu, rewrite the function to allow   */
/*               proper intra compensation for Luma16x16         */
/* ------------------------------------------------------------- */
{
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj    *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint x_coord, y_coord;
    int16 tmp_value;
    uint8 *cb, *cr;

    pCurr_frame = pEnc->curf;
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    frame_x_16_offset = pMB->id % pEnc->nx_mb;
    frame_y_16_offset = pMB->id / pEnc->nx_mb;

    // compute chroma size for YCbCr 4:2:0 format
    chroma_frame_width = pEnc->width / 2;
    chroma_mb_size = MB_SIZE / 2;

    // reconstruct chroma component
    cb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    cr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;

    for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
    {
        for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
        {
            // cb component
            tmp_value = pMB->cb_residual[x_coord + y_coord*chroma_mb_size] + (*cb);
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *cb = (uint8) tmp_value;
			// for further intra prediction value, p[-1, -1]
			// @chingho May 31 2005
			if( x_coord == chroma_mb_size-1 && y_coord == chroma_mb_size-1 )
				pSlice->upleft_cb = pSlice->top_cb[frame_x_16_offset*chroma_mb_size + x_coord];
            if(y_coord==(chroma_mb_size-1))
                pSlice->top_cb[frame_x_16_offset*chroma_mb_size + x_coord] = *cb;
            cb++;

            // cr component
            tmp_value = pMB->cr_residual[x_coord + y_coord*chroma_mb_size] + (*cr);
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *cr = (uint8) tmp_value;
			// for further intra prediction value, p[-1, -1]
			// @chingho May 31 2005
			if( x_coord == chroma_mb_size-1 && y_coord == chroma_mb_size-1 )
				pSlice->upleft_cr = pSlice->top_cr[frame_x_16_offset*chroma_mb_size + x_coord];
            if(y_coord==(chroma_mb_size-1))
                pSlice->top_cr[frame_x_16_offset*chroma_mb_size + x_coord] = *cr;
            cr++;
        }
        pSlice->left_cb[y_coord] = *(cb-1);
        pSlice->left_cr[y_coord] = *(cr-1);
        cb += (chroma_frame_width-chroma_mb_size);
        cr += (chroma_frame_width-chroma_mb_size);
    }

    return MMES_NO_ERROR;
}

xint
dec_intra_16x16_compensation(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Ching-Ho Chen                                       */
/*   Date  : Jul/17/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra luma 16x16 compensation.               */
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
    uint8  *y;
    xint    frame_x_16_offset, frame_y_16_offset;
    xint    width;
    xint    x_coord, y_coord;
    int16   tmp_value;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    width = pDec->width;
    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

	// @chingho 06 02 2005
	pSlice->upleft_y[0] = pSlice->top_y[ (frame_x_16_offset+1)*MB_SIZE-1 ];

    // reconstruct y component
    y = pCurr_frame->y + frame_y_16_offset * MB_SIZE * width +
        frame_x_16_offset * MB_SIZE;
    for (y_coord = 0; y_coord < MB_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < MB_SIZE; x_coord++)
        {
            tmp_value =
                pMB->best_I16x16_residual[y_coord * MB_SIZE +
                                          x_coord] + (*y);
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *y = (uint8) tmp_value;

			// @chingho 06 02 2005
			if( y_coord == 3 || y_coord == 7 || y_coord == 11 )
			{
				if( x_coord == (MB_SIZE-1) )
					pSlice->upleft_y[ (y_coord+1)/4 ] = *y;
			}

            if (y_coord == (MB_SIZE - 1))
                pSlice->top_y[frame_x_16_offset * MB_SIZE + x_coord] = *y;

            y++;
        }

        pSlice->left_y[y_coord] = *(y - 1);
        y += (width - MB_SIZE);
    }

	for (x_coord = 0; x_coord < (MB_SIZE/B_SIZE); x_coord++)
	{
		pSlice->top_mode[frame_x_16_offset * (MB_SIZE/B_SIZE)+x_coord] = INTRA_4x4_DC;
		pSlice->left_mode[x_coord] = INTRA_4x4_DC;
	}    

    return MMES_NO_ERROR;
}

xint dec_intra_4x4_compensation(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Ching-Ho Chen                                       */
/*   Date  : Jun/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra luma 4x4 compensation.                 */
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
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj    *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width, tmp_value;
    uint8 *block_start;
    int16 *residual_start;
    xint x_coord, y_coord;

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

    // reconstruct luma 4x4 
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
			// @chingho 06 20 2005
			tmp_value = *block_start;
			tmp_value += residual_start[x_coord + y_coord*MB_SIZE];
			tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *block_start = (uint8) tmp_value;
			//*block_start += residual_start[x_coord + y_coord*MB_SIZE];
			if( x_coord == 3 && y_coord == 3 )
			{
				pSlice->upleft_y[mb_y_4_offset] = pSlice->top_y[frame_x_4_offset*4+x_coord];
			}

            if(y_coord == (B_SIZE-1) )
            {	
                pSlice->top_y[frame_x_4_offset*4+x_coord] = *block_start;
            }
            
            block_start ++;
        }
        pSlice->left_y[mb_y_4_offset*4+y_coord] = *(block_start - 1);
        block_start += (frame_width - B_SIZE);
    }

	pSlice->top_valid[frame_x_4_offset] = MMES_VALID;// @chingho 06 16 2005
	pSlice->top_mb_intra[frame_x_4_offset] = MMES_VALID;// @chingho 11 07 2005

	// for further intra prediction value, p[-1, -1]
	// @chingho 06 24 2005
	if( frame_y_4_offset == 0 )		// top 4x4 block in the picture
		pSlice->upleft_valid[ mb_y_4_offset ] = MMES_INVALID;
	else if( pSlice->no_mbs < pDec->nx_mb && mb_y_4_offset == 0 )	// top 4x4 block in slice
		pSlice->upleft_valid[ mb_y_4_offset ] = MMES_INVALID;
	else
		pSlice->upleft_valid[ mb_y_4_offset ] = MMES_VALID;

    if(frame_x_4_offset == ( (pDec->nx_mb)*4 - 1) )     //leftest block4x4 in the picture
        pSlice->left_valid[mb_y_4_offset] = MMES_INVALID;
    else
        pSlice->left_valid[mb_y_4_offset] = MMES_VALID;

    if( frame_x_4_offset == ( (pDec->nx_mb)*4 - 1) )     //leftest block4x4 in the picture
        pSlice->left_mb_intra[mb_y_4_offset] = MMES_INVALID;
    else
        pSlice->left_mb_intra[mb_y_4_offset] = MMES_VALID;

    return MMES_NO_ERROR;
    
}

xint
dec_intra_chroma_compensation(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Ching-Ho Chen                                       */
/*   Date  : Jun/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of intra chroma compensation.                   */
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
    mb_obj    *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint x_coord, y_coord;
    int16 tmp_value;
    uint8 *cb, *cr;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    // compute chroma size for YCbCr 4:2:0 format
    chroma_frame_width = pDec->width / 2;
    chroma_mb_size = MB_SIZE / 2;

    // reconstruct chroma component
    cb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    cr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;

    for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
    {
        for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
        {
            // cb component
            tmp_value = pMB->cb_residual[x_coord + y_coord*chroma_mb_size] + (*cb);
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *cb = (uint8) tmp_value;
			// for further intra prediction value, p[-1, -1]
			// @chingho May 31 2005
			if( x_coord == chroma_mb_size-1 && y_coord == chroma_mb_size-1 )
				pSlice->upleft_cb = pSlice->top_cb[frame_x_16_offset*chroma_mb_size + x_coord];
            if(y_coord==(chroma_mb_size-1))
                pSlice->top_cb[frame_x_16_offset*chroma_mb_size + x_coord] = *cb;
            cb++;

            // cr component
            tmp_value = pMB->cr_residual[x_coord + y_coord*chroma_mb_size] + (*cr);
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *cr = (uint8) tmp_value;
			// for further intra prediction value, p[-1, -1]
			// @chingho May 31 2005
			if( x_coord == chroma_mb_size-1 && y_coord == chroma_mb_size-1 )
				pSlice->upleft_cr = pSlice->top_cr[frame_x_16_offset*chroma_mb_size + x_coord];
            if(y_coord==(chroma_mb_size-1))
                pSlice->top_cr[frame_x_16_offset*chroma_mb_size + x_coord] = *cr;
            cr++;
        }
        pSlice->left_cb[y_coord] = *(cb-1);
        pSlice->left_cr[y_coord] = *(cr-1);
        cb += (chroma_frame_width-chroma_mb_size);
        cr += (chroma_frame_width-chroma_mb_size);
    }

    return MMES_NO_ERROR;
}

xint dec_inter_4x4_compensation(h264dec_obj *pDec, xint block)
/* ------------------------------------------------------------- */
/*   Author: Ching-Ho Chen                                       */
/*   Date  : Jan/13/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of inter luma 4x4 compensation.                 */
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
    frame_obj *pCurr_frame;
    slice_obj *pSlice;
    mb_obj    *pMB;
    xint frame_x_16_offset, frame_x_4_offset;
    xint frame_y_16_offset, frame_y_4_offset;
    xint mb_x_4_offset, mb_y_4_offset;
    xint frame_width, tmp_value;
    uint8 *block_start;
    int16 *residual_start;
    xint x_coord, y_coord;

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

    // reconstruct luma 4x4 
    block_start = pCurr_frame->y + frame_y_4_offset*4*frame_width + frame_x_4_offset*4;
    residual_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
			// @chingho 06 20 2005
			tmp_value = *block_start;
			tmp_value += residual_start[x_coord + y_coord*MB_SIZE];
			tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *block_start = (uint8) tmp_value;
			// for further intra prediction value, p[-1, -1]
			if( x_coord == 3 && y_coord == 3 )
			{
				pSlice->upleft_y[mb_y_4_offset] = pSlice->top_y[frame_x_4_offset*4+x_coord];
			}

            if(y_coord == (B_SIZE-1) )
            {	
                pSlice->top_y[frame_x_4_offset*4+x_coord] = *block_start;
            }
            
            block_start ++;
        }

        pSlice->left_y[mb_y_4_offset*4+y_coord] = *(block_start - 1);
        block_start += (frame_width - B_SIZE);
    }

	pSlice->top_valid[frame_x_4_offset] = MMES_VALID;
	pSlice->top_mb_intra[frame_x_4_offset] = MMES_INVALID;

	// for further intra prediction value, p[-1, -1]
	if( frame_y_4_offset == 0 )		// top 4x4 block in the picture
		pSlice->upleft_valid[ mb_y_4_offset ] = MMES_INVALID;
	else if( pSlice->no_mbs < pDec->nx_mb && mb_y_4_offset == 0 )	// top 4x4 block in slice
		pSlice->upleft_valid[ mb_y_4_offset ] = MMES_INVALID;
	else
		pSlice->upleft_valid[ mb_y_4_offset ] = MMES_VALID;

    if(frame_x_4_offset == ( (pDec->nx_mb)*4 - 1) )     //leftest block4x4 in the picture
        pSlice->left_valid[mb_y_4_offset] = MMES_INVALID;
    else
        pSlice->left_valid[mb_y_4_offset] = MMES_VALID;

    pSlice->left_mb_intra[mb_y_4_offset] = MMES_INVALID;

    return MMES_NO_ERROR;
    
}

xint
dec_inter_chroma_compensation(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Ching-Ho Chen                                       */
/*   Date  : Jan/13/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Computation of inter chroma compensation.                   */
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
    mb_obj    *pMB;
    xint frame_x_16_offset, frame_y_16_offset;
    xint chroma_frame_width, chroma_mb_size;
    xint x_coord, y_coord;
    int16 tmp_value;
    uint8 *cb, *cr;

    pCurr_frame = pDec->curf;
    pSlice = pDec->curr_slice;
    pMB = &(pSlice->cmb);

    frame_x_16_offset = pMB->id % pDec->nx_mb;
    frame_y_16_offset = pMB->id / pDec->nx_mb;

    // compute chroma size for YCbCr 4:2:0 format
    chroma_frame_width = pDec->width / 2;
    chroma_mb_size = MB_SIZE / 2;

    // reconstruct chroma component
    cb = pCurr_frame->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
    cr = pCurr_frame->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;

    for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
    {
        for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
        {
            // cb component
            tmp_value = pMB->cb_residual[x_coord + y_coord*chroma_mb_size] + (*cb);
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *cb = (uint8) tmp_value;
			// for further intra prediction value, p[-1, -1]

			if( x_coord == chroma_mb_size-1 && y_coord == chroma_mb_size-1 )
				pSlice->upleft_cb = pSlice->top_cb[frame_x_16_offset*chroma_mb_size + x_coord];
            if(y_coord==(chroma_mb_size-1))
                pSlice->top_cb[frame_x_16_offset*chroma_mb_size + x_coord] = *cb;
            cb++;

            // cr component
            tmp_value = pMB->cr_residual[x_coord + y_coord*chroma_mb_size] + (*cr);
            tmp_value = CLIP3(MIN_PIXEL_VALUE, MAX_PIXEL_VALUE, tmp_value);
            *cr = (uint8) tmp_value;
			// for further intra prediction value, p[-1, -1]

			if( x_coord == chroma_mb_size-1 && y_coord == chroma_mb_size-1 )
				pSlice->upleft_cr = pSlice->top_cr[frame_x_16_offset*chroma_mb_size + x_coord];
            if(y_coord==(chroma_mb_size-1))
                pSlice->top_cr[frame_x_16_offset*chroma_mb_size + x_coord] = *cr;
            cr++;
        }
        pSlice->left_cb[y_coord] = *(cb-1);
        pSlice->left_cr[y_coord] = *(cr-1);
        cb += (chroma_frame_width-chroma_mb_size);
        cr += (chroma_frame_width-chroma_mb_size);
    }

    return MMES_NO_ERROR;
}

