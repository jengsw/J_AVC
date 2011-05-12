/* ///////////////////////////////////////////////////////////// */
/*   File: rate_control.c                                        */
/*   Author: Jerry Peng                                          */
/*   Date: Jun/03/2006                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 Rate Control Module.             */
/*                                                               */
/*   Copyright, 2006.                                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */
#include <string.h>
#include "../../common/inc/rate_control.h"

#define RC_WINDOW_SIZE      16
#define INITIAL_QP          28
#define ALPHA               0.5
#define ALPHA_W             8
#define RC_RDMODEL_WIN_SIZE 16
#define REFINE_QP(QP)       ((QP<0) ? 0 : (QP>51) ? 51 : QP);
#define REFINE_PERCENT(P)   ((P<0.5) ? 0.5 : (P>1.5) ? 1.5 : P);
#define IP_QP_DIFF			5
#define I_OVER_P_SIZE		8

float q_step[52] =
{
    0.6250, 0.6875,      0.8125,     0.8750,
    1.0,    1.0*(1.125), 1.0*(1.25), 1.0*(1.375), 1.0*(1.625), 1.0*(1.75), 2.0, 2.25,
    2.5,    2.5*(1.125), 2.5*(1.25), 2.5*(1.375), 2.5*(1.625), 2.5*(1.75),
    5.0,    5.0*(1.125), 5.0*(1.25), 5.0*(1.375), 5.0*(1.625), 5.0*(1.75),
    10.0,   10 *(1.125), 10 *(1.25), 10 *(1.375), 10 *(1.625), 10 *(1.75),
    20.0,   20 *(1.125), 20 *(1.25), 20 *(1.375), 20 *(1.625), 20 *(1.75),
    40.0,   40 *(1.125), 40 *(1.25), 40 *(1.375), 40 *(1.625), 40 *(1.75),
    80.0,   80 *(1.125), 80 *(1.25), 80 *(1.375), 80 *(1.625), 80 *(1.75),
    160.0,  160*(1.125), 160*(1.25), 160*(1.375)
};

float i_coeff_hist[RC_RDMODEL_WIN_SIZE];
float p_coeff_hist[RC_RDMODEL_WIN_SIZE];

xint i_num = 0;
xint p_num = 0;

xint rc_init(h264enc_obj *pEnc, xint target_rate)
{
    rc_obj *pRC = &pEnc->pRC;

    pRC->frame_rate = FRAME_RATE;
    pRC->target_rate = target_rate;
    pRC->target_frame_size = (xint)(target_rate / pRC->frame_rate);
    pRC->avg_frame_size = (xint)(target_rate / pRC->frame_rate);

    /* uses a sliding frame window */
    pRC->buffer_size = pRC->avg_frame_size * RC_WINDOW_SIZE;
    pRC->buffer_size -= (xint)(pRC->buffer_size * ALPHA);

    return INITIAL_QP;
}

xint rc_update_buf(rc_obj *pRC, xint size)
{
    pRC->buffer_size -= size;
    pRC->buffer_size += pRC->target_frame_size;

    /* maintain 50% of full buffer */
    pRC->avg_frame_size = pRC->buffer_size + pRC->target_frame_size + (int32)((ALPHA-1)*pRC->target_frame_size*RC_WINDOW_SIZE);

    if((pRC->buffer_size/ALPHA_W) > pRC->avg_frame_size)
        pRC->avg_frame_size = pRC->buffer_size/ALPHA_W;

    if(pRC->avg_frame_size < 0)
    {
        pRC->avg_frame_size = 2;
    }

    return MMES_NO_ERROR;
}

xint rc_get_frame_QP(rc_obj *pRC, xint type)
{
    float coeff;
    xint frame_QP;

    if(type == I_SLICE || type == ALL_I_SLICE)
    {
        if(i_num == 0)
        {
            frame_QP = INITIAL_QP;
        }
        else
        {
			// give I frame I_OVER_P_SIZE times of frame size larger than P frame
			pRC->avg_frame_size *= I_OVER_P_SIZE;
            coeff = rc_get_i_coeff(pRC);
            frame_QP = (xint)((float)pRC->i_cpx / ((float)(pRC->avg_frame_size)*coeff));
            frame_QP = rc_constrain_i_qp(pRC, pRC->avg_frame_size, frame_QP);
        }

        /* reset i_cpx for next I frame */
        pRC->i_cpx = 0;
    }
    else
    {
        if(p_num == 0)
        {
            frame_QP = INITIAL_QP;
        }
        else
        {
            coeff = rc_get_p_coeff(pRC);
            frame_QP = (xint)((float)pRC->p_cpx / ((float)(pRC->avg_frame_size)*coeff));
            frame_QP = rc_constrain_p_qp(pRC, pRC->avg_frame_size, frame_QP);
        }

        /* reset p_cpx for next P frame */
        pRC->p_cpx = 0;
    }



    return frame_QP;
}

xint rc_update_data(rc_obj *pRC, xint type, xint qp, xint size)
{
    int cpx;
    if(type == I_SLICE || type == ALL_I_SLICE)
    {
        cpx = pRC->i_cpx;
        if(i_num < RC_RDMODEL_WIN_SIZE)
        {
            i_coeff_hist[i_num] = (float) cpx / ((float)size * (float)qp);
            i_num++;
        }
        else
        {
            memmove(i_coeff_hist, &i_coeff_hist[1], sizeof(float)*(RC_RDMODEL_WIN_SIZE-1));
            i_coeff_hist[RC_RDMODEL_WIN_SIZE-1] = (float) cpx / ((float)size * (float)qp);
        }

        pRC->i_qp = qp;
        pRC->i_size = size;
    }
    else if(type == P_SLICE)
    {
        cpx = pRC->p_cpx;
        if(p_num < RC_RDMODEL_WIN_SIZE)
        {
            p_coeff_hist[p_num] = (float) cpx / ((float)size * (float)qp);
            p_num++;
        }
        else
        {
            memmove(p_coeff_hist, &p_coeff_hist[1], sizeof(float)*(RC_RDMODEL_WIN_SIZE-1));
            p_coeff_hist[RC_RDMODEL_WIN_SIZE-1] = (float) cpx / ((float)size * (float)qp);
        }

        pRC->p_qp = qp;
        pRC->p_size = size;
    }
    return MMES_NO_ERROR;
}

float rc_get_i_coeff(rc_obj *pRC)
{
    float sum = 0;
    xint idx;
    for(idx=0; idx<i_num; idx++)
    {
        sum += i_coeff_hist[idx];
    }

    return sum/i_num;
}

float rc_get_p_coeff(rc_obj *pRC)
{
    float sum = 0;
    xint idx;
    for(idx=0; idx<p_num; idx++)
    {
        sum += p_coeff_hist[idx];
    }

    return sum/p_num;
}

xint rc_search_step(float step)
{
    xint qp;

    for(qp=0; qp<52; qp++)
    {
        if(q_step[qp]>step)
            break;
    }

    return qp;
}

xint rc_constrain_i_qp(rc_obj *pRC, xint target, xint frame_QP)
{
    xint qp;
    float percent, step_size;

    percent = (float)(target)/(float)(pRC->i_size);
    percent = (float)REFINE_PERCENT(percent);
    step_size = q_step[pRC->i_qp]/percent;
    qp = rc_search_step(step_size);

    if(percent >= 1)    // decrease QP
    {
        if(frame_QP <= qp)
        {
            qp = qp;
        }
        else if(frame_QP > pRC->i_qp)
        {
            qp = pRC->i_qp;
        }
        else
        {
            qp = frame_QP;
        }
    }
    else                // increase QP
    {
        if(frame_QP >= qp)
        {
            qp = qp;
        }
        else if(frame_QP < pRC->i_qp)
        {
            qp = pRC->i_qp;
        }
        else
        {
            qp = frame_QP;
        }
    }

	/* constraint QP difference between I & P frames */
	if     (qp - pRC->p_qp > IP_QP_DIFF)		qp = pRC->p_qp + IP_QP_DIFF;
	else if(pRC->p_qp - qp > IP_QP_DIFF)		qp = pRC->p_qp - IP_QP_DIFF;

    qp = REFINE_QP(qp);
    return qp;
}

xint rc_constrain_p_qp(rc_obj *pRC, xint target, xint frame_QP)
{
    xint qp;
    float percent, step_size;

    percent = (float)(target)/(float)(pRC->p_size);
    percent = (float)REFINE_PERCENT(percent);
    step_size = q_step[pRC->p_qp]/percent;
    qp = rc_search_step(step_size);

    if(percent >= 1)    // decrease QP
    {
        if(frame_QP <= qp)
        {
            qp = qp;
        }
        else if(frame_QP > pRC->p_qp)
        {
            qp = pRC->p_qp;
        }
        else
        {
            qp = frame_QP;
        }
    }
    else                // increase QP
    {
        if(frame_QP >= qp)
        {
            qp = qp;
        }
        else if(frame_QP < pRC->p_qp)
        {
            qp = pRC->p_qp;
        }
        else
        {
            qp = frame_QP;
        }
    }

    qp = REFINE_QP(qp);
    return qp;
}
