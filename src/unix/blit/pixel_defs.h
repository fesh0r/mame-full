#ifndef __PIXEL_DEFS_H
#define __PIXEL_DEFS_H

#define _32TO16_RGB_555(p) (unsigned short)((((p) & 0x00F80000) >> 9) | \
                                    (((p) & 0x0000F800) >> 6) | \
                                    (((p) & 0x000000F8) >> 3))

#define _32TO16_RGB_565(p) (unsigned short)((((p) & 0x00F80000) >> 8) | \
                                    (((p) & 0x0000FC00) >> 5) | \
                                    (((p) & 0x000000F8) >> 3))

#define RGB2YUV(r,g,b,y,u,v) \
    (y) = ((  9836*(r) + 19310*(g) +  3750*(b)          ) >> 15); \
    (u) = (( -5527*(r) - 10921*(g) + 16448*(b) + 4194304) >> 15); \
    (v) = (( 16448*(r) - 13783*(g) -  2665*(b) + 4194304) >> 15)

#ifdef LSB_FIRST    
#define Y1MASK  0x000000FF
#define  UMASK  0x0000FF00
#define Y2MASK  0x00FF0000
#define  VMASK  0xFF000000
#define Y1SHIFT  0
#define  USHIFT  8
#define Y2SHIFT 16
#define  VSHIFT 24
#else
#define Y1MASK  0xFF000000
#define  UMASK  0x00FF0000
#define Y2MASK  0x0000FF00
#define  VMASK  0x000000FF
#define Y1SHIFT 24
#define  USHIFT 16
#define Y2SHIFT  8
#define  VSHIFT  0
#endif

#define YMASK  (Y1MASK|Y2MASK)
#define UVMASK (UMASK|VMASK)

#define RMASK_SEMI(P) ( RMASK(P) | RMASK_INV_HALF(P) )
#define GMASK_SEMI(P) ( GMASK(P) | GMASK_INV_HALF(P) )
#define BMASK_SEMI(P) ( BMASK(P) | BMASK_INV_HALF(P) )
#define MEAN(P,Q) ( RMASK((RMASK(P)+RMASK(Q))/2) | GMASK((GMASK(P)+GMASK(Q))/2) | BMASK((BMASK(P)+BMASK(Q))/2) )

#endif
