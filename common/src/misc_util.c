/* ///////////////////////////////////////////////////////////// */
/*   File: misc_util.c                                           */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec utility routines.          */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/*   Multimedia Embedded Systems Lab.                            */
/*   Department of Computer Science and Information engineering  */
/*   National Chiao Tung University, Hsinchu 300, Taiwan         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#if !__CC_ARM
#include <sys/timeb.h>
#endif
#include <time.h>
#include "../../common/inc/nal.h"
#include "../../common/inc/misc_util.h"

int SubWidthC  [4]= { 1, 2, 2, 1};
int SubHeightC [4]= { 1, 2, 1, 1};

xint
write_frame(frame_obj *frm, 
            xint width, xint height, 
            xint crop_left, xint crop_right,
            xint crop_top, xint crop_bottom,
            xint chroma_format_idc,
            FILE *fp)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/08/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   output a frame to file with cropping                        */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint luma_l_start, luma_r_end, luma_t_start, luma_b_end;
    xint chroma_l_start, chroma_r_end, chroma_t_start, chroma_b_end;
    xint luma_size_x, chroma_size_x;
    xint chroma_width  = width>>1;
    xint chroma_height = height>>1;

    if(!crop_left && !crop_top && !crop_top && !crop_bottom)
    {
        fwrite(frm->y , width        * height       , 1, fp);
        fwrite(frm->cb, chroma_width * chroma_height, 1, fp);
        fwrite(frm->cr, chroma_width * chroma_height, 1, fp);
    }
    else
    {
        xint y = 0;
       
        luma_l_start  = SubWidthC[chroma_format_idc] * crop_left;
        luma_r_end    = SubWidthC[chroma_format_idc] * crop_right;
        luma_t_start  = SubHeightC[chroma_format_idc]* crop_top;
        luma_b_end    = SubHeightC[chroma_format_idc]* crop_bottom;

        chroma_l_start  = crop_left;
        chroma_r_end    = crop_right;
        chroma_t_start  = crop_top;
        chroma_b_end    = crop_bottom;

        luma_size_x   = width - luma_l_start - luma_r_end;
        chroma_size_x = width/2 - chroma_l_start - chroma_r_end;
        // Y
        for(y = luma_t_start ; y < height - luma_b_end ; y++)
            fwrite(&frm->y[y*width+luma_l_start], luma_size_x, 1, fp);
        
        // Cb
        for(y = chroma_t_start ; y < chroma_height - chroma_b_end ; y++)
            fwrite(&frm->cb[y*chroma_width+chroma_l_start], chroma_size_x, 1, fp);

        // Cr
        for(y = chroma_t_start ; y < chroma_height - chroma_b_end ; y++)
            fwrite(&frm->cr[y*chroma_width+chroma_l_start], chroma_size_x, 1, fp);
    }
       
    fflush(fp);
    return MMES_NO_ERROR;
}

frame_obj *
alloc_frame(xint width, xint height)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Allocate memory for a video frame.                          */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    frame_obj *pFrame;
    xint    chroma_size = (width>>1) * (height>>1); //for YCbCr 4:2:0

    pFrame = (frame_obj *) malloc(sizeof(frame_obj));
    if (pFrame == NULL)
    {
        alert_msg("alloc_frame", "alloc frame_obj failed!");
        return NULL;
    }

    /* initialize all pointers to NULL */
    memset(pFrame, 0, sizeof(frame_obj));

    pFrame->y = (uint8 *) malloc(sizeof(uint8) * width * height);
    if (pFrame->y == NULL)
    {
        alert_msg("alloc_frame", "alloc y component failed!");
        return NULL;
    }
    pFrame->cb = (uint8 *) malloc(sizeof(uint8) * chroma_size);
    if (pFrame->cb == NULL)
    {
        alert_msg("alloc_frame", "alloc cb component failed!");
        return NULL;
    }
    pFrame->cr = (uint8 *) malloc(sizeof(uint8) * chroma_size);
    if (pFrame->cr == NULL)
    {
        alert_msg("alloc_frame", "alloc cr component failed!");
        return NULL;
    }

	//: this two members are used for motion search
	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    pFrame->yx = (uint8 *) malloc(sizeof(uint8) * width * height / 16);
    if (pFrame->yx == NULL)
    {
        alert_msg("alloc_frame", "alloc yx component failed!");
        return NULL;
    }
    pFrame->y4 = (uint8 *) malloc(sizeof(uint8) * width * height / 4);
    if (pFrame->y4 == NULL)
    {
        alert_msg("alloc_frame", "alloc y4 component failed!");
        return NULL;
    }
	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    // : for reference frame list
    pFrame->is_ref    = 0;  /* frame can be referenced?    */
    pFrame->is_ltref  = 0;  /* frame is long-term ref?     */
    pFrame->frame_num = 0;  /* frame number                */
    pFrame->pic_num   = 0;  /* frame number wrapped        */
    pFrame->lt_pic_num= 0;  /* long term pic num           */
    pFrame->lt_frm_idx= 0;  /* long tern frame index       */
    pFrame->poc       = 0;  /* picture order count         */
    pFrame->is_output = 0;  /* output or not               */
    pFrame->non_exist = 0;

    return pFrame;
}

void
free_backup(backup_obj *pBackup)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : May/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Free memory of a backup object                              */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    if (pBackup != NULL)
    {
        if (pBackup->top_y_bup != NULL)
        {
            free(pBackup->top_y_bup), pBackup->top_y_bup = NULL;
        }

        if (pBackup->top_valid_bup != NULL)
        {
            free(pBackup->top_valid_bup), pBackup->top_valid_bup = NULL;
        }

        if (pBackup->top_valid_bup2 != NULL)
        {
            free(pBackup->top_valid_bup2), pBackup->top_valid_bup2 = NULL;
        }

        if (pBackup->top_mode_bup != NULL)
        {
            free(pBackup->top_mode_bup), pBackup->top_mode_bup = NULL;
        }

        if (pBackup->top_ref_idx_bup != NULL)
        {
            free(pBackup->top_ref_idx_bup), pBackup->top_ref_idx_bup = NULL;
        }

        if (pBackup->top_mvx_bup != NULL)
        {
            free(pBackup->top_mvx_bup), pBackup->top_mvx_bup = NULL;
        }

        if (pBackup->top_mvy_bup != NULL)
        {
            free(pBackup->top_mvy_bup), pBackup->top_mvy_bup = NULL;
        }

		if( pBackup->fp_intrapred != NULL )
		{
			fclose( pBackup->fp_intrapred ), pBackup->fp_intrapred = NULL;
		}

		if( pBackup->fp_qcoeff != NULL )
		{
			fclose( pBackup->fp_qcoeff ), pBackup->fp_qcoeff = NULL;
		}

        if( pBackup->fp_inter_input != NULL )
		{
			fclose( pBackup->fp_inter_input ), pBackup->fp_inter_input = NULL;
		}
        
        if( pBackup->fp_inter_pred_mv != NULL )
		{
			fclose( pBackup->fp_inter_pred_mv ), pBackup->fp_inter_pred_mv = NULL;
		}

		if( pBackup->fp_pframe_qcoeff != NULL )
		{
			fclose( pBackup->fp_pframe_qcoeff ), pBackup->fp_pframe_qcoeff = NULL;
		}

		if( pBackup->fp_eachMB != NULL )
		{
			fclose( pBackup->fp_eachMB ), pBackup->fp_eachMB = NULL;
		}

        if( pBackup->fp_golden_rec != NULL )
		{
			fclose( pBackup->fp_golden_rec ), pBackup->fp_golden_rec = NULL;
		}

		free( pBackup ), pBackup = NULL;
	}
}

void
free_frame(frame_obj *pFrame)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Free memory of a video frame buffer.                        */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    if (pFrame != NULL)
    {
        if (pFrame->y != NULL)
        {
            free(pFrame->y), pFrame->y = NULL;
        }

        if (pFrame->cb != NULL)
        {
            free(pFrame->cb), pFrame->cb = NULL;
        }

        if (pFrame->cr != NULL)
        {
            free(pFrame->cr), pFrame->cr = NULL;
        }

		//: this two members are used for motion search
		//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
        if (pFrame->yx != NULL)
        {
            free(pFrame->yx), pFrame->yx = NULL;
        }
        if (pFrame->y4 != NULL)
        {
            free(pFrame->y4), pFrame->y4 = NULL;
        }
		//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        free(pFrame), pFrame = NULL;
    }
}

backup_obj *
alloc_backup(xint width)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : May/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Allocate memory for backup object.                          */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
	backup_obj *pBackup;

	pBackup = (backup_obj *) malloc( sizeof(backup_obj) );
	if( pBackup == NULL )
	{
		alert_msg("alloc_slice", "alloc slice_obj failed!");
        return NULL;
	}

	/* initialize all pointers to NULL */
    memset(pBackup, 0, sizeof(backup_obj));

    pBackup->top_y_bup = (uint8 *) malloc(width * sizeof(uint8));
    if (pBackup->top_y_bup == NULL)
    {
        alert_msg("alloc_backup", "alloc top_y_bup failed!");
        return NULL;
    }

    pBackup->top_valid_bup = (uint8 *) malloc((width/B_SIZE)*sizeof(uint8));
    if (pBackup->top_valid_bup == NULL)
    {
        alert_msg("alloc_backup", "alloc top_valid_bup failed!");
        return NULL;
    }

    pBackup->top_valid_bup2 = (uint8 *) malloc((width/B_SIZE)*sizeof(uint8));
    if (pBackup->top_valid_bup2 == NULL)
    {
        alert_msg("alloc_backup", "alloc top_valid_bup2 failed!");
        return NULL;
    }

    pBackup->top_mode_bup = (int *) malloc((width/B_SIZE)*sizeof(int));
    if (pBackup->top_mode_bup == NULL)
    {
        alert_msg("alloc_backup", "alloc top_mode_bup failed!");
        return NULL;
    }

    pBackup->top_ref_idx_bup = (xint *) malloc(((width/B_SIZE)/2) * sizeof(xint));
    if (pBackup->top_ref_idx_bup == NULL)
    {
        alert_msg("alloc_backup", "alloc top_ref_idx_bup failed!");
        return NULL;
    }

    pBackup->top_mvx_bup = (xint *) malloc((width/B_SIZE) * sizeof(xint));
    if (pBackup->top_mvx_bup == NULL)
    {
        alert_msg("alloc_backup", "alloc top_mvx_bup failed!");
        return NULL;
    }

    pBackup->top_mvy_bup = (xint *) malloc((width/B_SIZE) * sizeof(xint));
    if (pBackup->top_mvy_bup == NULL)
    {
        alert_msg("alloc_backup", "alloc top_mvy_bup failed!");
        return NULL;
    }

	return pBackup;
}

slice_obj *
alloc_slice(xint width)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Allocate memory for a video slice.                          */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/*   Dec/18/2004 Jerry Peng, Enhanced error-checking code.    */
/*   Apr/19/2005 Jerry Peng, Add the deblock side info.      */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice;
    xint    chroma_width = width>>1; //for YCbCr 4:2:0

    pSlice = (slice_obj *) malloc(sizeof(slice_obj));
    if (pSlice == NULL)
    {
        alert_msg("alloc_slice", "alloc slice_obj failed!");
        return NULL;
    }

    /* initialize all pointers to NULL */
    memset(pSlice, 0, sizeof(slice_obj));

    pSlice->top_y = (uint8 *) malloc(width * sizeof(uint8));
    if (pSlice->top_y == NULL)
    {
        alert_msg("alloc_slice", "alloc top_y failed!");
        return NULL;
    }

    pSlice->top_cb = (uint8 *) malloc(chroma_width * sizeof(uint8));
    if (pSlice->top_cb == NULL)
    {
        alert_msg("alloc_slice", "alloc top_cb failed!");
        return NULL;
    }

    pSlice->top_cr = (uint8 *) malloc(chroma_width * sizeof(uint8));
    if (pSlice->top_cr == NULL)
    {
        alert_msg("alloc_slice", "alloc top_cr failed!");
        return NULL;
    }

    pSlice->top_valid = (uint8 *) malloc((width/B_SIZE) * sizeof(uint8));
    if (pSlice->top_valid == NULL)
    {
        alert_msg("alloc_slice", "alloc top_valid failed!");
        return NULL;
    }

    pSlice->deblock_top_valid = (uint8 *) malloc((width/MB_SIZE) * sizeof(uint8));
    if (pSlice->deblock_top_valid == NULL)
    {
        alert_msg("alloc_slice", "alloc deblock_top_valid failed!");
        return NULL;
    }

    pSlice->top_qp = (xint *) malloc((width/MB_SIZE) * sizeof(xint));
    if(pSlice->top_qp == NULL)
    {
        alert_msg("alloc_slice", "alloc top_qp failed!");
        return NULL;
    }

    pSlice->top_y_nzc = (int8 *) malloc((width/B_SIZE) * sizeof(int8));
    if(pSlice->top_y_nzc == NULL)
    {
        alert_msg("alloc_slice", "alloc top_y_nzc failed!");
        return NULL;
    }

    pSlice->top_cb_nzc = (int8 *) malloc((chroma_width/B_SIZE) * sizeof(int8));
    if(pSlice->top_cb_nzc == NULL)
    {
        alert_msg("alloc_slice", "alloc top_cb_nzc failed!");
        return NULL;
    }

    pSlice->top_cr_nzc = (int8 *) malloc((chroma_width/B_SIZE) * sizeof(int8));
    if(pSlice->top_cr_nzc == NULL)
    {
        alert_msg("alloc_slice", "alloc top_cr_nzc failed!");
        return NULL;
    }

	//  05 20 2005
    pSlice->top_mode = (int *) malloc((width/B_SIZE) * sizeof(int));
    if (pSlice->top_mode == NULL)
    {
        alert_msg("alloc_slice", "alloc top_mode failed!");
        return NULL;
    }

	//  11 07 2005
    pSlice->top_mb_intra = (uint8 *) malloc((width/B_SIZE) * sizeof(int8));
    if (pSlice->top_mb_intra == NULL)
    {
        alert_msg("alloc_slice", "alloc top_mb_intra failed!");
        return NULL;
    }

    pSlice->top_mvx = (xint *) malloc((width/B_SIZE) * sizeof(xint));
    if(pSlice->top_mvx == NULL)
    {
        alert_msg("alloc_slice", "alloc top_mvx failed!");
        return NULL;
    }

    pSlice->top_mvy = (xint *) malloc((width/B_SIZE) * sizeof(xint));
    if(pSlice->top_mvy == NULL)
    {
        alert_msg("alloc_slice", "alloc top_mvx failed!");
        return NULL;
    }

    pSlice->top_cbp_blk = (xint *) malloc((width/MB_SIZE) * sizeof(xint));
    if(pSlice->top_cbp_blk == NULL)
    {
        alert_msg("alloc_slice", "alloc top_cbp_blk failed!");
        return NULL;
    }

    pSlice->top_mb_mode = (xint *) malloc((width/MB_SIZE) * sizeof(xint));
    if(pSlice->top_mb_mode == NULL)
    {
        alert_msg("alloc_slice", "alloc top_mb_mode failed!");
        return NULL;
    }

    pSlice->top_ref_idx = (xint *) malloc((width/B_SIZE/2) * sizeof(xint));
    if(pSlice->top_ref_idx == NULL)
    {
        alert_msg("alloc_slice", "alloc top_ref_idx failed!");
        return NULL;
    }

    return pSlice;
}

void
free_slice(slice_obj *pSlice)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Free memory for a video slice.                              */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/*   Apr/19/2005 Jerry Peng, Move the code from h264enc_api.  */
/* ------------------------------------------------------------- */
{
    if (pSlice != NULL)
    {
        if (pSlice->top_y != NULL)
        {
            free(pSlice->top_y), pSlice->top_y = NULL;
        }
        if (pSlice->top_cb != NULL)
        {
            free(pSlice->top_cb), pSlice->top_cb = NULL;
        }
        if (pSlice->top_cr != NULL)
        {
            free(pSlice->top_cr), pSlice->top_cr = NULL;
        }
        if (pSlice->top_valid != NULL)
        {
            free(pSlice->top_valid), pSlice->top_valid = NULL;
        }
        if (pSlice->deblock_top_valid != NULL)
        {
            free(pSlice->deblock_top_valid), pSlice->deblock_top_valid = NULL;
        }
        if (pSlice->top_qp != NULL)
        {
            free(pSlice->top_qp), pSlice->top_qp = NULL;
        }
		//  05 20 2005
        if (pSlice->top_mode != NULL)
        {
            free(pSlice->top_mode), pSlice->top_mode = NULL;
        }
		//  11 07 2005
        if (pSlice->top_mb_intra != NULL)
        {
            free(pSlice->top_mb_intra), pSlice->top_mb_intra = NULL;
        }

        free(pSlice);
    }
}

void alert_msg(char *format, ...)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Feb/03/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   An alias for mmes_log().  The reason we created this alias  */
/*   is to allow all the error message logging code to be        */
/*   compiled out using a #define statement                      */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Mar/02/2005 Jerry Peng, merge the mmes_log() and        */
/*               alert_msg()                                     */
/* ------------------------------------------------------------- */
{
    va_list args;
    va_start(args, format);
    va_end(args);
}

xint
dup_frame_to_frame( frame_obj* pDst, frame_obj* pSrc, xint width, xint height )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/11/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Duplicate frame data.                                       */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDst  -> [I/O] pointer to the destination frame            */
/*   *pSrc  -> [I/O] pointer to the source frame                 */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	int framesize;
	int uvframesize;

	framesize = width*height;
	uvframesize = framesize >> 2;

	// y
	memcpy( pDst->y, pSrc->y, sizeof(uint8)*framesize );

	// cb
	memcpy( pDst->cb, pSrc->cb, sizeof(uint8)*uvframesize );

	// cb
	memcpy( pDst->cr, pSrc->cr, sizeof(uint8)*uvframesize );

	return MMES_NO_ERROR;
}

xint 
swap_frame(frame_obj* frame1, frame_obj* frame2)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Oct/25/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Swap two frames                                             */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{

    SWAP(frame1->y ,frame2->y , uint8 *);
    SWAP(frame1->cb,frame2->cb, uint8 *);
    SWAP(frame1->cr,frame2->cr, uint8 *);
    SWAP(frame1->y4,frame2->y4, uint8 *);
    SWAP(frame1->yx,frame2->yx, uint8 *);

    SWAP(frame1->type      ,frame2->type      , FRAME_TYPE);
    SWAP(frame1->id        ,frame2->id        , uint);
    SWAP(frame1->timestamp ,frame2->timestamp , uint32);
    SWAP(frame1->valid     ,frame2->valid     , xint);
    SWAP(frame1->QP        ,frame2->QP        , uint);

    SWAP(frame1->is_ref    ,frame2->is_ref    , xint);
    SWAP(frame1->is_ltref  ,frame2->is_ltref  , xint);
    SWAP(frame1->frame_num ,frame2->frame_num , xint);
    SWAP(frame1->lt_pic_num,frame2->lt_pic_num, xint);
    SWAP(frame1->lt_frm_idx,frame2->lt_frm_idx, xint);
    SWAP(frame1->pic_num   ,frame2->pic_num   , xint);
    SWAP(frame1->idr_flag  ,frame2->idr_flag  , xint);

    SWAP(frame1->poc       ,frame2->poc       , xint);
    SWAP(frame1->is_output ,frame2->is_output , xint);
    SWAP(frame1->non_exist ,frame2->non_exist , xint);

    return MMES_NO_ERROR;
}

xint 
gen_ref_frame(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Sep/5/2005                                          */
/* -----------------------------------------i-------------------- */
/*   COMMENTS                                                    */
/*   Generate y4 and y16 reference frame and uv-pack frame       */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint        x, y, i, j;
    xint        width    = pEnc->width, height = pEnc->height;
    xint        width2   = width >> 1, height2 = height >> 1;
    xint        width4   = width >> 2, height4 = height >> 2;
	uint8       *src_y;
    uint8       *dst;
    uint        sum = 0;

    src_y  = pEnc->reff[0]->y;

    // generate y4 frame
    dst    = pEnc->ref_y4;
	for(y = 0 ; y < height2 ; y++)
        for(x = 0 ; x < width2 ; x++)
	    {	
		    dst[y*width2+x] = (  src_y[2*y*width+2*x] + 
                                 src_y[2*y*width+2*x+1] + 
                                 src_y[(2*y+1)*width+2*x] + 
                                 src_y[(2*y+1)*width+2*x+1]+2)>>2;
	    }

    //generate y16 frame
    dst    = pEnc->ref_y16;
    for(i = 0 ; i < height4 ; i++)
        for(j = 0 ; j < width4 ; j++)
        {
            sum = 0;
            for(y = 0 ; y < B_SIZE ; y++)
                for(x = 0 ; x < B_SIZE ; x++)
                    sum += src_y[i*4*width+j*4 + y*width+x];

            dst[i*width4+j] = ((sum+8)>>4);
        }

	return MMES_NO_ERROR;
}



//  
xint dup_mb(frame_obj* pDst, frame_obj* pSrc, xint mb_id, xint width)
{
	xint      frame_x_16_offset, frame_y_16_offset;
    xint      idx, offset;
    xint      mb_width = width / MB_SIZE;
	xint      chroma_mb_size, chroma_frame_width;
	uint8     *src_y, *src_cb, *src_cr;
    uint8     *dst_y, *dst_cb, *dst_cr;

    if(!pDst || !pSrc)
        return MMES_MEM_ERROR;
    
    frame_x_16_offset  = mb_id % mb_width;
    frame_y_16_offset  = mb_id / mb_width;

	chroma_frame_width = width >> 1;
	chroma_mb_size     = MB_SIZE >> 1;

    offset= frame_y_16_offset * MB_SIZE * width + frame_x_16_offset * MB_SIZE;
    src_y = pSrc->y + offset;
    dst_y = pDst->y + offset;

	for(idx = 0 ; idx < MB_SIZE ; idx++)
	{	
		memcpy( dst_y, src_y, sizeof(uint8)*MB_SIZE);
		src_y += width;
        dst_y += width;
	}

    offset = frame_y_16_offset * chroma_mb_size * chroma_frame_width + frame_x_16_offset * chroma_mb_size;
    src_cb = pSrc->cb + offset;
    dst_cb = pDst->cb + offset;
    src_cr = pSrc->cr + offset;
    dst_cr = pDst->cr + offset;
	
	for(idx = 0 ; idx < chroma_mb_size ; idx++)
	{		
		memcpy( dst_cb, src_cb, sizeof(uint8)*chroma_mb_size);
        memcpy( dst_cr, src_cr, sizeof(uint8)*chroma_mb_size);
		src_cb += chroma_frame_width;
        dst_cb += chroma_frame_width;
        src_cr += chroma_frame_width;
        dst_cr += chroma_frame_width;
	}

	return MMES_NO_ERROR;
}

xint
dup_16x16_to_frame( frame_obj* pDst, int16 *py, int16 *pcb, int16 *pcr, xint mb_id, xint width )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/11/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Duplicate 16x16 one-dimension data to frame.                */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDst  -> [I/O] pointer to the destination frame            */
/*   *py    -> [I]   pointer to the 16x16 source data            */
/*   *pcb   -> [I]   pointer to the 8x8 source data              */
/*   *pcr   -> [I]   pointer to the 8x8 source data              */
/*   mb_id  -> [I]   macroblock ID to fill the source data       */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	xint nx_mb;
    xint frame_x_16_offset, frame_y_16_offset;
	xint x_coord, y_coord;
	xint chroma_mb_size;
	xint chroma_frame_width;
	uint8 *y, *cb, *cr;

	chroma_frame_width = width>>1;
	chroma_mb_size = MB_SIZE>>1;       // for YCbCr420
	nx_mb = width >> 4;
    frame_x_16_offset = mb_id % nx_mb;
    frame_y_16_offset = mb_id / nx_mb;	
	
	// y
	if( py )
	{
		y = pDst->y + frame_y_16_offset*MB_SIZE*width + frame_x_16_offset*MB_SIZE;
		for(y_coord=0; y_coord<MB_SIZE; y_coord++)
		{
			for(x_coord=0; x_coord<MB_SIZE; x_coord++)
			{
				*y = (uint8)*py;
				y++;
				py++;
			}
			y += (width-MB_SIZE);
		}
	}

	// cb
	if( pcb )
	{
		cb = pDst->cb + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
		for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
		{
			for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
			{
				*cb = (uint8)*pcb;
				cb++;
				pcb++;
			}
			cb += (chroma_frame_width-chroma_mb_size);
		}
	}

	// cr
	if( pcr )
	{
		cr = pDst->cr + frame_y_16_offset*chroma_mb_size*chroma_frame_width + frame_x_16_offset*chroma_mb_size;
		for(y_coord=0; y_coord<chroma_mb_size; y_coord++)
		{
			for(x_coord=0; x_coord<chroma_mb_size; x_coord++)
			{
				*cr = (uint8)*pcr;
				cr++;
				pcr++;
			}
			cr += (chroma_frame_width-chroma_mb_size);
		}
	}
	return MMES_NO_ERROR;
}

xint
dup_4x4_to_frame( frame_obj* pDst, int16 *pSrc, xint mb_id, xint width, xint block )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Aug/11/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Duplicate 4x4 one-dimension data to frame.                  */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDst  -> [I/O] pointer to the destination frame            */
/*   *pSrc  -> [I]   pointer to the 4x4 source data              */
/*   mb_id  -> [I]   macroblock ID to fill the source data       */
/*   block  -> [I]   which 4x4 block to fill                     */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	xint nx_mb;
    xint frame_x_16_offset, frame_y_16_offset;
    xint mb_x_4_offset, mb_y_4_offset;
	xint frame_x_4_offset, frame_y_4_offset;
	xint x_coord, y_coord;
	uint8 *y;
	int16 *ps;

	nx_mb = width >> 4;
    frame_x_16_offset = mb_id % nx_mb;
    frame_y_16_offset = mb_id / nx_mb;

    mb_x_4_offset = mb_x_4_idx[block];
    mb_y_4_offset = mb_y_4_idx[block];

    frame_x_4_offset = frame_x_16_offset*4 + mb_x_4_offset;
    frame_y_4_offset = frame_y_16_offset*4 + mb_y_4_offset;
	
	y = pDst->y + frame_y_4_offset*4*width + frame_x_4_offset*4;
	ps = pSrc + mb_y_4_offset*4*MB_SIZE + mb_x_4_offset*4;
	
	// y
	for(y_coord=0; y_coord<B_SIZE; y_coord++)
	{
		for(x_coord=0; x_coord<B_SIZE; x_coord++)
		{
			*y = (uint8)ps[x_coord + y_coord*MB_SIZE];
			y++;			
		}		
		y += (width-B_SIZE);
	}

	return MMES_NO_ERROR;
}

static int 
compare_pic_by_pic_num_desc( const void *arg1, const void *arg2 )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Oct/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   compares two stored pictures by picture number for          */ 
/*   qsort in descending order                                   */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
  if ( (*(frame_obj**)arg1)->pic_num < (*(frame_obj**)arg2)->pic_num)
    return 1;
  if ( (*(frame_obj**)arg1)->pic_num > (*(frame_obj**)arg2)->pic_num)
    return -1;
  else
    return 0;
}

static int 
compare_pic_by_lt_pic_num_asc( const void *arg1, const void *arg2 )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : NOV/23/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   compares two stored pictures by picture number for          */ 
/*   qsort in descending order                                   */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
  if ( (*(frame_obj **)arg1)->lt_pic_num < (*(frame_obj **)arg2)->lt_pic_num)
    return -1;
  if ( (*(frame_obj **)arg1)->lt_pic_num > (*(frame_obj **)arg2)->lt_pic_num)
    return 1;
  else
    return 0;
}


void remove_frame_from_dpb(h264dec_obj *pDec, xint pos)
{
    xint idx;
    frame_obj **reff = pDec->reff;
    frame_obj *tmp   = reff[pos];
    
    tmp->is_ref      = 0;
    tmp->is_ltref    = 0;
    for(idx = pos ; idx < pDec->used_size ; idx++)
        pDec->reff[idx] = pDec->reff[idx+1];
    pDec->reff[pDec->used_size - 1] = tmp;
    pDec->used_size--;
}

void update_ltref_list(h264dec_obj *pDec)
{
    xint i, j;
    for (i=0, j=0; i<pDec->used_size; i++)
    {
        if (pDec->reff[i]->is_ref && pDec->reff[i]->is_ltref)
            pDec->ltreff_list[j++] = pDec->reff[i];
    }

    pDec->ltref_frms_in_buf = j;

    while (j < pDec->dpbsize)
        pDec->ltreff_list[j++] = NULL;
}

void update_ref_list(h264dec_obj *pDec)
{
    xint i, j;
    for (i=0, j=0; i<pDec->used_size; i++)
    {
        if (pDec->reff[i]->is_ref && !pDec->reff[i]->is_ltref)
            pDec->reff_list[j++] = pDec->reff[i];
    }

    pDec->ref_frms_in_buf = j;

    while ( j < pDec->dpbsize)
        pDec->reff_list[j++] = NULL;
}

xint
init_ref_lists(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Oct/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Initialize the reference frame list.                        */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint idx, listidx       = 0;
    frame_obj **ltreff_list = pDec->ltreff_list;
    frame_obj **reff_list   = pDec->reff_list;
    frame_obj **rlist       = pDec->rlist;

    for (idx = 0 ; idx < pDec->ref_frms_in_buf ; idx++)
    {
        if (reff_list[idx]->is_ref)
        {
            if( reff_list[idx]->frame_num > pDec->cur_frame_no)
                reff_list[idx]->pic_num = reff_list[idx]->frame_num - pDec->max_frame_no;
            else
                reff_list[idx]->pic_num = reff_list[idx]->frame_num;
        }
    }

    // update long_term_pic_num
    for (idx = 0 ; idx < pDec->ltref_frms_in_buf ; idx++)
    {
        if (ltreff_list[idx]->is_ltref)
            ltreff_list[idx]->lt_pic_num = ltreff_list[idx]->lt_frm_idx;
    }

    //  for hw ref list compatible
    /*
    if(IS_ISLICE(pSlice->type))
    {
        pDec->list_size = 0;    
        return MMES_NO_ERROR;
    }
    */

    //if(IS_PSLICE(pSlice->type))
    {
        // Calculate FrameNumWrap and PicNum
        memset(rlist, 0, sizeof(frame_obj*)*MAXLIST);
        for(idx = 0 ; idx < pDec->ref_frms_in_buf ; idx++)
        {
            if (reff_list[idx]->is_ref)
                rlist[listidx++] = reff_list[idx];
        }
        // order list 0 by PicNum
        qsort((void *) rlist, listidx, sizeof(frame_obj *), compare_pic_by_pic_num_desc);
        pDec->list_size = listidx;

        // long term handling
        for (idx = 0 ; idx < pDec->ltref_frms_in_buf ; idx++)
        {
            if (ltreff_list[idx]->is_ltref)
                rlist[listidx++] = ltreff_list[idx];
        }
    
        qsort((void *)&rlist[pDec->list_size], listidx-pDec->list_size, sizeof(frame_obj *), compare_pic_by_lt_pic_num_asc);
        pDec->list_size = listidx;
    }
    
    return MMES_NO_ERROR;
}

static void 
reorder_short_term(h264dec_obj *pDec, xint picNumLX, xint *refIdxLX)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/22/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Reordering process for short-term reference pictures.       */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint cIdx;
 
    frame_obj **RefPicListX = pDec->rlist;
    slice_obj *pSlice = pDec->curr_slice;
    frame_obj *picLX = NULL, *picTMP1 = NULL, *picTMP2 = NULL;

    // get short-term reference pic
    for(cIdx = 0 ; cIdx < pDec->ref_frms_in_buf ; cIdx++)
    {
        if(!pDec->reff_list[cIdx]->is_ltref && pDec->reff_list[cIdx]->pic_num == picNumLX)
        {
            picLX = pDec->reff_list[cIdx];
            break;
        }
    }

#if 1
    picTMP1 = RefPicListX[*refIdxLX];
    if(picTMP1 != picLX)
    {
        RefPicListX[*refIdxLX] = picLX;
        cIdx = ++(*refIdxLX);
        while(cIdx < pSlice->num_ref_idx_l0_active)
        {
            picTMP2 = RefPicListX[cIdx];
            RefPicListX[cIdx] = picTMP1;
            picTMP1 = picTMP2;

            if(!picTMP1->is_ltref && picTMP1->pic_num == picNumLX)
                break;
            else
                cIdx++;
        }
    }
    else
        ++(*refIdxLX);
    
#else
    for( cIdx = pSlice->num_ref_idx_l0_active ; cIdx > *refIdxLX ; cIdx-- )
        RefPicListX[cIdx] = RefPicListX[cIdx - 1];
  
    RefPicListX[(*refIdxLX)++] = picLX;

    nIdx = *refIdxLX;

    for(cIdx = *refIdxLX; cIdx <= pSlice->num_ref_idx_l0_active ; cIdx++)
        if (RefPicListX[ cIdx ])
            if( (RefPicListX[ cIdx ]->is_ltref ) ||  (RefPicListX[ cIdx ]->pic_num != picNumLX ))
                RefPicListX[ nIdx++ ] = RefPicListX[ cIdx ];

    RefPicListX[pSlice->num_ref_idx_l0_active] = NULL;
#endif
}

static void 
reorder_long_term(h264dec_obj *pDec, xint LongTermPicNum, xint *refIdxLX)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/22/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Reordering process for long-term reference pictures.        */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint cIdx;
    frame_obj **RefPicListX = pDec->rlist;
    slice_obj *pSlice = pDec->curr_slice;
    frame_obj *picLX = NULL, *picTMP1 = NULL, *picTMP2 = NULL;

    for (cIdx = 0 ; cIdx < pDec->ltref_frms_in_buf ; cIdx++)
    {
        if ( pDec->ltreff_list[cIdx]->is_ltref && pDec->ltreff_list[cIdx]->lt_pic_num == LongTermPicNum)
        {
            picLX = pDec->ltreff_list[cIdx];
            break;
        }
    }
    
#if 1
    picTMP1 = RefPicListX[*refIdxLX];
    if(picTMP1 != picLX)
    {
        RefPicListX[*refIdxLX] = picLX;
        cIdx = ++(*refIdxLX);
        while(cIdx < pSlice->num_ref_idx_l0_active)
        {
            picTMP2 = RefPicListX[cIdx];
            RefPicListX[cIdx] = picTMP1;
            picTMP1 = picTMP2;

            if(picTMP1->is_ltref && picTMP1->lt_pic_num == LongTermPicNum)
                break;
            else
                cIdx++;
        }
    }
    else
        ++(*refIdxLX);
    
#else
    for( cIdx = pSlice->num_ref_idx_l0_active ; cIdx > *refIdxLX ; cIdx-- )
        RefPicListX[ cIdx ] = RefPicListX[ cIdx - 1];
  
    RefPicListX[ (*refIdxLX)++ ] = picLX;

    nIdx = *refIdxLX;

    for( cIdx = *refIdxLX ; cIdx <= pSlice->num_ref_idx_l0_active ; cIdx++ )
        if( (!RefPicListX[ cIdx ]->is_ltref ) ||  (RefPicListX[ cIdx ]->lt_pic_num != LongTermPicNum ))
            RefPicListX[ nIdx++ ] = RefPicListX[ cIdx ];

    RefPicListX[pSlice->num_ref_idx_l0_active] = NULL;
#endif
}

xint 
reorder_lists(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/22/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Reorder reference frame lists                               */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{

    xint idx;
    slice_obj *pSlice     = pDec->curr_slice;

    xint maxPicNum, currPicNum, picNumLXNoWrap, picNumLXPred, picNumLX;
    xint refIdxLX = 0;

    if ((pSlice->type != I_SLICE) && (pSlice->type != SI_SLICE))
    {
        if (pSlice->ref_pic_list_reordering_flag_l0)
        {
            maxPicNum    = pDec->max_frame_no;
            currPicNum   = pDec->cur_frame_no;
            picNumLXPred = currPicNum;

            for (idx = 0 ; pSlice->remapping_of_pic_nums_idc_l0[idx] != 3; idx++)
            {
                if (pSlice->remapping_of_pic_nums_idc_l0[idx] < 2)
                {
                    if (pSlice->remapping_of_pic_nums_idc_l0[idx] == 0)
                    {
                        if( (picNumLXPred - pSlice->abs_diff_pic_l0[idx]) < 0 )
                          picNumLXNoWrap = picNumLXPred - pSlice->abs_diff_pic_l0[idx] + maxPicNum;
                        else
                          picNumLXNoWrap = picNumLXPred - pSlice->abs_diff_pic_l0[idx];
                    }
                    else // (remapping_of_pic_nums_idc[i] == 1)
                    {
                        if( (picNumLXPred + pSlice->abs_diff_pic_l0[idx]) >= maxPicNum)
                            picNumLXNoWrap = picNumLXPred + pSlice->abs_diff_pic_l0[idx] - maxPicNum;
                        else
                            picNumLXNoWrap = picNumLXPred + pSlice->abs_diff_pic_l0[idx];
                    }
                    picNumLXPred = picNumLXNoWrap;

                    if( picNumLXNoWrap > currPicNum )
                        picNumLX = picNumLXNoWrap - maxPicNum;
                    else
                        picNumLX = picNumLXNoWrap;

                    reorder_short_term(pDec, picNumLX, &refIdxLX);
                }
                else //(remapping_of_pic_nums_idc[i] == 2)
                {
                    reorder_long_term(pDec, pSlice->long_term_pic_idx_l0[idx], &refIdxLX);
                }
            }
        }

        //pDec->list_size = pSlice->num_ref_idx_l0_active;
    }

    return MMES_NO_ERROR;
}

static void
adaptive_memory_management(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/23/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Perform adaptive memory control decoded reference picture   */
/*   marking process.                                            */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint           idx, i;
    slice_obj      *pSlice = pDec->curr_slice;
    dec_refpicmark *cur_dec_refpicmark;
    xint           pic_num;

    pDec->last_has_mmco_5 = 0;
    for(idx = 0 ; pSlice->dec_refpicmark_buf[idx].mem_manage_ctrl_op != 0 ; idx++)
    {
        cur_dec_refpicmark = pSlice->dec_refpicmark_buf + idx;
        switch(cur_dec_refpicmark->mem_manage_ctrl_op)
        {
        case 1:
            pic_num = pDec->cur_frame_no - cur_dec_refpicmark->diff_of_pic_nums;
            for (i = 0 ; i < pDec->ref_frms_in_buf ; i++)
            {
                if (pDec->reff_list[i]->is_ref && !pDec->reff_list[i]->is_ltref)
                    if (pDec->reff_list[i]->pic_num == pic_num)
                    {
                        pDec->reff_list[i]->is_ref = 0;
                        break;
                    }
            }   
            update_ref_list(pDec);
            break;
        case 2:
            for ( i = 0 ; i < pDec->ltref_frms_in_buf ; i++)
            {
                if (pDec->ltreff_list[i]->is_ref && pDec->ltreff_list[i]->is_ltref )
                {
                    if (pDec->ltreff_list[i]->lt_pic_num == cur_dec_refpicmark->lt_pic_num)
                    {
                        pDec->ltreff_list[i]->is_ref   = 0;
                        pDec->ltreff_list[i]->is_ltref = 0;
                    }
                }
            }
            update_ltref_list(pDec);
            break;
        case 3:
            pic_num = pDec->cur_frame_no - cur_dec_refpicmark->diff_of_pic_nums;
            for ( i = 0 ; i < pDec->ltref_frms_in_buf ; i++)
            {
                if (pDec->ltreff_list[i]->is_ref && pDec->ltreff_list[i]->is_ltref )
                {
                    if (pDec->ltreff_list[i]->lt_frm_idx == cur_dec_refpicmark->lt_frm_idx)
                    {
                        pDec->ltreff_list[i]->is_ref   = 0;
                        pDec->ltreff_list[i]->is_ltref = 0;
                    }
                }
            }

            for ( i = 0 ; i < pDec->ref_frms_in_buf ; i++)
            {
                if (pDec->reff_list[i]->is_ref)
                {
                    if (!pDec->reff_list[i]->is_ltref && pDec->reff_list[i]->pic_num == pic_num)
                    {
                        pDec->reff_list[i]->lt_frm_idx = cur_dec_refpicmark->lt_frm_idx;
                        pDec->reff_list[i]->is_ltref   = 1;              
                        break;
                    }
                }
            }

            update_ref_list(pDec);
            update_ltref_list(pDec);
            break;
        case 4:
            // check for invalid frames
            pDec->max_long_term_pic_idx = cur_dec_refpicmark->max_lt_frm_idx ;
            for ( i = 0 ; i < pDec->ltref_frms_in_buf ; i++)
            {
                if (pDec->ltreff_list[i]->lt_frm_idx > pDec->max_long_term_pic_idx)
                {
                    pDec->ltreff_list[i]->is_ref   = 0;
                    pDec->ltreff_list[i]->is_ltref = 0;
                }
            }
            update_ltref_list(pDec);
            break;
        case 5:
            pDec->last_has_mmco_5 = 1;
            for (i = 0 ; i < pDec->ref_frms_in_buf ; i++)
                pDec->reff_list[i]->is_ref = 0;
            update_ref_list(pDec);
            for (i = 0 ; i < pDec->ltref_frms_in_buf ; i++)
            {
                pDec->ltreff_list[i]->is_ref   = 0;
                pDec->ltreff_list[i]->is_ltref = 0;
            }
            break;
        case 6:
            for ( i = 0 ; i < pDec->ltref_frms_in_buf ; i++)
            {
                if (pDec->ltreff_list[i]->is_ref && pDec->ltreff_list[i]->is_ltref )
                {
                    if (pDec->ltreff_list[i]->lt_frm_idx == cur_dec_refpicmark->lt_frm_idx)
                    {
                        pDec->ltreff_list[i]->is_ref   = 0;
                        pDec->ltreff_list[i]->is_ltref = 0;
                    }
                }
            }
            pDec->recf->is_ltref  = 1;
            pDec->recf->lt_frm_idx= cur_dec_refpicmark->lt_frm_idx;
            break;
        default:
            fprintf(stderr, "Error : invalid memory_management_control_operation in buffer.\n");
            exit(1);
            break;
        }
    }

    if ( pDec->last_has_mmco_5 )
    {
        pDec->recf->pic_num = pDec->recf->frame_num = 0;
    }
  
}

static __inline void 
sliding_window_memory_management(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jan/11/2006                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   sliding window memory management, which is a marking mode   */
/*   providing a first-in first-out mechanism for short-term     */
/*   reference pictures                                          */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint idx;
    if (pDec->ref_frms_in_buf == pDec->max_reff_no - pDec->ltref_frms_in_buf)
    {
        for (idx = 0 ; idx < pDec->used_size ; idx++)
        {
            if (pDec->reff[idx]->is_ref && !pDec->reff[idx]->is_ltref)
            {
                pDec->reff[idx]->is_ref = 0;
                update_ref_list(pDec);
                break;
            }
        }
    }
    pDec->recf->is_ltref = 0;
}

xint 
check_frame_gaps(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jan/11/2006                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Check if gaps between previous and current frame and fill   */
/*   gaps.                                                       */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{

    seq_paraset_obj *sps;
    pic_paraset_obj *pps;
    slice_obj       *slice = pDec->curr_slice;
    xint             frame_no_shallbe = (pDec->past_frame_no + 1) % pDec->max_frame_no;
    xint             idx;

    pps = &pDec->pps[slice->pic_parameter_set_id];
    sps = &pDec->sps[pps->seq_parameter_set_id];
    
    if (pDec->cur_frame_no != pDec->past_frame_no && 
        pDec->cur_frame_no != frame_no_shallbe) 
    {
        if(!sps->gaps_in_frame_num_value_allowed_flag)
        {
            fprintf(stderr, "Loss of frame happens when checking the frame gap!\n");
            exit(1);
        }
        
        //fill the frame gaps
        for(; frame_no_shallbe != pDec->cur_frame_no ; frame_no_shallbe = (frame_no_shallbe+1)%pDec->max_frame_no)
        {
            frame_obj *p_blank_frame;
    
            sliding_window_memory_management(pDec);            

            //if (pDec->used_size==pDec->dpbsize)
            {
                // try to remove unused frames
                for (idx = 0 ; idx < pDec->used_size ; idx++)
                {
                    if (!pDec->reff[idx]->is_ref)
                    {
                        remove_frame_from_dpb(pDec, idx);
                        // for hw ref list compatible
                        //break;
                    }
                }
            }

            p_blank_frame            = pDec->reff[pDec->used_size];
            p_blank_frame->frame_num = frame_no_shallbe;
            p_blank_frame->pic_num   = frame_no_shallbe;
            p_blank_frame->non_exist = 1;
            p_blank_frame->is_ref    = 1;
            p_blank_frame->is_ltref  = 0;
            p_blank_frame->idr_flag  = 0;

            pDec->used_size++;
            update_ref_list(pDec);
        }
    }

    if(pDec->nal.nal_reference_idc)
        pDec->past_frame_no   = pDec->cur_frame_no;

    return MMES_NO_ERROR;
}

xint
adjust_ref_lists(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/2/2005                                          */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Adjust the reference frame list.                            */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint      idx;
    slice_obj *pSlice = pDec->curr_slice;

    //check_frame_gaps(pDec);    

    // store a picture in dpb
    if(pDec->recf->idr_flag)
    {
        /* flush reference frame list */
        for(idx = 0 ; idx < pDec->used_size ; idx++)
            pDec->reff[idx]->is_ref = 0;
                
        update_ref_list(pDec);
        update_ltref_list(pDec);

        if(pDec->long_term_reference_flag)
        {
            pDec->max_long_term_pic_idx = 0;
            pDec->recf->is_ltref        = 1;
            pDec->recf->lt_frm_idx      = 0;
        }
        else
        {
            pDec->max_long_term_pic_idx = -1;
            pDec->recf->is_ltref        = 0;
        }
    }
    else
    {
        // adaptive memory management
        if (pDec->recf->is_ref)
        {
            if(pSlice->adaptive_ref_pic_buf_flag)
                adaptive_memory_management(pDec);
            else
                sliding_window_memory_management(pDec);
        }
    }

    //if (pDec->used_size==pDec->dpbsize)
    {
        // try to remove unused frames
        for (idx = 0 ; idx < pDec->used_size ; idx++)
        {
            if (!pDec->reff[idx]->is_ref)
            {
                remove_frame_from_dpb(pDec, idx);
                // for hw ref list compatible
                //break;
            }
        }
    }

    swap_frame(pDec->recf, pDec->reff[pDec->used_size++]);

    update_ref_list(pDec);
    update_ltref_list(pDec);

    return MMES_NO_ERROR;
}

xint 
calc_poc(h264dec_obj *pDec, frame_obj *frm)
/* ------------------------------------------------------------- */
/*   Author: JM                                                  */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Get picture order count                                     */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    slice_obj *slice = pDec->curr_slice;

    switch(pDec->poc_type)
    {
    case 0:
        if(frm->idr_flag)
            pDec->prev_poc_msb = pDec->prev_poc_lsb = 0;            
        else
        {
            if(pDec->last_has_mmco_5)
                pDec->prev_poc_lsb = pDec->prev_poc_msb = 0;
            else
            {
                pDec->prev_poc_msb = 0;
                pDec->prev_poc_lsb = pDec->toppoc;
            }
        }
        // Calculate the MSBs of current picture
        if( slice->pic_order_cnt_lsb < pDec->prev_poc_lsb && 
            (pDec->prev_poc_lsb - slice->pic_order_cnt_lsb) >= (pDec->max_poc_lsb>>1))
            pDec->poc_msb = pDec->prev_poc_msb + pDec->max_poc_lsb;
        else if(slice->pic_order_cnt_lsb > pDec->prev_poc_lsb &&
                (slice->pic_order_cnt_lsb - pDec->prev_poc_lsb) > (pDec->max_poc_lsb>>1))
            pDec->poc_msb = pDec->prev_poc_msb - pDec->max_poc_lsb;
        else
            pDec->poc_msb = pDec->prev_poc_msb;


        if(slice->field_pic_flag==0)
        {
            pDec->toppoc = pDec->poc_msb + slice->pic_order_cnt_lsb;
            pDec->btmpoc = pDec->toppoc  + slice->delta_poc_btm;
            frm->poc     = (pDec->toppoc < pDec->btmpoc) ? pDec->toppoc : pDec->btmpoc;
        }
        else if (slice->bottom_field_flag == 0)
        {
            pDec->toppoc = pDec->poc_msb + slice->pic_order_cnt_lsb;
            frm->poc     = pDec->toppoc;
        }
        else
        {  
            /* bottom field */           
            pDec->btmpoc = pDec->poc_msb + slice->pic_order_cnt_lsb;
            frm->poc     = pDec->btmpoc;
        }

        if(pDec->nal.nal_unit_type != NALU_PRIORITY_DISPOSABLE)
        {
            pDec->prev_poc_lsb = slice->pic_order_cnt_lsb;
            pDec->prev_poc_msb = pDec->poc_msb;
        }
        break;
    case 1:
        fprintf(stderr, "POC type 1 presents\n");
        if(frm->idr_flag)
            slice->delta_poc[0] = 0;
        break;
    case 2:
        break;
    default:
        fprintf(stderr, "ERROR : POC type\n");
        exit(1);
    }
    return MMES_NO_ERROR;
}
    
xint
init_picture(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jan/11/2006                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Initialize the reconsructed picture.                        */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    //xint    poc;       /* picture order count         */
    pDec->recf->is_ref    = (pDec->nal.nal_reference_idc != 0) ? 3 : 0;
    pDec->recf->idr_flag  = (pDec->nal.nal_unit_type == NALU_TYPE_IDR);
    
    pDec->recf->pic_num   = pDec->cur_frame_no;
    pDec->recf->frame_num = pDec->cur_frame_no;
    pDec->recf->non_exist = 0;

    if (pDec->last_has_mmco_5)
    {
        pDec->past_frame_no  = 0;
        pDec->last_has_mmco_5= 0;
    }

    return MMES_NO_ERROR;
}

xint
reset_slice(slice_obj *pSlice, int width)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Jun/22/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Reset slice for further intra or inter processing           */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoding session parameter   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    /* reset top and left neighbor info (set to no predictors) */
    memset(pSlice->top_valid, MMES_INVALID, (width/B_SIZE));
    memset(pSlice->left_valid, MMES_INVALID, MB_SIZE/B_SIZE);
	memset(pSlice->upleft_valid, MMES_INVALID, MB_SIZE/B_SIZE);

    /* reset deblock top and left neighbor info.               */
    memset(pSlice->deblock_top_valid, MMES_INVALID, (width/MB_SIZE));
    pSlice->deblock_left_valid = MMES_INVALID;
	
	//  05 20 2005
	memset(pSlice->top_mode, INTRA_4x4_DC, (width/B_SIZE));

	//  11 07 2005
	memset(pSlice->top_mb_intra, 0, (width/B_SIZE));
	memset(pSlice->left_mb_intra , 0, sizeof(int8) * (MB_SIZE/B_SIZE));

    //  reset for entropy coding
    memset(pSlice->left_y_nzc , -1, sizeof(int8) * (MB_SIZE/B_SIZE));
    memset(pSlice->left_cb_nzc, -1, sizeof(int8) * ((MB_SIZE/2)/B_SIZE));
    memset(pSlice->left_cr_nzc, -1, sizeof(int8) * ((MB_SIZE/2)/B_SIZE));
    memset(pSlice->top_y_nzc  , -1, sizeof(int8) * (width/B_SIZE));
    memset(pSlice->top_cb_nzc , -1, sizeof(int8) * ((width/2)/B_SIZE));
    memset(pSlice->top_cr_nzc , -1, sizeof(int8) * ((width/2)/B_SIZE));

	return MMES_NO_ERROR;
}

xint
binary_format(int8 *bin, int32 value, xint len)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/??/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Format a decimal value into a binary string                 */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    uint32 mask;
    xint   idx;
    
    if(len > 0 && len <= 32)
    {
        for(idx = 0, mask = 1 << (len - 1) ; idx < len ; idx++, mask >>= 1)
        {
            if(idx && idx%4 == 0)
                *bin++ = '_';

            if(value & mask)
                *bin++ = '1';
            else
                *bin++ = '0';
        }

        return MMES_NO_ERROR;
    }

    return MMES_ERROR;	
}

xint 
update_MB(slice_obj *pSlice, int width)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date: Aug/24/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Update macroblock information                               */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the encoding session parameter  */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	mb_obj *pMB = &(pSlice->cmb);
    xint frame_x_16_offset, frame_y_16_offset;
	xint x_coord;
	xint nx_mb;

	nx_mb = width >> 4;
    frame_x_16_offset = pMB->id % nx_mb;
    frame_y_16_offset = pMB->id / nx_mb;

	if( IS_ISLICE(pSlice->type) )
	{
		/* for constraint intra-prediction */
		memset(&pSlice->top_mb_intra[frame_x_16_offset<<2], 1, 4);//  11 07 2005

		// left
        /* for constraint intra-prediction */
        memset(pSlice->left_mb_intra, 1, sizeof(uint8)*4);
	}
	else
	{
		if( pMB->best_MB_mode && pMB->best_MB_mode != I_PCM )// Inter
		{
			// top
			for (x_coord = 0; x_coord < (MB_SIZE/B_SIZE); x_coord++)
			{
				// mv
				pSlice->top_mvx[frame_x_16_offset * (MB_SIZE/B_SIZE)+x_coord] = pMB->mv.x[(MB_SIZE/B_SIZE)*3+x_coord];
				pSlice->top_mvy[frame_x_16_offset * (MB_SIZE/B_SIZE)+x_coord] = pMB->mv.y[(MB_SIZE/B_SIZE)*3+x_coord];
			}

            /* for constraint intra-prediction */
            if( frame_x_16_offset )
            {
                pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)-1] = pSlice->left_mb_intra[0];//  12 07 2005
            }
            pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)  ] = 0;//  12 07 2005
            pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)+1] = 0;//  12 07 2005
            pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)+2] = 0;//  12 07 2005
			if(frame_x_16_offset == (nx_mb-1))
			{
				pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)+3] = 0;//  01 13 2006
			}

			//  08 29 2005
			for (x_coord = 0; x_coord < 2; x_coord++)
			{
				pSlice->top_ref_idx[(frame_x_16_offset<<1)+x_coord] = pMB->ref_idx[(MB_SIZE/B_SIZE/2)+x_coord];
			}

			// left
			for (x_coord = 0; x_coord < (MB_SIZE/B_SIZE); x_coord++)
			{
				// mv
				pSlice->left_mvx[x_coord] = pMB->mv.x[(x_coord<<2)+3];
				pSlice->left_mvy[x_coord] = pMB->mv.y[(x_coord<<2)+3];

				/* for constraint intra-prediction */
				pSlice->left_mb_intra[x_coord] = 0;//  11 07 2005
			}

			//  08 29 2005
			for (x_coord = 0; x_coord < 2; x_coord++)
			{
				pSlice->left_ref_idx[x_coord] = pMB->ref_idx[(x_coord<<1)+1];//  10 26 2005
			}

			//  08 26 2005
			// upleft
			for (x_coord = 1; x_coord < (MB_SIZE/B_SIZE); x_coord++)
			{
				// mv
				pSlice->upleft_mvx[x_coord] = pMB->mv.x[(x_coord<<2)-1];
				pSlice->upleft_mvy[x_coord] = pMB->mv.y[(x_coord<<2)-1];
			}
			// ref_idx
			pSlice->upleft_ref_idx[1] = pSlice->upleft_ref_idx[2] = pMB->ref_idx[1];
			pSlice->upleft_ref_idx[3] = pMB->ref_idx[3];
		}
		else	// Intra
		{
			// top
			for (x_coord = 0; x_coord < (MB_SIZE/B_SIZE); x_coord++)
			{
				// mv
				pSlice->top_mvx[frame_x_16_offset*(MB_SIZE/B_SIZE) + x_coord] = 0;
				pSlice->top_mvy[frame_x_16_offset*(MB_SIZE/B_SIZE) + x_coord] = 0;

				/* for constraint intra-prediction */
//				pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE) + x_coord] = 1;//  11 07 2005
			}

            /* for constraint intra-prediction */
            if( frame_x_16_offset )
            {
                pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)-1] = pSlice->left_mb_intra[0];//  12 07 2005
            }
            pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)  ] = 1;//  12 07 2005
            pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)+1] = 1;//  12 07 2005
            pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)+2] = 1;//  12 07 2005
			if(frame_x_16_offset == (nx_mb-1))
			{
				pSlice->top_mb_intra[frame_x_16_offset*(MB_SIZE/B_SIZE)+3] = 1;//  01 13 2006
			}

			pSlice->top_ref_idx[(frame_x_16_offset<<1)+1] = pSlice->top_ref_idx[(frame_x_16_offset<<1)] = -1;//  08 29 2005

			// left
			for (x_coord = 0; x_coord < (MB_SIZE/B_SIZE); x_coord++)
			{
				// mv
				pSlice->left_mvx[x_coord] = 0;
				pSlice->left_mvy[x_coord] = 0;
		
				/* for constraint intra-prediction */
				pSlice->left_mb_intra[x_coord] = 1;//  11 07 2005
			}
			pSlice->left_ref_idx[1] = pSlice->left_ref_idx[0] = -1;//  08 29 2005

			//  08 26 2005
			// upleft
			for (x_coord = 1; x_coord < (MB_SIZE/B_SIZE); x_coord++)
			{
				// mv
				pSlice->upleft_mvx[x_coord] = 0;
				pSlice->upleft_mvy[x_coord] = 0;
			}
			// ref_idx
			pSlice->upleft_ref_idx[1] = pSlice->upleft_ref_idx[2] = pSlice->upleft_ref_idx[3] = -1;
			
		}	
	}

    //  04 28 2006
    for (x_coord = 0; x_coord < (MB_SIZE/B_SIZE); x_coord++)
    {
        pSlice->top_valid[frame_x_16_offset * (MB_SIZE/B_SIZE)+x_coord] = MMES_VALID;
    }
	return MMES_NO_ERROR;
}

xint
check_reconstruct_frame(h264enc_obj *pEnc)
{
	xint width = pEnc->width;
	xint height = pEnc->height;
	xint error, mb, stop, i, j, k;
	uint8 *data, *dY, *dCb, *dCr, *sY, *sCb, *sCr;

	data = (uint8 *)malloc(width*height*3/2);
	if (data == NULL) {
		printf("check_reconstruct_frame : Cannot Allocate Memory for golden reconstructed data\n");
		return MMES_NO_ERROR;
	}

	fread(data, width*height*3/2, 1, pEnc->backup->fp_golden_rec);

	error = 0;
	error |= memcmp(pEnc->recf->y, data, width*height);
	error |= memcmp(pEnc->recf->cb, data+width*height, width*height/4);
	error |= memcmp(pEnc->recf->cr, data+width*height*5/4, width*height/4);
	if (error) {
		mb = 0;
		for (j=0; j<height; j+=16) {
			for (i=0; i<width; i+=16) {
				sY = pEnc->recf->y + j*width + i;
				sCb = pEnc->recf->cb + j*width/4 + i/2;
				sCr = pEnc->recf->cr + j*width/4 + i/2;			
				dY = data + j*width + i;
				dCb = data + width*height + j*width/4 + i/2;
				dCr = data + width*height*5/4 + j*width/4 + i/2;
				for (k=0; k<8; k++) {
					stop |= memcmp(sY, dY, 16);
					sY += width;
					dY += width;
					stop |= memcmp(sY, dY, 16);
					sY += width;
					dY += width;
					stop |= memcmp(sCb, dCb, 8);
					sCb += width/2;
					dCb += width/2;
					stop |= memcmp(sCr, dCr, 8);
					sCr += width/2;
					dCr += width/2;
				}
				mb++;
			}
		}
	}

	return MMES_NO_ERROR;
}

double
calc_psnr(uint8 *orig, uint8 *buf, uint32 width, uint32 height)
{
    int idx, size;
    double error, diff;

    size = width * height;
    error = 0.0;
    idx = 0;
    for (idx = 0; idx < size; idx++)
    {
        diff = orig[idx] - buf[idx];
        error += (diff*diff);
    }
    return 10.0*log10((65025.0*size)/error);
}

uint32 GetTimerCount()
{
#if !__CC_ARM
    struct _timeb tick;
    _ftime(&tick);
    return (tick.time*1000 + tick.millitm);
#else
    return 0;
#endif
   
}

void restore_intra_flag( uint8 *dest, uint8 *src, int size)
{
	memcpy(dest, src, size);
}
