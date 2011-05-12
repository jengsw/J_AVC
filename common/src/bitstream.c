/* ///////////////////////////////////////////////////////////// */
/*   File: bitstream.c                                           */
/*   Author: Jerry Peng                                          */
/*   Date: Mar/06/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 bitstream read/write module.     */
/*                                                               */
/*   Copyright, 2004-2005.                                       */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ///////////////////////////////////////////////////////////// */

#include <string.h>
#include "../../common/inc/bitstream.h"
#include "../../common/inc/misc_util.h"

static const uint32 prefix_mask[33] =
{
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

void dectobin(uint8 *buf, uint32 value, uint32 nbits)
{
    uint32 i, mask;

    if(buf)
    {
        for(i = 0, mask = 1 << (nbits - 1); i < nbits ; i++, mask >>= 1)
        {
            if(mask & value)
                buf[i] = '1';
            else
                buf[i] = '0';
        }
        buf[i] = '\x00';
    }
}

void
stream_init(h264dec_obj *pDec, uint8 *bitbuf, uint32 bufsize)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Initialize the bitstream buffer for the decoder object.     */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec  -> [I/O] pointer to the decoder session parameters   */
/*   bitbuf -> [I]   bitstream buffer (setup by the application) */
/*   bufsize-> [I]   the size of the buffer in bits              */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    pDec->stream_buf = bitbuf;
    pDec->stream_len = bufsize;
    pDec->bitcnt = 0;
}

uint32
residual_bits(h264dec_obj *pDec)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/15/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Show the number of residual bits in the bitstream.          */
/*                                                               */
/*   RETURN                                                      */
/*   The number of residual bits.                                */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec   -> [I/O] pointer to the decoder session parameters  */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    uint32 nbits_left;

    nbits_left = (pDec->stream_len<<3) - pDec->bitcnt;

    return nbits_left;
}

void
skip_bits(h264dec_obj *pDec, uint32 nbits)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/02/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   skip the next 'nbits' from the bitstream.                   */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec   -> [I/O] pointer to the decoder session parameters  */
/*   nbits   -> [I]   number of bits to read                     */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    uint32   code;
    code = show_bits(pDec, nbits);
    pDec->bitcnt += nbits;
}

uint32
get_bits(h264dec_obj *pDec, uint32 nbits)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Aug/02/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Read the next 'nbits' from the bitstream.                   */
/*                                                               */
/*   RETURN                                                      */
/*   The value of the next 'nbits' bits.                         */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec   -> [I/O] pointer to the decoder session parameters  */
/*   nbits   -> [I]   number of bits to read                     */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    uint32 code;
    code = show_bits(pDec, nbits);
    pDec->bitcnt += nbits;

    return code;
}

uint32
show_bits(h264dec_obj *pDec, uint32 nbits)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                       */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Show the next 'nbits' from the bitstream.                   */
/*                                                               */
/*   RETURN                                                      */
/*   The value of the next 'nbits' bits.                         */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec   -> [I/O] pointer to the decoder session parameters  */
/*   nbits   -> [I]   number of bits to show                     */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    uint8 cache[4], *v;
    uint32 b, b1, code;
    uint32 c, offset, nbits_left, nbytes_left;

    nbits_left = (pDec->stream_len<<3) - pDec->bitcnt;

    if (nbits_left < nbits)
    {
	    /* This should never happen */
        alert_msg("alloc_slice", "insufficient bits left in bitstream.");
	    return -1;
    }

    offset = pDec->bitcnt >> 3;
    nbytes_left = pDec->stream_len-offset+1;
    if (nbytes_left >= 4)
    {
	    v = pDec->stream_buf + offset;
    }
    else
    {
	    memset(cache, 0, 4);
	    memcpy(cache, pDec->stream_buf + offset, nbytes_left);
	    v = cache;
    }
    b = ((int32)v[0]<<24) | ((int32)v[1]<<16) | ((int32)v[2]<<8) | (int32)v[3];
    c = 32 - (pDec->bitcnt%8);

    if (c < nbits)
    {
	    /* the only place we need to read more than 24 bits is  */
	    /*  parsing the parameter set timescale & units_in_tick */
	    /*  so reading v[4] should not cause memory fault.      */
	    b1 = v[4];
	    code = ((b << (nbits - c)) | (b1 >> (8 - (nbits - c)))) & prefix_mask[nbits];
    }
    else
    {
	    code = ((b >> (c - nbits)) & prefix_mask[nbits]);
    }

    return code;
}

xint
byte_align(void *pHnd, xint EorD)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Oct/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   return whether byte align or not.                           */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*    EorD -> 1:encode 0:decode
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
    h264enc_obj *pEnc;
    h264dec_obj *pDec;

    if(EorD)
        pEnc = (h264enc_obj *)pHnd;
    else
        pDec = (h264dec_obj *)pHnd;

    return EorD ? (!pEnc->bit_pos%8) : !(pDec->bitcnt%8);
}

void
rewind_bits(h264dec_obj *pDec, int32 nbits)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   Rewind the read pointer by 'nbits'.                         */
/*                                                               */
/*   RETURN                                                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec   -> [I/O] pointer to the decoder session parameters  */
/*   nbits   -> [I]   number of bits to rewind                   */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*                                                               */
/* ------------------------------------------------------------- */
{
	pDec->bitcnt -= nbits;
	if (pDec->bitcnt < 0) pDec->bitcnt = 0;
}

xint
put_bits(h264enc_obj *pEnc, uint32 code, xint nbits)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   write 'nbits' code to the bitstream buffer.                 */
/*                                                               */
/*   RETURN                                                      */
/*   The status code of the write operation                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pEnc -> [I/O] pointer to the encoder session parameters    */
/*   code  -> [I]   the pattern to be written to the bitstream   */
/*   nbits -> [I]   number of bits to rewind                     */
/*   *des  -> [I]   description for the bitstream parameter      */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jul/??/2005 Jerry Peng, Add the logging mechanism.          */
/* ------------------------------------------------------------- */
{
    xint    idx;
    uint32  mask;

    mask = 1 << (nbits - 1);

    for (idx = 0; idx < nbits; idx++)
    {
        pEnc->curr_byte <<= 1;
        if (code & mask) 
        {
            pEnc->curr_byte |= 1;
            /* temporal bitstream buffer */
            pEnc->bit_buf[idx] = '1';
        }
        else pEnc->bit_buf[idx] = '0';
        pEnc->bit_pos++;
        mask >>= 1;

        if (pEnc->bit_pos == 8) /* now, store one byte to the buffer */
        {
#if EMULATION_PREVENTION_ON
            if(nbits != 32 && pEnc->byte_pos >= 2)
                if(pEnc->stream_buf[pEnc->byte_pos-2] == 0x00 && pEnc->stream_buf[pEnc->byte_pos-1] == 0x00)
                    if(!(pEnc->curr_byte & 0xFC))
                        pEnc->stream_buf[pEnc->byte_pos++] = 0x03;
#endif

            pEnc->stream_buf[pEnc->byte_pos] = pEnc->curr_byte;
            pEnc->byte_pos++;

            if (pEnc->byte_pos >= pEnc->stream_len)
            {
                alert_msg("put_bits", "Encoder bit buffer full!\n");
                return MMES_BITSTREAM_ERROR;
            }

            pEnc->bit_pos = 0;
            pEnc->curr_byte = 0;
        }
    }

    /* temporal bitstream buffer */
    pEnc->bit_buf[idx] = '\x00';

    return MMES_NO_ERROR;
}

xint
put_one_bit(h264enc_obj *pEnc, uint32 code)
/* ------------------------------------------------------------- */
/*   Author: Jerry Peng                                          */
/*   Date  : Mar/06/2005                                         */
/* ------------------------------------------------------------- */
/*   COMMENTS                                                    */
/*   write one bit to the bitstream buffer.                      */
/*                                                               */
/*   RETURN                                                      */
/*   The status code of the write operation                      */
/*                                                               */
/*   PARAMETERS:                                                 */
/*   *pDec -> [I/O] pointer to the decoder session parameters    */
/*   code  -> [I]   the pattern to be written to the bitstream   */
/*   nbits -> [I]   number of bits to rewind                     */
/*   *des  -> [I]   description for the bitstream parameter      */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   Jul/??/2005 Jerry Peng, Add the logging mechanism.          */
/* ------------------------------------------------------------- */
{
    pEnc->curr_byte <<= 1;
    pEnc->curr_byte |= code;
    pEnc->bit_pos++;

    /* temporal bitstream buffer */
    pEnc->bit_buf[0] = (code) ? '1' : '0';
    pEnc->bit_buf[1] = '\x00';

    if (pEnc->bit_pos == 8) /* now, store one byte to the buffer */
    {
#if EMULATION_PREVENTION_ON
        if(pEnc->byte_pos >= 2)
            if(pEnc->stream_buf[pEnc->byte_pos-2] == 0x00 && pEnc->stream_buf[pEnc->byte_pos-1] == 0x00)
            {
                if(!(pEnc->curr_byte & 0xFC))
                    pEnc->stream_buf[pEnc->byte_pos++] = 0x03;
            }
#endif

        pEnc->stream_buf[pEnc->byte_pos] = pEnc->curr_byte;
        pEnc->byte_pos++;
        if (pEnc->byte_pos >= pEnc->stream_len)
        {
            alert_msg("put_bits", "Encoder bit buffer full!\n");
            return MMES_BITSTREAM_ERROR;
        }

        pEnc->bit_pos = 0;
        pEnc->curr_byte = 0;
    }

    return MMES_NO_ERROR;
}
