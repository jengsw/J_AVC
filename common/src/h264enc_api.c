/* ///////////////////////////////////////////////////////////// */
/*   File: h264enc_api.c                                         */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 encoder API.                     */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/inc/h264enc_api.h"
#include "../../common/inc/bitstream.h"
#include "../../common/inc/macroblock.h"
#include "../../common/inc/misc_util.h"
#include "../../common/inc/quant.h"
#include "../../common/inc/nal.h"
#include "../../common/inc/mv_search.h"
#include "../../common/inc/rate_control.h"

xint
h264_init_encoder(h264enc_obj *pEnc, enc_cfg *pCtrl, xint width, xint height)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Allocate memory required by the H.264 encoder session.      */
/*                                                               */
/*   RETURN                                                      */
/*   The status code of memory allocation.                       */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/*   width  -> [I] width of the input video frame                */
/*   hgight -> [I] height of the input video frame               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    xint i;

    /* initialize all pointers to NULL */
    memset(pEnc, 0, sizeof(h264enc_obj));

    pEnc->width         = width;
    pEnc->height        = height;
    pEnc->nx_mb         = width  / MB_SIZE;
    pEnc->ny_mb         = height / MB_SIZE;
    pEnc->total_mb      = pEnc->nx_mb * pEnc->ny_mb;

	pEnc->max_reff_no	= 1;  /* maximum reference index */    
    
    pEnc->max_frame_num = 32; /* always set to 256 for now */
    pEnc->log2_max_fno  = pCtrl->log2_max_fno;
    pEnc->log2_max_poc  = 4;

    pEnc->pCtrl         = pCtrl; 
    pEnc->cur_frame_no  = 0;
    pEnc->cur_slice_no  = 0;
    pEnc->cur_mb_no     = 0;

    memset(pEnc->bit_buf, 0, sizeof(uint8)*256);

    pEnc->curf = alloc_frame(width, height);
    if (pEnc->curf == NULL)
    {
        alert_msg("h264_init_encoder", "alloc current frame failed!");
        return MMES_ALLOC_ERROR;
    }

    pEnc->recf = alloc_frame(width, height);
    if (pEnc->recf == NULL)
    {
        alert_msg("h264_init_encoder", "alloc reconstructed frame failed!");
        return MMES_ALLOC_ERROR;
    }

	pEnc->reff = (frame_obj **)malloc(sizeof(frame_obj *)*pEnc->max_reff_no);
	for (i=0 ; i<pEnc->max_reff_no ; i++)
	{
		pEnc->reff[i] = alloc_frame(width, height);
		if (pEnc->reff[i] == NULL)
		{
			alert_msg("h264_init_encoder", "alloc reference frame failed!");
			return MMES_ALLOC_ERROR;
		}
	}

    pEnc->ref_y4 = (uint8 *) malloc(sizeof(uint8) * width/2 * height/2);
    if(pEnc->ref_y4 == NULL)
    {
        alert_msg("h264_init_encoder", "alloc reference frame failed!");
        return MMES_ALLOC_ERROR;
    }

    pEnc->ref_y16 = (uint8 *) malloc(sizeof(uint8) * width/4 * height/4);
    if(pEnc->ref_y16 == NULL)
    {
        alert_msg("h264_init_encoder", "alloc reference frame failed!");
        return MMES_ALLOC_ERROR;
    }

    pEnc->curr_slice = alloc_slice(width);
    if (pEnc->curr_slice == NULL)
    {
        alert_msg("h264_init_encoder", "alloc current slice failed!");
        return MMES_ALLOC_ERROR;
    }

    pEnc->scratch = (uint8 *) malloc(sizeof(uint8)*MB_SIZE*MB_SIZE*2);
    if (pEnc->scratch == NULL)
    {
        alert_msg("h264_init_encoder", "alloc scratch space failed!");
        return MMES_ALLOC_ERROR;
    }

	pEnc->backup = alloc_backup(width);
	if (pEnc->backup == NULL)
    {
        alert_msg("h264_init_encoder", "alloc backup failed!");
        return MMES_ALLOC_ERROR;
    }

    init_quant_matrix();
	
	me_init_encoder(pEnc);

	/* initialize rate control */
	if(pCtrl->luma_qp < 0)
	{
		pEnc->rc_enable = 1;
		pCtrl->luma_qp = rc_init(pEnc, -pCtrl->luma_qp);
	}
	else
	{
		pEnc->rc_enable = 0;
	}

    return MMES_NO_ERROR;
}

xint
h264_free_encoder(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Free all the memory used by the H.264 encoder session.      */
/*                                                               */
/*   RETURN                                                      */
/*   The status code of memory de-allocation.                    */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint i;

    /* free current slice */
    free_slice(pEnc->curr_slice);

    /* free curf frame  */
    free_frame(pEnc->curf);

    /* free recf frame  */
    free_frame(pEnc->recf);

    if(pEnc->ref_y4)
        free(pEnc->ref_y4);
    if(pEnc->ref_y16)
        free(pEnc->ref_y16);
    /* free scratch space */
    if (pEnc->scratch != NULL)
    {
        free(pEnc->scratch);
    }

    /* free reff frames */
    for (i=0 ; i<pEnc->max_reff_no ; i++)
		free_frame(pEnc->reff[i]);

	/* free backup */
	free_backup(pEnc->backup);

	me_free_encoder(pEnc);

    return MMES_NO_ERROR;
}

xint
h264_init_bitstream(h264enc_obj *pEnc, uint8 *pBuf, xint size)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Initalize encoder output bitstream buffer & pointer.        */
/*                                                               */
/*   RETURN                                                      */
/*   The status code of initialization.                          */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   *pBuf -> [I] pointer to the output bitstream buffer         */
/*    size -> [I] the size of the bitstream buffer in bits        */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    pEnc->stream_buf = pBuf;
    pEnc->curr_byte  = 0;
    pEnc->stream_len = size;
    pEnc->byte_pos = 0;
    pEnc->bit_pos = 0;
    return MMES_NO_ERROR;
}

xint
h264_init_slice(h264enc_obj *pEnc, RBSP_TYPE type, xint no_mb)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Apr/10/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Initalize slice encoding parameters.                        */
/*                                                               */
/*   RETURN                                                      */
/*   The status code of initialization.                          */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc ->  [I/O] pointer to the encoder session parameters   */
/*    type ->  [I] slice type                                    */
/*    no_mb -> [I] the maximum number of MBs in slice            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/*   Apr/19/2005 Jerry Peng, Add init for deblock side info. */
/*   May/15/2005 Jerry Peng, Remove the default QP parameter. */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice = pEnc->curr_slice;
    xint      chroma_frame_width;
    xint      chroma_mb_size;

    chroma_frame_width = pEnc->width>>1;        // for YCbCr420
    chroma_mb_size     = MB_SIZE>>1;            // for YCbCr420

    pSlice->type = (type != AUTO_SLICE)? type : slice_mode_select(pEnc);
    pSlice->max_no_mbs = (no_mb == 0) ? pEnc->nx_mb*pEnc->ny_mb : no_mb;

    pSlice->QP                     = pEnc->QP;
    pSlice->chroma_qp_index_offset = 0;
    pSlice->cmb.id                 = 0;
    pSlice->first_mb_in_slice      = 0;
    pSlice->mb_skip_run            = 0;

    if(pEnc->pCtrl->config_loop_filter)
    {
        pSlice->lf_disable_idc         = pEnc->pCtrl->dis_loop_filter_idc;
        pSlice->lf_alpha_c0_offset_div2= pEnc->pCtrl->alpha_c0_offset_div2;
        pSlice->lf_bata_offset_div2    = pEnc->pCtrl->beta_offset_div2;
    }
    else
    {
        pSlice->lf_disable_idc         = 0;
        pSlice->lf_alpha_c0_offset_div2= 0;
        pSlice->lf_bata_offset_div2    = 0;
    }

    /* reset top and left neighbor info (set to no predictors) */
    memset(pSlice->top_valid, MMES_INVALID, (pEnc->width/B_SIZE));
    memset(pSlice->left_valid, MMES_INVALID, MB_SIZE/B_SIZE);

    /* reset deblock top and left neighbor info.               */
    memset(pSlice->deblock_top_valid, MMES_INVALID, (pEnc->width/MB_SIZE));
    pSlice->deblock_left_valid = MMES_INVALID;

    /* reset the top and left neighbor nnz info. */
    memset(pSlice->top_y_nzc, -1, (pEnc->width/B_SIZE));
    memset(pSlice->top_cb_nzc, -1, (chroma_frame_width/B_SIZE));
    memset(pSlice->top_cr_nzc, -1, (chroma_frame_width/B_SIZE));
    memset(pSlice->left_y_nzc, -1, (MB_SIZE/B_SIZE));
    memset(pSlice->left_cb_nzc, -1, (chroma_mb_size/B_SIZE));
    memset(pSlice->left_cr_nzc, -1, (chroma_mb_size/B_SIZE));

	memset(pSlice->top_mode, INTRA_4x4_DC, (pEnc->width/B_SIZE));

    memset(pSlice->top_mvx, 0, (pEnc->width/B_SIZE) * sizeof(xint));
    memset(pSlice->top_mvy, 0, (pEnc->width/B_SIZE) * sizeof(xint));
    memset(pSlice->top_cbp_blk, 0, (pEnc->width/MB_SIZE) * sizeof(xint));
    memset(pSlice->top_mb_mode, 0, (pEnc->width/MB_SIZE) * sizeof(xint));
    memset(pSlice->top_ref_idx, 0, (pEnc->width/B_SIZE/2) * sizeof(xint));

    return MMES_NO_ERROR;
}

xint
h264_encode_video_header(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Entry point for bitstream header (Annex B) encoding.        */
/*                                                               */
/*   RETURN                                                      */
/*   The size (in bits) of the video bitstream header.           */
/*   There is something wrong if the size is negative.           */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   *pBuf -> [O] pointer to the output bitstream buffer         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint nbits;

    nbits = get_bitpos(pEnc);
    write_nal_header(pEnc, 3u, SEQ_PARASET);
    write_seq_paraset_rbsp(pEnc);
    write_nal_header(pEnc, 3u, PIC_PARASET);
    write_pic_paraset_rbsp(pEnc);
    nbits = get_bitpos(pEnc) - nbits;

    return nbits;
}

xint
h264_encode_slice_header(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Entry point for slice header encoding.                      */
/*                                                               */
/*   RETURN                                                      */
/*   The size (in bits) of the slice header.                     */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   *pBuf -> [O] pointer to the output bitstream buffer         */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    /* set this value to zero for transmission without signaling 
       that the whole picture has the same slice type */
    xint same_slicetype_for_whole_frame = 5;

    xint nbits, qp_delta;
    slice_obj *pSlice = pEnc->curr_slice;
    uint fid = ((pEnc->curf->id*2) & ~((((uint)(-1)) << pEnc->log2_max_poc)));
    xint nal_unit_type, nal_reference_idc;

    if(pEnc->idr_flag)
    {
        nal_unit_type     = NALU_TYPE_IDR;
        nal_reference_idc = NALU_PRIORITY_HIGHEST;
    }
    else
    {
        nal_unit_type     = NALU_TYPE_SLICE;
        if(pEnc->ref_flag != 0)
            nal_reference_idc = NALU_PRIORITY_HIGH;
        else nal_reference_idc= NALU_PRIORITY_DISPOSABLE;
    }

    /* This is a placeholder */
    nbits = get_bitpos(pEnc);

    write_nal_header(pEnc, nal_reference_idc, nal_unit_type);
    write_uvlc_codeword(pEnc, pSlice->first_mb_in_slice);
    write_uvlc_codeword(pEnc, pSlice->type+same_slicetype_for_whole_frame);
    write_uvlc_codeword(pEnc, pSlice->pic_parameter_set_id);

    /* write max frame number */
    put_bits(pEnc, pEnc->cur_frame_no, pEnc->log2_max_fno);

    if (pEnc->idr_flag)
    {
        write_uvlc_codeword(pEnc, fid %2);
    }

    /* write max picture order count */
    put_bits(pEnc, fid, pEnc->log2_max_poc);

    if(pSlice->type == P_SLICE || pSlice->type == B_SLICE)
    {
        /* num_ref_idx_active_override_flag */
        put_one_bit(pEnc, 0);
    }

    /* ref_pic_list_reordering */
    if(pSlice->type != I_SLICE && pSlice->type != SI_SLICE)
    {
        put_one_bit(pEnc, 0);
    }

    /* encode_ref_pic_marking() */
    put_one_bit(pEnc, 0); /* no_output_of_prior_pics_flag */
    if(pEnc->idr_flag)
    {
        put_one_bit(pEnc, 0); /* long_term_reference_flag */
    }
        
    /* output initial QP */
    qp_delta = pEnc->QP - 26;
    write_signed_uvlc_codeword(pEnc, qp_delta);

    if(pEnc->pCtrl->config_loop_filter)
    {
        /* disable_deblocking_filter_idc */
        write_uvlc_codeword(pEnc, pSlice->lf_disable_idc);
        if(pEnc->pCtrl->dis_loop_filter_idc != 1)
        {
            /* slice_alpha_c0_offset_div2 */
            write_signed_uvlc_codeword(pEnc, pSlice->lf_alpha_c0_offset_div2);
            /* slice_beta_offset_div2 */
            write_signed_uvlc_codeword(pEnc, pSlice->lf_bata_offset_div2);
        }
    }

    nbits = get_bitpos(pEnc) - nbits;

    return nbits;
}

__inline xint write_trailing_bits(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/15/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Write trailing bits.                                        */
/* ------------------------------------------------------------- */
{
    xint  nbits = 0;
    /* write trailing bits */
    put_one_bit(pEnc, 1);
    nbits++;
    while (pEnc->bit_pos)
    {
        put_one_bit(pEnc, 0);
        nbits++;
    }
    return nbits;
}

xint
h264_encode_slice(h264enc_obj *pEnc, xint *pDone)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Entry point for slice encoding.                             */
/*                                                               */
/*   RETURN                                                      */
/*   The size (in bits) of encoded bitstream size.               */
/*   There is something wrong if the size is negative.           */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoding session parameter   */
/*   *pDone -> [O] returns '1' when all MBs are encoded, or '0'  */
/*             when there are more MBs to be encoded.            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    slice_obj   *pSlice = pEnc->curr_slice;
    xint        nbits;
    xint        total_mb = pEnc->total_mb;

    /* write slice header */
    pSlice->pic_parameter_set_id = 0; /* Is this right??? */
	pSlice->no_mbs = 0;

    nbits = h264_encode_slice_header(pEnc);

#if !SPEEDUP
	me_init_slice(pEnc);
#endif

    /* encode all the macroblock in the slice */
    do
    {
        /* initialize macroblock */
        pSlice->cmb.QP = pSlice->QP;

		/* initialize Largrange Multiplier */
		encode_init_RDParams( pEnc );

        /* initialize MB information       */
		init_mb_info( pEnc->curr_slice, pEnc->width );

#if SPEEDUP
        nbits += encode_intra_mb(pEnc);
#else
        /* encode one macroblock */
        switch (pEnc->curr_slice->type)
        {
        case I_SLICE:
            nbits += encode_intra_mb(pEnc);
            break;
        case P_SLICE:
            nbits += encode_inter_mb(pEnc);
            break;
        case B_SLICE:
            nbits += encode_bidir_mb(pEnc);
            break;
        }
#endif
         
        pEnc->cur_mb_no++;
        *pDone = (++(pSlice->cmb.id) >= total_mb);
		pSlice->no_mbs++;

    } while (! *pDone && pSlice->no_mbs < pSlice->max_no_mbs);

    pSlice->first_mb_in_slice = pSlice->cmb.id;

    nbits += write_trailing_bits(pEnc);

    return nbits;
}

RBSP_TYPE
slice_mode_select(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Feb/12/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Frame(slice) type decision routine. Note that this routine  */
/*   can be used for any video codec, not just for H.264         */
/*                                                               */
/*   RETURN                                                      */
/*   The type of the slice.                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoding session parameter    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    enc_cfg *pCtrl = pEnc->pCtrl;

	if( pCtrl->intra_period )
	{
		if( pEnc->cur_frame_no % pCtrl->intra_period == 0 )	return I_SLICE;
		else												return P_SLICE;
	}
	else
	{
		if( pEnc->cur_frame_no )	return P_SLICE;
		else						return I_SLICE;
	}
}

