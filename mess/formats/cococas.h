#ifndef COCOCAS_H
#define COCOCAS_H

#include "driver.h"

#define COCO_WAVESAMPLES_HEADER  3000
#define COCO_WAVESAMPLES_TRAILER 1000

extern int coco_wave_size;

int coco_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes);

#endif /* COCOCAS_H */
