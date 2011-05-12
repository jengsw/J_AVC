/* ///////////////////////////////////////////////////////////// */
/*   File: mv_search.c                                           */
/*   Author: Jerry Peng                                          */
/*   Date: Oct/06/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 Motion Estimation Module.        */
/*                                                               */
/*   Copyright, 2005.                                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "../../common/inc/mv_search.h"
#include "../../common/inc/mv_pred.h"
#include "../../common/inc/misc_util.h"

//////////////////////////////////////////////////////////////////////
//  Macro Definition												//
//////////////////////////////////////////////////////////////////////

#define ME_STAGE_Y4     		0
#define ME_STAGE_YQ		        1
#define ME_STAGE_SZ		        2

#define ME_Y16_SEARCH_RANGE     16

#define ME_16x16                1
#define ME_16x8                 2
#define ME_8x16                 3
#define ME_8x8                  4

#define Clip(x, l, u)	(((x)<=(l))?(l):(((x)>(u))?(u):(x)))
#define Clip1Y(x)		Clip(x, 0, 255)
#if __CC_ARM
#define min(a, b)		(((xint)a>(xint)b)?b:a)
#endif

#define WEIGHTED_COST(factor,bits)  (((factor)*(bits))>>LAMBDA_ACCURACY_BITS)
#define MV_COST(f,s,cx,cy,px,py)    (WEIGHTED_COST(f, mvbits[((cx)<<(s))-px]+mvbits[((cy)<<(s))-py]))
#define REF_COST(f,ref,list_offset) (WEIGHTED_COST(f,((listXsize[list_offset]<=1)? 0:refbits[(ref)])))
	
//////////////////////////////////////////////////////////////////////
//  Data Structure Definition										//
//////////////////////////////////////////////////////////////////////

typedef struct _coorf_obj_
{
	int16	x;
	int16	y;
} coord_obj;

typedef struct _me_obj_
{
	coord_obj	can[7][4];				// candidates for 7 levels search and 4 targets
										// level 0 : YX
										// level 1 : Y4
										// level 2 : Y1 mode INTER_16x16
										// level 3 : Y1 mode INTER_16x8
										// level 4 : Y1 mode INTER_8x16
										// level 5 : Y1 mode INTER_8x8
										// level 6 : Y1 mode INTER_PSKIP
	coord_obj	yqmv[4][4];				// best mv for 4 modes (16x16, 16x8, 8x16, 8x8) and 4 blocks
	xint		yqcst[4][4];			// min cost for 4 modes (16x16, 16x8, 8x16, 8x8) and 4 blocks
	xint		y1cst[4][4];			// min cost for 4 modes (16x16, 16x8, 8x16, 8x8) and 4 blocks
	xint		y1can[4][4];			// best candidate for 4 modes and 4 blocks
	uint8		curyx[4][4];			// source Y16
	uint8		cury4[8][8];			// source Y4
	uint8		cury1[16][16];			// source Y
	uint8		refyx[1][36][36];		// full search
	uint8		refy4[3][12][12];		// 3 candidates
	uint8		refy1[2][20][20];		// 2 candidates
	uint8		refyq[2][112][112];		// 2 candidates
	xint		bmode;
	xint		sady1[2][25][4];		// sad for 2 candidates, 25 positions and 4 blocks
	coord_obj	mvskip;					// skip mv predictor
	pred_obj	leftpred[4];			// left predictor for motition estimation
} me_obj;


//////////////////////////////////////////////////////////////////////
// Function Declaration												//
//////////////////////////////////////////////////////////////////////

static uint8 getpel(uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_16x16(uint8 dst[16][16], uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_8x8(uint8 dst[8][8], uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_4x4(uint8 dst[4][4], uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_12x12(uint8 dst[12][12], uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_20x20(uint8 dst[20][20], uint8 *src, xint x, xint y, xint width, xint height);
static xint me_pred_16x16_yx(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy);
static xint me_pred_skip_y4(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy);
static xint me_pred_skip_y1(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy);
static xint me_pred_yq(slice_obj *pSlice, xint width, xint mode, xint part, int16 *pmvx, int16 *pmvy);
static xint me_pred_16x16_yq(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy);
static xint me_pred_16x8_yq(slice_obj *pSlice, xint width, xint part, int16 *pmvx, int16 *pmvy);
static xint me_pred_8x16_yq(slice_obj *pSlice, xint width, xint part, int16 *pmvx, int16 *pmvy);
static xint me_pred_8x8_yq(slice_obj *pSlice, xint width, xint part, int16 *pmvx, int16 *pmvy);
static xint me_setup_block_sad(h264enc_obj *pEnc);

static xint me_init_mb(h264enc_obj *pEnc);
static xint me_finish_mb(h264enc_obj *pEnc);

//////////////////////////////////////////////////////////////////////
//  Static and External Data										//
//////////////////////////////////////////////////////////////////////

me_obj		thisme;
uint8		*absb;
xint		*mvbits;
xint		*refbits;
xint		listXsize[] = {1, 0, 2, 0, 2, 0};

// for motion prediction
extern pred_obj	pred_A, pred_B, pred_C, pred_D, pred_X;
extern mb_part_16x8_idx[];

/*
coord_obj spiral_search[25] = 
{
	{  0,  0 },	{  0, -1 },	{  0,  1 },	{ -1, -1 },	{  1, -1 },		// 15  9 11 13 16
	{ -1,  0 },	{  1,  0 },	{ -1,  1 },	{  1,  1 },	{ -1, -2 },		// 17  3  1  4 18
	{ -1,  2 },	{  0, -2 },	{  0,  2 },	{  1, -2 },	{  1,  2 },		// 19  5  0  6 20
	{ -2, -2 },	{  2, -2 },	{ -2, -1 },	{  2, -1 },	{ -2,  0 },		// 21  7  2  8 22
	{  2,  0 },	{ -2,  1 },	{  2,  1 },	{ -2,  2 },	{  2,  2 }		// 23 10 12 14 24
};
*/
coord_obj spiral_search[25] = 
{
    { -2, -2 },	{ -1, -2 },	{  0, -2 },	{  1, -2 },	{  2, -2 },		
    { -2, -1 },	{ -1, -1 },	{  0, -1 },	{  1, -1 },	{  2, -1 },		
    { -2,  0 },	{ -1,  0 },	{  0,  0 },	{  1,  0 },	{  2,  0 },		
    { -2,  1 },	{ -1,  1 },	{  0,  1 },	{  1,  1 },	{  2,  1 },		
    { -2,  2 },	{ -1,  2 },	{  0,  2 },	{  1,  2 },	{  2,  2 }
};

coord_obj part_start[4][4] = 
{
	{ {0, 0}, {0, 0}, {0, 0}, {0, 0} },
	{ {0, 0}, {0, 8}, {0, 0}, {0, 0} },
	{ {0, 0}, {8, 0}, {0, 0}, {0, 0} },
	{ {0, 0}, {8, 0}, {0, 8}, {8, 8} }
};


coord_obj part_end[4][4] = 
{
	{ {16, 16}, { 0,  0}, {0,  0}, {0,   0} },
	{ {16,  8}, {16, 16}, {0,  0}, {0,   0} },
	{ { 8, 16}, {16, 16}, {0,  0}, {0,   0} },
	{ { 8,  8}, {16,  8}, {8, 16}, {16, 16} }
};

coord_obj part_size[4][4] = 
{
	{ {16, 16}, { 0,  0}, {0, 0}, {0, 0} },
	{ {16,  8}, {16,  8}, {0, 0}, {0, 0} },
	{ { 8, 16}, { 8, 16}, {0, 0}, {0, 0} },
	{ { 8,  8}, { 8,  8}, {8, 8}, {8, 8} }
};

//////////////////////////////////////////////////////////////////////
//	Application Description											//
//////////////////////////////////////////////////////////////////////
//	1. Frame initialization yx and y4 for search					//
//////////////////////////////////////////////////////////////////////

int SATD(int* diff, int use_hadamard)
{
	int k, satd = 0, m[16], d[16];
  
	if (use_hadamard) {
		/*===== hadamard transform =====*/
		m[ 0] = diff[ 0] + diff[12];
		m[ 1] = diff[ 1] + diff[13];
		m[ 2] = diff[ 2] + diff[14];
		m[ 3] = diff[ 3] + diff[15];
		m[ 4] = diff[ 4] + diff[ 8];
		m[ 5] = diff[ 5] + diff[ 9];
		m[ 6] = diff[ 6] + diff[10];
		m[ 7] = diff[ 7] + diff[11];
		m[ 8] = diff[ 4] - diff[ 8];
		m[ 9] = diff[ 5] - diff[ 9];
		m[10] = diff[ 6] - diff[10];
		m[11] = diff[ 7] - diff[11];
		m[12] = diff[ 0] - diff[12];    
		m[13] = diff[ 1] - diff[13];        
		m[14] = diff[ 2] - diff[14];            
		m[15] = diff[ 3] - diff[15];

		d[ 0] = m[ 0] + m[ 4];
		d[ 1] = m[ 1] + m[ 5];
		d[ 2] = m[ 2] + m[ 6];
		d[ 3] = m[ 3] + m[ 7];
		d[ 4] = m[ 8] + m[12];
		d[ 5] = m[ 9] + m[13];
		d[ 6] = m[10] + m[14];
		d[ 7] = m[11] + m[15];
		d[ 8] = m[ 0] - m[ 4];
		d[ 9] = m[ 1] - m[ 5];
		d[10] = m[ 2] - m[ 6];
		d[11] = m[ 3] - m[ 7];
		d[12] = m[12] - m[ 8];
		d[13] = m[13] - m[ 9];
		d[14] = m[14] - m[10];
		d[15] = m[15] - m[11];
    
		m[ 0] = d[ 0] + d[ 3];
		m[ 1] = d[ 1] + d[ 2];
		m[ 2] = d[ 1] - d[ 2];
		m[ 3] = d[ 0] - d[ 3];
		m[ 4] = d[ 4] + d[ 7];
		m[ 5] = d[ 5] + d[ 6];
		m[ 6] = d[ 5] - d[ 6];
		m[ 7] = d[ 4] - d[ 7];
		m[ 8] = d[ 8] + d[11];
		m[ 9] = d[ 9] + d[10];
		m[10] = d[ 9] - d[10];
		m[11] = d[ 8] - d[11];
		m[12] = d[12] + d[15];
		m[13] = d[13] + d[14];
		m[14] = d[13] - d[14];
		m[15] = d[12] - d[15];
    
		d[ 0] = m[ 0] + m[ 1];
		d[ 1] = m[ 0] - m[ 1];
		d[ 2] = m[ 2] + m[ 3];
		d[ 3] = m[ 3] - m[ 2];
		d[ 4] = m[ 4] + m[ 5];
		d[ 5] = m[ 4] - m[ 5];
		d[ 6] = m[ 6] + m[ 7];
		d[ 7] = m[ 7] - m[ 6];
		d[ 8] = m[ 8] + m[ 9];
		d[ 9] = m[ 8] - m[ 9];
		d[10] = m[10] + m[11];
		d[11] = m[11] - m[10];
		d[12] = m[12] + m[13];
		d[13] = m[12] - m[13];
		d[14] = m[14] + m[15];
		d[15] = m[15] - m[14];
		/*===== sum up =====*/
		for (k=0; k<16; k++) {
			satd += abs(d[k]);
		}
		satd = ((satd+1)>>1);
	} else {
		/*===== sum up =====*/
		for (k=0; k<16; k++) {
			satd += abs(diff[k]);
		}
	} 

    satd = Clip(satd, 0, 4095);
	return satd;
}

xint me_downsample_frame(frame_obj *pFrame, int width, int height)
{
	uint8 *pY, *pY4, *pYX;
	xint i, j;

	pY	= pFrame->y;
	pY4 = pFrame->y4;

	// downsample from Y to Y4 by ((Y00 + Y01 + Y10 + Y11 + 2) / 4)
	for (j=0; j<height; j+=2) {
		for (i=0; i<width; i+= 2) {
			*pY4++ = (pY[j*width+i] + pY[j*width+(i+1)] + pY[(j+1)*width+i] + pY[(j+1)*width+(i+1)] + 2) >> 2;
		}
	}

	width >>= 1;
	height >>= 1;

	pYX	= pFrame->yx;
	pY4 = pFrame->y4;
	// downsample from Y4 to YX by ((Y00 + Y01 + Y10 + Y11 + 2) / 4)
	for (j=0; j<height; j+=2) {
		for (i=0; i<width; i+= 2) {
			*pYX++ = (pY4[j*width+i] + pY4[j*width+(i+1)] + pY4[(j+1)*width+i] + pY4[(j+1)*width+(i+1)] + 2) >> 2;
		}
	}
	return 0;
}

xint me_init_encoder(h264enc_obj *pEnc)
{
	xint bits, imax, imin, i;

	xint search_range				= 32;
	xint number_of_subpel_positions	= 4 * (2*search_range+3);
	xint max_mv_bits				= 3 + 2 * (int)ceil(log(number_of_subpel_positions+1) / log(2) + 1e-10);
	xint max_mvd					= (1<<(max_mv_bits >>1))-1;
	xint max_ref_bits				= 1 + 2 * (int)floor(log(16) / log(2) + 1e-10);
	xint max_ref					= (1<<((max_ref_bits>>1)+1))-1;

	absb = (uint8 *)malloc(2*256-1);
    if (absb == NULL)
    {
        alert_msg("me_init_encoder", "allocate abs array failed!");
        return -1;
    }
	mvbits	= (xint *)malloc((2*max_mvd+1)*sizeof(xint));
    if (mvbits == NULL)
    {
        alert_msg("me_init_encoder", "allocate mvbits array failed!");
        return -1;
    }
	refbits = (xint *)malloc(max_ref*sizeof(xint));
    if (refbits == NULL)
    {
        alert_msg("me_init_encoder", "allocate refbits array failed!");
        return -1;
    }

	absb += 255;
	for (i=-255; i<=255; i++)
		absb[i] = (i<0)?-i:i;
	
	mvbits += max_mvd;
	mvbits[0] = 1;
	for (bits=3; bits<=max_mv_bits; bits+=2) {
		imax = 1    << (bits >> 1);
		imin = imax >> 1;
		for (i = imin; i < imax; i++)
			mvbits[-i] = mvbits[i] = bits;
	}

	refbits[0] = 1;
	for (bits=3; bits<=max_ref_bits; bits+=2) {
		imax = (1   << ((bits >> 1) + 1)) - 1;
		imin = imax >> 1;
		for (i = imin; i < imax; i++)
			refbits[i] = bits;
	}

	pEnc->use_hardmard = 1;
	return MMES_NO_ERROR;
}

xint me_free_encoder(h264enc_obj *pEnc)
{
	xint search_range				= 32;
	xint number_of_subpel_positions	= 4 * (2*search_range+3);
	xint max_mv_bits				= 3 + 2 * (int)ceil(log(number_of_subpel_positions+1) / log(2) + 1e-10);
	xint max_mvd					= (1<<(max_mv_bits >>1))-1;

    if (absb) {
		absb -= 255;
		free(absb);
    }
	if (mvbits) {
		mvbits -= max_mvd;
		free(mvbits);
    }
    if (refbits) {
		free(refbits);
    }

	return MMES_NO_ERROR;
}

xint me_init_frame(h264enc_obj *pEnc)
{
	xint i;
	me_downsample_frame(pEnc->curf, pEnc->width, pEnc->height);
	for (i=0; i<pEnc->max_reff_no; i++)
		me_downsample_frame(pEnc->reff[i], pEnc->width, pEnc->height);
	return 0;
}

xint me_init_slice(h264enc_obj *pEnc)
{
	xint i;

	//: To do .... clean all motion predition at here

	for (i=0; i<4; i++)
		mv_pred_set_invalid(&thisme.leftpred[i]);

	return 0;
}

static
xint me_init_mb(h264enc_obj *pEnc)
{
	xint width, height, mbx, mby, i;
	mb_obj *pMB;

	width	= pEnc->width;
	height	= pEnc->height;

	pMB = &(pEnc->curr_slice->cmb);
	mbx = pEnc->curr_slice->cmb.id % pEnc->nx_mb;
	mby = pEnc->curr_slice->cmb.id / pEnc->nx_mb;

	load_block_16x16(thisme.cury1, pEnc->curf->y,  mbx<<4, mby<<4, width,   height  );
	load_block_8x8  (thisme.cury4, pEnc->curf->y4, mbx<<3, mby<<3, width/2, height/2);
	load_block_4x4  (thisme.curyx, pEnc->curf->yx, mbx<<2, mby<<2, width/4, height/4);

	for (i=0; i<4; i++)
		pMB->ref_idx[i] = 0;
	for (i=0; i<16; i++) {
		pMB->mv.x[i]	= 0;
		pMB->mv.y[i]	= 0;
	}

	if (mbx == 0) {
		for (i=0; i<4; i++)
			mv_pred_set_invalid(&thisme.leftpred[i]);
	}

	return 0;
}

static xint
me_finish_mb(h264enc_obj *pEnc)
{
	xint i;
	mb_obj *pMB;

	pMB = &(pEnc->curr_slice->cmb);

	for (i=0; i<4; i++) {
		pMB->best_8x8_blk_mode[i] = INTER_8x8;
	}
	switch (pMB->best_Inter_mode) {
	case INTER_16x16:
	case INTER_PSKIP:
		for (i=0; i<16; i++) {
			pMB->mv.x[i] = thisme.yqmv[0][0].x;
			pMB->mv.y[i] = thisme.yqmv[0][0].y;
		}
		break;
	case INTER_16x8:
		for (i=0; i<16; i++) {
			pMB->mv.x[i] = thisme.yqmv[1][i/8].x;
			pMB->mv.y[i] = thisme.yqmv[1][i/8].y;
		}
		break;
	case INTER_8x16:
		for (i=0; i<16; i++) {
			pMB->mv.x[i] = thisme.yqmv[2][(i/2)%2].x;
			pMB->mv.y[i] = thisme.yqmv[2][(i/2)%2].y;
		}
		break;
	case INTER_P8x8:
		for (i=0; i<16; i++) {
			pMB->mv.x[i] = thisme.yqmv[3][(i/8)*2+((i/2)%2)].x;
			pMB->mv.y[i] = thisme.yqmv[3][(i/8)*2+((i/2)%2)].y;
		}
		break;
	default:
		break;
	}
	for (i=0; i<4; i++) {
		thisme.leftpred[i].valid   = 1;
		thisme.leftpred[i].ref_idx = 0;
		thisme.leftpred[i].x	   = pMB->mv.x[(i<<2)+3];
		thisme.leftpred[i].y	   = pMB->mv.y[(i<<2)+3];
	}

    pMB->flag_cur_mb_mv_16x16_eq_skip = ((pMB->mv.x[0]==thisme.mvskip.x) && (pMB->mv.y[0]==thisme.mvskip.y));

	return 0;
}

static
uint8 getpel(uint8 *src, xint x, xint y, xint width, xint height)
{
	x = Clip(x, 0, width-1);
	y = Clip(y, 0, height-1);
	return src[y*width+x];
}


static void
load_block_12x12(uint8 dst[12][12], uint8 *src,	xint x,	xint y,	xint width,	xint height)
{
	xint i, j;
	if ((x>=0) && ((x+12)<=width) && (y>=0) && ((y+12)<=height)) {
		for (j=0; j<12; j++) {
			for (i=0; i<12; i++) {
				dst[j][i] = src[(y+j)*width+(x+i)];
			}
		}
	} else {
		for (j=0; j<12; j++) {
			for (i=0; i<12; i++) {
				dst[j][i] = getpel(src, x+i, y+j, width, height);
			}
		}
	}
}

static void
load_block_20x20(uint8 dst[20][20],	uint8 *src,	xint x,	xint y,	xint width,	xint height)
{
	xint i, j;
	if ((x>=0) && ((x+20)<=width) && (y>=0) && ((y+20)<=height)) {
		for (j=0; j<20; j++) {
			for (i=0; i<20; i++) {
				dst[j][i] = src[(y+j)*width+(x+i)];
			}
		}
	} else {
		for (j=0; j<20; j++) {
			for (i=0; i<20; i++) {
				dst[j][i] = getpel(src, x+i, y+j, width, height);
			}
		}
	}
}

static void
load_block_36x36(uint8 dst[36][36],	uint8 *src,	xint x,	xint y,	xint width,	xint height)
{
	xint i, j;
	if ((x>=0) && ((x+36)<=width) && (y>=0) && ((y+36)<=height)) {
		for (j=0; j<36; j++) {
			for (i=0; i<36; i++) {
				dst[j][i] = src[(y+j)*width+(x+i)];
			}
		}
	} else {
		for (j=0; j<36; j++) {
			for (i=0; i<36; i++) {
				dst[j][i] = getpel(src, x+i, y+j, width, height);
			}
		}
	}
}

static void
load_block_16x16(uint8 dst[16][16],	uint8 *src,	xint x,	xint y,	xint width,	xint height)
{
	xint i, j;
	if ((x>=0) && ((x+16)<=width) && (y>=0) && ((y+16)<=height)) {
		for (j=0; j<16; j++) {
			for (i=0; i<16; i++) {
				dst[j][i] = src[(y+j)*width+(x+i)];
			}
		}
	} else {
		for (j=0; j<16; j++) {
			for (i=0; i<16; i++) {
				dst[j][i] = getpel(src, x+i, y+j, width, height);
			}
		}
	}
}

static void
load_block_8x8(uint8 dst[8][8],	uint8 *src,	xint x,	xint y,	xint width,	xint height)
{
	xint i, j;
	if ((x>=0) && ((x+8)<=width) && (y>=0) && ((y+8)<=height)) {
		for (j=0; j<8; j++) {
			for (i=0; i<8; i++) {
				dst[j][i] = src[(y+j)*width+(x+i)];
			}
		}
	} else {
		for (j=0; j<8; j++) {
			for (i=0; i<8; i++) {
				dst[j][i] = getpel(src, x+i, y+j, width, height);
			}
		}
	}
}

static void
load_block_4x4(uint8 dst[4][4],	uint8 *src,	xint x,	xint y,	xint width,	xint height)
{
	xint i, j;
	if ((x>=0) && ((x+4)<=width) && (y>=0) && ((y+4)<=height)) {
		for (j=0; j<4; j++) {
			for (i=0; i<4; i++) {
				dst[j][i] = src[(y+j)*width+(x+i)];
			}
		}
	} else {
		for (j=0; j<4; j++) {
			for (i=0; i<4; i++) {
				dst[j][i] = getpel(src, x+i, y+j, width, height);
			}
		}
	}
}

static void
cal_ref_qpel_112x112(uint8 dst[112][112], uint8 src[20][20])
{
	xint is;
	xint i, j, i2, j2, m0, m1, m2, m3, m4, m5, n;
	xint tmp[28][56];
   

	for (j=-4; j<24; j++) { //honrizontal <-> 1/2 pel filtering
		n = Clip(j, 0, 19);
		for (i=-4; i<24; i++) {
			m0 = Clip(i-2,  0, 19);
			m1 = Clip(i-1,  0, 19);
			m2 = Clip(i,    0, 19);
			m3 = Clip(i+1,  0, 19);
			m4 = Clip(i+2,  0, 19);
			m5 = Clip(i+3,  0, 19);
			is = ( (20) * (src[n][m2] + src[n][m3]) +
				   (-5) * (src[n][m1] + src[n][m4]) +
				   ( 1) * (src[n][m0] + src[n][m5]));
            if (is < 0)
                is = 0;
			tmp[j+4][(i+4)*2]   = src[n][m2];   // 1/1 pix pos
			tmp[j+4][(i+4)*2+1] = min(((is+16)/32), 255);  // 1/2 pix pos
		}
	}
	for (j=0; j<28; j++) {
		m0 = Clip(j-2, 0, 27);
		m1 = Clip(j-1, 0, 27);
		m2 = Clip(j,   0, 27);
		m3 = Clip(j+1, 0, 27);
		m4 = Clip(j+2, 0, 27);
		m5 = Clip(j+3, 0, 27);
		for (i=0; i<56; i++) { //vertical | 1/2 pel filtering 
			is = (	(20) * (tmp[m2][i] + tmp[m3][i]) +
					(-5) * (tmp[m1][i] + tmp[m4][i]) +
					( 1) * (tmp[m0][i] + tmp[m5][i]) );
            if(is < 0)
                is = 0;
			dst[(j<<2)  ][i<<1] = Clip1Y(tmp[j][i]);
			dst[(j<<2)+2][i<<1] = Clip1Y((is+16)/32);
		}
	}
  
	// 1/4 pix
	for (j=0; j<112; j+=2) {
		j2 = min(110, j+2);
		for (i=0; i<112; i+=2) {
			i2 = min(112, i+2);
			//  '-' 
			dst[j][i+1] = Clip1Y((dst[j][i]+dst[j][i2]+1)/2);
			//  "|"
			dst[j+1][i] = Clip1Y((dst[j][i]+dst[j2][i]+1)/2);
			if (((j%4 == 0) && (i%4 == 0)) || ((j%4 == 2) && (i%4 == 2))) {
				//  "/"
				dst[j+1][i+1] = Clip1Y((dst[j2][i]+dst[j][i2]+1)/2);
			} else {
				//  "\"
				dst[j+1][i+1] = Clip1Y((dst[j][i]+dst[j2][i2]+1)/2);
			}
		}
	}
    return;
}

static void me_hierarchy_yx(h264enc_obj *pEnc)
{
	short pmvx, pmvy;
	xint mbx, mby, width, height;
	xint i, j, m, n, s, t;
	mb_obj *pMB = &pEnc->curr_slice->cmb;
	xint sad, minsad[2];

	mbx = (pMB->id) % pEnc->nx_mb;
	mby = (pMB->id) / pEnc->nx_mb;
	width = pEnc->width;
	height = pEnc->height;

	load_block_36x36(thisme.refyx[0], pEnc->reff[0]->yx, (mbx<<2)-16, (mby<<2)-16, width/4, height/4);
	me_pred_16x16_yx(pEnc->curr_slice, width, &pmvx, &pmvy);

	for (s=0; s<2; s++)
		minsad[s] = 0x7fffffff;

	// scan from top to down then left to right
	for (i=-ME_Y16_SEARCH_RANGE; i<=ME_Y16_SEARCH_RANGE; i++) {
    	for (j=-ME_Y16_SEARCH_RANGE; j<=ME_Y16_SEARCH_RANGE; j++) {		//: Search Range = -64 ~ +64
			sad = 0;
			for (n=0; n<4; n++)
				for (m=0; m<4; m++)
					sad += absb[thisme.curyx[n][m] - thisme.refyx[0][j+n+16][i+m+16]];
			sad *= 16;
			sad += MV_COST(pEnc->lambda_me, 4, i, j, pmvx, pmvy); // 16 * SAD + beta * MV

			//#define WEIGHTED_COST(factor,bits)  (((factor)*(bits))>>LAMBDA_ACCURACY_BITS)
			//#define MV_COST(f,s,cx,cy,px,py)    (WEIGHTED_COST(f, mvbits[((cx)<<(s))-px]+mvbits[((cy)<<(s))-py]))

			// 2 candidates for next motion search
			for (s=0; s<2; s++) {
				if (sad < minsad[s]) {
					for (t=1; t>s; t--) {
						minsad[t] = minsad[t-1];
						thisme.can[0][t].x = thisme.can[0][t-1].x;
						thisme.can[0][t].y = thisme.can[0][t-1].y;
					}
					minsad[s] = sad;
					thisme.can[0][s].x = i;
					thisme.can[0][s].y = j;
					break;
				}
			}
		}
	}
	for (s=0; s<2; s++) {
		thisme.can[0][s].x <<= 1;
		thisme.can[0][s].y <<= 1;
	}
	return;
}

static void me_hierarchy_y4(h264enc_obj *pEnc)
{
	short pmvx, pmvy;
	xint mbx, mby, width, height;
	xint i, j, m, n, s, t, k;
	mb_obj *pMB = &pEnc->curr_slice->cmb;
	xint sad, minsad[1];

	mbx = (pMB->id) % pEnc->nx_mb;
	mby = (pMB->id) / pEnc->nx_mb;
	width = pEnc->width;
	height = pEnc->height;

	me_pred_16x16_yx(pEnc->curr_slice, width, &pmvx, &pmvy);
	minsad[0] = 0x7ffffff;
	for (k=0; k<2; k++) {
		load_block_12x12(thisme.refy4[k], pEnc->reff[0]->y4, (mbx<<3)+thisme.can[0][k].x-2, (mby<<3)+thisme.can[0][k].y-2, width/2, height/2);
		// scan from top to down then left to right
		for (j=-2; j<=2; j++) {		//: Search Range = -16 ~ +16
			for (i=-2; i<=2; i++) {
				sad = 0;
				for (n=0; n<8; n++)
					for (m=0; m<8; m++)
						sad += absb[thisme.cury4[n][m] - thisme.refy4[k][j+n+2][i+m+2]];
				sad *= 4;
				sad += MV_COST(pEnc->lambda_me, 3, i+thisme.can[0][k].x, j+thisme.can[0][k].y, pmvx, pmvy); // 16 * SAD + beta * MV
				// 1 candidates for next motion search
				for (s=0; s<1; s++) {
					if (sad < minsad[s]) {
						for (t=0; t>s; t--) {
							minsad[t] = minsad[t-1];
							thisme.can[1][t].x = thisme.can[1][t-1].x;
							thisme.can[1][t].y = thisme.can[1][t-1].y;
						}
						minsad[s] = sad;
						thisme.can[1][s].x = i+thisme.can[0][k].x;
						thisme.can[1][s].y = j+thisme.can[0][k].y;
						break;
					}
				}
			}
		}
	}
	thisme.can[1][0].x <<= 1;
	thisme.can[1][0].y <<= 1;

    me_pred_skip_y4(pEnc->curr_slice, width, &pmvx, &pmvy);

    thisme.can[1][1].x = pmvx/4 + (pmvx%4)/2;
    thisme.can[1][1].y = pmvy/4 + (pmvy%4)/2;
	
	return;
}

static xint
me_part_search_fme(h264enc_obj *pEnc, xint k, xint mode, xint part)
{
	xint  diff[16];
	xint  mbx, mby, search_abort;
	xint  i, j, m, n, s, ix, iy, mcost;
	xint  minsad, stx, sty, bestx, besty;
	xint  lambda;
	int16 pmvx, pmvy;
	mb_obj *pMB;
	xint  check_position_zero;

    int   mvcost_temp  =0;
    int   satd_cost    =0;
    int   min_satd_cost=INT_MAX;

	
	check_position_zero =	(mode == 0) &&
							(!thisme.can[2+mode][part].x) &&
							(!thisme.can[2+mode][part].y) && 
							(pEnc->use_hardmard)		  &&
							(pEnc->curr_slice->type != B_SLICE);

	pMB = &(pEnc->curr_slice->cmb);
	mbx = (pMB->id) % pEnc->nx_mb;
	mby = (pMB->id) / pEnc->nx_mb;
	lambda = pEnc->lambda_me;

	if (mode < 4) {
		// motion prediction
		me_pred_yq(pEnc->curr_slice, pEnc->width, mode, part, &pmvx, &pmvy);
		// half-pel search
		minsad = 0x7fffffff;
		for (j=-2; j<=2; j+=2) {
			n	= j + thisme.can[2+mode][part].y;
			sty = j + thisme.can[2+mode][part].y - (thisme.can[1][k].y<<2) + 24;
			for (i=-2; i<=2; i+=2) {
				m	= i + thisme.can[2+mode][part].x;
				stx = i + thisme.can[2+mode][part].x - (thisme.can[1][k].x<<2) + 24;
				mvcost_temp = MV_COST(lambda, 0, m, n, pmvx, pmvy);
				if (check_position_zero && (!i) && (!j)) {
					mvcost_temp -= WEIGHTED_COST(lambda, 16);
				}

				search_abort = 0;
                mcost    =0;
            	satd_cost=0;

				for (iy=part_start[mode][part].y; iy<part_end[mode][part].y; iy+=4) {
					for (ix=part_start[mode][part].x; ix<part_end[mode][part].x; ix+=4) {
						for (s=0; s<16; s++) {
							diff[s] = thisme.cury1[iy+(s>>2)][ix+(s%4)] - thisme.refyq[k][sty+iy*4+(s>>2)*4][stx+ix*4+(s%4)*4];
						}
						mcost     += SATD(diff, 0); // HME only use SAD
                        satd_cost += SATD(diff, pEnc->use_hardmard);
						//if (mcost > minsad) {
						//	search_abort = 1;
						//	break;
						//}
					}
					//if (search_abort)
					//	break;
				}
                
                mcost    += mvcost_temp;
                if (mcost < 0)
                    mcost = 0;

                satd_cost+= mvcost_temp;                
                if(satd_cost < 0)
                    satd_cost = 0;

                if (mcost < minsad) {
					minsad        = mcost;
					bestx         = m;
					besty         = n;
                    min_satd_cost = satd_cost;
				}
			}
		}

		thisme.yqmv[mode][part].x  = bestx;
		thisme.yqmv[mode][part].y  = besty;
		// quater-pel search
		for (j=-1; j<=1; j++) {
			n	= j + besty;
			sty = j + besty - (thisme.can[1][k].y<<2) + 24;
			for (i=-1; i<=1; i++) {
				m	= i + bestx;
				stx = i + bestx - (thisme.can[1][k].x<<2) + 24;
				mcost    = MV_COST(lambda, 0, m, n, pmvx, pmvy);
                satd_cost=mcost;
				//if (mcost >= minsad) {
				//	continue;
				//}
				search_abort = 0;
				for (iy=part_start[mode][part].y; iy<part_end[mode][part].y; iy+=4) {
					for (ix=part_start[mode][part].x; ix<part_end[mode][part].x; ix+=4) {
						for (s=0; s<16; s++) {
							diff[s] = thisme.cury1[iy+(s>>2)][ix+(s%4)] - thisme.refyq[k][sty+iy*4+(s>>2)*4][stx+ix*4+(s%4)*4];
						}

						mcost     += SATD(diff, 0);                  // QME uses only SAD
                        satd_cost += SATD(diff, pEnc->use_hardmard); // use_hardmard
						//if (mcost > minsad) {
						//	search_abort = 1;
						//	break;
						//}
					}
					//if (search_abort)
					//	break;
				}
				if (mcost < minsad) {
					minsad = mcost;
					thisme.yqmv[mode][part].x = m;
					thisme.yqmv[mode][part].y = n;
                    min_satd_cost = satd_cost;
				}
			}
		}

		thisme.yqcst[mode][part]   = minsad;
		for (j=part_start[mode][part].y; j<part_end[mode][part].y; j+=4) {
			for (i=part_start[mode][part].y; i<part_end[mode][part].y; i+=4) {
				pMB->mv.x[j+i/4] = thisme.yqmv[mode][part].x;
				pMB->mv.y[j+i/4] = thisme.yqmv[mode][part].y;
			}
		}
        minsad = min_satd_cost;
	} else { // skip
		sty = thisme.can[6][0].y - (thisme.can[1][k].y<<2) + 24;
		stx = thisme.can[6][0].x - (thisme.can[1][k].x<<2) + 24;
		minsad = 0;
		for (iy=0; iy<16; iy+=4) {
			for (ix=0; ix<16; ix+=4) {
				for (s=0; s<16; s++) {
					diff[s] = thisme.cury1[iy+(s>>2)][ix+(s%4)] - thisme.refyq[k][sty+iy*4+(s>>2)*4][stx+ix*4+(s%4)*4];
				}
				minsad += SATD(diff, 1); // use_hardmard
			}
		}
	}
	
	return minsad;
}


static void me_hierarchy_y1(h264enc_obj *pEnc)
{
	xint  k, pos, m, n, i, sad, part;
	xint  mbx, mby, width, height, minsad, mcost, bmode;
	int16 pmvx, pmvy;

    xint  ime_cost[]={INT_MAX,0,0,0,0};  // 2:16x8, 3: 8x16, 4: 8x8
    xint  fme_cost[]={INT_MAX,0,0,0,0};  // 2:16x8, 3: 8x16, 4: 8x8
    xint  best_sub_blk_mode, min_cost = INT_MAX;


	mbx = (pEnc->curr_slice->cmb.id) % pEnc->nx_mb;
	mby = (pEnc->curr_slice->cmb.id) / pEnc->nx_mb;
	width = pEnc->width;
	height = pEnc->height;

	me_setup_block_sad(pEnc);

	for (k=0; k<2; k++) {
		load_block_20x20(thisme.refy1[k], pEnc->reff[0]->y, (mbx<<4)+thisme.can[1][k].x-2, (mby<<4)+thisme.can[1][k].y-2, width, height);
		cal_ref_qpel_112x112(thisme.refyq[k], thisme.refy1[k]);
	}

	//16x16
	me_pred_16x16_yx(pEnc->curr_slice, width, &pmvx, &pmvy);
	thisme.y1cst[0][0] = 0x7fffffff;
	for (k=0; k<2-h264_bw_lite; k++) {
		for (pos=0; pos<25; pos++) {
			sad = 0;
			for (i=0; i<4; i++)
				sad += thisme.sady1[k][pos][i];
			if (sad < thisme.y1cst[0][0]) {
				m = spiral_search[pos].x + (thisme.can[1][k].x);
				n = spiral_search[pos].y + (thisme.can[1][k].y);
				sad += MV_COST(pEnc->lambda_me, 2, m, n, pmvx, pmvy);
				if (sad < thisme.y1cst[0][0]) {
					thisme.y1cst[0][0] = sad;
					thisme.can[2][0].x = m<<2;
					thisme.can[2][0].y = n<<2;
					thisme.y1can[0][0]   = k;
				}
			}
		}
	}
    ime_cost[ME_16x16] = thisme.y1cst[0][0];
	fme_cost[ME_16x16] = minsad = mcost = me_part_search_fme(pEnc, thisme.y1can[0][0], 0, 0);
	//bmode  = INTER_16x16;

	// check inter skip mode
	me_pred_skip_y1(pEnc->curr_slice, width, &pmvx, &pmvy);
	if (!h264_bw_lite && (pmvx <= (thisme.can[1][1].x*4+8)) && (pmvx >= (thisme.can[1][1].x*4-8)) &&
			(pmvy <= (thisme.can[1][1].y*4+8)) && (pmvy >= (thisme.can[1][1].y*4-8))) {
		    thisme.can[6][0].x = pmvx;
		    thisme.can[6][0].y = pmvy;
		    mcost = me_part_search_fme(pEnc, 1, 4, 0); // 1 : candidates 1, 4: skip mode
		    mcost -= WEIGHTED_COST(pEnc->lambda_me, 8);
            if (mcost < 0)
                mcost = 0;
            if (mcost <= minsad) {
                fme_cost[ME_16x16]  = minsad = mcost;
			    thisme.yqmv[0][0].x = pmvx;
			    thisme.yqmv[0][0].y = pmvy;
		    }
	} else if ((pmvx <= (thisme.can[1][0].x*4+8)) && (pmvx >= (thisme.can[1][0].x*4-8)) &&
			(pmvy <= (thisme.can[1][0].y*4+8)) && (pmvy >= (thisme.can[1][0].y*4-8))) {
		thisme.can[6][0].x = pmvx;
		thisme.can[6][0].y = pmvy;
		mcost = me_part_search_fme(pEnc, 0, 4, 0); // 4: skip mode
		mcost -= pEnc->lambda_skip;
        if (mcost < 0)
            mcost = 0;
		if (mcost <= minsad) {
			fme_cost[ME_16x16]  = minsad = mcost;
			thisme.yqmv[0][0].x = pmvx;
			thisme.yqmv[0][0].y = pmvy;
		}
	}

	//16x8
	mcost = 0;
	for (part=0; part<2; part++) {
		me_pred_16x16_yx(pEnc->curr_slice, width, &pmvx, &pmvy);
		thisme.y1cst[1][part] = 0x7fffffff;
		for (k=0; k< 2-h264_bw_lite ; k++) {
			for (pos=0; pos<25; pos++) {
				sad = thisme.sady1[k][pos][part*2] + thisme.sady1[k][pos][2*part+1];
				if (sad < thisme.y1cst[1][part]) {
					m = spiral_search[pos].x + (thisme.can[1][k].x);
					n = spiral_search[pos].y + (thisme.can[1][k].y);
					sad += MV_COST(pEnc->lambda_me, 2, m, n, pmvx, pmvy);
					if (sad < thisme.y1cst[1][part]) {
						thisme.y1cst[1][part] = sad;
						thisme.can[3][part].x = m << 2;
						thisme.can[3][part].y = n << 2;
						thisme.y1can[1][part] = k;
					}
				}
			}
		}
        ime_cost[ME_16x8] += thisme.y1cst[1][part];
		mcost += me_part_search_fme(pEnc, thisme.y1can[1][part], 1, part);
	}

    fme_cost[ME_16x8] = mcost;

    /*
	if (mcost < minsad) {
		minsad = mcost;
		bmode  = INTER_16x8;
	}
    */

	//8x16
	mcost = 0;
	for (part=0; part<2; part++) {
		me_pred_16x16_yx(pEnc->curr_slice, width, &pmvx, &pmvy);
		thisme.y1cst[2][part] = 0x7fffffff;
		for (k=0; k<2-h264_bw_lite; k++) {
			for (pos=0; pos<25; pos++) {
				sad = thisme.sady1[k][pos][part] + thisme.sady1[k][pos][part+2];
				if (sad < thisme.y1cst[2][part]) {
					m = spiral_search[pos].x + (thisme.can[1][k].x);
					n = spiral_search[pos].y + (thisme.can[1][k].y);
					sad += MV_COST(pEnc->lambda_me, 2, m, n, pmvx, pmvy);
					if (sad < thisme.y1cst[2][part]) {
						thisme.y1cst[2][part] = sad;
						thisme.can[4][part].x = m << 2;
						thisme.can[4][part].y = n << 2;
						thisme.y1can[2][part] = k;
					}
				}
			}
		}
        ime_cost[ME_8x16] += thisme.y1cst[2][part];
		mcost += me_part_search_fme(pEnc, thisme.y1can[2][part], 2, part);
	}

    fme_cost[ME_8x16] = mcost;

    /*
	if (mcost < minsad) {
		minsad = mcost;
		bmode  = INTER_8x16;
	}
    */

	//8x8
	mcost = 0;
	for (part=0; part<4; part++) {
		me_pred_16x16_yx(pEnc->curr_slice, width, &pmvx, &pmvy);
		thisme.y1cst[3][part] = 0x7fffffff;
		for (k=0; k<2-h264_bw_lite; k++) {
			for (pos=0; pos<25; pos++) {
				sad = thisme.sady1[k][pos][part];
				if (sad < thisme.y1cst[3][part]) {
					m = spiral_search[pos].x + (thisme.can[1][k].x);
					n = spiral_search[pos].y + (thisme.can[1][k].y);
					sad += MV_COST(pEnc->lambda_me, 2, m, n, pmvx, pmvy);
					if (sad < thisme.y1cst[3][part]) {
						thisme.y1cst[3][part] = sad;
						thisme.can[5][part].x = m << 2;
						thisme.can[5][part].y = n << 2;
						thisme.y1can[3][part] = k;
					}
				}
			}
		}

        ime_cost[ME_8x8] += thisme.y1cst[3][part];
        
		mcost += me_part_search_fme(pEnc, thisme.y1can[3][part], 3, part);
		mcost += 0; //: REF_COEF
	}
    if (mcost < 0)
        mcost = 0;
    fme_cost[ME_8x8] = mcost;
    /*
	if (mcost < minsad) {
		minsad = mcost;
		bmode  = INTER_P8x8;
	}
    */

    /* choose the smallest cost among 16x8, 8x16, 8x8 */
    for(i = ME_16x8 ; i <= ME_8x8 ; i++)
    {
        if(min_cost > ime_cost[i])
        {
            min_cost          = ime_cost[i];
            best_sub_blk_mode = i;
        }
    }

    if(fme_cost[best_sub_blk_mode] < fme_cost[ME_16x16])
    {
        minsad = fme_cost[best_sub_blk_mode];
        switch(best_sub_blk_mode)
        {
        case ME_16x8:
            bmode  = INTER_16x8;
            break;
        case ME_8x16:
            bmode  = INTER_8x16;
            break;
        case ME_8x8:
            bmode  = INTER_P8x8;
            break;
        default:
            fprintf(stderr, "Error : me_hierarchy_y1 at %d line.\n", __LINE__);
            exit(1);
        }
    }
    else
    {
        minsad = fme_cost[ME_16x16];
        bmode  = INTER_16x16;
    }

	pEnc->curr_slice->cmb.best_Inter_mode = bmode;
	pEnc->curr_slice->cmb.best_Inter_cost = minsad;
}

static xint me_setup_block_sad(h264enc_obj *pEnc)
{
	xint k, pos, m, n, i, j;
	xint mbx, mby, width, height;

	mbx = (pEnc->curr_slice->cmb.id) % pEnc->nx_mb;
	mby = (pEnc->curr_slice->cmb.id) / pEnc->nx_mb;
	width = pEnc->width;
	height = pEnc->height;

	for (k=0; k<2; k++) {
		m = (mbx<<4) + thisme.can[1][k].x - 2;
		n = (mby<<4) + thisme.can[1][k].y - 2;
		load_block_20x20(thisme.refy1[k], pEnc->reff[0]->y, m, n, width, height);
		for (pos=0; pos<25; pos++) { //: Search Range = -2 ~ +2
			m = spiral_search[pos].x + 2;
			n = spiral_search[pos].y + 2;
			for (i=0; i<4; i++)
				thisme.sady1[k][pos][i] = 0;
			for (j=0; j<16; j++)
				for (i=0; i<16; i++)
					thisme.sady1[k][pos][(j>>3)*2+(i>>3)] += absb[thisme.cury1[j][i] - thisme.refy1[k][n+j][m+i]];
		}
	}
	return MMES_NO_ERROR;

}


xint enc_inter_mode_select_mb(h264enc_obj *pEnc)
{
	me_init_mb(pEnc);

	me_hierarchy_yx(pEnc);
	me_hierarchy_y4(pEnc);
	me_hierarchy_y1(pEnc);

	me_finish_mb(pEnc);

	return MMES_NO_ERROR;

}

//: added for motion search
static xint
me_pred_16x16_yx(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy)
{
	xint predx, predy;
	mb_obj *pMB = &(pSlice->cmb);

	pMB->best_Inter_mode = INTER_16x16;

	/* set pred_X */
	pred_X.valid		 = 1;
	pred_X.ref_idx		 = pMB->ref_idx[0];
	pred_X.x			 = pMB->mv.x[0];
	pred_X.y			 = pMB->mv.y[0];

	/* get neighboring block */
	mv_pred_get_neighbor(pSlice, width, 0, 0);

	// use preditor D instead of A
	pred_A = pred_D;

	mv_pred_get_prd_mv(pSlice, 0, &predx, &predy);

	*pmvx = predx;
	*pmvy = predy;

	return MMES_NO_ERROR;
}

static xint
me_pred_skip_y4(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy)
{
	xint predx, predy, zeroLeft, zeroAbove;
	mb_obj *pMB = &(pSlice->cmb);
	xint inter_mode;

	inter_mode = pMB->best_Inter_mode;
	pMB->best_Inter_mode = INTER_PSKIP;

	/* set pred_X */
	pred_X.valid		 = 1;
	pred_X.ref_idx		 = pMB->ref_idx[0];
	pred_X.x			 = pMB->mv.x[0];
	pred_X.y			 = pMB->mv.y[0];

	/* get neighboring block */
	mv_pred_get_neighbor(pSlice, width, 0, 0);

	zeroLeft  = (!pred_A.valid)?1:0;
	zeroAbove = (!pred_B.valid)?1:(pred_B.ref_idx==0 && pred_B.x==0 && pred_B.y==0);
	
	if (zeroLeft || zeroAbove) {
		*pmvx = 0;
		*pmvy = 0;
	} else {
		pred_A.ref_idx = -1;
		pred_A.x	   = 0;
		pred_A.y	   = 0;
		mv_pred_get_prd_mv(pSlice, 0, &predx, &predy);
		*pmvx = predx;
		*pmvy = predy;
	}

	pMB->best_Inter_mode = inter_mode;

	return MMES_NO_ERROR;
}

static xint
me_pred_skip_y1(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy)
{
	xint predx, predy, zeroLeft, zeroAbove, inter_mode;
	mb_obj *pMB = &(pSlice->cmb);

	inter_mode = pMB->best_Inter_mode;
	pMB->best_Inter_mode = INTER_PSKIP;

	/* set pred_X */
	pred_X.valid		 = 1;
	pred_X.ref_idx		 = pMB->ref_idx[0];
	pred_X.x			 = pMB->mv.x[0];
	pred_X.y			 = pMB->mv.y[0];

	/* get neighboring block */
	mv_pred_get_neighbor(pSlice, width, 0, 0);

	zeroLeft  = (!pred_A.valid)?1:(pred_A.ref_idx==0 && pred_A.x==0 && pred_A.y==0);
	zeroAbove = (!pred_B.valid)?1:(pred_B.ref_idx==0 && pred_B.x==0 && pred_B.y==0);
	
	if (zeroLeft || zeroAbove) {
		*pmvx = 0;
		*pmvy = 0;
	} else {
		mv_pred_get_prd_mv(pSlice, 0, &predx, &predy);
		*pmvx = predx;
		*pmvy = predy;
	}

	pMB->best_Inter_mode = inter_mode;

	thisme.mvskip.x = *pmvx;
	thisme.mvskip.y = *pmvy;

	return MMES_NO_ERROR;
}

static xint
me_pred_yq(slice_obj *pSlice, xint width, xint mode, xint part, int16 *pmvx, int16 *pmvy)
{
	switch (mode) {
	case 0:
		me_pred_16x16_yq(pSlice, width, pmvx, pmvy);
		break;
	case 1:
		me_pred_16x8_yq(pSlice, width, part, pmvx, pmvy);
		break;
	case 2:
		me_pred_8x16_yq(pSlice, width, part, pmvx, pmvy);
		break;
	case 3:
		me_pred_8x8_yq(pSlice, width, part, pmvx, pmvy);
		break;
	default:
		break;
	}

	return MMES_NO_ERROR;
}

static xint
me_pred_16x16_yq(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy)
{
	xint predx, predy;
	mb_obj *pMB = &(pSlice->cmb);

	pMB->best_Inter_mode = INTER_16x16;

	/* set pred_X */
	pred_X.valid		 = 1;
	pred_X.ref_idx		 = pMB->ref_idx[0];
	pred_X.x			 = pMB->mv.x[0];
	pred_X.y			 = pMB->mv.y[0];

	/* get neighboring block */
	mv_pred_get_neighbor(pSlice, width, 0, 0);

	pred_A = thisme.leftpred[0];

	mv_pred_get_prd_mv(pSlice, 0, &predx, &predy);

	*pmvx = predx;
	*pmvy = predy;

	return MMES_NO_ERROR;
}

static xint
me_pred_16x8_yq(slice_obj *pSlice, xint width, xint part, int16 *pmvx, int16 *pmvy)
{
	xint predx, predy;
	mb_obj *pMB = &(pSlice->cmb);

	pMB->best_Inter_mode = INTER_16x8;
	pred_X.valid		 = 1;
	pred_X.ref_idx		 = pMB->ref_idx[ mb_part_16x8_idx[part] ];
	pred_X.x			 = pMB->mv.x[ mb_part_16x8_idx[part] ];
	pred_X.y			 = pMB->mv.y[ mb_part_16x8_idx[part] ];

	/* get neighboring block */
	mv_pred_get_neighbor (pSlice, width, part, 0);

	if (part) {
		pred_B.valid   = 1;
		pred_B.ref_idx = 0;
		pred_B.x	   = thisme.yqmv[1][0].x;
		pred_B.y	   = thisme.yqmv[1][0].y;
		pred_A = thisme.leftpred[2];
		pred_D = thisme.leftpred[1];
	} else {
		pred_A = thisme.leftpred[0];
	}

	mv_pred_get_prd_mv (pSlice, part, &predx, &predy);

	*pmvx = predx;
	*pmvy = predy;

	return MMES_NO_ERROR;
}

static xint
me_pred_8x16_yq(slice_obj *pSlice, xint width, xint part, int16 *pmvx, int16 *pmvy)
{
	xint predx, predy;
	mb_obj    *pMB = &(pSlice->cmb);

	pMB->best_Inter_mode = INTER_8x16;

	/* set pred_X */
	pred_X.valid = 1;
	pred_X.ref_idx = pMB->ref_idx[ part ];
	pred_X.x = pMB->mv.x[ (part<<1) ];
	pred_X.y = pMB->mv.y[ (part<<1) ];

	/* get neighboring block */
	mv_pred_get_neighbor( pSlice, width, part, 0);

	if (part) {
		pred_A.valid   = 1;
		pred_A.ref_idx = 0;
		pred_A.x	   = thisme.yqmv[2][0].x;
		pred_A.y	   = thisme.yqmv[2][0].y;
	} else {
		pred_A = thisme.leftpred[0];
	}

	mv_pred_get_prd_mv(pSlice, part, &predx, &predy);

	*pmvx = predx;
	*pmvy = predy;

	return MMES_NO_ERROR;
}

static xint
me_pred_8x8_yq(slice_obj *pSlice, xint width, xint part, int16 *pmvx, int16 *pmvy)
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint predx, predy, i, xoffset;

	pMB->best_Inter_mode = INTER_P8x8;
	for (i=0; i<4; i++) {
		pMB->best_8x8_blk_mode[part] = INTER_8x8;
	}

	/* get neighboring block */
	mv_pred_get_neighbor(pSlice, width, part, 0);

	switch (part) {
	case 1:
		xoffset = (pMB->id % (width/16)) * 4 + 1;
		pred_A.valid   = 1;
		pred_A.ref_idx = 0;
		pred_A.x	   = thisme.yqmv[3][0].x;
		pred_A.y	   = thisme.yqmv[3][0].y;
		pred_D.valid   = pSlice->top_valid[xoffset];
		pred_D.ref_idx = pSlice->top_ref_idx[xoffset/2];
		pred_D.x	   = pSlice->top_mvx[xoffset];
		pred_D.y	   = pSlice->top_mvy[xoffset];
		break;
	case 2:
		pred_B.valid   = 1;
		pred_B.ref_idx = 0;
		pred_B.x	   = thisme.yqmv[3][0].x;
		pred_B.y	   = thisme.yqmv[3][0].y;
		pred_C.valid   = 1;
		pred_C.ref_idx = 0;
		pred_C.x	   = thisme.yqmv[3][1].x;
		pred_C.y	   = thisme.yqmv[3][1].y;
		pred_A		   = thisme.leftpred[2];
		pred_D		   = thisme.leftpred[1];
		break;
	case 3:
		pred_A.valid   = 1;
		pred_A.ref_idx = 0;
		pred_A.x	   = thisme.yqmv[3][2].x;
		pred_A.y	   = thisme.yqmv[3][2].y;
		pred_B.valid   = 1;
		pred_B.ref_idx = 0;
		pred_B.x	   = thisme.yqmv[3][1].x;
		pred_B.y	   = thisme.yqmv[3][1].y;
		pred_D.valid   = 1;
		pred_D.ref_idx = 0;
		pred_D.x	   = thisme.yqmv[3][0].x;
		pred_D.y	   = thisme.yqmv[3][0].y;
		break;
	default:
		pred_A		   = thisme.leftpred[0];
		break;
	}

	mv_pred_get_prd_mv(pSlice, part, &predx, &predy);

	*pmvx = predx;
	*pmvy = predy;

	return MMES_NO_ERROR;
}

void enc_skip_mode_select_mb(h264enc_obj *pEnc)
{
	mb_obj *pMB = &(pEnc->curr_slice->cmb);
	
	if (pMB->best_Inter_mode==INTER_16x16) {
		if ((pMB->mv.x[0]==thisme.mvskip.x) && (pMB->mv.y[0]==thisme.mvskip.y) && (pMB->cbp==0)) {
			pMB = &(pEnc->curr_slice->cmb);
			pMB->best_Inter_mode = INTER_PSKIP;
		}
	}
}
/* ///////////////////////////////////////////////////////////// */
/*   File: mv_search.c                                           */
/*   Author: Jerry Peng                                          */
/*   Date: Oct/06/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 Motion Estimation Module.        */
/*                                                               */
/*   Copyright, 2005.                                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "../../common/inc/mv_search.h"
#include "../../common/inc/mv_pred.h"
#include "../../common/inc/misc_util.h"

//////////////////////////////////////////////////////////////////////
//  Macro Definition												//
//////////////////////////////////////////////////////////////////////

#define ME_STAGE_Y4     		0
#define ME_STAGE_YQ		        1
#define ME_STAGE_SZ		        2

#define ME_Y16_SEARCH_RANGE     16

#define ME_16x16                1
#define ME_16x8                 2
#define ME_8x16                 3
#define ME_8x8                  4

#define Clip(x, l, u)	(((x)<=(l))?(l):(((x)>(u))?(u):(x)))
#define Clip1Y(x)		Clip(x, 0, 255)
#if __CC_ARM
#define min(a, b)		(((xint)a>(xint)b)?b:a)
#endif

#define WEIGHTED_COST(factor,bits)  (((factor)*(bits))>>LAMBDA_ACCURACY_BITS)
#define MV_COST(f,s,cx,cy,px,py)    (WEIGHTED_COST(f, mvbits[((cx)<<(s))-px]+mvbits[((cy)<<(s))-py]))
#define REF_COST(f,ref,list_offset) (WEIGHTED_COST(f,((listXsize[list_offset]<=1)? 0:refbits[(ref)])))
	
//////////////////////////////////////////////////////////////////////
//  Data Structure Definition										//
//////////////////////////////////////////////////////////////////////
/*
typedef struct _coorf_obj_
{
	int16	x;
	int16	y;
} coord_obj;

typedef struct _me_obj_
{
	coord_obj	can[7][4];				// candidates for 7 levels search and 4 targets
										// level 0 : YX
										// level 1 : Y4
										// level 2 : Y1 mode INTER_16x16
										// level 3 : Y1 mode INTER_16x8
										// level 4 : Y1 mode INTER_8x16
										// level 5 : Y1 mode INTER_8x8
										// level 6 : Y1 mode INTER_PSKIP
	coord_obj	yqmv[4][4];				// best mv for 4 modes (16x16, 16x8, 8x16, 8x8) and 4 blocks
	xint		yqcst[4][4];			// min cost for 4 modes (16x16, 16x8, 8x16, 8x8) and 4 blocks
	xint		y1cst[4][4];			// min cost for 4 modes (16x16, 16x8, 8x16, 8x8) and 4 blocks
	xint		y1can[4][4];			// best candidate for 4 modes and 4 blocks
	uint8		curyx[4][4];			// source Y16
	uint8		cury4[8][8];			// source Y4
	uint8		cury1[16][16];			// source Y
	uint8		refyx[1][36][36];		// full search
	uint8		refy4[3][12][12];		// 3 candidates
	uint8		refy1[2][20][20];		// 2 candidates
	uint8		refyq[2][112][112];		// 2 candidates
	xint		bmode;
	xint		sady1[2][25][4];		// sad for 2 candidates, 25 positions and 4 blocks
	coord_obj	mvskip;					// skip mv predictor
	pred_obj	leftpred[4];			// left predictor for motition estimation
} me_obj;

*/
//////////////////////////////////////////////////////////////////////
// Function Declaration												//
//////////////////////////////////////////////////////////////////////

static uint8 getpel(uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_16x16(uint8 dst[16][16], uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_8x8(uint8 dst[8][8], uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_4x4(uint8 dst[4][4], uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_12x12(uint8 dst[12][12], uint8 *src, xint x, xint y, xint width, xint height);
static void load_block_20x20(uint8 dst[20][20], uint8 *src, xint x, xint y, xint width, xint height);
static xint me_pred_16x16_yx(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy);
static xint me_pred_skip_y4(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy);
static xint me_pred_skip_y1(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy);
static xint me_pred_yq(slice_obj *pSlice, xint width, xint mode, xint part, int16 *pmvx, int16 *pmvy);
static xint me_pred_16x16_yq(slice_obj *pSlice, xint width, int16 *pmvx, int16 *pmvy);
static xint me_pred_16x8_yq(slice_obj *pSlice, xint width, xint part, int16 *pmvx, int16 *pmvy);
static xint me_pred_8x16_yq(slice_obj *pSlice, xint width, xint part, int16 *pmvx, int16 *pmvy);
static xint me_pred_8x8_yq(slice_obj *pSlice, xint width, xint part, int16 *pmvx, int16 *pmvy);
static xint me_setup_block_sad(h264enc_obj *pEnc);

static xint me_init_mb(h264enc_obj *pEnc);
static xint me_finish_mb(h264enc_obj *pEnc);

//////////////////////////////////////////////////////////////////////
//  Static and External Data										//
//////////////////////////////////////////////////////////////////////

me_obj		thisme;
uint8		*absb;
xint		*mvbits;
xint		*refbits;
//xint		listXsize[] = {1, 0, 2, 0, 2, 0};

// for motion prediction
extern pred_obj	pred_A, pred_B, pred_C, pred_D, pred_X;
extern mb_part_16x8_idx[];

