#ifndef WAVE_H_INCLUDED
#define WAVE_H_INCLUDED

/*****************************************************************************
 *	CassetteWave interface
 *****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "messdrv.h"

struct Wave_interface
{
	int num;
	int mixing_level[MAX_DEV_INSTANCES];
};

extern int wave_sh_start(const struct MachineSound *msound);
extern void wave_sh_stop(void);
extern void wave_sh_update(void);

/*****************************************************************************
 * functions for the IODevice entry IO_CASSETTE. Example for the macro
 * IO_CASSETTE_WAVE(1,"wav\0cas\0",mycas_id,mycas_init,mycas_exit)
 *****************************************************************************/

void cassette_exit(mess_image *img);

void wave_specify(struct IODevice *iodev, int count, char *actualext, const char *fileext,
	device_load_handler loadproc, device_unload_handler unloadproc);

#define CONFIG_DEVICE_CASSETTEX(count, fileext, loadproc, unloadproc)		\
	if (cfg->device_num-- == 0)												\
	{																		\
		static struct IODevice iodev;										\
		static char actualext[sizeof(fileext)+4];							\
		wave_specify(&iodev, (count), actualext, (fileext), (loadproc), (unloadproc));\
		cfg->dev = &iodev;													\
	}																		\

#define CONFIG_DEVICE_CASSETTE(count, fileext, loadproc)	\
	CONFIG_DEVICE_CASSETTEX((count), (fileext), (loadproc), cassette_exit)

/*****************************************************************************
 * Use this structure for the "void *args" argument of device_open()
 * file
 *	  file handle returned by mame_fopen() (mandatory)
 * display
 *	  display cassette icon, playing time and total time on screen
 * fill_wave
 *	  callback to fill in samples (optional)
 * smpfreq
 *	  sample frequency when the wave is generated (optional)
 *	  used for fill_wave() and for writing (creating) wave files
 * header_samples
 *	  number of samples for a cassette header (optional)
 * trailer_samples
 *	  number of samples for a cassette trailer (optional)
 * chunk_size
 *	  number of bytes to convert at once (optional)
 * chunk_samples
 *	  number of samples produced for a data chunk (optional)
 *****************************************************************************/
struct wave_args_legacy
{
    mame_file *file;
	int (*fill_wave)(INT16 *buffer, int length, UINT8 *bytes);
	int smpfreq;
    int header_samples;
    int trailer_samples;
    int chunk_size;
	int chunk_samples;
};

/*****************************************************************************
 * Your (optional) fill_wave callback will be called with "UINT8 *bytes" set
 * to one of these values if you should fill in the (optional) header or
 * trailer samples into the buffer.
 * Otherwise 'bytes' is a pointer to the chunk of data
 *****************************************************************************/
#define CODE_HEADER 	((UINT8*)-1)
#define CODE_TRAILER	((UINT8*)-2)

#define WAVE_STATUS_MOTOR_ENABLE	1
#define WAVE_STATUS_MUTED			2
#define WAVE_STATUS_MOTOR_INHIBIT	4
#define WAVE_STATUS_WRITE_ONLY		8

#ifdef __cplusplus
}
#endif

#endif

