#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string.h>

#include "h264dec_api.h"
#include "misc_util.h"
#include "nal.h"

#define WIDTH       352
#define HEIGHT      288
#define ENC_FRAME   1
#define QP_LUMA		28
#define QP_CHROMA	28
#define QCOEFF_FILE "pframe_qcoeff.dat"
//#define DECODE_P_FRAME

h264dec_obj h264_dec;
h264dec_obj *pDec = &h264_dec;
extern uint32 prefix_mask_rev[33];
extern uint32 prefix_mask[33];
extern xint me_downsample_frame(frame_obj *pFrame, int width, int height);
xint decoded_frame_num = 0;

// @jerry for bw_lite
xint h264_bw_lite = 0;

void print_help()
{
    fprintf(stdout, "usage: h264_dec [-h] [-i bitstream] [-o decoded_frame] [-p ptn_mode start end] [-log log_mode start end]\n");
    fprintf(stdout, "-h : help\n");
    fprintf(stdout, "-i : Input File\n");
    fprintf(stdout, "-o : Output File\n");
    fprintf(stdout, "-p : Pattern Mode\n");
    fprintf(stdout, "   0x00 -- None\n");
    fprintf(stdout, "   0x01 -- INTRAPRED\n");
    fprintf(stdout, "   0x02 -- CAVLC\n");
    fprintf(stdout, "   0x04 -- DEBLOCK\n");
    fprintf(stdout, "   0x08 -- RECONSTRUCTED FRAME\n");
    fprintf(stdout, "   0x10 -- IRA MODE\n");
    fprintf(stdout, "   0x40 -- REF LIST\n");
    fprintf(stdout, "-log : Log Mode\n");
    fprintf(stdout, "   0x00 -- None\n");
    fprintf(stdout, "   0x01 -- Bitstream LOG\n");
    fprintf(stdout, "-fn : Frame Num\n");
}

xint 
main(int argc, char *argv[])
{
    FILE    *fp_bit = NULL, *fp_yuvout = NULL;
    xint    EndOFFrame = 1;
    xint    f_init_decoder = 1;
    
#if __CC_ARM
    if((fp_bit = fopen("../../../../../temp/foreman_26.264", "rb")) == NULL)
    {
        fprintf(stderr, "Cannot open bit_file !\n");
        goto free_all_memory;
    }
    if((fp_yuvout = fopen("../../../../../temp/output.yuv", "wb")) == NULL)
    {
        fprintf(stderr, "Cannot open decode file !\n");
        goto free_all_memory;
    }
#else
    xint    idx;
    xint    start_ptn_frame_no, end_ptn_frame_no;
    xint    start_log_frame_no, end_log_frame_no;
    xint    h264_pattern_options, h264_pattern, h264_log_options, h264_log_mode;

    for(idx = 1 ; idx < argc ; idx++)
    {
        if(!strcmp(argv[idx], "-h"))
        {
            print_help();
            return 0;
        }
        else if(!strcmp(argv[idx], "-i"))
        {
            if((fp_bit = fopen(argv[idx+1], "rb")) == NULL)
            {
                fprintf(stderr, "Cannot open bit_file !\n");
                goto free_all_memory;
            }
            idx++;
        }
        else if(!strcmp(argv[idx], "-o"))
        {
            if((fp_yuvout = fopen(argv[idx+1], "wb")) == NULL)
            {
                fprintf(stderr, "Cannot open decode file !\n");
                goto free_all_memory;
            }
            idx++;
        }
        else if(!strcmp(argv[idx], "-p"))
        {
            sscanf(argv[++idx], "%x", &h264_pattern_options);
            sscanf(argv[++idx], "%d", &start_ptn_frame_no);
            sscanf(argv[++idx], "%d", &end_ptn_frame_no);

            h264_pattern = h264_pattern_options;
        }
        else if(!strcmp(argv[idx], "-log"))
        {
            sscanf(argv[++idx], "%x", &h264_log_options);
            sscanf(argv[++idx], "%d", &start_log_frame_no);
            sscanf(argv[++idx], "%d", &end_log_frame_no);

            h264_log_mode = h264_log_options;
        }
        else if(!strcmp(argv[idx], "-fn"))
        {
            sscanf(argv[++idx], "%d", &decoded_frame_num);
        }
    }
#endif

    if(!fp_bit || !fp_yuvout)
    {
        if(!fp_bit)    fprintf(stderr, "Error : Input file must be provided.\n");
        if(!fp_yuvout) fprintf(stderr, "Error : Output file must be provided.\n");
        goto free_all_memory;
    }

    init_nalu(&pDec->nal);

    pDec->frame_no = 0;

    while(!feof(fp_bit))
    {
        /* decode h264 header to retrieve decoding information */
        if(h264_decode_header(pDec, fp_bit) != MMES_NO_ERROR)
        {
            fprintf(stderr, "Decode h264 header error!\n");
            goto free_all_memory;
        }

        /* ================================================================== */
        /*                        Initialize codec                            */
        /* ================================================================== */
        if (h264_init_decoder(pDec, f_init_decoder) != MMES_NO_ERROR)
        {
            fprintf(stderr, "Decoder initial error!\n");
            goto free_all_memory;
        }

        /* ================================================================== */
        /*                        Parse Slice Data                            */
        /* ================================================================== */
        parse_slice_data(pDec, &EndOFFrame);

        f_init_decoder = 0;
        if(EndOFFrame)
        {
            uint8  frame_type = IS_ISLICE(pDec->curr_slice->type) ? 'I' : 'P';

            write_frame(pDec->recf, pDec->width, pDec->height, 
                pDec->crop_left, pDec->crop_right, pDec->crop_top, pDec->crop_bottom,
                pDec->chroma_format_idc, fp_yuvout);
            
            adjust_ref_lists(pDec);
            
            pDec->cur_slice_no = pDec->cur_mb_no = 0;

            printf( "<%c> [%3d]\n", frame_type, pDec->frame_no );

            pDec->frame_no++;
        }
        if(decoded_frame_num > 0 && pDec->frame_no >= decoded_frame_num)
            break;
    }

    fprintf(stdout, "\nDecoding Info\n");
    fprintf(stdout, "---------------------\n");
    fprintf(stdout, "Decoded frame num : %d\n", pDec->frame_no);
    fprintf(stdout, "Frame Size : %dx%d\n", pDec->width, pDec->height);

    destroy_nalu(&pDec->nal);
    h264_free_decoder(pDec);
    
free_all_memory:

    if (fp_bit) fclose(fp_bit), fp_bit = NULL;
    if (fp_yuvout) fclose(fp_yuvout), fp_yuvout = NULL;
    if (pDec->fp_qcoeff) fclose(pDec->fp_qcoeff), pDec->fp_qcoeff = NULL;// @chingho 06 27 2005

    return MMES_NO_ERROR;
}