/* ///////////////////////////////////////////////////////////// */
/*   File: cavlc.h                                               */
/*   Author: JM Authors                                          */
/*   Date: Jan/??/2005                                           */
/* ------------------------------------------------------------- */
/*   ISO MPEG-4 AVC/ITU-T H.264 codec CAVLC routines.            */
/*   extracted from JM                                           */
/*                                                               */
/*   See JM Copyright Notice.                                    */
/* ------------------------------------------------------------- */
/*   MODIFICATIONS                                               */
/*   @Jerry Peng Modify the mechanism                            */
/* ///////////////////////////////////////////////////////////// */


#ifndef _CAVLC_H_

#include <stdlib.h>
#include "metypes.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TOTRUN_NUM          15
#define RUNBEFORE_NUM       7
#define MAXRTPPAYLOADLEN    (65536 - 40)    //!< Maximum payload size of an RTP packet
#define BIT_FILE            "../temp/test.bit"
#define RESIDUAL_TYPE       LumaLevel

#define _LOG_

/* for CAVLC, 4x4, except I8x8_DC_CHROMA = 2x2 */
typedef enum
{
    Intra16x16DCLevel, Intra16x16ACLevel, LumaLevel, ChromaDCLevel, ChromaACLevel
} block_type;

typedef enum
{
    Y_block, Cb_block, Cr_block
} block_component;

//! Bitstream
typedef struct
{
  xint             byte_pos;           //!< current position in bitstream;
  xint             bits_to_go;         //!< current bitcounter
  uint8            byte_buf;           //!< current buffer for last written byte

  uint8            *streamBuffer;      //!< actual buffer for written bytes

  //for decoding
  uint16           frame_bitoffset;    //!< actual position in the codebuffer, bit-oriented, UVLC only
  uint16           bitstream_length;   //!< over codebuffer lnegth, byte oriented, UVLC only

} Bitstream;

//! Syntaxelement
typedef struct syntaxelement
{
  xint                 type;           //!< type of syntax element for data part.
  xint                 value1;         //!< numerical value of syntax element
  xint                 value2;         //!< for blocked symbols, e.g. run/level
  xint                 len;            //!< length of code
  xint                 inf;            //!< info part of UVLC code
  uint16               bitpattern;     //!< UVLC bitpattern

} SyntaxElement;

static xint __inline floor_log2(xint value)
{
    xint num;

    num = 0;

    while(value >1)
    {
        value >>= 1;
        num += 1;
    }

    return num;
}

//error message
#define NO_ERROR        0
#define FILE_ERROR      1
#define ALLOC_ERROR     2
#define TABLE_ERROR     3

#define INVALID     0
#define VALID       1

#define MAX_PIXEL_VALUE   255
#define MIN_PIXEL_VALUE   0


#define ABS(X)  ((X)> 0 ? (X) : -(X))
#define SIGN(X,Y) ((Y)>0 ? (X) : -(X)) 

#ifdef __cplusplus
}
#endif

#define _CAVLC_H_
#endif
