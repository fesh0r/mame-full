#ifndef ORICCAS_H
#define ORICCAS_H


#include "driver.h"


/* frequency of wave */
/* tapes use 1200hz and 2400hz samples */
#define ORIC_WAV_FREQUENCY	4800

/* 13 bits define a byte on the cassette */
/* 1 start bit, 8 data bits, 1 parity bit and 3 stop bits */
#define ORIC_BYTE_TO_BITS_ON_CASSETTE 13


extern int oric_wave_size;

int oric_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes);

#endif /* ORICCAS_H */
