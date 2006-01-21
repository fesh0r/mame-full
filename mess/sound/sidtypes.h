
#ifndef MYTYPES_H
#define MYTYPES_H

#include "osd_cpu.h"
/*#include "compconf.h" */

/* Wanted: 8-bit signed/unsigned. */
#define sbyte INT8
#define ubyte UINT8

/* Wanted: 16-bit signed/unsigned. */
#define sword INT16
#define uword UINT16

/* Wanted: 32-bit signed/unsigned. */
#define sdword INT32
#define udword UINT32


/* Some common type shortcuts. */
#define uchar unsigned char
#define uint  unsigned int
#define ulong unsigned long


#if 1||defined(SID_FPUFILTER)
  typedef float filterfloat;
#else
  #include "fixpoint.h"
  typedef fixed filterfloat;
#endif

#endif
