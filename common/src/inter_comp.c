/* ///////////////////////////////////////////////////////////// */
/*   File: inter_comp.c                                          */
/*   Author: Ryan Lin                                            */
/*   Date: Oct/05/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 motion compensation module.      */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Oct/05/2005 Ryan Lin, Initial							     */
/* ///////////////////////////////////////////////////////////// */
#include <string.h>
#include "../../common/inc/inter_comp.h"

#define max(a, b)				(((xint)a>(xint)b)?a:b)
#define min(a, b)				(((xint)a>(xint)b)?b:a)
#define Clip(x, lower, upper)	max(lower, min(upper, x))
#define Clip1Y(x)				Clip(x, 0, 255)
#define Clip3(x)				Clip(x, 0, 255)
#define blkcpy(dst, src, stx, sty, szx, szy, w)							\
					{													\
						xint i, j;										\
						for (j=0; j<szy; j++) 							\
							for (i=0; i<szx; i++)						\
								dst[(sty+j)*w+stx+i] = src[j*szx+i];	\
					}


uint8 getpel(uint8 *pSrc, xint x, xint y, xint width, xint height);
xint get_inter_luma(int16 *pDst, uint8 *pSrc, xint posx, xint posy, xint blockx, xint blocky, xint width, xint height);
xint get_inter_chroma(int16 *pDst, uint8 *pSrc, xint posx, xint posy, xint blockx, xint blocky, xint width, xint height);
xint get_inter_4x4_luma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height);
xint get_inter_4x8_luma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height);
xint get_inter_8x4_luma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height);
xint get_inter_8x8_luma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height);
xint get_inter_2x2_chroma(int16 *dst, uint8 *pSrc,xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height);
xint get_inter_4x2_chroma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height);
xint get_inter_2x4_chroma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height);
xint get_inter_4x4_chroma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height);
xint get_inter_comp_mb(slice_obj *pSlice, frame_obj **ppfr, xint width, xint height);

xint dec_cal_inter_comp_mb(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Ryan Lin                                            */
/*   Date  : Oct/05/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Calculate compensation Luma and chroma data                 */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] Decoder object                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Oct/05/2005 Ryan Lin, Initial                               */
/* ------------------------------------------------------------- */
{
	xint   status;
    status = get_inter_comp_mb(pDec->curr_slice, pDec->rlist, pDec->width, pDec->height);
	return status;
}

xint enc_cal_inter_comp_mb(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Ryan Lin                                            */
/*   Date  : Oct/05/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Calculate compensation Luma and chroma data                 */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] Decoder object                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Oct/05/2005 Ryan Lin, Initial                               */
/* ------------------------------------------------------------- */
{
	return get_inter_comp_mb(pEnc->curr_slice, pEnc->reff, pEnc->width, pEnc->height);
}

xint get_inter_comp_mb(slice_obj *pSlice, frame_obj **ppfr, xint width, xint height)
/* ------------------------------------------------------------- */
/*   Author: Ryan Lin                                            */
/*   Date  : Oct/05/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Calculate compensation Luma and chroma data                 */
/*                                                               */
/*   RETURN                                                      */
/*   Status code.                                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] macroblock data                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Oct/05/2005 Ryan Lin, Initial                               */
/* ------------------------------------------------------------- */
{
	mb_obj	*pMB;
	uint8	*pY, *pCb, *pCr;
	frame_obj *pFrame;
	xint mbx, mby, posx, posy, partIdx, subPartIdx;

	pMB		= &pSlice->cmb;

	mbx		= pMB->id % (width/16);
	mby		= pMB->id / (width/16);

	switch (pMB->best_Inter_mode) {
	case INTER_PSKIP:
	case INTER_16x16:
	case INTER_16x8:
	case INTER_8x16:
		for (partIdx=0; partIdx<4; partIdx++) {
			posx = (mbx<<6) + pMB->mv.x[(partIdx/2)*8 + (partIdx%2)*2];
			posy = (mby<<6) + pMB->mv.y[(partIdx/2)*8 + (partIdx%2)*2];
			pFrame = ppfr[pMB->ref_idx[partIdx]];
			pY	 = pFrame->y;
			pCb	 = pFrame->cb;
			pCr	 = pFrame->cr;
			get_inter_8x8_luma  (pMB->Inter_cmp,    pY,  posx, posy, partIdx, 0, width,   height);
			get_inter_4x4_chroma(pMB->Inter_cb_cmp, pCb, posx, posy, partIdx, 0, width/2, height/2);
			get_inter_4x4_chroma(pMB->Inter_cr_cmp, pCr, posx, posy, partIdx, 0, width/2, height/2);
		}
		break;
	case INTER_P8x8:
		for (partIdx=0; partIdx<4; partIdx++) {
			pFrame = ppfr[pMB->ref_idx[partIdx]];
			pY	 = pFrame->y;
			pCb	 = pFrame->cb;
			pCr	 = pFrame->cr;
			switch (pMB->best_8x8_blk_mode[partIdx]) {
			case INTER_8x8:
				posx = (mbx<<6) + pMB->mv.x[(partIdx/2)*8 + (partIdx%2)*2];
				posy = (mby<<6) + pMB->mv.y[(partIdx/2)*8 + (partIdx%2)*2];
				get_inter_8x8_luma  (pMB->Inter_cmp,    pY,  posx, posy, partIdx, 0, width,   height);
				get_inter_4x4_chroma(pMB->Inter_cb_cmp, pCb, posx, posy, partIdx, 0, width/2, height/2);
				get_inter_4x4_chroma(pMB->Inter_cr_cmp, pCr, posx, posy, partIdx, 0, width/2, height/2);
				break;
			case INTER_8x4:
				for (subPartIdx=0; subPartIdx<2; subPartIdx++) {
					posx = (mbx<<6) + pMB->mv.x[(partIdx/2)*8 + (partIdx%2)*2 + subPartIdx*4];
					posy = (mby<<6) + pMB->mv.y[(partIdx/2)*8 + (partIdx%2)*2 + subPartIdx*4];
					get_inter_8x4_luma  (pMB->Inter_cmp,    pY,  posx, posy, partIdx, subPartIdx, width,   height);
					get_inter_4x2_chroma(pMB->Inter_cb_cmp, pCb, posx, posy, partIdx, subPartIdx, width/2, height/2);
					get_inter_4x2_chroma(pMB->Inter_cr_cmp, pCr, posx, posy, partIdx, subPartIdx, width/2, height/2);
				}
				break;
			case INTER_4x8:
				for (subPartIdx=0; subPartIdx<2; subPartIdx++) {
					posx = (mbx<<6) + pMB->mv.x[(partIdx/2)*8 + (partIdx%2)*2 + subPartIdx];
					posy = (mby<<6) + pMB->mv.y[(partIdx/2)*8 + (partIdx%2)*2 + subPartIdx];
					get_inter_4x8_luma  (pMB->Inter_cmp,    pY,  posx, posy, partIdx, subPartIdx, width,   height);
					get_inter_2x4_chroma(pMB->Inter_cb_cmp, pCb, posx, posy, partIdx, subPartIdx, width/2, height/2);
					get_inter_2x4_chroma(pMB->Inter_cr_cmp, pCr, posx, posy, partIdx, subPartIdx, width/2, height/2);
				}
				break;
			case INTER_4x4:
				for (subPartIdx=0; subPartIdx<4; subPartIdx++) {
					posx = (mbx<<6) + pMB->mv.x[(partIdx/2)*8 + (partIdx%2)*2 + (subPartIdx/2)*4 + (subPartIdx%2)];
					posy = (mby<<6) + pMB->mv.y[(partIdx/2)*8 + (partIdx%2)*2 + (subPartIdx/2)*4 + (subPartIdx%2)];
					get_inter_4x4_luma  (pMB->Inter_cmp,    pY,  posx, posy, partIdx, subPartIdx, width,   height);
					get_inter_2x2_chroma(pMB->Inter_cb_cmp, pCb, posx, posy, partIdx, subPartIdx, width/2, height/2);
					get_inter_2x2_chroma(pMB->Inter_cr_cmp, pCr, posx, posy, partIdx, subPartIdx, width/2, height/2);
				}
				break;
			}
		}
		break;
	default:
		break;
	}
	return 0;
}


uint8 getpel(uint8 *pSrc, xint x, xint y, xint width, xint height)
{
	x = Clip(x, 0, width-1);
	y = Clip(y, 0, height-1);
	return *(pSrc+y*width+x);
}

xint get_inter_luma(int16 *pDst, uint8 *pSrc, xint posx, xint posy, xint blockx, int blocky, xint width, xint height)
{
	xint	ix, iy, jx, jy;
	xint	posxi, posyi, posxf, posyf;
	xint	data[21][34], val[4][4];
	xint	A, B, C, D, E, F, G, H, I, J, M, N, R, S, T, U;
	xint	a, b, c, d, e, f, g, h, i, j, k, m, n, p, q, r, s;
	xint	b1, s1, aa, bb, gg, hh;

	posxi = posx >> 2;
	posyi = posy >> 2;
	posxf = posx & 3;
	posyf = posy & 3;

	for (iy=0; iy<blocky+5; iy++) {
		jy = posyi + iy - 2;
		for (ix=0; ix<blockx+1; ix++) {
			jx  = posxi + ix;
			E  = getpel(pSrc, jx-2, jy, width, height);
			F  = getpel(pSrc, jx-1, jy, width, height);
			G  = getpel(pSrc, jx  , jy, width, height);
			H  = getpel(pSrc, jx+1, jy, width, height);
			I  = getpel(pSrc, jx+2, jy, width, height);
			J  = getpel(pSrc, jx+3, jy, width, height);
			b1 = (20) * (G + H) + (-5) * (F + I) +  (1) * (E + J);
			data[iy][ix*2]   = G;
			data[iy][ix*2+1] = b1;
		}
	}
	for (jy=0; jy<blocky; jy++) {
		for (ix=0; ix<blockx; ix++) {
			A  = data [ jy   ][ 2*ix   ];
			C  = data [ jy+1 ][ 2*ix   ];
			G  = data [ jy+2 ][ 2*ix   ];
			M  = data [ jy+3 ][ 2*ix   ];
			R  = data [ jy+4 ][ 2*ix   ];
			T  = data [ jy+5 ][ 2*ix   ];
			aa = data [ jy   ][ 2*ix+1 ];
			bb = data [ jy+1 ][ 2*ix+1 ];
			b1 = data [ jy+2 ][ 2*ix+1 ];
			s1 = data [ jy+3 ][ 2*ix+1 ];
			gg = data [ jy+4 ][ 2*ix+1 ];
			hh = data [ jy+5 ][ 2*ix+1 ];
			B  = data [ jy   ][ 2*ix+2 ];
			D  = data [ jy+1 ][ 2*ix+2 ];
			H  = data [ jy+2 ][ 2*ix+2 ];
			N  = data [ jy+3 ][ 2*ix+2 ];
			S  = data [ jy+4 ][ 2*ix+2 ];
			U  = data [ jy+5 ][ 2*ix+2 ];
			j  = Clip1Y(((20) * (b1 + s1) + (-5) * (bb + gg) +  (1) * (aa + hh) + 512) >> 10);
			h  = Clip1Y(((20) * (G + M) + (-5) * (C + R) +  (1) * (A + T) + 16) >> 5);
			m  = Clip1Y(((20) * (H + N) + (-5) * (D + S) +  (1) * (B + U) + 16) >> 5);
			b  = Clip1Y((b1 + 16) >> 5);
			s  = Clip1Y((s1 + 16) >> 5);
			a  = (G + b + 1) >> 1;
			c  = (b + H + 1) >> 1;
			d  = (G + h + 1) >> 1;
			e  = (b + h + 1) >> 1;
			f  = (b + j + 1) >> 1;
			g  = (b + m + 1) >> 1;
			i  = (h + j + 1) >> 1;
			k  = (j + m + 1) >> 1;
			n  = (h + M + 1) >> 1;
			p  = (h + s + 1) >> 1;
			q  = (j + s + 1) >> 1;
			r  = (m + s + 1) >> 1;
			val[0][0] = G;
			val[0][1] = a;
			val[0][2] = b;
			val[0][3] = c;
			val[1][0] = d;
			val[1][1] = e;
			val[1][2] = f;
			val[1][3] = g;
			val[2][0] = h;
			val[2][1] = i;
			val[2][2] = j;
			val[2][3] = k;
			val[3][0] = n;
			val[3][1] = p;
			val[3][2] = q;
			val[3][3] = r;
			*pDst++ = val[posyf][posxf];
		}
	}
	return 0;
}


xint get_inter_chroma(int16 *pDst, uint8 *pSrc, xint posx, xint posy, xint blockx, xint blocky, xint width, xint height)
{
	xint i, j, m, n, A, B, C, D, posxi, posyi, posxf, posyf;

	posxi = posx >> 3;
	posyi = posy >> 3;
	posxf = posx & 7;
	posyf = posy & 7;

	for (j=0; j<blocky; j++) {
		n = posyi + j;
		for (i=0; i<blockx; i++) {
			m = posxi + i;
			A  = getpel(pSrc, m  , n  , width, height);
			B  = getpel(pSrc, m+1, n  , width, height);
			C  = getpel(pSrc, m  , n+1, width, height);
			D  = getpel(pSrc, m+1, n+1, width, height);
			*pDst++ = ((8-posxf)*(8-posyf)*A + posxf*(8-posyf)*B + (8-posxf)*posyf*C + posxf*posyf*D + 32) >> 6;
		}
	}
	return 0;
}

xint get_inter_4x4_luma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height)
{
	int16	tmp[16];
	xint	stx, sty;
	stx = (partIdx & 1) * 8  + (subPartIdx & 1) * 4;
	sty = (partIdx >> 1) * 8 + (subPartIdx >> 1) * 4;
	get_inter_luma(tmp, pSrc, posx+(stx*4), posy+(sty*4), 4, 4, width, height);
	blkcpy(dst, tmp, stx, sty, 4, 4, 16);
	return 0;
}

xint get_inter_4x8_luma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height)
{
	int16	tmp[32];
	xint	stx, sty;
	stx = (partIdx & 1) * 8  + (subPartIdx & 1) * 4;
	sty = (partIdx >> 1) * 8;
	get_inter_luma(tmp, pSrc, posx+(stx*4), posy+(sty*4), 4, 8, width, height);
	blkcpy(dst, tmp, stx, sty, 4, 8, 16);
	return 0;
}

xint get_inter_8x4_luma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height)
{
	int16	tmp[32];
	xint	stx, sty;
	stx = (partIdx & 1) * 8;
	sty = (partIdx >> 1) * 8 + (subPartIdx & 1) * 4;
	get_inter_luma(tmp, pSrc, posx+(stx*4), posy+(sty*4), 8, 4, width, height);
	blkcpy(dst, tmp, stx, sty, 8, 4, 16);
	return 0;
}

xint get_inter_8x8_luma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height)
{
	int16	tmp[64];
	xint	stx, sty;
	stx = (partIdx & 1) * 8;
	sty = (partIdx >> 1) * 8;
	get_inter_luma(tmp, pSrc, posx+(stx*4), posy+(sty*4), 8, 8, width, height);
#if 0
    {
        xint i, j;
        printf("------Luma------\n");
        for(i = 0; i < 8 ; i++)
        {
            for(j = 0 ; j < 8 ; j++)
            {
                printf("%4d", tmp[j*8+i]);
                if((j+1)%4 == 0)printf(" ");
            }
            if((i+1)%4 == 0)printf("\n");
            printf("\n");
        }
        printf("\n");
    }
#endif
	blkcpy(dst, tmp, stx, sty, 8, 8, 16);
	return 0;
}

xint get_inter_2x2_chroma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height)
{
	int16	tmp[4];
	xint	stx, sty;
	stx = (partIdx & 1) * 4  + (subPartIdx & 1) * 2;
	sty = (partIdx >> 1) * 4 + (subPartIdx >> 1) * 2;
	get_inter_chroma(tmp, pSrc, posx+(stx*8), posy+(sty*8), 2, 2, width, height);
	blkcpy(dst, tmp, stx, sty, 2, 2, 8);
	return 0;
}

xint get_inter_4x2_chroma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height)
{
	int16	tmp[8];
	xint	stx, sty;
	stx = (partIdx & 1) * 4;
	sty = (partIdx >> 1) * 4 + (subPartIdx & 1) * 2;
	get_inter_chroma(tmp, pSrc, posx+(stx*8), posy+(sty*8), 4, 2, width, height);
	blkcpy(dst, tmp, stx, sty, 4, 2, 8);
	return 0;
}

xint get_inter_2x4_chroma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height)
{
	int16	tmp[8];
	xint	stx, sty;
	stx = (partIdx & 1) * 4 + (subPartIdx & 1) * 2;
	sty = (partIdx >> 1) * 4;
	get_inter_chroma(tmp, pSrc, posx+(stx*8), posy+(sty*8), 2, 4, width, height);
	blkcpy(dst, tmp, stx, sty, 2, 4, 8);
	return 0;
}

xint get_inter_4x4_chroma(int16 *dst, uint8 *pSrc, xint posx, xint posy, xint partIdx, xint subPartIdx, xint width, xint height)
{
	int16	tmp[16];
	xint	stx, sty;
	stx = (partIdx & 1) * 4;
	sty = (partIdx >> 1) * 4;
	get_inter_chroma(tmp, pSrc, posx+(stx*8), posy+(sty*8), 4, 4, width, height);
#if 0
    {
        xint i, j;
        printf("------Chroma------\n");
        for(i = 0; i < 4 ; i++)
        {
            for(j = 0 ; j < 4 ; j++)
            {
                printf("%4d", tmp[i*4+j]);
                if((j+1)%2 == 0)printf(" ");
            }
            if((i+1)%2 == 0)printf("\n");
            printf("\n");
        }
        printf("\n");
    }
#endif
	blkcpy(dst, tmp, stx, sty, 4, 4, 8);
	return 0;
}
