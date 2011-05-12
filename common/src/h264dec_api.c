/* ///////////////////////////////////////////////////////////// */
/*   File: h264dec_api.c                                         */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 decoder API.                     */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#include <stdio.h>
#include <stdlib.h>
#if !__CC_ARM
#include <memory.h>
#else
#include <string.h>
#endif

#include "../../common/inc/h264dec_api.h"
#include "../../common/inc/bitstream.h"
#include "../../common/inc/macroblock.h"
#include "../../common/inc/misc_util.h"
#include "../../common/inc/intra_pred.h"
#include "../../common/inc/quant.h"
#include "../../common/inc/nal.h"

static __inline
xint
get_dpbsize(h264dec_obj *pDec)
{
    seq_paraset_obj *sps;
    pic_paraset_obj *pps;
    slice_obj       *slice = pDec->curr_slice;
    xint            pic_size;
    xint            size = 0;

    pps = &pDec->pps[slice->pic_parameter_set_id];
    sps = &pDec->sps[pps->seq_parameter_set_id];

    pic_size = (pDec->sps->pic_width_in_mbs_minus1 + 1) * 
               (sps->pic_height_in_map_units_minus1 + 1) * 
               (sps->frame_mbs_only_flag?1:2) * 384;

    //get MaxDPB size depended on Level num
    switch (sps->level_idc)
    {
    case 10:
        //level number 1
        size = 152064;
        break;
    case 11:
        //1.1
        size = 345600;
        break;
    case 12:
        //1.2
        size = 912384;
        break;
    case 13:
        //1.3
        size = 912384;
        break;
    case 20:
        //2
        size = 912384;
        break;
    case 21:
        //2.1
        size = 1824768;
        break;
    case 22:
        //2.2
        size = 3110400;
        break;
    case 30:
        //3
        size = 3110400;
        break;
    case 31:
        //3.1
        size = 6912000;
        break;
    case 32:
        //3.2
        size = 7864320;
        break;
    case 40:
        //4
        size = 12582912;
        break;
    case 41:
        //4.1
        size = 12582912;
        break;
    case 42:
        //4.2
        size = 12582912;
        break;
    case 50:
        //5
        size = 42393600;
        break;
    case 51:
        //5.1
        size = 70778880;
        break;
    default:
        fprintf(stderr, "Error : undefined level.\n");
        break;
    }

    size /= pic_size;
    size = MIN(size, 16);

    return size;
}

xint
h264_init_decoder(h264dec_obj *pDec, xint realloc)
{
    xint    chroma_frame_width;
    xint    chroma_mb_size;
    xint    width, height, i;
    slice_obj       *pSlice = pDec->curr_slice;
    pic_paraset_obj *pps    = &pDec->pps[pSlice->pic_parameter_set_id];
    seq_paraset_obj *sps    = &pDec->sps[pps->seq_parameter_set_id];
    
    if(realloc)
    {
        pDec->ref_frms_in_buf   = 0;
        pDec->ltref_frms_in_buf = 0;
        pDec->max_reff_no       = sps->num_ref_frames;
        pDec->dpbsize           = get_dpbsize(pDec);
        pDec->used_size         = 0;
 
        width = pDec->width ;
        height= pDec->height;
    
        chroma_frame_width = width / 2;     // for YCbCr420

        chroma_mb_size = MB_SIZE / 2;       // for YCbCr420

        pDec->curf = alloc_frame(width, height);
        if (pDec->curf == NULL)
        {
            printf("h264_init_decoder alloc current frame failed !\n");
            return MMES_ALLOC_ERROR;
        }
        pDec->recf = alloc_frame(width, height);
        if (pDec->recf == NULL)
        {
            printf("h264_init_decoder alloc reconstrcted frame failed !\n");
            return MMES_ALLOC_ERROR;
        }

  	    pDec->reff        = (frame_obj **)malloc(sizeof(frame_obj *)*pDec->dpbsize);
        pDec->ltreff_list = (frame_obj **)malloc(sizeof(frame_obj *)*pDec->dpbsize);
        pDec->reff_list   = (frame_obj **)malloc(sizeof(frame_obj *)*pDec->dpbsize);
        pDec->rlist       = (frame_obj **)malloc(sizeof(frame_obj *)*MAXLIST);
        
        for (i=0; i<pDec->dpbsize; i++) {
	        pDec->reff[i]      = alloc_frame(width, height);
		    if (pDec->reff[i] == NULL)
		    {
			    printf("h264_init_decoder alloc reference frame failed !\n");
			    return MMES_ALLOC_ERROR;
		    }
	    }
        
        memset(pDec->ltreff_list, 0, sizeof(frame_obj*)*pDec->dpbsize);
        memset(pDec->reff_list  , 0, sizeof(frame_obj*)*pDec->dpbsize);
        memset(pDec->rlist      , 0, sizeof(frame_obj*)*MAXLIST);

	    pDec->backup = alloc_backup(width);

	    init_quant_matrix();
    }
    
    init_picture(pDec);

    return MMES_NO_ERROR;
}

xint 
h264_decode_header(h264dec_obj *pDec, FILE *fp_bit)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/28/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Decode H264 Video Header                                    */
/*                                                               */
/*   RETURN                                                      */
/*   The status code of decoding H264 header.                    */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoding session parameter   */
/*   *hdrbuf-> [I]   pointer to h264 header buffer               */
/*   hdrlen -> [I]   number of bits header buffer contains       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    nal_obj *nalu = &pDec->nal;

    do
    {
        get_nalu(pDec, fp_bit);

        ebsptorbsp(nalu);

        stream_init(pDec, nalu->buf+1, nalu->len-1);
        
        if(nalu->len)
        {
            switch (pDec->nal.nal_unit_type)
            {
            case NALU_TYPE_SLICE:
            case NALU_TYPE_IDR:
                parse_slice_header(pDec);
                return MMES_NO_ERROR;
                break;
            case NALU_TYPE_PPS:
                parse_pic_paraset_rbsp(pDec);
                break;
            case NALU_TYPE_SPS:
                parse_seq_paraset_rbsp(pDec);
                break;
            case NALU_TYPE_SEI:
                fprintf(stderr, "read_new_slice: Found NALU_TYPE_SEI, len %d\n", nalu->len);
                fprintf(stderr, "Not support SEI Message yet!\n");
                break;
            default:
                break;
            }
        }
        else return MMES_ERROR;

    }while(!feof(fp_bit));

    return MMES_NO_ERROR;
}

xint
h264_free_decoder(h264dec_obj * pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Jul/19/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Free all the memory used by the H.264 decoder session.      */
/*                                                               */
/*   RETURN                                                      */
/*   The status code of memory de-allocation.                    */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoding session parameter   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Sep/05/2005 Jerry Peng, Add backup object.               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint i;

    /* free current slice */
    free_slice(pDec->curr_slice), pDec->curr_slice = NULL;

    /* free curf frame; */
    free_frame(pDec->curf),       pDec->curf = NULL;

    /* free recf frame; */
    free_frame(pDec->recf),       pDec->recf = NULL;

    /* free reff frame */
	for (i=0; i<pDec->dpbsize; i++)
	    free_frame(pDec->reff[i]),pDec->reff[i] = NULL;

    /* free reff frame container */
    free(pDec->reff),             pDec->reff = NULL; 
    free(pDec->ltreff_list),      pDec->ltreff_list = NULL;
    free(pDec->reff_list),        pDec->reff_list = NULL; 
    free(pDec->rlist),            pDec->rlist = NULL;

        
	/* free backup */
	free_backup(pDec->backup),    pDec->backup = NULL;

    return MMES_NO_ERROR;
}