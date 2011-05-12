/* ///////////////////////////////////////////////////////////// */
/*   File: metypes.h                                             */
/*   Author: Jerry Peng                                          */
/*   Date: Dec/24/2002                                           */
/* ------------------------------------------------------------- */
/*   Basic data type definitions.                                */
/*   Copyright, 2002.                                            */
/* ///////////////////////////////////////////////////////////// */

#ifndef __METYPES_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* The following two definitions are for fast, */
/*    word-size independent operations         */
typedef int              xint;
typedef unsigned int     uint;

typedef char             mbchar; /* this is for multibyte, unicode- */
                                 /* like character definitions.     */
                                 /* It is used for defining a zero- */
                                 /* terminated string.  For 8-bit   */
                                 /* data array, use int8 or uint8.  */

#if defined(WIN32) || defined(_WIN32_WCE) /* 32-bit Windows Platform */

typedef char             int8;
typedef short            int16;
typedef long             int32;
typedef __int64          int64;

typedef unsigned char    uint8;
typedef unsigned short   uint16;
typedef unsigned long    uint32;
typedef unsigned __int64 uint64;

#else /* 32-bit Linux */

/* Rewrite by Feng-Cheng Chang. */
/* Use stdint.h in glibc for future compatibility. */
#include <stdint.h>
typedef int8_t           int8;
typedef int16_t          int16;
typedef int32_t          int32;
typedef int64_t          int64;

typedef uint8_t          uint8;
typedef uint16_t         uint16;
typedef uint32_t         uint32;
typedef uint64_t         uint64;

#endif

#ifdef __cplusplus
//extern "C"
}
#endif

#define __METYPES_H__
#endif /* __METYPES_H__ */
