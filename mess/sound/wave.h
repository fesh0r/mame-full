#ifndef WAVE_H_INCLUDED
#define WAVE_H_INCLUDED

#define MAX_WAVE 4

/*****************************************************************************
 *	CassetteWave interface
 *****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "messdrv.h"

struct Wave_interface {
	int num;
	int mixing_level[MAX_WAVE];
};

extern int wave_sh_start(const struct MachineSound *msound);
extern void wave_sh_stop(void);
extern void wave_sh_update(void);

/*****************************************************************************
 * functions for the IODevice entry IO_CASSETTE. Example for the macro
 * IO_CASSETTE_WAVE(1,"wav\0cas\0",mycas_id,mycas_init,mycas_exit)
 *****************************************************************************/

extern int wave_init(int id, const char *name);
extern void wave_exit(int id);
extern const void *wave_info(int id, int whatinfo);
extern int wave_open(int id, int mode, void *args);
extern void wave_close(int id);
extern int wave_status(int id, int newstatus);
extern int wave_seek(int id, int offset, int whence);
extern int wave_tell(int id);
extern int wave_input(int id);
extern void wave_output(int id, int data);
extern int wave_input_chunk(int id, void *dst, int chunks);
extern int wave_output_chunk(int id, void *src, int chunks);

extern void cassette_exit(int id);

extern void wave_specify(struct IODevice *iodev, int count, char *actualext, const char *fileext,
	int (*init)(int id, mame_file *fp, int open_mode), void (*exit_)(int id));

#define CONFIG_DEVICE_CASSETTEX(count,fileext,init,exit)					\
	if (cfg->device_num-- == 0)												\
	{																		\
		static struct IODevice iodev;										\
		static char actualext[sizeof(fileext)+4];							\
		wave_specify(&iodev, (count), actualext, (fileext), (init), (exit));\
		cfg->dev = &iodev;													\
	}																		\

#define CONFIG_DEVICE_CASSETTE(count,fileext,init)	\
	CONFIG_DEVICE_CASSETTEX((count), (fileext), (init), cassette_exit)

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
    void *file;
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

