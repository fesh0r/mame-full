#ifndef LVIV_LVT_H
#define LVIV_LVT_H

#include "driver.h"

int lviv_cassette_fill_wave(INT16 *buffer, int length, UINT8 *bytes);
int lviv_cassette_calculate_size_in_samples(int length, UINT8 *bytes);

#endif /* LVIV_LVT_H */
