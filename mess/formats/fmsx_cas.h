/*
 * converts fMSX's .cas files into samples for the MSX driver
 */

#include "driver.h"

int fmsx_cas_to_wav (UINT8 *casdata, int caslen, INT16 **wavdata, int *wavlen);
int fmsx_cas_to_wav_size (UINT8 *casdata, int caslen);

