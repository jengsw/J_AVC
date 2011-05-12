/* ///////////////////////////////////////////////////////////// */
/*   File: trans.c                                               */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec transform module.          */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/28/2005 Jerry Peng, add location index array for    */
/*                               Luma Intra_4x4                  */
/* ///////////////////////////////////////////////////////////// */
#include <string.h>
#include "../../common/inc/trans.h"

#define DQ_BITS     6
#define DQ_ROUND    (1<<(DQ_BITS-1))

xint
inv_core_transform(int16 * transform_buffer)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   H.264 4x4 DCT-like integer inverse transform.               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    x_coord, y_coord;
    int16   tmp_buffer[4];

    //horizontal transform
    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        tmp_buffer[0] =
            transform_buffer[0 + y_coord * B_SIZE] +
            transform_buffer[2 + y_coord * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[0 + y_coord * B_SIZE] -
            transform_buffer[2 + y_coord * B_SIZE];
        tmp_buffer[2] =
            (transform_buffer[1 + y_coord * B_SIZE] >> 1) -
            transform_buffer[3 + y_coord * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[1 + y_coord * B_SIZE] +
            (transform_buffer[3 + y_coord * B_SIZE] >> 1);

        transform_buffer[0 + y_coord * B_SIZE] =
            tmp_buffer[0] + tmp_buffer[3];
        transform_buffer[3 + y_coord * B_SIZE] =
            tmp_buffer[0] - tmp_buffer[3];
        transform_buffer[1 + y_coord * B_SIZE] =
            tmp_buffer[1] + tmp_buffer[2];
        transform_buffer[2 + y_coord * B_SIZE] =
            tmp_buffer[1] - tmp_buffer[2];
    }

    for (x_coord = 0; x_coord < B_SIZE; x_coord++)
    {
        tmp_buffer[0] =
            transform_buffer[x_coord + 0 * B_SIZE] +
            transform_buffer[x_coord + 2 * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[x_coord + 0 * B_SIZE] -
            transform_buffer[x_coord + 2 * B_SIZE];
        tmp_buffer[2] =
            (transform_buffer[x_coord + 1 * B_SIZE] >> 1) -
            transform_buffer[x_coord + 3 * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[x_coord + 1 * B_SIZE] +
            (transform_buffer[x_coord + 3 * B_SIZE] >> 1);

        transform_buffer[x_coord + 0 * B_SIZE] =
            tmp_buffer[0] + tmp_buffer[3];
        transform_buffer[x_coord + 3 * B_SIZE] =
            tmp_buffer[0] - tmp_buffer[3];
        transform_buffer[x_coord + 1 * B_SIZE] =
            tmp_buffer[1] + tmp_buffer[2];
        transform_buffer[x_coord + 2 * B_SIZE] =
            tmp_buffer[1] - tmp_buffer[2];

        // have some difference to jm90
        transform_buffer[x_coord + 0 * B_SIZE] =
            (transform_buffer[x_coord + 0 * B_SIZE] +
             (int16) DQ_ROUND) >> DQ_BITS;
        transform_buffer[x_coord + 1 * B_SIZE] =
            (transform_buffer[x_coord + 1 * B_SIZE] +
             (int16) DQ_ROUND) >> DQ_BITS;
        transform_buffer[x_coord + 2 * B_SIZE] =
            (transform_buffer[x_coord + 2 * B_SIZE] +
             (int16) DQ_ROUND) >> DQ_BITS;
        transform_buffer[x_coord + 3 * B_SIZE] =
            (transform_buffer[x_coord + 3 * B_SIZE] +
             (int16) DQ_ROUND) >> DQ_BITS;

    }
    return MMES_NO_ERROR;
}

xint
hadamard4x4_transform(int16 * transform_buffer)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   H.264 4x4 Hadamard forward transform.                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    int16   tmp_buffer[4];
    xint    x_coord, y_coord;

    //Horizontal hadamard transform
    for (y_coord = 0; y_coord < 4; y_coord++)
    {
        tmp_buffer[0] =
            transform_buffer[0 + y_coord * B_SIZE] +
            transform_buffer[3 + y_coord * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[1 + y_coord * B_SIZE] +
            transform_buffer[2 + y_coord * B_SIZE];
        tmp_buffer[2] =
            transform_buffer[1 + y_coord * B_SIZE] -
            transform_buffer[2 + y_coord * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[0 + y_coord * B_SIZE] -
            transform_buffer[3 + y_coord * B_SIZE];

        transform_buffer[0 + y_coord * B_SIZE] =
            tmp_buffer[0] + tmp_buffer[1];
        transform_buffer[2 + y_coord * B_SIZE] =
            tmp_buffer[0] - tmp_buffer[1];
        transform_buffer[1 + y_coord * B_SIZE] =
            tmp_buffer[3] + tmp_buffer[2];
        transform_buffer[3 + y_coord * B_SIZE] =
            tmp_buffer[3] - tmp_buffer[2];
    }

    //Vertical hadamard transform
    for (x_coord = 0; x_coord < 4; x_coord++)
    {
        tmp_buffer[0] =
            transform_buffer[x_coord + 0 * B_SIZE] +
            transform_buffer[x_coord + 3 * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[x_coord + 1 * B_SIZE] +
            transform_buffer[x_coord + 2 * B_SIZE];
        tmp_buffer[2] =
            transform_buffer[x_coord + 1 * B_SIZE] -
            transform_buffer[x_coord + 2 * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[x_coord + 0 * B_SIZE] -
            transform_buffer[x_coord + 3 * B_SIZE];

        transform_buffer[x_coord + 0 * B_SIZE] =
            (tmp_buffer[0] + tmp_buffer[1]) >> 1;
        transform_buffer[x_coord + 2 * B_SIZE] =
            (tmp_buffer[0] - tmp_buffer[1]) >> 1;
        transform_buffer[x_coord + 1 * B_SIZE] =
            (tmp_buffer[3] + tmp_buffer[2]) >> 1;
        transform_buffer[x_coord + 3 * B_SIZE] =
            (tmp_buffer[3] - tmp_buffer[2]) >> 1;
    }

    return MMES_NO_ERROR;

}

xint
hadamard4x4_transform_orig(int16 * transform_buffer)
/* ------------------------------------------------------------- */
/*   Author: Modified from hadamard4x4_transform                 *?
/*   Date  : May/20/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   H.264 4x4 Hadamard forward transform.                       */
/*   In order to match JM software, this function is used to     */
/*   give original coefficients                                  */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    int16   tmp_buffer[4];
    xint    x_coord, y_coord;

    //Horizontal hadamard transform
    for (y_coord = 0; y_coord < 4; y_coord++)
    {
        tmp_buffer[0] =
            transform_buffer[0 + y_coord * B_SIZE] +
            transform_buffer[3 + y_coord * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[1 + y_coord * B_SIZE] +
            transform_buffer[2 + y_coord * B_SIZE];
        tmp_buffer[2] =
            transform_buffer[1 + y_coord * B_SIZE] -
            transform_buffer[2 + y_coord * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[0 + y_coord * B_SIZE] -
            transform_buffer[3 + y_coord * B_SIZE];

        transform_buffer[0 + y_coord * B_SIZE] =
            tmp_buffer[0] + tmp_buffer[1];
        transform_buffer[2 + y_coord * B_SIZE] =
            tmp_buffer[0] - tmp_buffer[1];
        transform_buffer[1 + y_coord * B_SIZE] =
            tmp_buffer[3] + tmp_buffer[2];
        transform_buffer[3 + y_coord * B_SIZE] =
            tmp_buffer[3] - tmp_buffer[2];
    }

    //Vertical hadamard transform
    for (x_coord = 0; x_coord < 4; x_coord++)
    {
        tmp_buffer[0] =
            transform_buffer[x_coord + 0 * B_SIZE] +
            transform_buffer[x_coord + 3 * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[x_coord + 1 * B_SIZE] +
            transform_buffer[x_coord + 2 * B_SIZE];
        tmp_buffer[2] =
            transform_buffer[x_coord + 1 * B_SIZE] -
            transform_buffer[x_coord + 2 * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[x_coord + 0 * B_SIZE] -
            transform_buffer[x_coord + 3 * B_SIZE];

        transform_buffer[x_coord + 0 * B_SIZE] =
            (tmp_buffer[0] + tmp_buffer[1]);
        transform_buffer[x_coord + 2 * B_SIZE] =
            (tmp_buffer[0] - tmp_buffer[1]);
        transform_buffer[x_coord + 1 * B_SIZE] =
            (tmp_buffer[3] + tmp_buffer[2]);
        transform_buffer[x_coord + 3 * B_SIZE] =
            (tmp_buffer[3] - tmp_buffer[2]);
    }

    return MMES_NO_ERROR;

}

xint
inv_hadamard_transform(int16 * transform_buffer)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   H.264 4x4 Hadamard inverse transform.                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    int     tmp_buffer[4];
    xint    x_coord, y_coord;

    for (y_coord = 0; y_coord < 4; y_coord++)
    {
        tmp_buffer[0] =
            transform_buffer[0 + y_coord * B_SIZE] +
            transform_buffer[2 + y_coord * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[0 + y_coord * B_SIZE] -
            transform_buffer[2 + y_coord * B_SIZE];
        tmp_buffer[2] =
            transform_buffer[1 + y_coord * B_SIZE] -
            transform_buffer[3 + y_coord * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[1 + y_coord * B_SIZE] +
            transform_buffer[3 + y_coord * B_SIZE];

        transform_buffer[0 + y_coord * B_SIZE] =
            tmp_buffer[0] + tmp_buffer[3];
        transform_buffer[3 + y_coord * B_SIZE] =
            tmp_buffer[0] - tmp_buffer[3];
        transform_buffer[1 + y_coord * B_SIZE] =
            tmp_buffer[1] + tmp_buffer[2];
        transform_buffer[2 + y_coord * B_SIZE] =
            tmp_buffer[1] - tmp_buffer[2];
    }

    for (x_coord = 0; x_coord < 4; x_coord++)
    {
        tmp_buffer[0] =
            transform_buffer[x_coord + 0 * B_SIZE] +
            transform_buffer[x_coord + 2 * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[x_coord + 0 * B_SIZE] -
            transform_buffer[x_coord + 2 * B_SIZE];
        tmp_buffer[2] =
            transform_buffer[x_coord + 1 * B_SIZE] -
            transform_buffer[x_coord + 3 * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[x_coord + 1 * B_SIZE] +
            transform_buffer[x_coord + 3 * B_SIZE];

        transform_buffer[x_coord + 0 * B_SIZE] =
            tmp_buffer[0] + tmp_buffer[3];
        transform_buffer[x_coord + 3 * B_SIZE] =
            tmp_buffer[0] - tmp_buffer[3];
        transform_buffer[x_coord + 1 * B_SIZE] =
            tmp_buffer[1] + tmp_buffer[2];
        transform_buffer[x_coord + 2 * B_SIZE] =
            tmp_buffer[1] - tmp_buffer[2];
    }

    return MMES_NO_ERROR;
}

xint
intra_RDCost_16x16(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : May/20/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Calculate RD cost for Intra 16x16 =                         */
/*   ACs of Hardamard Transform(MB) +                            */
/*   all of Hardamard Trasnform(DC block)                        */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice;
    mb_obj *pMB;
    int16  tmp_transform_buffer[B_SIZE*B_SIZE];
    int16  *b4x4_start;
    xint    y_coord, x_coord, idx;
    xint    chroma_mb_size;
	xint	I16x16_sad2, tmp_I16x16;
	xint    block;
	xint    mb_x_4_offset, mb_y_4_offset;
    enc_cfg     *e_ctrl = pEnc->pCtrl;

    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);
    chroma_mb_size = MB_SIZE / 2;       // for YCbCr420
	I16x16_sad2 = 0;					// for distortion of Intra16x16 mode

    //"hardamard transform" for each luma 4x4 block
    for(block=0; block<NBLOCKS; block++)
    {
        mb_x_4_offset = mb_x_4_idx[block];
        mb_y_4_offset = mb_y_4_idx[block];

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE + 
                               mb_x_4_offset*4;

        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(&(tmp_transform_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }

        hadamard4x4_transform_orig(tmp_transform_buffer);

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE + 
                               mb_x_4_offset*4;

        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(b4x4_start, &(tmp_transform_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }
		// distortion of AC coefficients after transforming 4x4 block
		for( idx=1, tmp_I16x16=0; idx<B_SIZE*B_SIZE; idx++ )
		{
			I16x16_sad2 += ABS( tmp_transform_buffer[idx] );
            tmp_I16x16 += ABS( tmp_transform_buffer[idx] );
		}
    }

    // hadamard transform for luma DC coeff for Intra16x16
    b4x4_start = pMB->best_I16x16_residual;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
#if HW_METHOD
            tmp_transform_buffer[x_coord+y_coord*B_SIZE] = 
                *(b4x4_start + y_coord*B_SIZE*MB_SIZE + x_coord*B_SIZE);
#else
            tmp_transform_buffer[x_coord+y_coord*B_SIZE] = 
                *(b4x4_start + y_coord*B_SIZE*MB_SIZE + x_coord*B_SIZE)/4;
#endif
        }
    }

    hadamard4x4_transform_orig(tmp_transform_buffer);

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
#if HW_METHOD
            tmp_transform_buffer[x_coord+y_coord*B_SIZE] >>= 2;
#endif
            *(b4x4_start + y_coord*B_SIZE*MB_SIZE + x_coord*B_SIZE) = 
                tmp_transform_buffer[x_coord+y_coord*B_SIZE];
        }
    }

	// distortion of all coefficients after transforming 4x4 DC block
	for( idx=0, tmp_I16x16=0; idx<B_SIZE*B_SIZE; idx++ )
	{
		I16x16_sad2 += ABS( tmp_transform_buffer[idx] );
        tmp_I16x16 += ABS( tmp_transform_buffer[idx] );	
    }

    return I16x16_sad2;
}

xint
intra_chroma_RDCost(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : May/26/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Calculate RD cost for Intra chroma =                        */
/*   all of Hardamard Transform(U block) +                       */
/*   all of Hardamard Transform(V block) +                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice;
    mb_obj *pMB;
    int16  tmp_transform_buffer[B_SIZE * B_SIZE];
    int16  *b4x4_start;
    xint    mb_x_4_offset, mb_y_4_offset;
    xint    y_coord;
    xint    chroma_mb_size, sad, IChroma_sad = 0, idx;
	
    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);
    chroma_mb_size = MB_SIZE / 2;       // for YCbCr420

    //"hardamard transform" for chroma cb component
    for (mb_y_4_offset = 0; mb_y_4_offset < (chroma_mb_size / B_SIZE);
         mb_y_4_offset++)
    {
        for (mb_x_4_offset = 0;
             mb_x_4_offset < (chroma_mb_size / B_SIZE); mb_x_4_offset++)
        {		    
            b4x4_start =
                pMB->cb_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            //copy data to temp buffer
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(&(tmp_transform_buffer[y_coord * B_SIZE]),
                       b4x4_start, B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }

            hadamard4x4_transform_orig(tmp_transform_buffer);

            //copy transform coeff. to original residual buffer
            b4x4_start =
                pMB->cb_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(b4x4_start,
                       &(tmp_transform_buffer[y_coord * B_SIZE]),
                       B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }

			// distortion of all coefficients after transforming 4x4 block
			for( sad=0, idx=0; idx<B_SIZE*B_SIZE; idx++ )
			{
				sad += ABS( tmp_transform_buffer[idx] );
			}

			sad = (sad+1)>>1;
			IChroma_sad += sad;
        }
		
    }

	//"hardamard transform" for chroma cr component
    for (mb_y_4_offset = 0; mb_y_4_offset < (chroma_mb_size / B_SIZE);
         mb_y_4_offset++)
    {
        for (mb_x_4_offset = 0;
             mb_x_4_offset < (chroma_mb_size / B_SIZE); mb_x_4_offset++)
        {
            b4x4_start =
                pMB->cr_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            //copy data to temp buffer
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(&(tmp_transform_buffer[y_coord * B_SIZE]),
                       b4x4_start, B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }

            hadamard4x4_transform_orig(tmp_transform_buffer);

            //copy transform coeff. to original residual buffer
            b4x4_start =
                pMB->cr_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(b4x4_start,
                       &(tmp_transform_buffer[y_coord * B_SIZE]),
                       B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }

			// distortion of all coefficients after transforming 4x4 block
			for( sad=0, idx=0; idx<B_SIZE*B_SIZE; idx++ )
			{
				sad += ABS( tmp_transform_buffer[idx] );
			}
			
			sad = (sad+1)>>1;
			IChroma_sad += sad;
        }
		
    }
	
	return IChroma_sad;
}

intra_RDCost_4x4(h264enc_obj * pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : May/20/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Hardamard Transform one 4x4 intra macroblock.               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB;
    xint mb_x_4_offset, mb_y_4_offset;
    xint y_coord, idx;
    int16 tmp_transform_buffer[B_SIZE*B_SIZE];
    int16 *b4x4_start;
	xint I4x4sad = 0;

    pMB = &(pEnc->curr_slice->cmb);
    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    b4x4_start = pMB->best_I4x4_residual+mb_y_4_offset*4*MB_SIZE+mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        memcpy(&(tmp_transform_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
        b4x4_start += MB_SIZE;
    }

    hadamard4x4_transform_orig(tmp_transform_buffer);

    b4x4_start = pMB->best_I4x4_residual+mb_y_4_offset*4*MB_SIZE+mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        memcpy(b4x4_start, &(tmp_transform_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
        b4x4_start += MB_SIZE;
    }

	for( idx=0; idx<B_SIZE*B_SIZE; idx++ )
	{
		I4x4sad += ABS( tmp_transform_buffer[ idx ] );
	}
	I4x4sad = (I4x4sad+1)>>1;
    
    return I4x4sad;
}

xint
inv_trans_16x16(slice_obj *pSlice)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Inverse transform one 16x16 intra MB for decoder.           */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jun/27/2005 Chingho-Ho Chen, modify parameter for           */
/*   integrating decoder                                         */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB;
    int16  tmp_transform_buffer[B_SIZE * B_SIZE];
    int16  *b4x4_start;
    xint    y_coord;
    xint    mb_x_4_offset, mb_y_4_offset;
    xint    block;


    pMB = &(pSlice->cmb);// @chingho 06 27 2005

    for(block=0; block<NBLOCKS; block++)
    {
        mb_x_4_offset = mb_x_4_idx[block];
        mb_y_4_offset = mb_y_4_idx[block];

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE +
                                mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(&(tmp_transform_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }

        inv_core_transform(tmp_transform_buffer);

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE +
                                mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(b4x4_start, &(tmp_transform_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }

    }

    return MMES_NO_ERROR;
}

xint
inv_trans_4x4(slice_obj *pSlice, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Mar/01/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Inverse transform one 16x16 intra MB for decoder.           */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jun/27/2005 Chingho-Ho Chen, modify parameter for           */
/*   integrating decoder                                         */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB;
    xint mb_x_4_offset, mb_y_4_offset;
    xint y_coord;
    int16 tmp_transform_buffer[B_SIZE*B_SIZE];
    int16 *b4x4_start;

    pMB = &(pSlice->cmb);// @chingho 06 27 2005
    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    b4x4_start = pMB->best_I4x4_residual+mb_y_4_offset*4*MB_SIZE+mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        memcpy(&(tmp_transform_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
        b4x4_start += MB_SIZE;
    }

    inv_core_transform(tmp_transform_buffer);

    b4x4_start = pMB->best_I4x4_residual+mb_y_4_offset*4*MB_SIZE+mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        memcpy(b4x4_start, &(tmp_transform_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
        b4x4_start += MB_SIZE;
    }
    
    return MMES_NO_ERROR;
}

xint
inv_trans_chroma(slice_obj *pSlice)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Inverse transform one chroma intra MB for decoder.          */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jun/27/2005 Chingho-Ho Chen, modify parameter for           */
/*   integrating decoder                                         */
/*                                                               */
/* ------------------------------------------------------------- */
{
//    slice_obj *pSlice;
    mb_obj *pMB;
    int16  tmp_transform_buffer[B_SIZE * B_SIZE];
    int16  *b4x4_start;
    xint    mb_x_4_offset, mb_y_4_offset;
    xint    y_coord;
    xint    chroma_mb_size;

//    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);
    chroma_mb_size = MB_SIZE / 2;       // for YCbCr420

    // Cb component
    for (mb_y_4_offset = 0; mb_y_4_offset < (chroma_mb_size / B_SIZE);
         mb_y_4_offset++)
    {
        for (mb_x_4_offset = 0;
             mb_x_4_offset < (chroma_mb_size / B_SIZE); mb_x_4_offset++)
        {
            // copy data to temp buffer
            b4x4_start =
                pMB->cb_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(&(tmp_transform_buffer[y_coord * B_SIZE]),
                       b4x4_start, B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }

            inv_core_transform(tmp_transform_buffer);

            b4x4_start =
                pMB->cb_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(b4x4_start,
                       &(tmp_transform_buffer[y_coord * B_SIZE]),
                       B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }
        }
    }

    // Cr component
    for (mb_y_4_offset = 0; mb_y_4_offset < (chroma_mb_size / B_SIZE);
         mb_y_4_offset++)
    {
        for (mb_x_4_offset = 0;
             mb_x_4_offset < (chroma_mb_size / B_SIZE); mb_x_4_offset++)
        {
            // copy data to temp buffer
            b4x4_start =
                pMB->cr_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(&(tmp_transform_buffer[y_coord * B_SIZE]),
                       b4x4_start, B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }

            inv_core_transform(tmp_transform_buffer);

            b4x4_start =
                pMB->cr_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(b4x4_start,
                       &(tmp_transform_buffer[y_coord * B_SIZE]),
                       B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }
        }
    }

    return MMES_NO_ERROR;
}


xint
core_transform(int16 * transform_buffer)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   H.264 4x4 DCT-like integer forward transform.               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    int16   tmp_buffer[B_SIZE];
    xint    y_coord, x_coord;

    //Horizontal Transform
    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {

        tmp_buffer[0] =
            transform_buffer[0 + y_coord * B_SIZE] +
            transform_buffer[3 + y_coord * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[0 + y_coord * B_SIZE] -
            transform_buffer[3 + y_coord * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[1 + y_coord * B_SIZE] +
            transform_buffer[2 + y_coord * B_SIZE];
        tmp_buffer[2] =
            transform_buffer[1 + y_coord * B_SIZE] -
            transform_buffer[2 + y_coord * B_SIZE];

        transform_buffer[0 + y_coord * B_SIZE] =
            tmp_buffer[0] + tmp_buffer[1];
        transform_buffer[2 + y_coord * B_SIZE] =
            tmp_buffer[0] - tmp_buffer[1];
        transform_buffer[1 + y_coord * B_SIZE] =
            tmp_buffer[3] * 2 + tmp_buffer[2];
        transform_buffer[3 + y_coord * B_SIZE] =
            tmp_buffer[3] - tmp_buffer[2] * 2;
    }

    //Vertical transform
    for (x_coord = 0; x_coord < B_SIZE; x_coord++)
    {

        tmp_buffer[0] =
            transform_buffer[x_coord + 0 * B_SIZE] +
            transform_buffer[x_coord + 3 * B_SIZE];
        tmp_buffer[3] =
            transform_buffer[x_coord + 0 * B_SIZE] -
            transform_buffer[x_coord + 3 * B_SIZE];
        tmp_buffer[1] =
            transform_buffer[x_coord + 1 * B_SIZE] +
            transform_buffer[x_coord + 2 * B_SIZE];
        tmp_buffer[2] =
            transform_buffer[x_coord + 1 * B_SIZE] -
            transform_buffer[x_coord + 2 * B_SIZE];

        transform_buffer[x_coord + 0 * B_SIZE] =
            tmp_buffer[0] + tmp_buffer[1];
        transform_buffer[x_coord + 2 * B_SIZE] =
            tmp_buffer[0] - tmp_buffer[1];
        transform_buffer[x_coord + 1 * B_SIZE] =
            tmp_buffer[3] * 2 + tmp_buffer[2];
        transform_buffer[x_coord + 3 * B_SIZE] =
            tmp_buffer[3] - tmp_buffer[2] * 2;
    }

    return MMES_NO_ERROR;
}

xint
trans_16x16(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Transform one 16x16 intra macroblock.                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Chun-Jen Tsai, Change the name from trans() to  */
/*               trans_16x16(). This is a temporary change.      */
/*   Apr/14/205  Jerry Peng, Change 4x4 block scan order     */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB;
    int16  tmp_transform_buffer[B_SIZE * B_SIZE];
    int16  *b4x4_start;
    xint    y_coord, x_coord;
    xint    block;
    xint    mb_x_4_offset, mb_y_4_offset;

    pMB = &(pEnc->curr_slice->cmb);

    //"core transform" for each luma 4x4 block
    for(block=0; block<NBLOCKS; block++)
    {
        mb_x_4_offset = mb_x_4_idx[block];
        mb_y_4_offset = mb_y_4_idx[block];

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE + 
                               mb_x_4_offset*4;

        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(&(tmp_transform_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }

        core_transform(tmp_transform_buffer);

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE + 
                               mb_x_4_offset*4;

        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(b4x4_start, &(tmp_transform_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }
    }

    // hadamard transform for luma DC coeff for Intra16x16
    b4x4_start = pMB->best_I16x16_residual;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            tmp_transform_buffer[x_coord+y_coord*B_SIZE] = 
                *(b4x4_start + y_coord*B_SIZE*MB_SIZE + x_coord*B_SIZE);
        }
    }
	
    hadamard4x4_transform(tmp_transform_buffer);

    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            *(b4x4_start + y_coord*B_SIZE*MB_SIZE + x_coord*B_SIZE) = 
                tmp_transform_buffer[x_coord+y_coord*B_SIZE];
        }
    }
    
    return MMES_NO_ERROR;
}

xint
trans_4x4(h264enc_obj * pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Mar/01/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Transform one 4x4 intra macroblock.                         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB;
    xint mb_x_4_offset, mb_y_4_offset;
    xint y_coord;
    int16 tmp_transform_buffer[B_SIZE*B_SIZE];
    int16 *b4x4_start;

    pMB = &(pEnc->curr_slice->cmb);
    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];
    b4x4_start = pMB->best_I4x4_residual+mb_y_4_offset*4*MB_SIZE+mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        memcpy(&(tmp_transform_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
        b4x4_start += MB_SIZE;
    }

    core_transform(tmp_transform_buffer);

    b4x4_start = pMB->best_I4x4_residual+mb_y_4_offset*4*MB_SIZE+mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        memcpy(b4x4_start, &(tmp_transform_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
        b4x4_start += MB_SIZE;
    }
    
    return MMES_NO_ERROR;
}

xint
trans_chroma(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Transform one intra chroma macroblock.                      */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice;
    mb_obj *pMB;
    int16  tmp_transform_buffer[B_SIZE * B_SIZE];
    int16  *b4x4_start;
    xint    mb_x_4_offset, mb_y_4_offset;
    xint    y_coord;
    xint    chroma_mb_size;

    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);
    chroma_mb_size = MB_SIZE / 2;       // for YCbCr420

    //"core transform" for chroma cb component
    for (mb_y_4_offset = 0; mb_y_4_offset < (chroma_mb_size / B_SIZE);
         mb_y_4_offset++)
    {
        for (mb_x_4_offset = 0;
             mb_x_4_offset < (chroma_mb_size / B_SIZE); mb_x_4_offset++)
        {
            b4x4_start =
                pMB->cb_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            //copy data to temp buffer
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(&(tmp_transform_buffer[y_coord * B_SIZE]),
                       b4x4_start, B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }

            core_transform(tmp_transform_buffer);

            //copy transform coeff. to original residual buffer
            b4x4_start =
                pMB->cb_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(b4x4_start,
                       &(tmp_transform_buffer[y_coord * B_SIZE]),
                       B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }
        }

    }

    // hadamard2x2 transform for DC chroma coeff;
    tmp_transform_buffer[0] = pMB->cb_residual[0 + 0 * chroma_mb_size]; //(0, 0)

    tmp_transform_buffer[1] = pMB->cb_residual[4 + 0 * chroma_mb_size]; //(4, 0)

    tmp_transform_buffer[2] = pMB->cb_residual[0 + 4 * chroma_mb_size]; //(0, 4)

    tmp_transform_buffer[3] = pMB->cb_residual[4 + 4 * chroma_mb_size]; //(4, 4)

    pMB->cb_residual[0 + 0 * chroma_mb_size] =
        tmp_transform_buffer[0] + tmp_transform_buffer[1] +
        tmp_transform_buffer[2] + tmp_transform_buffer[3];
    pMB->cb_residual[4 + 0 * chroma_mb_size] =
        tmp_transform_buffer[0] - tmp_transform_buffer[1] +
        tmp_transform_buffer[2] - tmp_transform_buffer[3];
    pMB->cb_residual[0 + 4 * chroma_mb_size] =
        tmp_transform_buffer[0] + tmp_transform_buffer[1] -
        tmp_transform_buffer[2] - tmp_transform_buffer[3];
    pMB->cb_residual[4 + 4 * chroma_mb_size] =
        tmp_transform_buffer[0] - tmp_transform_buffer[1] -
        tmp_transform_buffer[2] + tmp_transform_buffer[3];

    //"core transform" for chroma cr component
    for (mb_y_4_offset = 0; mb_y_4_offset < (chroma_mb_size / B_SIZE);
         mb_y_4_offset++)
    {
        for (mb_x_4_offset = 0;
             mb_x_4_offset < (chroma_mb_size / B_SIZE); mb_x_4_offset++)
        {
            b4x4_start =
                pMB->cr_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            //copy data to temp buffer
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(&(tmp_transform_buffer[y_coord * B_SIZE]),
                       b4x4_start, B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }

            core_transform(tmp_transform_buffer);

            //copy transform coeff. to original residual buffer
            b4x4_start =
                pMB->cr_residual +
                mb_y_4_offset * B_SIZE * chroma_mb_size +
                mb_x_4_offset * B_SIZE;
            for (y_coord = 0; y_coord < B_SIZE; y_coord++)
            {
                memcpy(b4x4_start,
                       &(tmp_transform_buffer[y_coord * B_SIZE]),
                       B_SIZE * sizeof(int16));
                b4x4_start += chroma_mb_size;
            }
        }

    }

    // hadamard2x2 transform for DC chroma coeff;
    tmp_transform_buffer[0] = pMB->cr_residual[0 + 0 * chroma_mb_size]; //(0, 0)

    tmp_transform_buffer[1] = pMB->cr_residual[4 + 0 * chroma_mb_size]; //(4, 0)

    tmp_transform_buffer[2] = pMB->cr_residual[0 + 4 * chroma_mb_size]; //(0, 4)

    tmp_transform_buffer[3] = pMB->cr_residual[4 + 4 * chroma_mb_size]; //(4, 4)

    pMB->cr_residual[0 + 0 * chroma_mb_size] =
        tmp_transform_buffer[0] + tmp_transform_buffer[1] +
        tmp_transform_buffer[2] + tmp_transform_buffer[3];
    pMB->cr_residual[4 + 0 * chroma_mb_size] =
        tmp_transform_buffer[0] - tmp_transform_buffer[1] +
        tmp_transform_buffer[2] - tmp_transform_buffer[3];
    pMB->cr_residual[0 + 4 * chroma_mb_size] =
        tmp_transform_buffer[0] + tmp_transform_buffer[1] -
        tmp_transform_buffer[2] - tmp_transform_buffer[3];
    pMB->cr_residual[4 + 4 * chroma_mb_size] =
        tmp_transform_buffer[0] - tmp_transform_buffer[1] -
        tmp_transform_buffer[2] + tmp_transform_buffer[3];

    return MMES_NO_ERROR;
}

