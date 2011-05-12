/* ///////////////////////////////////////////////////////////// */
/*   File: mv_pred.c                                             */
/*   Author: Jerry Peng                                          */
/*   Date: Aug/23/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 Motion Vector Prediction Module. */
/*                                                               */
/*   Copyright, 2005.                                            */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */
#include <string.h>
#include "../../common/inc/mv_pred.h"

#define DEBUG_MB_NUM 152

/* prediction A, B, C, D */
/*  08 25 2005   */
pred_obj pred_A, pred_B, pred_C, pred_D, pred_X;

xint mb_part_16x8_idx [] = { 0, 8 };
xint mb_part_8x16_idx [] = { 0, 4 };

xint mb_8x16_raster_scan_idx [] = { 0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15 };
xint mb_P8x8_raster_scan_idx [] = { 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15 };
xint mb_upleft_blk_idx [] = { 1, 3, 5, 7 };


xint mv_pred_inter( slice_obj *pSlice, backup_obj *pBackup, xint width, xint encode )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date: Aug/23/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Motion vector predition                                     */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/* ------------------------------------------------------------- */
{	
//	slice_obj *pSlice = pEnc->curr_slice;
  //  backup_obj *pBackup = pEnc->backup;
	mb_obj *pMB = &(pSlice->cmb);

	width = width;

	/* backup information */
	mv_pred_backup( pBackup, pSlice, width );

	switch( pMB->best_Inter_mode )
	{
	case INTER_16x16:
		mv_pred_16x16( pSlice, width );
		break;
	case INTER_16x8:
		mv_pred_16x8( pSlice, width, encode );
		break;
	case INTER_8x16:
		mv_pred_8x16( pSlice, width, encode );
		break;
	case INTER_P8x8:
		mv_pred_P8x8( pSlice, width, encode );
		break;
	case INTER_PSKIP:
		mv_pred_PSKIP( pSlice, width );
		break;
	default:
		break;
	}
	
	//  08 25 2005
	/* restore information */
	mv_pred_restore( pBackup, pSlice, width );

	return MMES_NO_ERROR;
}

xint mv_pred_backup( backup_obj *pBackup, slice_obj *pSlice, int width )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/25/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Backup current top_valid, left_valid, and upleft_valid to   */
/*   bup2.                                                       */
/*   The initial status before coding this MB is already stored  */
/*   in bup.                                                     */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	// backup top_valid, left_valid, and upleft_valid
	memcpy( pBackup->top_valid_bup2,	pSlice->top_valid,	  (width/B_SIZE)*sizeof(uint8)   );
	memcpy( pBackup->left_valid_bup2,   pSlice->left_valid,   (MB_SIZE/B_SIZE)*sizeof(uint8) );
	memcpy( pBackup->upleft_valid_bup2, pSlice->upleft_valid, (MB_SIZE/B_SIZE)*sizeof(uint8) );

	// restore initial status (top_valid, left_valid, and upleft_valid)
	memcpy( pSlice->top_valid,    pBackup->top_valid_bup,	 (width/B_SIZE)*sizeof(uint8)   );
	memcpy( pSlice->left_valid,	  pBackup->left_valid_bup,   (MB_SIZE/B_SIZE)*sizeof(uint8) );
	memcpy( pSlice->upleft_valid, pBackup->upleft_valid_bup, (MB_SIZE/B_SIZE)*sizeof(uint8) );

	return MMES_NO_ERROR;
}

xint mv_pred_restore( backup_obj *pBackup, slice_obj *pSlice, int width )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/25/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	// restore status to next operation (top_valid, left_valid, and upleft_valid)
	memcpy( pSlice->top_valid,    pBackup->top_valid_bup2,	  (width/B_SIZE)*sizeof(uint8)   );
	memcpy( pSlice->left_valid,	  pBackup->left_valid_bup2,   (MB_SIZE/B_SIZE)*sizeof(uint8) );
	memcpy( pSlice->upleft_valid, pBackup->upleft_valid_bup2, (MB_SIZE/B_SIZE)*sizeof(uint8) );

	// restore initial status(before coding current MB)
	memcpy( pSlice->top_ref_idx,	pBackup->top_ref_idx_bup,	((width/B_SIZE)/2)*sizeof(xint)   );//  08 26 2005
	memcpy( pSlice->left_ref_idx,	pBackup->left_ref_idx_bup,	((MB_SIZE/B_SIZE)/2)*sizeof(xint) );//  08 26 2005

	memcpy( pSlice->top_mvx,	    pBackup->top_mvx_bup,	    (width/B_SIZE)*sizeof(xint)       );//  08 26 2005
	memcpy( pSlice->left_mvx,       pBackup->left_mvx_bup,      (MB_SIZE/B_SIZE)*sizeof(xint)     );//  08 26 2005
	memcpy( pSlice->upleft_mvx,     pBackup->upleft_mvx_bup,    (MB_SIZE/B_SIZE)*sizeof(xint)     );//  08 26 2005

	memcpy( pSlice->top_mvy,	    pBackup->top_mvy_bup,	    (width/B_SIZE)*sizeof(xint)       );//  08 26 2005
	memcpy( pSlice->left_mvy,       pBackup->left_mvy_bup,      (MB_SIZE/B_SIZE)*sizeof(xint)     );//  08 26 2005
	memcpy( pSlice->upleft_mvy,     pBackup->upleft_mvy_bup,    (MB_SIZE/B_SIZE)*sizeof(xint)     );//  08 26 2005

	return MMES_NO_ERROR;
}

xint mv_pred_16x16( slice_obj *pSlice, xint width )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/25/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Motion vector predition for Inter16x16                      */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint predictor_x, predictor_y;
	xint x_coord;

	/* set pred_X */
	pred_X.valid = 1;
	pred_X.ref_idx = pMB->ref_idx[0];
	pred_X.x = pMB->mv.x[0];
	pred_X.y = pMB->mv.y[0];

	/* get neighboring block */
	mv_pred_get_neighbor( pSlice, width, 0, 0 );
	
	mv_pred_get_prd_mv( pSlice, 0, &predictor_x, &predictor_y );

	for( x_coord=0; x_coord<NBLOCKS; x_coord++ )
	{
		pMB->pred_mv.x[x_coord] = predictor_x;
		pMB->pred_mv.y[x_coord] = predictor_y;
	}

	return MMES_NO_ERROR;
}

xint mv_pred_16x8( slice_obj *pSlice, xint width, xint encode )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/25/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Motion vector predition for Inter16x8                       */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	mb_obj    *pMB = &(pSlice->cmb);	
	xint predictor_x, predictor_y;
	xint x_coord, part;

	for( part=0; part<2; part++ )
	{
		/* set pred_X */
		pred_X.valid = 1;
		pred_X.ref_idx = pMB->ref_idx[ part << 1];
		pred_X.x = pMB->mv.x[ mb_part_16x8_idx[part] ];
		pred_X.y = pMB->mv.y[ mb_part_16x8_idx[part] ];

		/* get neighboring block */
		mv_pred_get_neighbor( pSlice, width, part, 0 );
		mv_pred_get_prd_mv( pSlice, part, &predictor_x, &predictor_y );

		for( x_coord=0; x_coord<8; x_coord++ )
		{
			pMB->pred_mv.x[(x_coord+mb_part_16x8_idx[part])] = predictor_x;
			pMB->pred_mv.y[(x_coord+mb_part_16x8_idx[part])] = predictor_y;

			// if decode
			if( encode == 0 )
			{
				pMB->mv.x[(x_coord+mb_part_16x8_idx[part])] += predictor_x;
				pMB->mv.y[(x_coord+mb_part_16x8_idx[part])] += predictor_y;
			}
		}
	}
	return MMES_NO_ERROR;
}

xint mv_pred_8x16( slice_obj *pSlice, xint width, xint encode )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/26/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Motion vector predition for Inter8x16                       */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint predictor_x, predictor_y;
	xint x_coord, part;

	for( part=0; part<2; part++ )
	{
		/* set pred_X */
		pred_X.valid = 1;
		pred_X.ref_idx = pMB->ref_idx[ part ];
		pred_X.x = pMB->mv.x[ (part<<1) ];
		pred_X.y = pMB->mv.y[ (part<<1) ];

		/* get neighboring block */
		mv_pred_get_neighbor( pSlice, width, part, 0 );
		mv_pred_get_prd_mv( pSlice, part, &predictor_x, &predictor_y );

		for( x_coord=0; x_coord<8; x_coord++ )
		{
			pMB->pred_mv.x[mb_8x16_raster_scan_idx[(part<<3)+x_coord]] = predictor_x;
			pMB->pred_mv.y[mb_8x16_raster_scan_idx[(part<<3)+x_coord]] = predictor_y;

			// if decode
			if( encode == 0 )
			{
				pMB->mv.x[mb_8x16_raster_scan_idx[(part<<3)+x_coord]] += predictor_x;
				pMB->mv.y[mb_8x16_raster_scan_idx[(part<<3)+x_coord]] += predictor_y;
			}
		}
	}
	return MMES_NO_ERROR;
}

xint mv_pred_P8x8( slice_obj *pSlice, xint width, xint encode )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/26/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Motion vector predition for InterP8x8                       */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Aug/30/2005 Chingho-Ho Chen, add prediction for 8x4, 4x8,   */
/*               4x4 block.                                      */
/* ------------------------------------------------------------- */
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint predictor_x, predictor_y;
	xint part, part_mode, sub_part_num, sub_part;

	for( part=0; part<4; part++ )
	{
		part_mode = pMB->best_8x8_blk_mode[part];
		sub_part_num = (part_mode==INTER_8x8) ? 1 : (part_mode==INTER_4x4) ? 4 : 2;

		for( sub_part=0; sub_part<sub_part_num; sub_part++ )
		{
			/* set pred_X */
			mv_pred_set_pred_X( pSlice, width, part, sub_part );

			/* get neighboring block */
			mv_pred_get_neighbor( pSlice, width, part, sub_part );
			mv_pred_get_prd_mv( pSlice, part, &predictor_x, &predictor_y );
			mv_pred_update_P8x8( pSlice, width, part, sub_part, predictor_x, predictor_y, encode );
		}
	}

	return MMES_NO_ERROR;
}

xint mv_pred_PSKIP( slice_obj *pSlice, xint width )
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint predictor_x, predictor_y;
	xint x_coord;

	mv_pred_get_neighbor( pSlice, width, 0, 0 );

	if( !pred_A.valid || !pred_B.valid )
	{
		predictor_x = 0;
		predictor_y = 0;
	}
	else if( pred_A.ref_idx == 0 && pred_A.x == 0 && pred_A.y == 0 )
	{
		predictor_x = 0;
		predictor_y = 0;
	}
	else if( pred_B.ref_idx == 0 && pred_B.x == 0 && pred_B.y == 0 )
	{
		predictor_x = 0;
		predictor_y = 0;
	}
	else
	{
		/* set pred_X */
		pred_X.valid = 1;
		pred_X.ref_idx = 0;
		pred_X.x = pMB->mv.x[0];
		pred_X.y = pMB->mv.y[0];


		mv_pred_get_prd_mv( pSlice, 0, &predictor_x, &predictor_y );
	}

	for( x_coord=0; x_coord<NBLOCKS; x_coord++ )
	{
		pMB->pred_mv.x[x_coord] = predictor_x;
		pMB->pred_mv.y[x_coord] = predictor_y;
	}	
	return MMES_NO_ERROR;
}

xint mv_pred_get_neighbor( slice_obj *pSlice, xint width, xint part, xint sub_part )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/25/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Get neighboring blocks for motion vector prediction.        */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Aug/30/2005 Chingho-Ho Chen, add description for 8x4, 4x8,  */
/*               4x4 block.                                      */
/* ------------------------------------------------------------- */
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint nx_mb, block;
	xint frame_x_16_offset, frame_x_4_offset;
	xint mb_x_4_offset, mb_y_4_offset;
	xint part_mode;
	
	nx_mb = width >> 4;
	frame_x_16_offset = pMB->id % nx_mb;
	frame_x_4_offset = frame_x_16_offset<<2;

	switch( pMB->best_Inter_mode )
	{
	case INTER_PSKIP:
	case INTER_16x16:
		// pred_A
		pred_A.valid = pSlice->left_valid[0];
		if( !pred_A.valid )
			mv_pred_set_invalid( &pred_A );
		else
		{
			pred_A.ref_idx = pSlice->left_ref_idx[0];
			pred_A.x = pSlice->left_mvx[0];
			pred_A.y = pSlice->left_mvy[0];
		}

		// pred_B
		pred_B.valid = pSlice->top_valid[frame_x_4_offset];
		if( !pred_B.valid )
			mv_pred_set_invalid( &pred_B );
		else
		{
			pred_B.ref_idx = pSlice->top_ref_idx[frame_x_4_offset>>1];
			pred_B.x = pSlice->top_mvx[frame_x_4_offset];
			pred_B.y = pSlice->top_mvy[frame_x_4_offset];
		}

		// pred_C
		if( ((frame_x_4_offset+(MB_SIZE/B_SIZE))<<2) >= width )
			mv_pred_set_invalid( &pred_C );
		else
		{
			pred_C.valid = pSlice->top_valid[frame_x_4_offset+(MB_SIZE/B_SIZE)];
			if( !pred_C.valid )
				mv_pred_set_invalid( &pred_C );
			else
			{
				pred_C.ref_idx = pSlice->top_ref_idx[((frame_x_4_offset+(MB_SIZE/B_SIZE))>>1)];
				pred_C.x = pSlice->top_mvx[frame_x_4_offset+(MB_SIZE/B_SIZE)];
				pred_C.y = pSlice->top_mvy[frame_x_4_offset+(MB_SIZE/B_SIZE)];
			}
		}
		
		// pred_D
		if( ((frame_x_4_offset-1)<<2) < 0 )
			mv_pred_set_invalid( &pred_D );
		else
		{
			pred_D.valid = pSlice->upleft_valid[0];
			if( !pred_D.valid )
				mv_pred_set_invalid( &pred_D );
			else
			{				
				pred_D.ref_idx = pSlice->upleft_ref_idx[0];
				pred_D.x = pSlice->upleft_mvx[0];
				pred_D.y = pSlice->upleft_mvy[0];
			}
		}
		break;
	case INTER_16x8:
		block = mb_part_16x8_idx[part];
		mb_x_4_offset = mb_x_4_idx[block];
		mb_y_4_offset = mb_y_4_idx[block];

		// pred_A
		pred_A.valid = pSlice->left_valid[mb_y_4_offset];
		if( !pred_A.valid )
			mv_pred_set_invalid( &pred_A );
		else
		{
			pred_A.ref_idx = pSlice->left_ref_idx[(mb_y_4_offset>>1)];
			pred_A.x = pSlice->left_mvx[mb_y_4_offset];
			pred_A.y = pSlice->left_mvy[mb_y_4_offset];
		}

		// pred_B
		pred_B.valid = (part==0) ? pSlice->top_valid[frame_x_4_offset] : MMES_VALID;
		if( !pred_B.valid )
			mv_pred_set_invalid( &pred_B );
		else
		{
			if( part == 0 )
			{
				pred_B.ref_idx = pSlice->top_ref_idx[frame_x_4_offset>>1];
				pred_B.x = pSlice->top_mvx[frame_x_4_offset];
				pred_B.y = pSlice->top_mvy[frame_x_4_offset];
			}
			else
			{
				pred_B.ref_idx = pMB->ref_idx[0];
				pred_B.x = pMB->mv.x[0];
				pred_B.y = pMB->mv.y[0];
			}
		}

		// pred_C
		if( ((frame_x_4_offset+(MB_SIZE/B_SIZE))<<2) >= width || part == 1 )
			mv_pred_set_invalid( &pred_C );
		else
		{
			pred_C.valid = pSlice->top_valid[frame_x_4_offset+(MB_SIZE/B_SIZE)];
			if( !pred_C.valid )
				mv_pred_set_invalid( &pred_C );
			else
			{
				pred_C.ref_idx = pSlice->top_ref_idx[((frame_x_4_offset+(MB_SIZE/B_SIZE))>>1)];
				pred_C.x = pSlice->top_mvx[frame_x_4_offset+(MB_SIZE/B_SIZE)];
				pred_C.y = pSlice->top_mvy[frame_x_4_offset+(MB_SIZE/B_SIZE)];
			}
		}
		
		// pred_D
		if( ((frame_x_4_offset-1)<<2) < 0 )
			mv_pred_set_invalid( &pred_D );
		else
		{
			pred_D.valid = pSlice->upleft_valid[mb_y_4_offset];
			if( !pred_D.valid )
				mv_pred_set_invalid( &pred_D );
			else
			{
				pred_D.ref_idx = pSlice->upleft_ref_idx[mb_y_4_offset];
				pred_D.x = pSlice->upleft_mvx[mb_y_4_offset];
				pred_D.y = pSlice->upleft_mvy[mb_y_4_offset];
			}
		}
		break;
	case INTER_8x16:		
		block = mb_part_8x16_idx[part];
		mb_x_4_offset = mb_x_4_idx[block];
		mb_y_4_offset = mb_y_4_idx[block];

		// pred_A
		pred_A.valid = (part==0) ? pSlice->left_valid[mb_y_4_offset] : MMES_VALID;
		if( !pred_A.valid )
			mv_pred_set_invalid( &pred_A );
		else
		{
			if( part == 0 )
			{
				pred_A.ref_idx = pSlice->left_ref_idx[(mb_y_4_offset>>1)];
				pred_A.x = pSlice->left_mvx[mb_y_4_offset];
				pred_A.y = pSlice->left_mvy[mb_y_4_offset];
			}
			else
			{				
				pred_A.ref_idx = pMB->ref_idx[0];
				pred_A.x = pMB->mv.x[0];
				pred_A.y = pMB->mv.y[0];
			}
		}

		// pred_B
		pred_B.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset];
		if( !pred_B.valid )
			mv_pred_set_invalid( &pred_B );
		else
		{
			pred_B.ref_idx = pSlice->top_ref_idx[(frame_x_4_offset+mb_x_4_offset)>>1];
			pred_B.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset];
			pred_B.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset];
		}

		// pred_C
		if( ((frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2))<<2) >= width )
			mv_pred_set_invalid( &pred_C );
		else
		{
			pred_C.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)];
			if( !pred_C.valid )
				mv_pred_set_invalid( &pred_C );
			else
			{
				pred_C.ref_idx = pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2))>>1)];
				pred_C.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)];
				pred_C.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)];
			}
		}

		// pred_D
		if( ((frame_x_4_offset+mb_x_4_offset-1)<<2) < 0 )
			mv_pred_set_invalid( &pred_D );
		else
		{
			pred_D.valid = (part==0) ? pSlice->upleft_valid[0] : pSlice->top_valid[frame_x_4_offset+mb_x_4_offset-1];
			if( !pred_D.valid )
				mv_pred_set_invalid( &pred_D );
			else
			{
				if( part == 0 )
				{
					pred_D.ref_idx = pSlice->upleft_ref_idx[0];
					pred_D.x = pSlice->upleft_mvx[0];
					pred_D.y = pSlice->upleft_mvy[0];
				}
				else
				{
					pred_D.ref_idx = pSlice->top_ref_idx[(frame_x_4_offset+mb_x_4_offset-1)>>1];
					pred_D.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset-1];
					pred_D.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset-1];
				}
			}
		}
		break;
	case INTER_P8x8:
		part_mode = pMB->best_8x8_blk_mode[part];		

		switch( part_mode )
		{
		case INTER_8x8:
			block = part<<2;
			mb_x_4_offset = mb_x_4_idx[block];
			mb_y_4_offset = mb_y_4_idx[block];

			// pred_A
			pred_A.valid = pSlice->left_valid[mb_y_4_offset];
			if( !pred_A.valid )
				mv_pred_set_invalid( &pred_A );
			else
			{
				pred_A.ref_idx = pSlice->left_ref_idx[(mb_y_4_offset>>1)];
				pred_A.x = pSlice->left_mvx[mb_y_4_offset];
				pred_A.y = pSlice->left_mvy[mb_y_4_offset];
			}

			// pred_B
			pred_B.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset];
			if( !pred_B.valid )
				mv_pred_set_invalid( &pred_B );
			else
			{
				pred_B.ref_idx = pSlice->top_ref_idx[(frame_x_4_offset+mb_x_4_offset)>>1];
				pred_B.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset];
				pred_B.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset];
			}

			// pred_C
			if( ((frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2))<<2) >= width || part == 3 )// out of frame width || right MB
				mv_pred_set_invalid( &pred_C );
			else
			{
				pred_C.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)];
				if( !pred_C.valid )
					mv_pred_set_invalid( &pred_C );
				else
				{
					pred_C.ref_idx = pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2))>>1)];
					pred_C.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)];
					pred_C.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)];
				}
			}

			// pred_D
			if( ((frame_x_4_offset+mb_x_4_offset-1)<<2) < 0 )
				mv_pred_set_invalid( &pred_D );
			else
			{
				pred_D.valid = pSlice->upleft_valid[mb_y_4_offset];
				if( !pred_D.valid )
					mv_pred_set_invalid( &pred_D );
				else
				{
					pred_D.ref_idx = pSlice->upleft_ref_idx[mb_y_4_offset];
					pred_D.x = pSlice->upleft_mvx[mb_y_4_offset];
					pred_D.y = pSlice->upleft_mvy[mb_y_4_offset];
				}
			}
			break;
		case INTER_8x4:
			block = (part<<2) + (sub_part<<1);
			mb_x_4_offset = mb_x_4_idx[block];
			mb_y_4_offset = mb_y_4_idx[block];

			// pred_A
			pred_A.valid = pSlice->left_valid[mb_y_4_offset];
			if( !pred_A.valid )
				mv_pred_set_invalid( &pred_A );
			else
			{
				pred_A.ref_idx = pSlice->left_ref_idx[(mb_y_4_offset>>1)];
				pred_A.x = pSlice->left_mvx[mb_y_4_offset];
				pred_A.y = pSlice->left_mvy[mb_y_4_offset];
			}

			// pred_B
			pred_B.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset];
			if( !pred_B.valid )
				mv_pred_set_invalid( &pred_B );
			else
			{
				pred_B.ref_idx = pSlice->top_ref_idx[(frame_x_4_offset+mb_x_4_offset)>>1];
				pred_B.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset];
				pred_B.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset];
			}

			// pred_C
			if( (((frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2))<<2) >= width) || (sub_part==1) || (part==3 && sub_part==0) )// out of frame width || (sub_part==1) || (part==3 && sub_part==0)
				mv_pred_set_invalid( &pred_C );
			else
			{
				pred_C.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)];
				if( !pred_C.valid )
					mv_pred_set_invalid( &pred_C );
				else
				{
					pred_C.ref_idx = pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2))>>1)];
					pred_C.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)];
					pred_C.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)];
				}
			}

			// pred_D
			if( ((frame_x_4_offset+mb_x_4_offset-1)<<2) < 0 )
				mv_pred_set_invalid( &pred_D );
			else
			{
				pred_D.valid = pSlice->upleft_valid[mb_y_4_offset];
				if( !pred_D.valid )
					mv_pred_set_invalid( &pred_D );
				else
				{
					pred_D.ref_idx = pSlice->upleft_ref_idx[mb_y_4_offset];
					pred_D.x = pSlice->upleft_mvx[mb_y_4_offset];
					pred_D.y = pSlice->upleft_mvy[mb_y_4_offset];
				}
			}
			break;
		case INTER_4x8:
			block = (part<<2) + sub_part;
			mb_x_4_offset = mb_x_4_idx[block];
			mb_y_4_offset = mb_y_4_idx[block];

			// pred_A
			pred_A.valid = pSlice->left_valid[mb_y_4_offset];
			if( !pred_A.valid )
				mv_pred_set_invalid( &pred_A );
			else
			{
				pred_A.ref_idx = pSlice->left_ref_idx[(mb_y_4_offset>>1)];
				pred_A.x = pSlice->left_mvx[mb_y_4_offset];
				pred_A.y = pSlice->left_mvy[mb_y_4_offset];
			}

			// pred_B
			pred_B.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset];
			if( !pred_B.valid )
				mv_pred_set_invalid( &pred_B );
			else
			{
				pred_B.ref_idx = pSlice->top_ref_idx[(frame_x_4_offset+mb_x_4_offset)>>1];
				pred_B.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset];
				pred_B.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset];
			}

			// pred_C
			if( ((frame_x_4_offset+mb_x_4_offset+1)<<2) >= width || (part==3&&sub_part==1) )// out of frame width || 2nd sub_part of right bottom part
				mv_pred_set_invalid( &pred_C );
			else
			{
				pred_C.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset+1];
				if( !pred_C.valid )
					mv_pred_set_invalid( &pred_C );
				else
				{
					pred_C.ref_idx = pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset+1)>>1)];
					pred_C.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset+1];
					pred_C.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset+1];
				}
			}

			// pred_D
			if( ((frame_x_4_offset+mb_x_4_offset-1)<<2) < 0 )
				mv_pred_set_invalid( &pred_D );
			else
			{
				pred_D.valid = pSlice->upleft_valid[mb_y_4_offset];
				if( !pred_D.valid )
					mv_pred_set_invalid( &pred_D );
				else
				{
					pred_D.ref_idx = pSlice->upleft_ref_idx[mb_y_4_offset];
					pred_D.x = pSlice->upleft_mvx[mb_y_4_offset];
					pred_D.y = pSlice->upleft_mvy[mb_y_4_offset];
				}
			}
			break;
		case INTER_4x4:
			block = (part<<2) + sub_part;
			mb_x_4_offset = mb_x_4_idx[block];
			mb_y_4_offset = mb_y_4_idx[block];

			// pred_A
			pred_A.valid = pSlice->left_valid[mb_y_4_offset];
			if( !pred_A.valid )
				mv_pred_set_invalid( &pred_A );
			else
			{
				pred_A.ref_idx = (sub_part&1) ? pMB->ref_idx[part] : pSlice->left_ref_idx[(mb_y_4_offset>>1)];
				pred_A.x = pSlice->left_mvx[mb_y_4_offset];
				pred_A.y = pSlice->left_mvy[mb_y_4_offset];
			}

			// pred_B
			pred_B.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset];
			if( !pred_B.valid )
				mv_pred_set_invalid( &pred_B );
			else
			{
				pred_B.ref_idx = pSlice->top_ref_idx[(frame_x_4_offset+mb_x_4_offset)>>1];
				pred_B.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset];
				pred_B.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset];
			}

			// pred_C
			if( (((frame_x_4_offset+mb_x_4_offset+1)<<2) >= width) || (sub_part == 3) || (part==3 && sub_part==1) )// out of frame width || sub_part == 3 || part==3 && sub_part==1
				mv_pred_set_invalid( &pred_C );
			else
			{
				pred_C.valid = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset+1];
				if( !pred_C.valid )
					mv_pred_set_invalid( &pred_C );
				else
				{
					pred_C.ref_idx = pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset+1)>>1)];
					pred_C.x = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset+1];
					pred_C.y = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset+1];
				}
			}

			// pred_D
			if( ((frame_x_4_offset+mb_x_4_offset-1)<<2) < 0 )
				mv_pred_set_invalid( &pred_D );
			else
			{
				pred_D.valid = pSlice->upleft_valid[mb_y_4_offset];
				if( !pred_D.valid )
					mv_pred_set_invalid( &pred_D );
				else
				{
					pred_D.ref_idx = pSlice->upleft_ref_idx[mb_y_4_offset];
					pred_D.x = pSlice->upleft_mvx[mb_y_4_offset];
					pred_D.y = pSlice->upleft_mvy[mb_y_4_offset];
				}
			}
			break;
		}
		break;
	}

	return MMES_NO_ERROR;
}

xint mv_pred_update_P8x8( slice_obj *pSlice, xint width, xint part, xint sub_part, xint predictor_x, xint predictor_y, xint encode )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/26/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   After predicting one partition, update prediction data.     */
/*   (ex. mv, ref_idx, valid, and etc.)                          */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Aug/30/2005 Chingho-Ho Chen, add prediction for 8x4, 4x8,   */
/*               4x4 block.                                      */
/*   Sep/06/2005 Chingho-Ho Chen, add prediction for integrating */
/*               decoder.                                        */
/* ------------------------------------------------------------- */
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint frame_x_16_offset, frame_x_4_offset;
	xint mb_x_4_offset, mb_y_4_offset;
	xint nx_mb, block, part_mode;
	xint x_coord;

	nx_mb = width >> 4;
	frame_x_16_offset = pMB->id % nx_mb;
	frame_x_4_offset = frame_x_16_offset<<2;

	part_mode = pMB->best_8x8_blk_mode[part];

	switch( part_mode )
	{
	case INTER_8x8:
		block = part<<2;// 0, 4, 8, 12
		mb_x_4_offset = mb_x_4_idx[block];
		mb_y_4_offset = mb_y_4_idx[block];

		// reconstructed mv should be add to mvd
		if( encode == 0 )
		{
			for( x_coord=block; x_coord<(block+4); x_coord++ )
			{
				pMB->mv.x[mb_P8x8_raster_scan_idx[x_coord]] += predictor_x;
				pMB->mv.y[mb_P8x8_raster_scan_idx[x_coord]] += predictor_y;
			}
		}

		// left
		for( x_coord=0; x_coord<2; x_coord++ )
		{
			pSlice->left_valid[mb_y_4_offset+x_coord] = MMES_VALID;
			pSlice->left_mvx[mb_y_4_offset+x_coord] = pMB->mv.x[mb_P8x8_raster_scan_idx[block]];
			pSlice->left_mvy[mb_y_4_offset+x_coord] = pMB->mv.y[mb_P8x8_raster_scan_idx[block]];
			pSlice->left_ref_idx[(mb_y_4_offset>>1)] = pMB->ref_idx[part];
		}

		// upleft (upleft should be updated before top is modified)
		for( x_coord=0; x_coord<2; x_coord++ )
		{
			pSlice->upleft_valid[mb_y_4_offset+x_coord] = (mb_y_4_offset==0 && x_coord==0) ? pSlice->top_valid[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)-1] : MMES_VALID;
			pSlice->upleft_mvx[mb_y_4_offset+x_coord] = (mb_y_4_offset==0 && x_coord==0 ) ? pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)-1] : pMB->mv.x[mb_upleft_blk_idx[part]+(x_coord<<2)];
			pSlice->upleft_mvy[mb_y_4_offset+x_coord] = (mb_y_4_offset==0 && x_coord==0 ) ? pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset+(MB_SIZE/B_SIZE/2)-1] : pMB->mv.y[mb_upleft_blk_idx[part]+(x_coord<<2)];
			pSlice->upleft_ref_idx[mb_y_4_offset+x_coord] = (x_coord==0) ? pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset+x_coord)>>1)] : pMB->ref_idx[part];
		}

		// top
		for( x_coord=0; x_coord<2; x_coord++ )
		{
			pSlice->top_valid[frame_x_4_offset+mb_x_4_offset+x_coord] = MMES_VALID;
			pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset+x_coord] = pMB->mv.x[mb_P8x8_raster_scan_idx[block]];
			pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset+x_coord] = pMB->mv.y[mb_P8x8_raster_scan_idx[block]];
			pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset+x_coord)>>1)] = pMB->ref_idx[part];
		}

		// predictor
		for( x_coord=0; x_coord<4; x_coord++ )
		{
			pMB->pred_mv.x[mb_P8x8_raster_scan_idx[(part<<2)+x_coord]] = predictor_x;
			pMB->pred_mv.y[mb_P8x8_raster_scan_idx[(part<<2)+x_coord]] = predictor_y;
		}
		break;
	case INTER_8x4:
		block = (part<<2) + (sub_part<<1);
		mb_x_4_offset = mb_x_4_idx[block];
		mb_y_4_offset = mb_y_4_idx[block];

		// reconstructed mv should be add to mvd
		if( encode == 0 )
		{		
			pMB->mv.x[mb_P8x8_raster_scan_idx[block]] += predictor_x;
			pMB->mv.y[mb_P8x8_raster_scan_idx[block]] += predictor_y;
			pMB->mv.x[mb_P8x8_raster_scan_idx[block+1]] += predictor_x;
			pMB->mv.y[mb_P8x8_raster_scan_idx[block+1]] += predictor_y;
		}

		// left
		pSlice->left_valid[mb_y_4_offset] = MMES_VALID;
		pSlice->left_mvx[mb_y_4_offset] = pMB->mv.x[mb_P8x8_raster_scan_idx[block]];
		pSlice->left_mvy[mb_y_4_offset] = pMB->mv.y[mb_P8x8_raster_scan_idx[block]];
		if( sub_part==1 )	pSlice->left_ref_idx[(mb_y_4_offset>>1)] = pMB->ref_idx[part];

		// upleft (upleft should be updated before top is modified)
		pSlice->upleft_valid[mb_y_4_offset] = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset+1];
		pSlice->upleft_mvx[mb_y_4_offset] = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset+1];
		pSlice->upleft_mvy[mb_y_4_offset] = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset+1];
		pSlice->upleft_ref_idx[mb_y_4_offset] = pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset+1)>>1)];

		// top
		for( x_coord=0; x_coord<2; x_coord++ )
		{
			pSlice->top_valid[frame_x_4_offset+mb_x_4_offset+x_coord] = MMES_VALID;
			pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset+x_coord] = pMB->mv.x[mb_P8x8_raster_scan_idx[block]];
			pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset+x_coord] = pMB->mv.y[mb_P8x8_raster_scan_idx[block]];
			pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset+x_coord)>>1)] = pMB->ref_idx[part];
		}

		// predictor
		for( x_coord=0; x_coord<2; x_coord++ )
		{
			pMB->pred_mv.x[ mb_P8x8_raster_scan_idx[block+x_coord] ] = predictor_x;
			pMB->pred_mv.y[ mb_P8x8_raster_scan_idx[block+x_coord] ] = predictor_y;
		}
		break;
	case INTER_4x8:
		block = (part<<2) + sub_part;
		mb_x_4_offset = mb_x_4_idx[block];
		mb_y_4_offset = mb_y_4_idx[block];

		// reconstructed mv should be add to mvd
		if( encode == 0 )
		{
			pMB->mv.x[mb_P8x8_raster_scan_idx[block]] += predictor_x;
			pMB->mv.y[mb_P8x8_raster_scan_idx[block]] += predictor_y;
			pMB->mv.x[mb_P8x8_raster_scan_idx[block+2]] += predictor_x;
			pMB->mv.y[mb_P8x8_raster_scan_idx[block+2]] += predictor_y;
		}

		// left
		for( x_coord=0; x_coord<2; x_coord++ )
		{
			pSlice->left_valid[mb_y_4_offset+x_coord] = MMES_VALID;
			pSlice->left_mvx[mb_y_4_offset+x_coord] = pMB->mv.x[mb_P8x8_raster_scan_idx[block]];
			pSlice->left_mvy[mb_y_4_offset+x_coord] = pMB->mv.y[mb_P8x8_raster_scan_idx[block]];
			pSlice->left_ref_idx[(mb_y_4_offset>>1)] = pMB->ref_idx[part];
		}

		// upleft (upleft should be updated before top is modified)
		for( x_coord=0; x_coord<2; x_coord++ )
		{
			pSlice->upleft_valid[mb_y_4_offset+x_coord] = (x_coord==0) ? pSlice->top_valid[frame_x_4_offset+mb_x_4_offset] : MMES_VALID;
			pSlice->upleft_mvx[mb_y_4_offset+x_coord] = ( x_coord==0 ) ? pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset] : pMB->mv.x[mb_P8x8_raster_scan_idx[block]];
			pSlice->upleft_mvy[mb_y_4_offset+x_coord] = ( x_coord==0 ) ? pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset] : pMB->mv.y[mb_P8x8_raster_scan_idx[block]];
			pSlice->upleft_ref_idx[mb_y_4_offset+x_coord] = ( x_coord==0 ) ? pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset)>>1)] : pMB->ref_idx[part];
		}

		// top
		pSlice->top_valid[frame_x_4_offset+mb_x_4_offset] = MMES_VALID;
		pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset] = pMB->mv.x[mb_P8x8_raster_scan_idx[block]];
		pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset] = pMB->mv.y[mb_P8x8_raster_scan_idx[block]];
		if( sub_part==1 )	pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset)>>1)] = pMB->ref_idx[part];
		
		// predictor
		for( x_coord=0; x_coord<2; x_coord++ )
		{
			pMB->pred_mv.x[ mb_P8x8_raster_scan_idx[block+(x_coord<<1)] ] = predictor_x;
			pMB->pred_mv.y[ mb_P8x8_raster_scan_idx[block+(x_coord<<1)] ] = predictor_y;
		}
		break;
	case INTER_4x4:
		block = (part<<2) + sub_part;
		mb_x_4_offset = mb_x_4_idx[block];
		mb_y_4_offset = mb_y_4_idx[block];

		// reconstructed mv should be add to mvd
		if( encode == 0 )
		{
			pMB->mv.x[mb_P8x8_raster_scan_idx[block]] += predictor_x;
			pMB->mv.y[mb_P8x8_raster_scan_idx[block]] += predictor_y;
		}

		// left
		pSlice->left_valid[mb_y_4_offset] = MMES_VALID;
		pSlice->left_mvx[mb_y_4_offset] = pMB->mv.x[mb_P8x8_raster_scan_idx[block]];
		pSlice->left_mvy[mb_y_4_offset] = pMB->mv.y[mb_P8x8_raster_scan_idx[block]];
		if( sub_part==3 )	pSlice->left_ref_idx[(mb_y_4_offset>>1)] = pMB->ref_idx[part];

		// upleft (upleft should be updated before top is modified)
		pSlice->upleft_valid[mb_y_4_offset] = pSlice->top_valid[frame_x_4_offset+mb_x_4_offset];
		pSlice->upleft_mvx[mb_y_4_offset] = pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset];
		pSlice->upleft_mvy[mb_y_4_offset] = pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset];		
		pSlice->upleft_ref_idx[mb_y_4_offset] = pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset)>>1)];

		// top
		pSlice->top_valid[frame_x_4_offset+mb_x_4_offset] = MMES_VALID;
		pSlice->top_mvx[frame_x_4_offset+mb_x_4_offset] = pMB->mv.x[mb_P8x8_raster_scan_idx[block]];
		pSlice->top_mvy[frame_x_4_offset+mb_x_4_offset] = pMB->mv.y[mb_P8x8_raster_scan_idx[block]];
		if( sub_part&1 )	pSlice->top_ref_idx[((frame_x_4_offset+mb_x_4_offset)>>1)] = pMB->ref_idx[part];

		// predictor
		pMB->pred_mv.x[ mb_P8x8_raster_scan_idx[block] ] = predictor_x;
		pMB->pred_mv.y[ mb_P8x8_raster_scan_idx[block] ] = predictor_y;
		break;
	}
	return MMES_NO_ERROR;
}

xint mv_pred_set_invalid( pred_obj *pred )
{
	pred->valid = 0;
	pred->ref_idx = -1;
	pred->x = 0;
	pred->y = 0;
	return MMES_NO_ERROR;
}

xint mv_pred_get_prd_mv( slice_obj *pSlice, xint part, xint *pred_mvx, xint *pred_mvy )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/??/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Procedure for getting motion vector predictor of current    */
/*   block.                                                      */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint predictor_x, predictor_y;

	// if C is not avaiable	
	if( !pred_C.valid )
	{
		pred_C = pred_D;
	}

	if( pMB->best_Inter_mode == INTER_16x8 && part == 0 && pred_B.ref_idx == pred_X.ref_idx )
	{
		*pred_mvx = pred_B.x;
		*pred_mvy = pred_B.y;
		return MMES_NO_ERROR;
	}
	else if( pMB->best_Inter_mode == INTER_16x8 && part == 1 && pred_A.ref_idx == pred_X.ref_idx )
	{
		*pred_mvx = pred_A.x;
		*pred_mvy = pred_A.y;
		return MMES_NO_ERROR;
	}
	else if( pMB->best_Inter_mode == INTER_8x16 && part == 0 && pred_A.ref_idx == pred_X.ref_idx )
	{
		*pred_mvx = pred_A.x;
		*pred_mvy = pred_A.y;
		return MMES_NO_ERROR;
	}
	else if( pMB->best_Inter_mode == INTER_8x16 && part == 1 && pred_C.ref_idx == pred_X.ref_idx )
	{
		*pred_mvx = pred_C.x;
		*pred_mvy = pred_C.y;
		return MMES_NO_ERROR;
	}

	if( !(pred_B.valid || pred_C.valid) )// if both B and C are not available, and A is available
	{
		predictor_x = pred_A.x;
		predictor_y = pred_A.y;
	}
	else if( pred_A.ref_idx == pred_X.ref_idx && pred_B.ref_idx != pred_X.ref_idx && pred_C.ref_idx != pred_X.ref_idx )
	// if one and only one refIdxN is equal
	{
		predictor_x = pred_A.x;
		predictor_y = pred_A.y;
	}
	else if( pred_B.ref_idx == pred_X.ref_idx && pred_A.ref_idx != pred_X.ref_idx && pred_C.ref_idx != pred_X.ref_idx )
	{
		predictor_x = pred_B.x;
		predictor_y = pred_B.y;
	}
	else if( pred_C.ref_idx == pred_X.ref_idx && pred_B.ref_idx != pred_X.ref_idx && pred_A.ref_idx != pred_X.ref_idx )
	{
		predictor_x = pred_C.x;
		predictor_y = pred_C.y;
	}
	else
	{
		predictor_x = pred_A.x+pred_B.x+pred_C.x-MIN(pred_A.x,MIN(pred_B.x,pred_C.x))-MAX(pred_A.x,MAX(pred_B.x,pred_C.x));
		predictor_y = pred_A.y+pred_B.y+pred_C.y-MIN(pred_A.y,MIN(pred_B.y,pred_C.y))-MAX(pred_A.y,MAX(pred_B.y,pred_C.y));
	}

	*pred_mvx = predictor_x;
	*pred_mvy = predictor_y;

	return MMES_NO_ERROR;
}

xint mv_pred_set_pred_X( slice_obj *pSlice, xint width, xint part, xint sub_part )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                              */
/*   Date: Aug/??/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   When macroblock mode is P8x8, pred_X is set by this         */
/*   procedure.                                                  */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint part_mode, idx;

	part_mode = pMB->best_8x8_blk_mode[part];

	switch( part_mode )
	{
	case INTER_8x8:
		idx = mb_P8x8_raster_scan_idx[ (part<<2) ];
		pred_X.valid = 1;
		pred_X.ref_idx = pMB->ref_idx[ part ];
		pred_X.x = pMB->mv.x[ idx ];
		pred_X.y = pMB->mv.y[ idx ];
		break;
	case INTER_8x4:
		idx = mb_P8x8_raster_scan_idx[ (part<<2) + (sub_part<<1) ];
		pred_X.valid = 1;
		pred_X.ref_idx = pMB->ref_idx[ part ];
		pred_X.x = pMB->mv.x[ idx ];
		pred_X.y = pMB->mv.y[ idx ];
		break;
	case INTER_4x8:
		idx = mb_P8x8_raster_scan_idx[ (part<<2) + sub_part ];
		pred_X.valid = 1;
		pred_X.ref_idx = pMB->ref_idx[ part ];
		pred_X.x = pMB->mv.x[ idx ];
		pred_X.y = pMB->mv.y[ idx ];
		break;
	case INTER_4x4:
		idx = mb_P8x8_raster_scan_idx[ (part<<2) + sub_part ];
		pred_X.valid = 1;
		pred_X.ref_idx = pMB->ref_idx[ part ];
		pred_X.x = pMB->mv.x[ idx ];
		pred_X.y = pMB->mv.y[ idx ];
		break;
	}
	return MMES_NO_ERROR;
}

xint mv_pred_add_mvd( slice_obj *pSlice )
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date: Sep/??/2005                                           */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Reconstruct motion vector                                   */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pSlice -> [I/O] pointer to the session parameter           */
/*                                                               */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	mb_obj    *pMB = &(pSlice->cmb);
	xint x_coord;

	if( pMB->best_Inter_mode == INTER_PSKIP )
	{
		for( x_coord=0; x_coord<NBLOCKS; x_coord++ )
		{
			pMB->mv.x[ x_coord ] = pMB->pred_mv.x[ x_coord ];
			pMB->mv.y[ x_coord ] = pMB->pred_mv.y[ x_coord ];
		}
	}
	else if( pMB->best_Inter_mode == INTER_16x16 )
	{
		for( x_coord=0; x_coord<NBLOCKS; x_coord++ )
		{
			pMB->mv.x[ x_coord ] += pMB->pred_mv.x[ x_coord ];
			pMB->mv.y[ x_coord ] += pMB->pred_mv.y[ x_coord ];
		}
	}

	return MMES_NO_ERROR;
}

