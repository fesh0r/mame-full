#define _32TO16_RGB_555(p) (unsigned short)((((p) & 0x00F80000) >> 9) | \
                                    (((p) & 0x0000F800) >> 6) | \
                                    (((p) & 0x000000F8) >> 3))

#define _32TO16_RGB_565(p) (unsigned short)((((p) & 0x00F80000) >> 8) | \
                                    (((p) & 0x0000FC00) >> 5) | \
                                    (((p) & 0x000000F8) >> 3))

#define RGB2YUV(r,g,b,y,u,v) \
    (y) = ((  8453*(r) + 16594*(g) +  3223*(b) +  524288) >> 15); \
    (u) = (( -4878*(r) -  9578*(g) + 14456*(b) + 4210688) >> 15); \
    (v) = (( 14456*(r) - 12105*(g) -  2351*(b) + 4210688) >> 15)

#define FOURCC_YUY2 0x32595559
#define FOURCC_YV12 0x32315659
#define FOURCC_I420 0x30323449
#define FOURCC_UYVY 0x59565955
