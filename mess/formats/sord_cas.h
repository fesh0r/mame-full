/*
 * converts Sord M5 .cas files into samples for the SORD driver
 */

#include "driver.h"

#define SORD_CAS_ERROR_SUCCESS        0
#define SORD_CAS_ERROR_CORRUPTIMAGE   1
#define SORD_CAS_ERROR_OUTOFMEMORY    2

int sord_cas_to_wav (UINT8 *casdata, int caslen, INT16 **wavdata, int *wavlen);
int sord_cas_to_wav_size (UINT8 *casdata, int caslen);

