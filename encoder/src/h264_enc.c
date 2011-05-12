/* ///////////////////////////////////////////////////////////// */
/*   File: h264_enc.c                                            */
/*   Author: Jerry Peng                                          */
/*   Date: Nov/??/2004                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 sample encoder.                  */
/*                                                               */
/*   Copyright, 2004.                                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ///////////////////////////////////////////////////////////// */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>

#include "../../common/inc/bitstream.h"
#include "../../common/inc/h264enc_api.h"
#include "../../common/inc/misc_util.h"
#include "../../common/inc/mv_search.h"
#include "../../common/inc/rate_control.h"
//#include "../../common/inc/quant.h"

#define SAVE_RECONSTRUCTED_FRAME 1

uint8  *bit_buffer = NULL; /* for output bitstream */

h264enc_obj h264_enc;

uint   timer_count, total_size = 0;
double total_psnr = 0;
double frame_rate = FRAME_RATE;

// @jerry for turning on BW-Lite ME
xint h264_bw_lite = 1;

xint
read_yuvframe(frame_obj *pframe, xint id, xint width, xint height, FILE *fp_src)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Read a YCbCr 4:2:0 video frame.                             */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    chroma_width, chroma_height;

    pframe->id = id;

    /* calculate the size for Cb&Cr 4:2:0 frames */
    chroma_width = width>>1;
    chroma_height = height>>1;

    if( fread(pframe->y , width * height,               1, fp_src) != 1 )	{ return MMES_FILE_ERROR;};
    if( fread(pframe->cb, chroma_width * chroma_height, 1, fp_src) != 1 )	{ return MMES_FILE_ERROR;};
    if( fread(pframe->cr, chroma_width * chroma_height, 1, fp_src) != 1 )	{ return MMES_FILE_ERROR;};

    return MMES_NO_ERROR;
}

void
panic(char *s)
{
    fprintf(stderr, "h264enc: %s\n\n", s);
}

xint
encode_one_frame(h264enc_obj *pEnc, enc_cfg *ctrl, uint8 *pBuf, xint bufsize, uint8 *cur_frm_y)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Nov/??/2004                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Encode a video frame.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    xint    width = ctrl->frame_width;
    xint    height= ctrl->frame_height;
    xint    nbits, total_bits, done;
    double  psnr;
    uint    frame_time = GetTimerCount();
    //printf("Encoded %d frames in %d msec.\n", frame_no, timer_count);

	rc_obj *pRC = &pEnc->pRC;

    /* initialize bitstream output buffer */
    h264_init_bitstream(pEnc, pBuf, (bufsize<<3));

    /* setup the slice parameters:                            */
    /*    slice type = auto select, slice size = 3 rows of MBs */
    h264_init_slice(pEnc, AUTO_SLICE, ctrl->mb_in_slice);

	/* frame level rate control */
	if(pEnc->rc_enable)
	{
		pEnc->curr_slice->QP = pEnc->QP = rc_get_frame_QP(pRC, pEnc->curr_slice->type);
	}

    /* slice encoding loop */
    total_bits = 0;

	me_init_frame(pEnc);

    /* set idr encoding flag */
    pEnc->idr_flag = (pEnc->cur_frame_no == 0);

    do
    {
        pEnc->ref_flag = 1;
        /* encode one slice */
        nbits = h264_encode_slice(&h264_enc, &done);

		/* reset one slice */
		reset_slice(pEnc->curr_slice, pEnc->width);

        if (nbits >= 0)
        {
            pBuf += nbits;
            total_bits += nbits;
        }
        else
        {
            return MMES_ERROR;
        }
        pEnc->cur_slice_no++;
    } while (! done);

    dup_frame_to_frame( pEnc->reff[0], pEnc->recf, pEnc->width, pEnc->height );

    frame_time  = GetTimerCount() - frame_time;

    psnr        = calc_psnr(cur_frm_y, pEnc->recf->y, width, height);
    total_psnr += psnr;
    total_size += total_bits;

	if(pEnc->rc_enable)
	{
		printf( "Frame%4d [%c] QP %2d target %5d bits %5d bits PSNR=%6.2f dB,%3d msec.\n", pEnc->cur_frame_no, VOP2STR(pEnc->curr_slice->type), h264_enc.QP, pRC->avg_frame_size, total_bits, psnr, frame_time);
		rc_update_buf(pRC, total_bits);
		rc_update_data(pRC, pEnc->curr_slice->type, h264_enc.QP, total_bits);
	}
	else
	{
		printf( "Frame%4d [%c] bits %5d bits PSNR=%6.2f dB,%3d msec.\n", pEnc->cur_frame_no, VOP2STR(pEnc->curr_slice->type), total_bits, psnr, frame_time);
	}
	
    return total_bits;
}

xint init_config(enc_cfg *ctrl)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/??/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Initialize the configuration file                           */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    CFG_TYPE    i;
    xint        j;
    char        buf[256];
    xint        next_param = 1;
    xint        status = MMES_NO_ERROR;

    ctrl->src_fp = ctrl->rec_fp = ctrl->bit_fp = NULL;

    i = SRC_FILE;
    while(fgets(buf, 255, ctrl->cfg_fp))
    {
        for(j = 0 ; j < 255 ; j++)
        {
            if(buf[j] == '#' || buf[j] == ' ' || buf[j] == '\n' || buf[j] == '\t' || buf[j] =='\x00')
                break;
        }
        buf[j]     = '\x00';
        
        if(j)
        {
            next_param = 1;

            switch(i)
            {
                case SRC_FILE:
                    strcpy(ctrl->src_file, buf);
                    break;
                case REC_FILE:
                    strcpy(ctrl->rec_file, buf);
                    break;
			    case BS_FILE:
                    strcpy(ctrl->bs_file, buf);
                    break;
			    case FRAME_WIDTH:
                    ctrl->frame_width         = atoi(buf);
                    break;
			    case FRAME_HEIGHT:
                    ctrl->frame_height        = atoi(buf);
                    break;
                case START_FRAME:
                    ctrl->start_frame         = atoi(buf);
                    break;
			    case FRAME_NUM:
                    ctrl->frame_num      	  = atoi(buf);
				    break;
                case MB_IN_SLICE:
                    ctrl->mb_in_slice 	      = atoi(buf);
                    break;
			    case LUMA_QP:
                    ctrl->luma_qp    	      = atoi(buf);
                    break;
                case LF_CFG:
                    ctrl->config_loop_filter  = atoi(buf);
                    break;
                case LF_DIS_IDC:
                    ctrl->dis_loop_filter_idc = atoi(buf);
                    break;
                case LF_ALPHA:
                    ctrl->alpha_c0_offset_div2= atoi(buf);
                    break;
                case LF_BETA:
                    ctrl->beta_offset_div2    = atoi(buf);
                    break;
                case PTN_MODE:
                    sscanf(buf, "%x", &ctrl->ptn_mode);
                    break;
                case START_PTN_FRAME_NO:
                    ctrl->start_ptn_frame_no  = atoi(buf);
                    break;            
                case END_PTN_FRAME_NO:
                    ctrl->end_ptn_frame_no    = atoi(buf);
                    break;
                case LOG_MODE:
                    ctrl->log_mode            = atoi(buf);
                    break;
                case START_LOG_FRAME_NO:
                    ctrl->start_log_frame_no  = atoi(buf);
                    break;
                case END_LOG_FRAME_NO:
                    ctrl->end_log_frame_no    = atoi(buf);
                    break;
                case DEBUG_MODE:
					sscanf( buf, "%x", &ctrl->debug_mode);
                    break;
                case START_DEBUG_FRAME_NO:
                    ctrl->start_debug_frame_no= atoi(buf);
                    break;
                case END_DEBUG_FRAME_NO:
                    ctrl->end_debug_frame_no  = atoi(buf);
					break;
                case START_DEBUG_MB_NO:
                    ctrl->start_debug_mb_no   = atoi(buf);
                    break;
                case OUTPUT_DEBUG_MODE:
                    sscanf( buf, "%x", &ctrl->output_debug_mode);
					break;
                case OUTPUT_START_DEBUG_FRAME_NO:
                    ctrl->output_start_debug_frame_no = atoi(buf);
					break;
                case OUTPUT_END_DEBUG_FRAME_NO:
                    ctrl->output_end_debug_frame_no   = atoi(buf);
					break;
                case OUTPUT_START_DEBUG_MB_NO:
                    ctrl->output_start_debug_mb_no    = atoi(buf);
                    break;
                case INTRA_REFRESH:
                    ctrl->intra_period        = atoi(buf);
					break;
                case LOGMAXFNO:
                    ctrl->log2_max_fno        = atoi(buf);
                    break;
                case INTERVAL_IPCM:
                    ctrl->interval_ipcm       = atoi(buf);
                    break;
                default:
                    next_param               = 0;
                    break;
            }
            if(next_param) i++;
        }
    }
    
    if ((ctrl->src_fp = fopen(ctrl->src_file, "rb")) == NULL)
    {
        panic("Cannot open the source video.");
        status = MMES_ERROR;
    }

    if ((ctrl->bit_fp = fopen(ctrl->bs_file, "wb")) == NULL)
    {
        panic("Cannot open the target bitstream file.");
        status = MMES_ERROR;
    }

#if SAVE_RECONSTRUCTED_FRAME
    if ((ctrl->rec_fp = fopen(ctrl->rec_file, "wb")) == NULL)
    {
        panic("Cannot open a file for reconstructed frame.");
        status = MMES_ERROR;
    }
#endif

    return status;

}

xint destroy_config(enc_cfg *ctrl)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Jul/??/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   Destroy the configuration file                              */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    if (ctrl->cfg_fp)     fclose(ctrl->cfg_fp),     ctrl->cfg_fp     = NULL;
    if (ctrl->src_fp)     fclose(ctrl->src_fp),     ctrl->src_fp     = NULL;
    if (ctrl->rec_fp)     fclose(ctrl->rec_fp),     ctrl->rec_fp     = NULL;
    if (ctrl->bit_fp)     fclose(ctrl->bit_fp),     ctrl->bit_fp     = NULL;

    return MMES_NO_ERROR;
}


/* ================================================================== */
/*                           Main program                             */
/* ================================================================== */
int
main(int argc, char **argv)
{
    xint        frame_size;
    xint        width, height;
    xint        frame_no;
    uint        nbytes, nbits;
    xint        status;
    uint8       *cur_frame_y;
    enc_cfg     e_ctrl;
    h264enc_obj *pEnc= &h264_enc;

    e_ctrl.cfg_fp = e_ctrl.src_fp = e_ctrl.rec_fp = e_ctrl.bit_fp = NULL;

#if __CC_ARM
    if ((e_ctrl.cfg_fp = fopen("../../../../../temp/config_qcif.ini", "r")) == NULL)
    {
        panic("Cannot open the config file.");
        goto free_codec;
    }
#else
	if( argc != 2 && argc < 4 )
	{
		panic("Usage: h264_enc.exe [configuration file] [-p MB=n] [-p bw_lite]");
        goto free_codec;
	}

    if(argc >= 4)
    {
        // @jerry for parsing extra setting
        xint arg_c = 2, h264_pattern_start_mb;
        do
        {
            if(!strcmp(argv[arg_c++], "-p"))
            {
                if(!strncmp(argv[arg_c], "MB=", sizeof(uint8)*3))
                    sscanf(argv[arg_c++]+3, "%d", &h264_pattern_start_mb);
                else if(!strcmp(argv[arg_c], "bw_lite"))
                {
                    h264_bw_lite = 1;
                    arg_c++;
                }
            }
        }while(arg_c < argc);
    }

    if ((e_ctrl.cfg_fp = fopen(argv[1], "r")) == NULL)
    {
        panic("Cannot open the config file.");
        goto free_codec;
    }
#endif

    if(init_config(&e_ctrl) == MMES_ERROR)
        goto free_codec;
    
    width      = e_ctrl.frame_width;
    height     = e_ctrl.frame_height;
    frame_size = (width * height * 3) >> 1;

    /* allocate some space for the output bitstream,  */
    /* the size should be smaller than the frame_size */
    bit_buffer = (uint8 *) malloc(frame_size * sizeof(uint8));

    // @jerry for calculate psnr
    cur_frame_y= (uint8 *) malloc(frame_size * sizeof(uint8));

    /* ================================================================== */
    /*                        Initialize codec                            */
    /* ================================================================== */
    h264_init_encoder(&h264_enc, &e_ctrl, width, height);

    timer_count = GetTimerCount();

    /* write bitstream header */
    /* initialize bitstream output buffer */
    h264_init_bitstream(&h264_enc, bit_buffer, (frame_size<<3));
    h264_enc.QP = e_ctrl.luma_qp;
    nbits = h264_encode_video_header(&h264_enc);
    nbytes = (nbits >> 3) + (xint)(nbits % 8 != 0);

    if ((fwrite(bit_buffer, sizeof(uint8), nbytes, e_ctrl.bit_fp) != nbytes))
    {
        panic("Bitstream header write error.");
        goto free_codec;
    }

    if(fseek(e_ctrl.src_fp, e_ctrl.start_frame*frame_size, SEEK_SET) != 0)
    {
        panic("can't seek the specified starting frame.");
        goto free_codec;
    }

    /* encoding loop */
    for (frame_no = 0; frame_no < e_ctrl.frame_num; frame_no++)
    {
        h264_enc.cur_frame_no = frame_no; 
        h264_enc.cur_mb_no    = 0;
        h264_enc.cur_slice_no = 0;

        /* initialize frame */
        status = read_yuvframe(pEnc->curf, frame_no,
            width, height, e_ctrl.src_fp);

        memcpy(cur_frame_y, pEnc->curf->y, sizeof(uint8)*frame_size);

        if (status != MMES_NO_ERROR)
        {
            panic("Call to read_yuvdata() caused error.");
            goto free_codec;
        }

        nbits = encode_one_frame(&h264_enc, &e_ctrl, bit_buffer, frame_size, cur_frame_y);

        if(nbits == MMES_ERROR)
        {
            panic("Something is wrong during encoding."); 
            goto free_codec;
        }

        nbytes = (nbits >> 3) + ((nbits % 8) != 0);

        /* write reconstruct frame */
        if (e_ctrl.rec_fp != NULL)
        {
            fwrite(h264_enc.reff[0]->y,  width*height,   1, e_ctrl.rec_fp);
            fwrite(h264_enc.reff[0]->cb, width*height/4, 1, e_ctrl.rec_fp);
            fwrite(h264_enc.reff[0]->cr, width*height/4, 1, e_ctrl.rec_fp);
        }

        /* write compressed bitstream */
        if ((fwrite(bit_buffer, sizeof(uint8), nbytes, e_ctrl.bit_fp) != nbytes))
        {
            panic("bitstream write error.");
            goto free_codec;
        }
    }

    timer_count = GetTimerCount() - timer_count;
    printf("\n--------Statistics--------\n");
    printf("Encoded %d frames in %d msec.\n", frame_no, timer_count);
    printf("Encoding frame rate = %.1f fps\n", (float) frame_no * 1000.0 / timer_count);
    printf("Average bitrate     = %d kbps.\n", (int) floor((total_size*frame_rate)/(frame_no*1000)));
    printf("Average PSNR        = %5.2f dB.\n", total_psnr/frame_no);

    /* ================================================================== */
    /*      Free codec                                                    */
    /* ================================================================== */
free_codec:
    h264_free_encoder(&h264_enc);
    destroy_config(&e_ctrl);

    if (bit_buffer) free(bit_buffer), bit_buffer = NULL;

    return MMES_NO_ERROR;
}
