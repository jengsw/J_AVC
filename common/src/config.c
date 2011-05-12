int init_config(enc_cfg *ctrl)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
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
    int        j;
    char        buf[256];
    int        next_param = 1;
    int        status = MMES_NO_ERROR;

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

int destroy_config(enc_cfg *ctrl)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
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