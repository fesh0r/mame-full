#ifndef __VIC4567_H_
#define __VIC4567_H_

/* idea:
   with const mask compiler should be able to discard several code
   misty compiler, even with -O2 nothing, but with the help of the 
   preprocessor
   with the macro I can generate a lot of desired functions */

/* HJB: rewrote this header file as a macro so it has to be included
 * only once. GCC (and any decent compiler) should very well be able
 * to eliminate constant false expressions (verified asm output)
 */
#define VIC3_MASK(M)                                            \
    if (M)                                                      \
    {                                                           \
        if (M & 0x01)                                    \
            colors[0] = c64_memory[VIC3_ADDR(0)+offset];        \
        if (M & 0x02)                                           \
            colors[1] = c64_memory[VIC3_ADDR(1)+offset]<<1;     \
        if (M & 0x04)                                    \
            colors[2] = c64_memory[VIC3_ADDR(2)+offset]<<2;     \
        if (M & 0x08)                                           \
            colors[3] = c64_memory[VIC3_ADDR(3)+offset]<<3;     \
        if (M & 0x10)                                           \
            colors[4] = c64_memory[VIC3_ADDR(4)+offset]<<4;     \
        if (M & 0x20)                                           \
            colors[5] = c64_memory[VIC3_ADDR(5)+offset]<<5;     \
        if (M & 0x40)                                           \
            colors[6] = c64_memory[VIC3_ADDR(6)+offset]<<6;     \
        if (M & 0x80)                                           \
            colors[7] = c64_memory[VIC3_ADDR(7)+offset]<<7;     \
        for (i=7; i >= 0; i--)                                  \
        {                                                       \
            p = 0;                                              \
            if (M & 0x01)                                \
            {                                                   \
                p = colors[0] & 0x01;                           \
                colors[0] >>= 1;                                \
            }                                                   \
            if (M & 0x02)                                       \
            {                                                   \
                p |= colors[1] & 0x02;                      \
                colors[1] >>= 1;                                \
            }                                                   \
            if (M & 0x04)                                       \
            {                                                   \
                p |= colors[2] & 0x04;                      \
                colors[2] >>= 1;                                \
            }                                                   \
            if (M & 0x08)                                       \
            {                                                   \
                p |= colors[3] & 0x08;                      \
                colors[3] >>= 1;                                \
            }                                                   \
            if (M & 0x10)                                       \
            {                                                   \
                p |= colors[4] & 0x10;                      \
                colors[4] >>= 1;                                \
            }                                                   \
            if (M & 0x20)                                       \
            {                                                   \
                p |= colors[5] & 0x20;                      \
                colors[5] >>= 1;                                \
            }                                                   \
            if (M & 0x40)                                       \
            {                                                   \
                p |= colors[6] & 0x40;                      \
                colors[6] >>= 1;                                \
            }                                                   \
            if (M & 0x80)                                       \
            {                                                   \
                p |= colors[7] & 0x80;                      \
                colors[7] >>= 1;                                \
            }                                                   \
            vic2.bitmap->line[YPOS+y][XPOS+x+i] =               \
                Machine->pens[p];                               \
        }                                                       \
    }

#endif

