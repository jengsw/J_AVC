/* ///////////////////////////////////////////////////////////// */
/*   File: quant.c                                               */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec quantization module.       */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/*   Multimedia Embedded Systems Lab.                            */
/*   Department of Computer Science and Information engineering  */
/*   National Chiao Tung University, Hsinchu 300, Taiwan         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ///////////////////////////////////////////////////////////// */
#include <string.h>
#include "../../common/inc/quant.h"
#include "../../common/inc/trans.h"

//copy from jm90 ...CNC
#define Q_BITS          15
#define DQ_BITS         6
#define DQ_ROUND        (1<<(DQ_BITS-1))
//~CNC

#define Cb              0
#define Cr              1

#define ABS(X)  ((X)> 0 ? (X) : -(X))
#define SIGN(X,Y) ((Y)>0 ? (X) : -(X))

//copy from jm90 ...CNC
static const int16 quant_coef[6][4][4] = {
    {
     {13107, 8066, 13107, 8066},
     {8066, 5243, 8066, 5243},
     {13107, 8066, 13107, 8066},
     {8066, 5243, 8066, 5243}},
    {
     {11916, 7490, 11916, 7490},
     {7490, 4660, 7490, 4660},
     {11916, 7490, 11916, 7490},
     {7490, 4660, 7490, 4660}},
    {
     {10082, 6554, 10082, 6554},
     {6554, 4194, 6554, 4194},
     {10082, 6554, 10082, 6554},
     {6554, 4194, 6554, 4194}},
    {
     {9362, 5825, 9362, 5825},
     {5825, 3647, 5825, 3647},
     {9362, 5825, 9362, 5825},
     {5825, 3647, 5825, 3647}},
    {
     {8192, 5243, 8192, 5243},
     {5243, 3355, 5243, 3355},
     {8192, 5243, 8192, 5243},
     {5243, 3355, 5243, 3355}},
    {
     {7282, 4559, 7282, 4559},
     {4559, 2893, 4559, 2893},
     {7282, 4559, 7282, 4559},
     {4559, 2893, 4559, 2893}}
};

static const int16 dequant_coef[6][4][4] = {
    {
     {10, 13, 10, 13},
     {13, 16, 13, 16},
     {10, 13, 10, 13},
     {13, 16, 13, 16}},
    {
     {11, 14, 11, 14},
     {14, 18, 14, 18},
     {11, 14, 11, 14},
     {14, 18, 14, 18}},
    {
     {13, 16, 13, 16},
     {16, 20, 16, 20},
     {13, 16, 13, 16},
     {16, 20, 16, 20}},
    {
     {14, 18, 14, 18},
     {18, 23, 18, 23},
     {14, 18, 14, 18},
     {18, 23, 18, 23}},
    {
     {16, 20, 16, 20},
     {20, 25, 20, 25},
     {16, 20, 16, 20},
     {20, 25, 20, 25}},
    {
     {18, 23, 18, 23},
     {23, 29, 23, 29},
     {18, 23, 18, 23},
     {23, 29, 23, 29}}
};

//! make chroma QP from quant
const uint8 QP_SCALE_CR[52] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
    28, 29, 29, 30, 31, 32, 32, 33, 34, 34, 35, 35, 36, 36, 37, 37,
    37, 38, 38, 38, 39, 39, 39, 39
};

//  May 30 2005
//! array used to find expencive coefficients
const xint COEFF_COST[2][16] =
{
  {3,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0},
  {9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9}
};

//! single scan pattern
const xint SNGL_SCAN[16][2] =
{
  {0,0},{1,0},{0,1},{0,2},
  {1,1},{2,0},{3,0},{2,1},
  {1,2},{0,3},{1,3},{2,2},
  {3,1},{3,2},{2,3},{3,3}
};

//~CNC

int16   LevelScaleLuma4x4_Intra[6][4][4];
int16   LevelScaleChroma4x4_Intra[2][6][4][4];

int16   InvLevelScaleLuma4x4_Intra[6][4][4];
int16   InvLevelScaleChroma4x4_Intra[2][6][4][4];

xint
init_quant_matrix()
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Initialize quantization matrix.                             */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    k, j, i;

    for (k = 0; k < 6; k++)
    {
        for (j = 0; j < B_SIZE; j++)
        {
            for (i = 0; i < B_SIZE; i++)
            {
                LevelScaleLuma4x4_Intra[k][j][i] = quant_coef[k][j][i];
                InvLevelScaleLuma4x4_Intra[k][j][i] =
                    dequant_coef[k][j][i] << 4;

                LevelScaleChroma4x4_Intra[Cb][k][j][i] =
                    quant_coef[k][j][i];
                InvLevelScaleChroma4x4_Intra[Cb][k][j][i] =
                    dequant_coef[k][j][i] << 4;

                LevelScaleChroma4x4_Intra[Cr][k][j][i] =
                    quant_coef[k][j][i];
                InvLevelScaleChroma4x4_Intra[Cr][k][j][i] =
                    dequant_coef[k][j][i] << 4;
            }
        }
    }

    return MMES_NO_ERROR;
}

#if HW_METHOD
xint
quantLumaDC_intra16x16(h264enc_obj * pEnc, int16 * quant_buffer, xint qp_rem,
                       xint qp_const, xint q_bits)
{
	mb_obj  *pMB = &(pEnc->curr_slice->cmb);
    xint    x_coord, y_coord;
    int32   tmp_value, quantized_coeff, remainder_coeff;

    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            tmp_value =
                ABS(quant_buffer[x_coord + y_coord * B_SIZE]) *
                LevelScaleLuma4x4_Intra[qp_rem][0][0];

			tmp_value >>= 1;// DC
			quantized_coeff = tmp_value >> q_bits;
			remainder_coeff = ( tmp_value & ((1<<q_bits)-1) ) << (23-q_bits);

			if( remainder_coeff > 5592405 )	quantized_coeff += 1;

			tmp_value = quantized_coeff;

            quant_buffer[x_coord + y_coord * B_SIZE] =
                (int16) SIGN(tmp_value,
                             quant_buffer[x_coord + y_coord * B_SIZE]);
        }
    }

    return MMES_NO_ERROR;

}

/* ps: 1 ~ 15 */
xint
quantLumaAC_intra16x16(h264enc_obj *pEnc, int16 * quant_buffer, xint qp_rem,
                       xint qp_const, xint q_bits)
{
	mb_obj  *pMB = &(pEnc->curr_slice->cmb);
    xint    x_coord, y_coord;
    int32   tmp_value, quantized_coeff, remainder_coeff;
    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            if ((x_coord == 0) && (y_coord == 0))
                continue;       //DC Coeff

            tmp_value =
                ABS(quant_buffer[x_coord + y_coord * B_SIZE]) *
                LevelScaleLuma4x4_Intra[qp_rem][x_coord][y_coord];
			
			quantized_coeff = tmp_value >> q_bits;
			remainder_coeff = ( tmp_value & ((1<<q_bits)-1) ) << (23-q_bits);

            if( remainder_coeff > 5592405 )	quantized_coeff += 1;

			tmp_value = quantized_coeff;

            quant_buffer[x_coord + y_coord * B_SIZE] =
                (int16) SIGN(tmp_value,
                             quant_buffer[x_coord + y_coord * B_SIZE]);
        }
    }

    return MMES_NO_ERROR;
}

xint
quant_4x4(h264enc_obj * pEnc, xint block)
{
    mb_obj *pMB;
    xint mb_x_4_offset, mb_y_4_offset;
    xint qp_per, qp_rem, qp_const, q_bits, level_offset;
    int16 *b4x4_start;
    xint x_coord, y_coord;
    int32   tmp_value, quantized_coeff, remainder_coeff;
    int16 *ACLevel_start;
	xint    coeff_cost;//  08 11 2005
	int16   tmp_quant_buffer[B_SIZE * B_SIZE];//  08 11 2005
    
    pMB = &(pEnc->curr_slice->cmb);
    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    qp_per = pMB->QP / 6;
    qp_rem = pMB->QP % 6;
    q_bits = Q_BITS + qp_per;
	
	//  Jun 16 2005
	// qp_const, will be different in inter MB
	if( pEnc->curr_slice->type == I_SLICE )	level_offset = 682;//  08 10 2005
	else									level_offset = 342;
	qp_const = level_offset << (qp_per+Q_BITS-11);// for INTRA Slice

    b4x4_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            tmp_value = (int32) ABS((*b4x4_start));//  May 26 2005
            tmp_value = tmp_value*LevelScaleLuma4x4_Intra[qp_rem][x_coord][y_coord];

			quantized_coeff = tmp_value >> q_bits;
			remainder_coeff = ( tmp_value & ((1<<q_bits)-1) ) << (23-q_bits);

			if( pEnc->curr_slice->type == I_SLICE )
			{
				if( remainder_coeff > 5592405 )	quantized_coeff += 1;
			}
			else
			{
				if( remainder_coeff > 6990506 )	quantized_coeff += 1;
			}
			tmp_value = quantized_coeff;

			tmp_value = SIGN( tmp_value, *b4x4_start );//  May 26 2005
            *b4x4_start = (int16) tmp_value;
			tmp_quant_buffer[ y_coord*B_SIZE+x_coord ] = (int16) tmp_value;//  08 11 2005

            b4x4_start++;
        }
        b4x4_start += (MB_SIZE-B_SIZE);
    }

	coeff_cost = quantLumaDCAC_cost( tmp_quant_buffer );

    // copy to CAVLC data structure
    b4x4_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    ACLevel_start = pMB->LumaACLevel + block*B_SIZE*B_SIZE;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        memcpy(ACLevel_start, b4x4_start, B_SIZE*sizeof(int16));
        ACLevel_start += B_SIZE;
        b4x4_start += MB_SIZE;
    }
    
    return coeff_cost;
}

xint
quantChromaDC_intra(h264enc_obj * pEnc, int16 * quant_buffer, xint qp_rem, xint qp_const,
                    xint q_bits, xint uv)
{
    int32   tmp_value, quantized_coeff, remainder_coeff;
    xint    idx;
    for (idx = 0; idx < 4; idx++)
    {
        tmp_value =
            ABS(quant_buffer[idx]) *
            LevelScaleChroma4x4_Intra[uv][qp_rem][0][0];

		tmp_value >>= 1;// DC
		quantized_coeff = tmp_value >> q_bits;
		remainder_coeff = ( tmp_value & ((1<<q_bits)-1) ) << (23-q_bits);

		if( pEnc->curr_slice->type == I_SLICE )
		{
			if( remainder_coeff > 5592405 )	quantized_coeff += 1;
		}
		else
		{
			if( remainder_coeff > 6990506 )	quantized_coeff += 1;
		}

		tmp_value = quantized_coeff;
        quant_buffer[idx] = (int16) SIGN(tmp_value, quant_buffer[idx]);
    }

    return MMES_NO_ERROR;
}

xint
quantChromaAC_intra(h264enc_obj * pEnc, int16 * quant_buffer, xint qp_rem, xint qp_const,
                    xint q_bits, xint uv)
{
    xint    x_coord, y_coord;
    int32   tmp_value, quantized_coeff, remainder_coeff;

    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            if ((x_coord == 0) && (y_coord == 0))
                continue;       // DC Coeff

            tmp_value =
                ABS(quant_buffer[x_coord + y_coord * B_SIZE]) *
                LevelScaleChroma4x4_Intra[uv][qp_rem][x_coord][y_coord];

			quantized_coeff = tmp_value >> q_bits;
			remainder_coeff = ( tmp_value & ((1<<q_bits)-1) ) << (23-q_bits);

			if( pEnc->curr_slice->type == I_SLICE )
			{
				if( remainder_coeff > 5592405 )	quantized_coeff += 1;
			}
			else
			{
				if( remainder_coeff > 6990506 )	quantized_coeff += 1;
			}

			tmp_value = quantized_coeff;
            quant_buffer[x_coord + y_coord * B_SIZE] =
                (int16) SIGN(tmp_value,
                             quant_buffer[x_coord + y_coord * B_SIZE]);
        }
    }

    return MMES_NO_ERROR;
}

#else
xint
quantLumaDC_intra16x16(h264enc_obj * pEnc, int16 * quant_buffer, xint qp_rem,
                       xint qp_const, xint q_bits)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Quantize intra luma DC coefficients.                        */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    x_coord, y_coord;
    int32   tmp_value;

    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            tmp_value =
                ABS(quant_buffer[x_coord + y_coord * B_SIZE]) *
                LevelScaleLuma4x4_Intra[qp_rem][0][0];

            tmp_value = (tmp_value + (qp_const << 1)) >> (q_bits + 1);
            quant_buffer[x_coord + y_coord * B_SIZE] =
                (int16) SIGN(tmp_value,
                             quant_buffer[x_coord + y_coord * B_SIZE]);
        }
    }

    return MMES_NO_ERROR;

}

/* ps: 1 ~ 15 */
xint
quantLumaAC_intra16x16(h264enc_obj *pEnc, int16 * quant_buffer, xint qp_rem,
                       xint qp_const, xint q_bits)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Quantize intra luma AC coefficients.                        */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    x_coord, y_coord;
    int32   tmp_value;
    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            if ((x_coord == 0) && (y_coord == 0))
                continue;       //DC Coeff

            tmp_value =
                ABS(quant_buffer[x_coord + y_coord * B_SIZE]) *
                LevelScaleLuma4x4_Intra[qp_rem][x_coord][y_coord];
			
            tmp_value = (tmp_value + qp_const) >> q_bits;
            quant_buffer[x_coord + y_coord * B_SIZE] =
                (int16) SIGN(tmp_value,
                             quant_buffer[x_coord + y_coord * B_SIZE]);
        }
    }

    return MMES_NO_ERROR;
}

xint
quant_4x4(h264enc_obj * pEnc, xint block)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Mar/01/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Quantize one 4x4 intra block.                               */
/*                                                               */
/*   RETURN                                                      */
/*   Cost of coefficients for this 4x4 block                     */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Aug/11/2005 Jerry Peng, calculate cost of coefficients.  */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB;
    xint mb_x_4_offset, mb_y_4_offset;
    xint qp_per, qp_rem, qp_const, q_bits, level_offset;
    int16 *b4x4_start;
    xint x_coord, y_coord;
    int32 tmp_value;	
    int16 *ACLevel_start;
	xint    coeff_cost;//  08 11 2005
	int16   tmp_quant_buffer[B_SIZE * B_SIZE];//  08 11 2005
    
    pMB = &(pEnc->curr_slice->cmb);
    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    qp_per = pMB->QP / 6;
    qp_rem = pMB->QP % 6;
    q_bits = Q_BITS + qp_per;
	
	//  Jun 16 2005
	// qp_const, will be different in inter MB
	if( pEnc->curr_slice->type == I_SLICE )	level_offset = 682;//  08 10 2005
	else									level_offset = 342;
	qp_const = level_offset << (qp_per+Q_BITS-11);// for INTRA Slice

    b4x4_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            tmp_value = (int32) ABS((*b4x4_start));//  May 26 2005
            tmp_value = tmp_value*LevelScaleLuma4x4_Intra[qp_rem][x_coord][y_coord];
            tmp_value = (tmp_value+qp_const)>>q_bits;
			tmp_value = SIGN( tmp_value, *b4x4_start );//  May 26 2005
            *b4x4_start = (int16) tmp_value;
			tmp_quant_buffer[ y_coord*B_SIZE+x_coord ] = (int16) tmp_value;//  08 11 2005

            b4x4_start++;
        }
        b4x4_start += (MB_SIZE-B_SIZE);
    }

	coeff_cost = quantLumaDCAC_cost( tmp_quant_buffer );

    // copy to CAVLC data structure
    b4x4_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    ACLevel_start = pMB->LumaACLevel + block*B_SIZE*B_SIZE;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        memcpy(ACLevel_start, b4x4_start, B_SIZE*sizeof(int16));
        ACLevel_start += B_SIZE;
        b4x4_start += MB_SIZE;
    }
    
    return coeff_cost;
}

xint
quantChromaDC_intra(h264enc_obj * pEnc, int16 * quant_buffer, xint qp_rem, xint qp_const,
                    xint q_bits, xint uv)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Quantize intra chroma DC coefficients.                      */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    int32   tmp_value;
    xint    idx;
    for (idx = 0; idx < 4; idx++)
    {
        tmp_value =
            ABS(quant_buffer[idx]) *
            LevelScaleChroma4x4_Intra[uv][qp_rem][0][0];
        tmp_value = (tmp_value + (qp_const << 1)) >> (q_bits + 1);
        quant_buffer[idx] = (int16) SIGN(tmp_value, quant_buffer[idx]);
    }

    return MMES_NO_ERROR;
}

xint
quantChromaAC_intra(h264enc_obj * pEnc, int16 * quant_buffer, xint qp_rem, xint qp_const,
                    xint q_bits, xint uv)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Quantize intra chroma AC coefficients.                      */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    x_coord, y_coord;
    int32   tmp_value;

    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {

            if ((x_coord == 0) && (y_coord == 0))
                continue;       // DC Coeff

            tmp_value =
                ABS(quant_buffer[x_coord + y_coord * B_SIZE]) *
                LevelScaleChroma4x4_Intra[uv][qp_rem][x_coord][y_coord];
            tmp_value = (tmp_value + qp_const) >> q_bits;
            quant_buffer[x_coord + y_coord * B_SIZE] =
                (int16) SIGN(tmp_value,
                             quant_buffer[x_coord + y_coord * B_SIZE]);
        }
    }

    return MMES_NO_ERROR;
}

#endif

xint
dequantLumaDC_intra16x16(int16 * dequant_buffer, xint qp_rem,
                         xint qp_per)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Inverse-quantize intra luma DC coefficients.                */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    x_coord, y_coord;
    int32   tmp_value;

    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            tmp_value =
                dequant_buffer[x_coord +
                               y_coord * B_SIZE] *
                InvLevelScaleLuma4x4_Intra[qp_rem][0][0];

            if (qp_per < 6)
                tmp_value =
                    (tmp_value + (1 << (5 - qp_per))) >> (6 - qp_per);
            else
                tmp_value = tmp_value << (qp_per - 6);

            dequant_buffer[x_coord + y_coord * B_SIZE] =
                (int16) tmp_value;
        }
    }

    return MMES_NO_ERROR;
}

xint
dequantLumaAC_intra16x16(int16 * dequant_buffer, xint qp_rem,
                         xint qp_per)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Inverse-quantize intra luma AC coefficients.                */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    x_coord, y_coord;
    int32   tmp_value;

    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            if ((x_coord == 0) && (y_coord == 0))
                continue;       //DC Coeff

            tmp_value =
                dequant_buffer[x_coord +
                               y_coord * B_SIZE] *
                InvLevelScaleLuma4x4_Intra[qp_rem][x_coord][y_coord];

            if (qp_per < 4)
                tmp_value =
                    (tmp_value + (1 << (3 - qp_per))) >> (4 - qp_per);
            else
                tmp_value = tmp_value << (qp_per - 4);

            dequant_buffer[x_coord + y_coord * B_SIZE] =
                (int16) tmp_value;
        }
    }
    return MMES_NO_ERROR;
}

xint
dequantChromaDC_intra(int16 * dequant_buffer, xint qp_rem,
                      xint qp_per, xint uv)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Inverse-quantize intra chroma DC coefficients.              */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    int32   tmp_value;
    xint    idx;

    for (idx = 0; idx < 4; idx++)
    {
        tmp_value =
            dequant_buffer[idx] *
            InvLevelScaleChroma4x4_Intra[uv][qp_rem][0][0];

        if (qp_per < 5)
            tmp_value = tmp_value >> (5 - qp_per);
        else
            tmp_value = tmp_value << (qp_per - 5);

        dequant_buffer[idx] = (int16) tmp_value;
    }

    return MMES_NO_ERROR;

}

xint
dequantChromaAC_intra(int16 * dequant_buffer, xint qp_rem,
                      xint qp_per, xint uv)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Inverse-quantize intra chroma AC coefficients.              */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    x_coord, y_coord;
    int32   tmp_value;

    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            if ((x_coord == 0) && (y_coord == 0))
                continue;       //DC Coeff

            tmp_value =
                dequant_buffer[x_coord +
                               y_coord * B_SIZE] *
                InvLevelScaleChroma4x4_Intra[uv][qp_rem][x_coord]
                [y_coord];

            if (qp_per < 4)
                tmp_value =
                    (tmp_value + (1 << (3 - qp_per))) >> (4 - qp_per);
            else
                tmp_value = tmp_value << (qp_per - 4);

            dequant_buffer[x_coord + y_coord * B_SIZE] =
                (int16) tmp_value;

        }
    }

    return MMES_NO_ERROR;
}

xint
quant_16x16(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng                 */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Quantize one 16x16 intra macroblock.                        */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Feb/23/2005 Chun-Jen Tsai, Change the name from quant() to  */
/*               quant_16x16(). This is a temporary change.      */
/*   Apr/14/2005 Jerry Peng, change 4x4 block scan order     */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice;
    mb_obj *pMB;
    xint    x_coord, y_coord;
    xint    mb_x_4_offset, mb_y_4_offset;
    xint    block;
    int16   tmp_quant_buffer[B_SIZE * B_SIZE];
    int16  *b4x4_start;
    xint    qp_per, qp_rem, qp_const, q_bits, level_offset;

    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    qp_per = pMB->QP / 6;
    qp_rem = pMB->QP % 6;
    q_bits = Q_BITS + qp_per;

	//  08 15 2005
	// qp_const, will be different in inter MB
	if( pEnc->curr_slice->type == I_SLICE )	level_offset = 682;//  08 10 2005
	else									level_offset = 342;
	qp_const = level_offset << (qp_per+Q_BITS-11);// for INTRA Slice

    // Luma DC coeff for Intra16x16
    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            tmp_quant_buffer[x_coord + y_coord * B_SIZE] =
                pMB->best_I16x16_residual[x_coord * B_SIZE +
                                          y_coord * B_SIZE * MB_SIZE];
        }
    }

    quantLumaDC_intra16x16(pEnc, tmp_quant_buffer, qp_rem, qp_const, q_bits);

    // copy to CAVLC data structure
    memcpy(pMB->LumaDCLevel, tmp_quant_buffer, B_SIZE*B_SIZE*sizeof(int16));

    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            pMB->best_I16x16_residual[x_coord * B_SIZE +
                                      y_coord * B_SIZE * MB_SIZE] =
                tmp_quant_buffer[x_coord + y_coord * B_SIZE];
        }
    }

    for(block=0; block<NBLOCKS; block++)
    {
        mb_x_4_offset = mb_x_4_idx[block];
        mb_y_4_offset = mb_y_4_idx[block];

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE +
                                 mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(&(tmp_quant_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }

        quantLumaAC_intra16x16(pEnc, tmp_quant_buffer, qp_rem, qp_const, q_bits);

        // copy to CAVLC data structure
        memcpy(&(pMB->LumaACLevel[block*B_SIZE*B_SIZE]), tmp_quant_buffer, B_SIZE*B_SIZE*sizeof(int16));

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE +
                                 mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(b4x4_start, &(tmp_quant_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }
    }

    return MMES_NO_ERROR;
}

xint
eliminateLumaDCAC( mb_obj *pMB, xint block )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/11/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Eliminate Luma DC AC coefficients.                          */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pMB -> [I] pointer to macroblock session                   */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	xint mb_x_4_offset, mb_y_4_offset;
	xint y_coord;
	int16 *b4x4_start;
	
	mb_x_4_offset = mb_x_4_idx[block];
	mb_y_4_offset = mb_y_4_idx[block];
	
	/* reset residual */
	b4x4_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
	for(y_coord=0; y_coord<B_SIZE; y_coord++)
	{
		memset(b4x4_start, 0, B_SIZE*sizeof(int16));
		b4x4_start += MB_SIZE;
	}

	/* clear AC coefficients */
	memset(pMB->LumaACLevel+block*MB_SIZE, 0, (MB_SIZE)*sizeof(int16));

	return MMES_NO_ERROR;
}

xint
quantLumaDCAC_cost( int16 *coeff )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/11/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Calculate cost of DC & AC coefficients in this 4x4 block.   */
/*                                                               */
/*   RETURN                                                      */
/*   Cost of coefficients                                        */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *coeff -> [I] pointer to coefficients of this 4x4 block     */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	xint coeff_cost = 0, idx, run, i, j;

	run = -1;
	for( idx=0; idx<B_SIZE*B_SIZE; idx++ )
	{
		i = SNGL_SCAN[idx][0];
		j = SNGL_SCAN[idx][1];

		++run;
		if( coeff[j*B_SIZE+i] != 0 )
		{
			if( ABS(coeff[j*B_SIZE+i]) > 1 )
				coeff_cost += MAX_INT16;
			else
				coeff_cost += COEFF_COST[0][run];

			run = -1;
		}		
	}
	
	return coeff_cost;
}

xint quantChromaAC_cost( int16 *coeff )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Jul/??/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Calculate cost of AC coefficients in this 4x4 block.        */
/*                                                               */
/*   RETURN                                                      */
/*   Cost of coefficients                                        */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *coeff -> [I] pointer to coefficients of this 4x4 block     */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	xint coeff_cost = 0, idx, run, i, j;

	run = -1;
	for( idx=1; idx<B_SIZE*B_SIZE; idx++ )
	{
		i = SNGL_SCAN[idx][0];
		j = SNGL_SCAN[idx][1];

		++run;
		if( coeff[j*B_SIZE+i] != 0 )
		{
			if( ABS(coeff[j*B_SIZE+i]) > 1 )
				coeff_cost += MAX_INT16;
			else
				coeff_cost += COEFF_COST[0][run];

			run = -1;
		}		
	}
	
	return coeff_cost;
}

xint
quant_chroma(h264enc_obj * pEnc)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng             */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Quantize one chroma intra block.                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   May/30/2005 Jerry Peng, calculate cost of coefficients   */
/*               to deciside if it's necessary to reset          */
/*               coeffients.                                     */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice;
    mb_obj *pMB;
    xint    y_coord;
    xint    mb_x_4_offset, mb_y_4_offset;
    xint    block;
    int16   tmp_quant_buffer[B_SIZE * B_SIZE];
    int16  *b4x4_start;
    xint    qp_c, cb_qp_per, cb_qp_rem, cb_qp_const, cb_q_bits;
    xint    cr_qp_per, cr_qp_rem, cr_qp_const, cr_q_bits, level_offset;
    xint    chroma_mb_size;
	xint    coeff_cost;

    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    qp_c = pMB->QP + pSlice->chroma_qp_index_offset;
    qp_c = CLIP3(0, 51, qp_c);
    if (qp_c >= 0)
        qp_c = QP_SCALE_CR[qp_c];
    cb_qp_per = qp_c / 6;
    cb_qp_rem = qp_c % 6;
    cb_q_bits = Q_BITS + cb_qp_per;

	//  Jun 16 2005
	// qp_const, will be different in inter MB
	if( pEnc->curr_slice->type == I_SLICE )	level_offset = 682;//  08 11 2005
	else									level_offset = 342;

	cb_qp_const = level_offset << (cb_qp_per+Q_BITS-11);// for INTRA Slice

    qp_c = pMB->QP + pSlice->chroma_qp_index_offset;
    qp_c = CLIP3(0, 51, qp_c);
    if (qp_c >= 0)
        qp_c = QP_SCALE_CR[qp_c];
    cr_qp_per = qp_c / 6;
    cr_qp_rem = qp_c % 6;
    cr_q_bits = Q_BITS + cb_qp_per;

	//  Jun 16 2005
	// qp_const, will be different in inter MB
	cr_qp_const = level_offset << (cr_qp_per+Q_BITS-11);// for INTRA Slice

    chroma_mb_size = MB_SIZE / 2;       // for YCbCr420

	coeff_cost = 0;
    // Chroma Cb DC component
    tmp_quant_buffer[0] = pMB->cb_residual[0 + 0 * chroma_mb_size];
    tmp_quant_buffer[1] = pMB->cb_residual[4 + 0 * chroma_mb_size];
    tmp_quant_buffer[2] = pMB->cb_residual[0 + 4 * chroma_mb_size];
    tmp_quant_buffer[3] = pMB->cb_residual[4 + 4 * chroma_mb_size];

    quantChromaDC_intra(pEnc, tmp_quant_buffer, cb_qp_rem, cb_qp_const, cb_q_bits, Cb);

    // copy to CAVLC data structure
    memcpy(pMB->CbDCLevel, tmp_quant_buffer, 4*sizeof(int16));

    pMB->cb_residual[0 + 0 * chroma_mb_size] = tmp_quant_buffer[0];
    pMB->cb_residual[4 + 0 * chroma_mb_size] = tmp_quant_buffer[1];
    pMB->cb_residual[0 + 4 * chroma_mb_size] = tmp_quant_buffer[2];
    pMB->cb_residual[4 + 4 * chroma_mb_size] = tmp_quant_buffer[3];
	
    // Chroma Cb AC component
    for(block=0; block<4; block++)      // for YCbCr420
    {
        mb_x_4_offset = mb_x_4_idx[block];
        mb_y_4_offset = mb_y_4_idx[block];
        
        b4x4_start = pMB->cb_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(&(tmp_quant_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
            b4x4_start += chroma_mb_size;
        }
        quantChromaAC_intra(pEnc, tmp_quant_buffer, cb_qp_rem, cb_qp_const, cb_q_bits, Cb);

		// calculate cost of coefficients
		//  May 30 2005
		coeff_cost += quantChromaAC_cost( tmp_quant_buffer );

        // copy to CALVC data structure
        memcpy(&(pMB->CbACLevel[block*B_SIZE*B_SIZE]), tmp_quant_buffer, B_SIZE*B_SIZE*sizeof(int16));

        b4x4_start = pMB->cb_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(b4x4_start, &(tmp_quant_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
            b4x4_start += chroma_mb_size;
        }
    }

	// if coefficient cost is small, reset Cb AC component
	//  May 30 2005
	if( coeff_cost < _CHROMA_COEFF_COST_ )
	{
		for(block=0; block<4; block++)      // for YCbCr420
		{
			mb_x_4_offset = mb_x_4_idx[block];
			mb_y_4_offset = mb_y_4_idx[block];

			b4x4_start = pMB->cb_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
			for(y_coord=0; y_coord<B_SIZE; y_coord++)
			{
				if( y_coord==0 )
					memset(b4x4_start+1, 0, (B_SIZE-1)*sizeof(int16));
				else
					memset(b4x4_start, 0, B_SIZE*sizeof(int16));
				b4x4_start += chroma_mb_size;
			}

			memset(pMB->CbACLevel+block*MB_SIZE+1, 0, (MB_SIZE-1)*sizeof(int16));
		}
	}


	coeff_cost = 0;
    // Chroma Cr DC component
    tmp_quant_buffer[0] = pMB->cr_residual[0 + 0 * chroma_mb_size];
    tmp_quant_buffer[1] = pMB->cr_residual[4 + 0 * chroma_mb_size];
    tmp_quant_buffer[2] = pMB->cr_residual[0 + 4 * chroma_mb_size];
    tmp_quant_buffer[3] = pMB->cr_residual[4 + 4 * chroma_mb_size];

    quantChromaDC_intra(pEnc, tmp_quant_buffer, cr_qp_rem, cr_qp_const, cr_q_bits, Cr);

    // copy to CAVLC data structure
    memcpy(pMB->CrDCLevel, tmp_quant_buffer, 4*sizeof(int16));

    pMB->cr_residual[0 + 0 * chroma_mb_size] = tmp_quant_buffer[0];
    pMB->cr_residual[4 + 0 * chroma_mb_size] = tmp_quant_buffer[1];
    pMB->cr_residual[0 + 4 * chroma_mb_size] = tmp_quant_buffer[2];
    pMB->cr_residual[4 + 4 * chroma_mb_size] = tmp_quant_buffer[3];

    // Chroma Cr AC component
    for(block=0; block<4; block++)  // for YCbCr420
    {
        mb_x_4_offset = mb_x_4_idx[block];
        mb_y_4_offset = mb_y_4_idx[block];

        b4x4_start = pMB->cr_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(&(tmp_quant_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
            b4x4_start += chroma_mb_size;
        }

        quantChromaAC_intra(pEnc, tmp_quant_buffer, cr_qp_rem, cr_qp_const, cr_q_bits, Cr);

		// calculate cost of coefficients
		//  May 30 2005
		coeff_cost += quantChromaAC_cost( tmp_quant_buffer );

        // copy to CAVLC dtat structure
        memcpy(&(pMB->CrACLevel[block*B_SIZE*B_SIZE]), tmp_quant_buffer, B_SIZE*B_SIZE*sizeof(int16));
        
         b4x4_start = pMB->cr_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(b4x4_start, &(tmp_quant_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
            b4x4_start += chroma_mb_size;
        }
    }

	// if coefficient cost is small, reset Cr AC component
	//  May 30 2005
	if( coeff_cost < _CHROMA_COEFF_COST_ )
	{
		for(block=0; block<4; block++)      // for YCbCr420
		{
			mb_x_4_offset = mb_x_4_idx[block];
			mb_y_4_offset = mb_y_4_idx[block];

			b4x4_start = pMB->cr_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
			for(y_coord=0; y_coord<B_SIZE; y_coord++)
			{
				if( y_coord==0 )
					memset(b4x4_start+1, 0, (B_SIZE-1)*sizeof(int16));
				else
					memset(b4x4_start, 0, B_SIZE*sizeof(int16));
				b4x4_start += chroma_mb_size;
			}

            memset(pMB->CrACLevel+block*MB_SIZE+1, 0, (MB_SIZE-1)*sizeof(int16));
		}
	}

    return MMES_NO_ERROR;
}

xint
inv_quant_16x16(slice_obj *pSlice)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng             */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Dequantize one intra 16x16 macroblock for decoder.          */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Apr/14/2005 Jerry Peng, change 4x4 block scan order     */
/* ------------------------------------------------------------- */
{
//    slice_obj *pSlice;
    mb_obj *pMB;
    xint    x_coord, y_coord;
    xint    mb_x_4_offset, mb_y_4_offset;
    xint    block;
    int16   tmp_dequant_buffer[B_SIZE * B_SIZE];
    int16  *b4x4_start;
    xint    qp_per, qp_rem, q_bits;
    
//    pSlice = pEnc->curr_slice;
    pMB = &(pSlice->cmb);

    qp_per = pMB->QP / 6;
    qp_rem = pMB->QP % 6;
    q_bits = Q_BITS + qp_per;

    //pick ou DC coeff
    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            tmp_dequant_buffer[x_coord + y_coord * B_SIZE] =
                pMB->best_I16x16_residual[x_coord * B_SIZE +
                                          y_coord * B_SIZE * MB_SIZE];
        }
    }

    // inverse hadamard DC transform for intra16x16 
    inv_hadamard_transform(tmp_dequant_buffer);

    dequantLumaDC_intra16x16(tmp_dequant_buffer, qp_rem, qp_per);

    for (y_coord = 0; y_coord < B_SIZE; y_coord++)
    {
        for (x_coord = 0; x_coord < B_SIZE; x_coord++)
        {
            pMB->best_I16x16_residual[x_coord * B_SIZE +
                                      y_coord * B_SIZE * MB_SIZE] =
                tmp_dequant_buffer[x_coord + y_coord * B_SIZE];
        }
    }

    //Luma AC dequant
    for(block=0; block<NBLOCKS; block++)
    {
        mb_x_4_offset = mb_x_4_idx[block];
        mb_y_4_offset = mb_y_4_idx[block];

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE +
                                mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(&(tmp_dequant_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }

        dequantLumaAC_intra16x16(tmp_dequant_buffer, qp_rem,
                                     qp_per);

        b4x4_start = pMB->best_I16x16_residual + mb_y_4_offset*4*MB_SIZE +
                                mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(b4x4_start, &(tmp_dequant_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
            b4x4_start += MB_SIZE;
        }

    }

    return MMES_NO_ERROR;
}

xint
inv_quant_4x4(slice_obj *pSlice, xint block)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng             */
/*   Date  : Mar/01/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Dequantize one intra 4x4 macroblock for decoder.            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jun/27/2005 Chingho-Ho Chen, modify parameter for           */
/*   integrating decoder                                         */
/*                                                               */
/* ------------------------------------------------------------- */
{
    mb_obj *pMB;
    xint mb_x_4_offset, mb_y_4_offset;
    xint qp_per, qp_rem;
    int16 *b4x4_start;
    int32 tmp_value;
    xint x_coord, y_coord;

    pMB = &(pSlice->cmb);//  06 27 2005
    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    qp_per = pMB->QP / 6;
    qp_rem = pMB->QP % 6;

    b4x4_start = pMB->best_I4x4_residual + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
    for(y_coord=0; y_coord<B_SIZE; y_coord++)
    {
        for(x_coord=0; x_coord<B_SIZE; x_coord++)
        {
            tmp_value = (int32) *b4x4_start;
            tmp_value = tmp_value*InvLevelScaleLuma4x4_Intra[qp_rem][x_coord][y_coord];
            
            if(qp_per<4)
                tmp_value = (tmp_value+(1<<(3-qp_per)))>>(4-qp_per);
            else
                tmp_value = tmp_value << (qp_per-4);

            *b4x4_start = (int16) tmp_value;
            b4x4_start ++;
        }
        b4x4_start += (MB_SIZE-B_SIZE);
    }


    return MMES_NO_ERROR;
}

xint inv_quant_chroma(slice_obj *pSlice)
/* ------------------------------------------------------------- */
/*   Author: Extracted from JM 9.0 by Jerry Peng             */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Dequantize one intra chroma macroblock for decoder.         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jun/27/2005 Chingho-Ho Chen, modify parameter for           */
/*   integrating decoder                                         */
/*                                                               */
/* ------------------------------------------------------------- */
{
//    slice_obj *pSlice;//  06 27 2005
    mb_obj *pMB;
    xint    y_coord;
    xint    mb_x_4_offset, mb_y_4_offset;
    xint    block;
    int16   tmp_dequant_buffer[B_SIZE * B_SIZE];
    int16  *b4x4_start;
    xint    qp_c, cb_qp_per, cb_qp_rem, cb_q_bits;
    xint    cr_qp_per, cr_qp_rem, cr_q_bits;
    xint    chroma_mb_size;

//    pSlice = pEnc->curr_slice;//  06 27 2005
    pMB = &(pSlice->cmb);

    qp_c = pMB->QP + pSlice->chroma_qp_index_offset;
    qp_c = CLIP3(0, 51, qp_c);
    if (qp_c >= 0)
        qp_c = QP_SCALE_CR[qp_c];
    cb_qp_per = qp_c / 6;
    cb_qp_rem = qp_c % 6;
    cb_q_bits = Q_BITS + cb_qp_per;

    qp_c = pMB->QP + pSlice->chroma_qp_index_offset;
    qp_c = CLIP3(0, 51, qp_c);
    if (qp_c >= 0)
        qp_c = QP_SCALE_CR[qp_c];
    cr_qp_per = qp_c / 6;
    cr_qp_rem = qp_c % 6;
    cr_q_bits = Q_BITS + cb_qp_per;

    chroma_mb_size = MB_SIZE / 2;       // for YCbCr420

    // inverse hadamard2x2 transform for DC chroma coeff
    tmp_dequant_buffer[0] = pMB->cb_residual[0 + 0 * chroma_mb_size];   //(0, 0)

    tmp_dequant_buffer[1] = pMB->cb_residual[4 + 0 * chroma_mb_size];   //(4, 0)

    tmp_dequant_buffer[2] = pMB->cb_residual[0 + 4 * chroma_mb_size];   //(0, 4)

    tmp_dequant_buffer[3] = pMB->cb_residual[4 + 4 * chroma_mb_size];   //(4, 4)

    pMB->cb_residual[0 + 0 * chroma_mb_size] =
        tmp_dequant_buffer[0] + tmp_dequant_buffer[1] +
        tmp_dequant_buffer[2] + tmp_dequant_buffer[3];
    pMB->cb_residual[4 + 0 * chroma_mb_size] =
        tmp_dequant_buffer[0] - tmp_dequant_buffer[1] +
        tmp_dequant_buffer[2] - tmp_dequant_buffer[3];
    pMB->cb_residual[0 + 4 * chroma_mb_size] =
        tmp_dequant_buffer[0] + tmp_dequant_buffer[1] -
        tmp_dequant_buffer[2] - tmp_dequant_buffer[3];
    pMB->cb_residual[4 + 4 * chroma_mb_size] =
        tmp_dequant_buffer[0] - tmp_dequant_buffer[1] -
        tmp_dequant_buffer[2] + tmp_dequant_buffer[3];

    // Chroma Cb DC component
    tmp_dequant_buffer[0] = pMB->cb_residual[0 + 0 * chroma_mb_size];
    tmp_dequant_buffer[1] = pMB->cb_residual[4 + 0 * chroma_mb_size];
    tmp_dequant_buffer[2] = pMB->cb_residual[0 + 4 * chroma_mb_size];
    tmp_dequant_buffer[3] = pMB->cb_residual[4 + 4 * chroma_mb_size];

    dequantChromaDC_intra(tmp_dequant_buffer, cb_qp_rem, cb_qp_per, Cb);

    pMB->cb_residual[0 + 0 * chroma_mb_size] = tmp_dequant_buffer[0];
    pMB->cb_residual[4 + 0 * chroma_mb_size] = tmp_dequant_buffer[1];
    pMB->cb_residual[0 + 4 * chroma_mb_size] = tmp_dequant_buffer[2];
    pMB->cb_residual[4 + 4 * chroma_mb_size] = tmp_dequant_buffer[3];

    // Chroma Cb AC component
    for(block=0; block<4; block++)      // for YCbCr420
    {
        mb_x_4_offset = mb_x_4_idx[block];
        mb_y_4_offset = mb_y_4_idx[block];
        
        b4x4_start = pMB->cb_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(&(tmp_dequant_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
            b4x4_start += chroma_mb_size;
        }
    
         dequantChromaAC_intra(tmp_dequant_buffer, cb_qp_rem,
                                  cb_qp_per, Cb);

        b4x4_start = pMB->cb_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(b4x4_start, &(tmp_dequant_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
            b4x4_start += chroma_mb_size;
        }

    }

    // inverse hadamard2x2 transform for DC chroma coeff
    tmp_dequant_buffer[0] = pMB->cr_residual[0 + 0 * chroma_mb_size];   //(0, 0)

    tmp_dequant_buffer[1] = pMB->cr_residual[4 + 0 * chroma_mb_size];   //(4, 0)

    tmp_dequant_buffer[2] = pMB->cr_residual[0 + 4 * chroma_mb_size];   //(0, 4)

    tmp_dequant_buffer[3] = pMB->cr_residual[4 + 4 * chroma_mb_size];   //(4, 4)

    pMB->cr_residual[0 + 0 * chroma_mb_size] =
        tmp_dequant_buffer[0] + tmp_dequant_buffer[1] +
        tmp_dequant_buffer[2] + tmp_dequant_buffer[3];
    pMB->cr_residual[4 + 0 * chroma_mb_size] =
        tmp_dequant_buffer[0] - tmp_dequant_buffer[1] +
        tmp_dequant_buffer[2] - tmp_dequant_buffer[3];
    pMB->cr_residual[0 + 4 * chroma_mb_size] =
        tmp_dequant_buffer[0] + tmp_dequant_buffer[1] -
        tmp_dequant_buffer[2] - tmp_dequant_buffer[3];
    pMB->cr_residual[4 + 4 * chroma_mb_size] =
        tmp_dequant_buffer[0] - tmp_dequant_buffer[1] -
        tmp_dequant_buffer[2] + tmp_dequant_buffer[3];

    // Chroma Cr DC component
    tmp_dequant_buffer[0] = pMB->cr_residual[0 + 0 * chroma_mb_size];
    tmp_dequant_buffer[1] = pMB->cr_residual[4 + 0 * chroma_mb_size];
    tmp_dequant_buffer[2] = pMB->cr_residual[0 + 4 * chroma_mb_size];
    tmp_dequant_buffer[3] = pMB->cr_residual[4 + 4 * chroma_mb_size];

    dequantChromaDC_intra(tmp_dequant_buffer, cr_qp_rem, cr_qp_per, Cr);

    pMB->cr_residual[0 + 0 * chroma_mb_size] = tmp_dequant_buffer[0];
    pMB->cr_residual[4 + 0 * chroma_mb_size] = tmp_dequant_buffer[1];
    pMB->cr_residual[0 + 4 * chroma_mb_size] = tmp_dequant_buffer[2];
    pMB->cr_residual[4 + 4 * chroma_mb_size] = tmp_dequant_buffer[3];

    // Chroma Cr AC component
     for(block=0; block<4; block++)  // for YCbCr420
    {
        mb_x_4_offset = mb_x_4_idx[block];
        mb_y_4_offset = mb_y_4_idx[block];

        b4x4_start = pMB->cr_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(&(tmp_dequant_buffer[y_coord*B_SIZE]), b4x4_start, B_SIZE*sizeof(int16));
            b4x4_start += chroma_mb_size;
        }

        dequantChromaAC_intra(tmp_dequant_buffer, cr_qp_rem,
                                  cr_qp_per, Cr);
        
        b4x4_start = pMB->cr_residual + mb_y_4_offset*4*chroma_mb_size + mb_x_4_offset*4;
        for(y_coord=0; y_coord<B_SIZE; y_coord++)
        {
            memcpy(b4x4_start, &(tmp_dequant_buffer[y_coord*B_SIZE]), B_SIZE*sizeof(int16));
            b4x4_start += chroma_mb_size;
        }

    }

    return MMES_NO_ERROR;
}
