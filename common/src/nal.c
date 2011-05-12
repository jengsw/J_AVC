/* ///////////////////////////////////////////////////////////// */
/*   File: nal.c                                                 */
/*   Author: Jerry Peng                                          */
/*   Date: Mar/06/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 NAL encode/decode module.        */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ///////////////////////////////////////////////////////////// */

#include "../../common/inc/h264_def.h"
#include "../../common/inc/misc_util.h"
#include "../../common/inc/bitstream.h"
#include "../../common/inc/nal.h"
#include "../../common/inc/macroblock.h"
#include <string.h>
#include <stdlib.h>

const uint8 bit_mask[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

__inline xint count_bits(uint32 code)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Calculate the number of bits of a code.                     */
/*                                                               */
/*   RETURN                                                      */
/*   Returns the number of bits.                                 */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   code -> [I] the value of the code                           */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint nbits = 0, bin = (code+1)>>1;

    while (bin)
    {
        bin >>= 1;
        nbits++;
    }

    return nbits;
}

void 
ebsptorbsp(nal_obj *nalu)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   to convert encapsulated byte sequence packets to RBSP,      */
/*   which is to get rid of 0x03 from 0x00000300, 0x00000301,    */
/*   0x00000302, 0x00000303                                      */
/*                                                               */
/*   RETURN                                                      */
/*   the length after convertion of rbsp                         */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *nalu  -> [I/O] nal unit                                    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{
    xint i, j, count;
    uint8 *buf = nalu->buf;
    count = 0;
  
    /* the starting byte of a NAL unit consists of forbidden_zero_bit(1), */
    /* nal_ref_idc(2), and nal_unit_type(5) so the start position of      */
    /* transforming is from 1                                             */

    for(i = 1, j = 1 ; i < nalu->len ; i++) 
    { 
        //starting from begin_bytepos to avoid header information
        if(count == 2 && buf[i] == 0x03) 
        {
            i++;
            count = 0;
        }
        buf[j] = buf[i];

        count = (buf[i]) ? 0 : count + 1;
        j++;
    }
    nalu->len = j;
}

__inline void sodbtorbsp(h264enc_obj *pEnc)
{
    /* write trailing bits */
    put_one_bit(pEnc, 1);
    while (pEnc->bit_pos)
    {
        put_one_bit(pEnc, 0);
    }
}

__inline void parse_slice_trailing_bits(h264dec_obj *pDec)
{
    get_bits(pDec, 1);
    
    while (pDec->bitcnt%8)
        get_bits(pDec, 1);
}

xint
write_uvlc_codeword(h264enc_obj *pEnc, uint value)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Write a UVLC codeword to the bitstream.                     */
/*                                                               */
/*   RETURN                                                      */
/*   Returns the status code.                                    */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/*   value  -> [I]   coding value                                */
/*   *des   -> [I]   coding description for bitstream log        */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jerry Peng - to debug                                       */
/* ------------------------------------------------------------- */
{
    xint idx, nbits = count_bits(value);
    uint32 info;

    if (nbits)
    {
        info = value + 1 - (1 << nbits);

        /* first, write prefix */
        for (idx = 0 ; idx < nbits ; idx++)
        {
            put_one_bit(pEnc, 0);
        }

        put_one_bit(pEnc, 1);

        /* now, write info bits */
        for(idx = (1<<(nbits-1)) ; idx ; idx>>=1)
        {
            put_one_bit(pEnc, (idx & info) ? 1 : 0);
        }
    }
    else
    {
        put_one_bit(pEnc, 1);
    }

	return MMES_NO_ERROR;
}

xint
write_signed_uvlc_codeword(h264enc_obj *pEnc, xint value)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Write a signed UVLC codeword to the bitstream.              */
/*                                                               */
/*   RETURN                                                      */
/*   Returns the status code.                                    */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    uint pos, code = 0;

    if (value != 0)
    {
        pos = 1;
        if (value < 0) value = -value, pos = 0;
        code = (value<<1) - pos;
    }
    write_uvlc_codeword(pEnc, code);

	return MMES_NO_ERROR;
}

xint
write_nal_header(h264enc_obj *pEnc, uint ref_idc, NAL_TYPE nal_unit_type)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Write a NAL unit header.                                    */
/*                                                               */
/*   RETURN                                                      */
/*   Returns zero if everything is fine, nonzero if something is */
/*   wrong.                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    /* Prepend a NAL units with 00 00 00 01 to generate */
    /* Annex B byte stream format bitstreams.           */
    put_bits(pEnc, 1, 32);

    /* Now, this is the ture starting byte of a NAL unit */
	/* The first bit is always zero */
	put_one_bit(pEnc, 0); /* forbidden_zero_bit */

    /* Is this NAL a reference picture/slice for others? */
    /* This can always be set to one since we don't care */
    /* about SEI, AU delimiter, End code, & Filler data. */
	put_bits(pEnc, ref_idc, 2); /* nal_ref_idc */

	/* NAL Unit type (e.g. I, P, B, ..., etc) */
	put_bits(pEnc, nal_unit_type, 5); /* nal_unit_type */

	return MMES_NO_ERROR;
}

xint
write_seq_paraset_rbsp(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Write a sequence parameter set RBSP.                        */
/*                                                               */
/*   RETURN                                                      */
/*   Returns zero if everything is fine, nonzero if something is */
/*   wrong.                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jerry Peng                                                  */
/*   - Encodes totally identical bitstream JM encodeds with the  */
/*     same configuration.                                       */
/* ------------------------------------------------------------- */
{
    xint width = pEnc->width;
    xint height = pEnc->height;
    pEnc->log2_max_poc = MAX( log2bin( 2*pEnc->pCtrl->frame_num - 1 ), 4);

    /* profile & level information */
    put_bits(pEnc, 66, 8);      /* profile_idc     */
    put_bits(pEnc,  0, 3);      /* constraint_flag */
    put_bits(pEnc,  0, 5);      /* reserved zero   */
    put_bits(pEnc, 30, 8);      /* level_idc       */

    /* seq_parameter_set_id */
    write_uvlc_codeword(pEnc, 0); 
    write_uvlc_codeword(pEnc, pEnc->log2_max_fno-4);

    /* picture order count information */
    /* pic_order_cnt_type              */
    write_uvlc_codeword(pEnc, 0); 
    write_uvlc_codeword(pEnc, pEnc->log2_max_poc-4);

    /* reference frame information */
    /* num_ref_frames              */
    write_uvlc_codeword(pEnc, 1);
    /* gaps_in_frame_num_allowed   */
    put_one_bit(pEnc, 0);         

    /* compose width & height */
    /* width_in_MB   */
    write_uvlc_codeword(pEnc, width/MB_SIZE-1);  
    /* height_in_MB  */
    write_uvlc_codeword(pEnc, height/MB_SIZE-1); 

    /* use MBAFF? */
    /* frame_mbs_only_flag */
    put_one_bit(pEnc, 1); 

    /* use special B frame direct mode? */
    put_one_bit(pEnc, 1);

    /* the width & size is not a multiple of MB_SIZE? */
    put_one_bit(pEnc, 0);

    /* do we have video usability information? */
    put_one_bit(pEnc, 0);

    /* copies the last couple of bits into the byte buffer */
    sodbtorbsp(pEnc);

	return MMES_NO_ERROR;
}

xint
write_pic_paraset_rbsp(h264enc_obj *pEnc)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Write a picture parameter set RBSP.                         */
/*                                                               */
/*   RETURN                                                      */
/*   Returns zero if everything is fine, nonzero if something is */
/*   wrong.                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc  -> [I/O] pointer to the encoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    slice_obj *pSlice = pEnc->curr_slice;
//    xint qp_delta;

    /* pic_parameter_set_id */
    write_uvlc_codeword(pEnc, 0); 
    /* seq_parameter_set_id */
    write_uvlc_codeword(pEnc, 0); 
    /* entropy_coding_mode_flag */
    put_one_bit(pEnc, 0);       

    /* pic_order_present_flag */
    put_one_bit(pEnc, 0);         
    /* num_slice_groups_minus1 */
    write_uvlc_codeword(pEnc, 0); 

    /* some strange parameters for POC */
    /* num_ref_idx_l0_active - 1  */
    write_uvlc_codeword(pEnc, 0); 
    /* num_ref_idx_l1_active - 1  */
    write_uvlc_codeword(pEnc, 0); 

    /* VCL parameters */
    put_one_bit(pEnc, 0);         /* weighted_pred_flag         */
    put_bits(pEnc, 0, 2);         /* weighted_bipred_flag       */
    //write_uvlc_codeword(pEnc, 0); /* num_ref_idx_l0_active - 1  */

    //qp_delta = pEnc->QP - 26;
    // pic_init_qp - 26
    //write_signed_uvlc_codeword(pEnc, qp_delta); 
    /* pic_init_qp - 26 */
    write_signed_uvlc_codeword(pEnc, 0);
    /* pic_init_qs - 26 */
    write_signed_uvlc_codeword(pEnc, 0);        
    write_signed_uvlc_codeword(pEnc, pSlice->chroma_qp_index_offset);
    /* use special inloop filter   */
    if(pEnc->pCtrl->config_loop_filter == 1)
    {
        put_one_bit(pEnc, 1);
    }
    else
    {
        put_one_bit(pEnc, 0);         
    }
    /* use_constrained_intra_pred  */
    put_one_bit(pEnc, 0);         
    /* redundant_pic_cnt_present   */
    put_one_bit(pEnc, 0);         

    /* write trailing bits */
    sodbtorbsp(pEnc);

	return MMES_NO_ERROR;
}

uint32
read_uvlc_codeword(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/02/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Parse a UVLC codeword from the bitstream.                   */
/*                                                               */
/*   RETURN                                                      */
/*   Returns the codeword.                                       */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	uint32 codeword;
    xint   leading_zeros = 0;
    xint   info;
    xint   bit;

    /* firstly, read in M leading zeros followed by 1 */
    do
    {
        bit = get_bits(pDec, 1);

        if(leading_zeros == 0 && bit)
        {
            return 0;
        }

        leading_zeros++;

    }while(bit == 0);

    info = get_bits(pDec, leading_zeros - 1);

    codeword = (1 << (leading_zeros-1)) + info - 1;

	return codeword;
}

int32
read_signed_uvlc_codeword(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/02/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Parse a signed UVLC codeword to the bitstream.              */
/*                                                               */
/*   RETURN                                                      */
/*   Returns the codeword.                                       */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	int32  codeword;
    uint32 uvlc_code;
    xint   sign;
    xint   leading_zeros = 0;
    xint   info;
    xint   bit;

    do
    {
        bit = get_bits(pDec, 1);

        if(leading_zeros == 0 && bit)
        {
            return 0;
        }

        leading_zeros++;

    }while(bit == 0);

    info = get_bits(pDec, leading_zeros - 1);

    uvlc_code = (1 << (leading_zeros-1)) + info - 1;
    
    sign      = (uvlc_code % 2) ? 1 : -1;

    codeword  = sign * ((uvlc_code+1) >> 1);

	return codeword;
}

xint 
parse_hdr_parameters(h264dec_obj *pDec, hrd_parameters_t *hrd)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/29/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Parse a hdr parameter set RBSP.                             */
/*                                                               */
/*   RETURN                                                      */
/*   Returns zero if everything is fine, nonzero if something is */
/*   wrong.                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    uint SchedSelIdx;

    hrd->cpb_cnt_minus1                           = read_uvlc_codeword(pDec);
    hrd->bit_rate_scale                           = get_bits(pDec, 4);
    hrd->cpb_size_scale                           = get_bits(pDec, 4);

	for( SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++ ) 
    {
		hrd->bit_rate_value_minus1[ SchedSelIdx ] = read_uvlc_codeword(pDec);
		hrd->cpb_size_value_minus1[ SchedSelIdx ] = read_uvlc_codeword(pDec);
		hrd->cbr_flag[ SchedSelIdx ]              = get_bits(pDec, 1);
	}

    hrd->initial_cpb_removal_delay_length_minus1  = get_bits(pDec, 5);
	hrd->cpb_removal_delay_length_minus1          = get_bits(pDec, 5);
	hrd->dpb_output_delay_length_minus1           = get_bits(pDec, 5);
	hrd->time_offset_length                       = get_bits(pDec, 5);

  return MMES_NO_ERROR;
}

xint
parse_seq_paraset_rbsp(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/29/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Parse a sequence parameter set RBSP.                        */
/*                                                               */
/*   RETURN                                                      */
/*   Returns zero if everything is fine, nonzero if something is */
/*   wrong.                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint profile_idc;
    xint con_set0_flag, con_set1_flag, con_set2_flag;
    xint level_idc;
    xint sps_id;
    xint residue_transform_flag, lossless_qpprime_flag;
    seq_paraset_obj *sps;
    uint idx;

    profile_idc   = get_bits(pDec, 8);
    con_set0_flag = get_bits(pDec, 1);
    con_set1_flag = get_bits(pDec, 1);
    con_set2_flag = get_bits(pDec, 1);
       
    /* reserved_zero_5bits */
    get_bits(pDec, 5);

    level_idc     = get_bits(pDec, 8);
        
    /* seq_parameter_set_id */
    sps_id        = read_uvlc_codeword(pDec);
    
    sps           = &pDec->sps[sps_id];

    sps->profile_idc           = profile_idc;
    sps->constrained_set0_flag = con_set0_flag;
    sps->constrained_set1_flag = con_set1_flag;
    sps->constrained_set2_flag = con_set2_flag;
    sps->level_idc             = level_idc;

    // Fidelity Range Extensions stuff
  
    sps->chroma_format_idc = 1;
    sps->bit_depth_luma_minus8   = 0;
  
    sps->bit_depth_chroma_minus8 = 0;

    lossless_qpprime_flag   = 0;
    // Residue Color Transform
    residue_transform_flag = 0;

    if((sps->profile_idc==FREXT_HP   ) ||
       (sps->profile_idc==FREXT_Hi10P) ||
       (sps->profile_idc==FREXT_Hi422) ||
       (sps->profile_idc==FREXT_Hi444))
    {
        sps->chroma_format_idc = read_uvlc_codeword(pDec);
    
        // Residue Color Transform
        if(sps->chroma_format_idc == 3)
            residue_transform_flag   = get_bits(pDec, 1);

        sps->bit_depth_luma_minus8   = read_uvlc_codeword(pDec);
        sps->bit_depth_chroma_minus8 = read_uvlc_codeword(pDec);
        lossless_qpprime_flag        = get_bits(pDec, 1);

        sps->seq_scaling_matrix_present_flag = get_bits(pDec, 1);

        if(sps->seq_scaling_matrix_present_flag)
        {
            for(idx = 0 ; idx < 8 ; idx++)
            {
                sps->seq_scaling_list_present_flag[idx] = get_bits(pDec, 1);
                if(sps->seq_scaling_list_present_flag[idx])
                {
                    /*
                    if(idx < 6)
                        Scaling_List(sps->ScalingList4x4[i], 16, &sps->UseDefaultScalingMatrix4x4Flag[i], s);
                    else
                        Scaling_List(sps->ScalingList8x8[i-6], 64, &sps->UseDefaultScalingMatrix8x8Flag[i-6], s);
                    */
                    fprintf(stderr, "not support sps:seq_scaling_list_present_flag[%d] == 1\n", idx);
                }
            }
        }
    }


    /* log2_max_frame_num_minus4 */
    sps->log2_max_frame_num_minus4 = read_uvlc_codeword(pDec);
    
    pDec->max_frame_no             = 1 << (sps->log2_max_frame_num_minus4 + 4);

    /* pic_order_cnt_type */
    sps->pic_order_cnt_type        = read_uvlc_codeword(pDec);
    pDec->poc_type                 = sps->pic_order_cnt_type;
    /* log2_max_pic_order_cnt_lsb_minus4 */
    if(sps->pic_order_cnt_type == 0)
    {
        sps->log2_max_pic_order_cnt_lsb_minus4 = read_uvlc_codeword(pDec);
        pDec->max_poc_lsb                      = (1<<(sps->log2_max_pic_order_cnt_lsb_minus4+4));
    }
    else if(sps->pic_order_cnt_type == 1)
    {
        sps->log2_max_pic_order_cnt_lsb_minus4     = 0;
        /* delta_pic_order_always_zero_flag */
        sps->delta_pic_order_always_zero_flag      = get_bits(pDec, 1);
        /* offset_for_non_ref_pic */
        sps->offset_for_non_ref_pic                = read_signed_uvlc_codeword(pDec);
        /* offset_for_top_to_bottom_field */
        sps->offset_for_top_to_bottom_field        = read_signed_uvlc_codeword(pDec);
        /* num_ref_frames_in_pic_order_cnt_cycle */
        sps->num_ref_frames_in_pic_order_cnt_cycle = read_uvlc_codeword(pDec);
        
        for(idx = 0; idx < sps->num_ref_frames_in_pic_order_cnt_cycle ; idx++)
            sps->offset_for_ref_frame[idx]         = read_signed_uvlc_codeword(pDec);
    }

    /* reference frame information */
    sps->num_ref_frames                        = read_uvlc_codeword(pDec);
    sps->gaps_in_frame_num_value_allowed_flag  = get_bits(pDec, 1);

    /* compose width & height */
    sps->pic_width_in_mbs_minus1               = read_uvlc_codeword(pDec);
    sps->pic_height_in_map_units_minus1        = read_uvlc_codeword(pDec);
    
    sps->frame_mbs_only_flag                   = get_bits(pDec, 1);
    if (!sps->frame_mbs_only_flag)
        sps->mb_adaptive_frame_field_flag      = get_bits(pDec, 1);
  
    sps->direct_8x8_inference_flag             = get_bits(pDec, 1);
  
    sps->frame_cropping_flag                   = get_bits(pDec, 1);

    if (sps->frame_cropping_flag)
    {
        sps->frame_cropping_rect_left_offset   = read_uvlc_codeword(pDec);
        sps->frame_cropping_rect_right_offset  = read_uvlc_codeword(pDec);
        sps->frame_cropping_rect_top_offset    = read_uvlc_codeword(pDec);
        sps->frame_cropping_rect_bottom_offset = read_uvlc_codeword(pDec);
    }
    else
    {
        sps->frame_cropping_rect_left_offset  = 0;
        sps->frame_cropping_rect_right_offset = 0;
        sps->frame_cropping_rect_top_offset   = 0;
        sps->frame_cropping_rect_bottom_offset= 0;
    }

    pDec->crop_left   = sps->frame_cropping_rect_left_offset;
    pDec->crop_right  = sps->frame_cropping_rect_right_offset;
    pDec->crop_top    = sps->frame_cropping_rect_top_offset;
    pDec->crop_bottom = sps->frame_cropping_rect_bottom_offset;

    pDec->chroma_format_idc = sps->chroma_format_idc;

    sps->vui_parameters_present_flag           = get_bits(pDec, 1);

    if(sps->vui_parameters_present_flag)
    {
        sps->vui_seq_parameters.matrix_coefficients = 2;
        sps->vui_seq_parameters.aspect_ratio_info_present_flag = get_bits(pDec, 1);
    
        if (sps->vui_seq_parameters.aspect_ratio_info_present_flag)
        {
            sps->vui_seq_parameters.aspect_ratio_idc           = get_bits(pDec, 8);
            if (255==sps->vui_seq_parameters.aspect_ratio_idc)
            {
                sps->vui_seq_parameters.sar_width              = get_bits(pDec, 16);
                sps->vui_seq_parameters.sar_height             = get_bits(pDec, 16);
            }
        }
        sps->vui_seq_parameters.overscan_info_present_flag     = get_bits(pDec, 1);
        if (sps->vui_seq_parameters.overscan_info_present_flag)
        {
            sps->vui_seq_parameters.overscan_appropriate_flag  = get_bits(pDec, 1);
        }

        sps->vui_seq_parameters.video_signal_type_present_flag = get_bits(pDec, 1);
        if (sps->vui_seq_parameters.video_signal_type_present_flag)
        {
            sps->vui_seq_parameters.video_format               = get_bits(pDec, 3);
            sps->vui_seq_parameters.video_full_range_flag      = get_bits(pDec, 1);
            sps->vui_seq_parameters.colour_description_present_flag = get_bits(pDec, 1);
            if(sps->vui_seq_parameters.colour_description_present_flag)
            {
                sps->vui_seq_parameters.colour_primaries              = get_bits(pDec, 8);
                sps->vui_seq_parameters.transfer_characteristics      = get_bits(pDec, 8);
                sps->vui_seq_parameters.matrix_coefficients           = get_bits(pDec, 8);
            }
        }
        sps->vui_seq_parameters.chroma_location_info_present_flag = get_bits(pDec, 1);
        if(sps->vui_seq_parameters.chroma_location_info_present_flag)
        {
            sps->vui_seq_parameters.chroma_sample_loc_type_top_field     = read_uvlc_codeword(pDec);
            sps->vui_seq_parameters.chroma_sample_loc_type_bottom_field  = read_uvlc_codeword(pDec);
        }
        sps->vui_seq_parameters.timing_info_present_flag          = get_bits(pDec, 1);
        if (sps->vui_seq_parameters.timing_info_present_flag)
        {
            sps->vui_seq_parameters.num_units_in_tick               = get_bits(pDec, 32);
            sps->vui_seq_parameters.time_scale                      = get_bits(pDec, 32);
            sps->vui_seq_parameters.fixed_frame_rate_flag           = get_bits(pDec, 1);
        }
        sps->vui_seq_parameters.nal_hrd_parameters_present_flag   = get_bits(pDec, 1);
        if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag)
        {
            parse_hdr_parameters(pDec, &(sps->vui_seq_parameters.nal_hrd_parameters));
        }
        sps->vui_seq_parameters.vcl_hrd_parameters_present_flag   = get_bits(pDec, 1);
        if (sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
        {
            parse_hdr_parameters(pDec, &(sps->vui_seq_parameters.vcl_hrd_parameters));
        }
        if (sps->vui_seq_parameters.nal_hrd_parameters_present_flag || sps->vui_seq_parameters.vcl_hrd_parameters_present_flag)
        {
          sps->vui_seq_parameters.low_delay_hrd_flag             =  get_bits(pDec, 1);
        }
        sps->vui_seq_parameters.pic_struct_present_flag          =  get_bits(pDec, 1);
        sps->vui_seq_parameters.bitstream_restriction_flag       =  get_bits(pDec, 1);
        if (sps->vui_seq_parameters.bitstream_restriction_flag)
        {
          sps->vui_seq_parameters.motion_vectors_over_pic_boundaries_flag =  get_bits(pDec, 1);
          sps->vui_seq_parameters.max_bytes_per_pic_denom                 =  read_uvlc_codeword(pDec);
          sps->vui_seq_parameters.max_bits_per_mb_denom                   =  read_uvlc_codeword(pDec);
          sps->vui_seq_parameters.log2_max_mv_length_horizontal           =  read_uvlc_codeword(pDec);
          sps->vui_seq_parameters.log2_max_mv_length_vertical             =  read_uvlc_codeword(pDec);
          sps->vui_seq_parameters.num_reorder_frames                      =  read_uvlc_codeword(pDec);
          sps->vui_seq_parameters.max_dec_frame_buffering                 =  read_uvlc_codeword(pDec);
        }
    }
  
    sps->Valid = 1;

    return MMES_NO_ERROR;
}

xint
parse_pic_paraset_rbsp(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/29/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Parse a picture parameter set RBSP.                         */
/*                                                               */
/*   RETURN                                                      */
/*   Returns zero if everything is fine, nonzero if something is */
/*   wrong.                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    uint i;
    xint NumberBitsPerSliceGroupId;
    xint pic_parameter_set_id;
    pic_paraset_obj *pps;

    pic_parameter_set_id                       = read_uvlc_codeword(pDec);

    pps = &pDec->pps[pic_parameter_set_id];

    pps->pic_parameter_set_id                  = pic_parameter_set_id;

    pps->seq_parameter_set_id                  = read_uvlc_codeword(pDec);
    pps->entropy_coding_mode_flag              = get_bits(pDec, 1);

    pps->pic_order_present_flag                = get_bits(pDec, 1);

    pps->num_slice_groups_minus1               = read_uvlc_codeword(pDec);
    
    // FMO stuff begins here
    if (pps->num_slice_groups_minus1 > 0)
    {
        fprintf(stderr, "Not support slice group.\n");
        exit(1);
        pps->slice_group_map_type              = read_uvlc_codeword(pDec);
        if (pps->slice_group_map_type == 0)
        {
            for (i=0; i<=pps->num_slice_groups_minus1; i++)
            {
                pps->run_length_minus1 [i]     = read_uvlc_codeword(pDec);
            }
        }
        else if (pps->slice_group_map_type == 2)
        {
            for (i=0; i<pps->num_slice_groups_minus1; i++)
            {
                //! JVT-F078: avoid reference of SPS by using ue(v) instead of u(v)
                pps->top_left [i]              = read_uvlc_codeword(pDec);

                pps->bottom_right [i]          = read_uvlc_codeword(pDec);
            }
        }
        else if (pps->slice_group_map_type == 3 ||
                 pps->slice_group_map_type == 4 ||
                 pps->slice_group_map_type == 5)
        {
            pps->slice_group_change_direction_flag     = get_bits(pDec, 1);
            pps->slice_group_change_rate_minus1        = read_uvlc_codeword(pDec);
        }
        else if (pps->slice_group_map_type == 6)
        {
            if (pps->num_slice_groups_minus1+1 >4)
                NumberBitsPerSliceGroupId = 3;
            else if (pps->num_slice_groups_minus1+1 > 2)
                NumberBitsPerSliceGroupId = 2;
            else
                NumberBitsPerSliceGroupId = 1;
            //! JVT-F078, exlicitly signal number of MBs in the map
            pps->num_slice_group_map_units_minus1      = read_uvlc_codeword(pDec);
            for (i = 0 ; i <= pps->num_slice_group_map_units_minus1 ; i++)
            {
                uint32 slice_group_id;
                slice_group_id = get_bits(pDec, NumberBitsPerSliceGroupId);
            }
        }
    }

    pps->num_ref_idx_l0_active_minus1          = read_uvlc_codeword(pDec);
    pps->num_ref_idx_l1_active_minus1          = read_uvlc_codeword(pDec);
    pps->weighted_pred_flag                    = get_bits(pDec, 1);
    pps->weighted_bipred_idc                   = get_bits(pDec, 2);
    pps->pic_init_qp_minus26                   = read_signed_uvlc_codeword(pDec);
    pps->pic_init_qs_minus26                   = read_signed_uvlc_codeword(pDec);

    pps->chroma_qp_index_offset                = read_signed_uvlc_codeword(pDec);

    pps->deblocking_filter_control_present_flag= get_bits(pDec, 1);
    pps->constrained_intra_pred_flag           = get_bits(pDec, 1);
    pps->redundant_pic_cnt_present_flag        = get_bits(pDec, 1);

    pps->Valid = 1;

	return MMES_NO_ERROR;
}

xint parse_slice_header(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/02/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Parse a slice header.                                       */
/*                                                               */
/*   RETURN                                                      */
/*   Returns zero if everything is fine, nonzero if something is */
/*   wrong.                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    slice_obj *slice;
    seq_paraset_obj *sps;
    pic_paraset_obj *pps;
    xint first_mb_in_slice, slice_type, pic_parameter_set_id;

    /* keep the info but do not support now */
    xint slice_qp_delta;

	first_mb_in_slice            = read_uvlc_codeword(pDec);

	slice_type                   = read_uvlc_codeword(pDec);
	pic_parameter_set_id         = read_uvlc_codeword(pDec);

    /* choose corresponding seq & pic parameter set */
    pps = &pDec->pps[pic_parameter_set_id];
    sps = &pDec->sps[pps->seq_parameter_set_id];

    pDec->nx_mb    = sps->pic_width_in_mbs_minus1 + 1; 
    pDec->ny_mb    = sps->pic_height_in_map_units_minus1 + 1;
    pDec->total_mb = pDec->nx_mb * pDec->ny_mb;

    /* allocate a new slice */
    pDec->width = pDec->nx_mb * MB_SIZE;
    pDec->height= pDec->ny_mb * MB_SIZE;

    if (pDec->curr_slice == NULL)
    {
        if((pDec->curr_slice = alloc_slice(pDec->width)) == NULL)
        {
            printf("h264_init_decoder alloc current slice failed !\n");
            return MMES_ALLOC_ERROR;
        }
    }

    slice = pDec->curr_slice;

    slice->first_mb_in_slice            = first_mb_in_slice;
    slice->type                         = ((slice_type > 4) ? slice_type-ALL_P_SLICE : slice_type);
    slice->pic_parameter_set_id         = pic_parameter_set_id;

    /* read frame number */
    pDec->cur_frame_no                  = get_bits(pDec, sps->log2_max_frame_num_minus4 + 4);
    
    if (sps->frame_mbs_only_flag)
    {
        slice->field_pic_flag = 0;
    }
    else
    {
        // field_pic_flag   u(1)
        slice->field_pic_flag           = get_bits(pDec, 1);
        if (slice->field_pic_flag)
        {
            // bottom_field_flag  u(1)
            slice->bottom_field_flag    = get_bits(pDec, 1);
        }
        else
            slice->bottom_field_flag    = 0;
    }

    if(pDec->nal.nal_unit_type == NALU_TYPE_IDR)
    {
        read_uvlc_codeword(pDec);
        pDec->past_frame_no = pDec->cur_frame_no;
    }
    else
        pDec->recf->idr_flag = 0;

     
    if (sps->pic_order_cnt_type == 0)
    {
        /* pic_order_cnt_lsb */
        slice->pic_order_cnt_lsb = get_bits(pDec, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    
        if( pps->pic_order_present_flag && !slice->field_pic_flag )
            slice->delta_poc_btm = read_signed_uvlc_codeword(pDec);
        else
            slice->delta_poc_btm = 0;  
    }
  
    if( sps->pic_order_cnt_type == 1 && !sps->delta_pic_order_always_zero_flag ) 
    {
        slice->delta_poc[0] = read_signed_uvlc_codeword(pDec);
        if( pps->pic_order_present_flag &&  !slice->field_pic_flag )
            slice->delta_poc[1] = read_signed_uvlc_codeword(pDec);
    }
    else
    {
        if (sps->pic_order_cnt_type == 1)
            slice->delta_poc[ 0 ] = slice->delta_poc[ 1 ] = 0;
    }
  
    //! redundant_pic_cnt is missing here
    if (pps->redundant_pic_cnt_present_flag)
        slice->redundant_pic_cnt = read_uvlc_codeword(pDec);

    slice->num_ref_idx_l0_active = pps->num_ref_idx_l0_active_minus1 + 1;
    slice->num_ref_idx_l1_active = pps->num_ref_idx_l1_active_minus1 + 1;

    if(slice->type==P_SLICE || slice->type == SP_SLICE || slice->type==B_SLICE)
    {
        if (get_bits(pDec, 1))
        {
            slice->num_ref_idx_l0_active = 1 + read_uvlc_codeword(pDec);

            if(slice->type==B_SLICE)
                slice->num_ref_idx_l1_active = 1 + read_uvlc_codeword(pDec);
        }
    }
    if (slice->type!=B_SLICE)
        slice->num_ref_idx_l1_active = 0;

    /* ref_pic_list_reordering */
    if(!IS_ISLICE(slice->type) && slice->type != SI_SLICE)
    {
	    slice->ref_pic_list_reordering_flag_l0 = get_bits(pDec, 1);
        if(slice->ref_pic_list_reordering_flag_l0)
        {
            xint idx = 0; 
            do
            {
                slice->remapping_of_pic_nums_idc_l0[idx] = read_uvlc_codeword(pDec);
                if(slice->remapping_of_pic_nums_idc_l0[idx] == 0 || slice->remapping_of_pic_nums_idc_l0[idx] == 1)
                    slice->abs_diff_pic_l0[idx] = read_uvlc_codeword(pDec) + 1;
                else if(slice->remapping_of_pic_nums_idc_l0[idx] == 2)
                    slice->long_term_pic_idx_l0[idx] = read_uvlc_codeword(pDec);
            }while(slice->remapping_of_pic_nums_idc_l0[idx] != 3 && ++idx);
        }
    }
    else slice->ref_pic_list_reordering_flag_l0 = 0;

    if ((pps->weighted_pred_flag && (slice->type==P_SLICE || slice->type == SP_SLICE))||
        (pps->weighted_bipred_idc&& (slice->type==B_SLICE)))
    {
        //pred_weight_table();
        fprintf(stderr, "Not support prediction weight table.\n");
        exit(1);
    }

    /* dec_ref_pic_marking */

    if (pDec->nal.nal_reference_idc)
    {
        if(pDec->nal.nal_unit_type == NALU_TYPE_IDR)
        {
            pDec->no_output_of_prior_pics_flag   = get_bits(pDec, 1);
            pDec->long_term_reference_flag       = get_bits(pDec, 1);
        }
        else
        {
            xint idx = 0;
            xint mem_manage_ctrl_op = 0;
            //pDec->no_output_of_prior_pics_flag   = 0;
            //pDec->long_term_reference_flag       = 0;
            
            slice->adaptive_ref_pic_buf_flag     = get_bits(pDec, 1);
            if(slice->adaptive_ref_pic_buf_flag)
            {
                do
                {
                    dec_refpicmark *cur_dec_refpicmark = &slice->dec_refpicmark_buf[idx];

                    memset(cur_dec_refpicmark, 0, sizeof(dec_refpicmark));

                    mem_manage_ctrl_op = cur_dec_refpicmark->mem_manage_ctrl_op 
                        = read_uvlc_codeword(pDec);

                    if ((mem_manage_ctrl_op == 1) || (mem_manage_ctrl_op == 3)) 
                        cur_dec_refpicmark->diff_of_pic_nums = read_uvlc_codeword(pDec) + 1;

                    if (mem_manage_ctrl_op == 2)
                        cur_dec_refpicmark->lt_pic_num = read_uvlc_codeword(pDec);
          
                    if ((mem_manage_ctrl_op == 3) || (mem_manage_ctrl_op == 6))
                        cur_dec_refpicmark->lt_frm_idx = read_uvlc_codeword(pDec);

                    if (mem_manage_ctrl_op == 4)
                        cur_dec_refpicmark->max_lt_frm_idx = read_uvlc_codeword(pDec) - 1;
              }while (mem_manage_ctrl_op != 0 && ++idx);
            }
        }

    }

    slice_qp_delta = read_signed_uvlc_codeword(pDec);
    slice->QP = slice_qp_delta + 26 + pps->pic_init_qp_minus26;

    slice->chroma_qp_index_offset = pps->chroma_qp_index_offset;

    if (pps->deblocking_filter_control_present_flag)
    {
        slice->lf_disable_idc = read_uvlc_codeword(pDec);

        if (slice->lf_disable_idc != 1)
        {
            slice->lf_alpha_c0_offset_div2 = read_signed_uvlc_codeword(pDec);
            slice->lf_bata_offset_div2     = read_signed_uvlc_codeword(pDec);
        }
        else
            slice->lf_alpha_c0_offset_div2 = slice->lf_bata_offset_div2 = 0;
    }
    else 
        slice->lf_disable_idc = slice->lf_alpha_c0_offset_div2 = slice->lf_bata_offset_div2 = 0;

    if (pps->num_slice_groups_minus1 > 0 && pps->slice_group_map_type >=3 &&
        pps->slice_group_map_type<=5)
    {   
        /* todo : slice_group */
        fprintf(stderr, "Not support slice_group_change_cycle.\n");
        exit(1);
    }

    return MMES_NO_ERROR;
}

xint
parse_slice_data(h264dec_obj *pDec, xint *pEndOFFrame)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Sep/??/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Parse a slice RBSP.                                         */
/*                                                               */
/*   RETURN                                                      */
/*   Returns zero if everything is fine, nonzero if something is */
/*   wrong.                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */ 
{
	slice_obj    *pSlice    = pDec->curr_slice;
    mb_obj       *pMB       = &pDec->curr_slice->cmb;
    xint         EndOFSlice = 0;
    xint         res_bits   = 0;

    const uint32 trailing_bits[9] = 
    {0, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

    reset_slice(pSlice, pDec->width);
    check_frame_gaps(pDec);
    init_ref_lists(pDec);
    reorder_lists(pDec);

    /* ===========  decode all mbs in the slice   =========== */
    /* decode all the macroblock in the slice */
    /* initialize macroblock */
    pMB->id        = pDec->cur_mb_no;
    pMB->QP        = pSlice->QP; 
    pSlice->no_mbs = 0;

    do
    {
		init_mb_info( pDec->curr_slice, pDec->width );

        /* decode one macroblock */
        switch (pSlice->type)
        {
        case I_SLICE:
        case ALL_I_SLICE:
            decode_intra_mb(pDec);
            break;
        case P_SLICE:
        case ALL_P_SLICE:
            decode_inter_mb(pDec);
            break;
        case B_SLICE:
            break;
        }

        {
            uint v = show_bits(pDec, 32);
            printf("%08X\n", v);
        }


        pMB->id++;
		pSlice->no_mbs++;
        pDec->cur_mb_no++;
        
        res_bits = residual_bits(pDec);
        if(pSlice->mb_skip_run <= 1 && res_bits <= 8)
        {
            if(trailing_bits[res_bits] == show_bits(pDec, res_bits))
            {
                EndOFSlice = 1;
                pSlice->mb_skip_run = 0;            
            }
        }

    } while ( !EndOFSlice && pMB->id < pDec->total_mb);// 06 22 2005

    *pEndOFFrame = (pMB->id < pDec->total_mb) ? 0 : 1;
    pDec->cur_slice_no++;

    /* =========== then, read slice trailing bits =========== */
    parse_slice_trailing_bits(pDec);

	return MMES_NO_ERROR;
}

xint 
init_nalu(nal_obj *nalu)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/02/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   initialize a NAL unit.                                      */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *nalu    -> to reserve the nal unit and side info           */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    memset(nalu, 0, sizeof(nal_obj));
    
    nalu->is_first_byte = 1;
    nalu->buf           = (uint8 *) malloc(sizeof(uint8) * NALU_MAXSIZE);

    if(nalu->buf == NULL)
    {
        fprintf(stderr, "h264_init_decoder alloc nalu buffer failed !\n");
        return MMES_ALLOC_ERROR;
    }

    memset(nalu->buf, 0, sizeof(uint8) * NALU_MAXSIZE);

    return MMES_NO_ERROR;
}

void 
destroy_nalu(nal_obj *nalu)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/02/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   destroy a NAL unit.                                         */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *nalu    -> to reserve the nal unit and side info           */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    /* free nal unit buffer */
    free(nalu->buf);
}

xint get_nalu(h264dec_obj *pDec, FILE *fp_bit)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/01/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Parse a NAL unit.(Extract from JM 9.6)                      */
/*                                                               */
/*   RETURN                                                      */
/*   Returns zero if everything is fine, nonzero if something is */
/*   wrong.                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *nalu    -> to reserve the nal unit and side info           */
/*   *fp_bit  -> file pointer to the bitstream                   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint pos = 0;
    xint s_code3 = 0, s_code4 = 0;
    xint rewind;
    uint8 *Buf;
    xint LeadingZero8BitsCount = 0, TrailingZero8Bits = 0;
    xint zero_byte = 0;
    nal_obj *nalu = &pDec->nal;
    
    Buf = (uint8 *)malloc(NALU_MAXSIZE * sizeof(uint8));

    if(Buf == NULL)
    {
        fprintf(stderr, "ERROR: allocating temporal nalu buffer fails\n");
        return MMES_ALLOC_ERROR;
    }

    while(!feof(fp_bit) && (Buf[pos++]=fgetc(fp_bit))==0);
  
    if(feof(fp_bit))
    {
        if(pos==0)
            return 0;
        else
        {
            fprintf(stderr, "ERROR : Can't read start code.\n");
            free(Buf);
            return MMES_ERROR;
        }
    }

    if(Buf[pos-1] != 1)
    {
        fprintf (stderr, "ERROR : no Start Code at the begin of the NALU\n");
        free(Buf);
        return MMES_ERROR;
    }

    if(pos < 3)
    {
        fprintf (stderr, "GetAnnexbNALU: no Start Code at the begin of the NALU\n");
        free(Buf);
        return -1;
    }
    else if(pos == 3)
    {
        nalu->startcodeprefix_len = 3;
        LeadingZero8BitsCount = 0;
    }
    else
    {
        LeadingZero8BitsCount = pos-4;
        nalu->startcodeprefix_len = 4;
    }

    //the 1st byte stream NAL unit can has leading_zero_8bits, but subsequent ones are not
    //allowed to contain it since these zeros(if any) are considered trailing_zero_8bits
    //of the previous byte stream NAL unit.
    if(!nalu->is_first_byte && LeadingZero8BitsCount>0)
    {
        fprintf (stderr, "ERROR : The leading_zero_8bits syntax can only be present in the first byte stream NAL unit.\n");
        free(Buf);
        return MMES_ERROR;
    }
    else nalu->is_first_byte = 0;

    while (s_code4 == 0 && s_code3 == 0)
    {
        if (feof (fp_bit))
        {
            //Count the trailing_zero_8bits
            while(Buf[pos-2-TrailingZero8Bits]==0)
                TrailingZero8Bits++;
            nalu->len = (pos-1)-nalu->startcodeprefix_len-LeadingZero8BitsCount-TrailingZero8Bits;
            memcpy(nalu->buf, &Buf[LeadingZero8BitsCount+nalu->startcodeprefix_len], nalu->len);     
            nalu->forbidden_bit = (nalu->buf[0]>>7) & 1;
            nalu->nal_reference_idc = (nalu->buf[0]>>5) & 3;
            nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;

            free(Buf);
            return pos-1;
        }
    
        Buf[pos] = fgetc (fp_bit);   

        if(Buf[pos] == 1)
        {
            if(zero_byte == 3)
                s_code4 = 1;    
            else if(zero_byte == 2)
                s_code3 = 1;
        }
    
        zero_byte = (Buf[pos]) ? 0 : ((zero_byte <= 3) ? (zero_byte + 1) : 3);
        pos++;
    }

    //Count the trailing_zero_8bits
    //if the detected start code is 00 00 01, trailing_zero_8bits is sure not to be present
    if(s_code4==1)	
    {
        while(Buf[pos-5-TrailingZero8Bits]==0)
            TrailingZero8Bits++;
    }

    // Here, we have found another start code (and read length of startcode bytes more than we should
    // have.  Hence, go back in the file
    rewind = 0;
    if(s_code4 == 1)
        rewind = -4;
    else if (s_code3 == 1)
        rewind = -3;

    if (0 != fseek (fp_bit, rewind, SEEK_CUR))
    {
        fprintf (stderr, "Error : Cannot fseek %d in the bit stream file", rewind);
        free(Buf);
        return MMES_ERROR;
    }

    // Here the leading zeros(if any), Start code, the complete NALU, trailing zeros(if any)
    // and the next start code is in the Buf.
    // The size of Buf is pos, pos+rewind are the number of bytes excluding the next
    // start code, and (pos+rewind)-startcodeprefix_len-LeadingZero8BitsCount-TrailingZero8Bits
    // is the size of the NALU.

    nalu->len = (pos+rewind)-nalu->startcodeprefix_len-LeadingZero8BitsCount-TrailingZero8Bits;
    memcpy (nalu->buf, &Buf[LeadingZero8BitsCount+nalu->startcodeprefix_len], nalu->len);
    nalu->forbidden_bit = (nalu->buf[0]>>7) & 1;
    nalu->nal_reference_idc = (nalu->buf[0]>>5) & 3;
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;

    free(Buf);
    return (pos+rewind);
}