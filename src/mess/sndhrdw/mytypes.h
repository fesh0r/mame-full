//
// /home/ms/source/sidplay/libsidplay/include/RCS/mytypes.h,v
//

#ifndef MYTYPES_H
#define MYTYPES_H


//#include "compconf.h"

#if 0
// A ``bool'' type for compilers that don't (yet) support one.
#if !HAVE_BOOL
  typedef int bool;

  #if defined(true) || defined(false)
//    #error Better check include file ``mytypes.h''.
    #undef true
    #undef false
  #endif
  #define true 1
  #define false 0
#endif
#endif

// Wanted: 8-bit signed/unsigned.
typedef signed char sbyte;
typedef unsigned char ubyte;

// Wanted: 16-bit signed/unsigned.
typedef signed short int sword;
typedef unsigned short int uword;

// Wanted: 32-bit signed/unsigned.
typedef signed long int sdword;
typedef unsigned long int udword;


// Some common type shortcuts.
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long int ulong;


#if 1||defined(SID_FPUFILTER)
  typedef float filterfloat;
#else
  #include "fixpoint.h"
  typedef fixed filterfloat;
#endif


typedef void (*ptr2func)(void);


#endif
