/***********************************************************************

	apf.c

	Functions to emulate general aspects of the machine (RAM, ROM,
	interrupts, I/O ports)

***********************************************************************/

#include "driver.h"
#include "includes/apf.h"
#include "devices/cassette.h"
#include "formats/apfapt.h"
#include "devices/basicdsk.h"
#include "image.h"

#if 0
int apf_cassette_init(mess_image *img, mame_file *fp, int open_mode)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, &args);
}
#endif

int apf_cassette_init(int id, mame_file *file, int effective_mode)
{
	struct wave_args_legacy wa;


	if (file == NULL)
		return INIT_PASS;


	if( file )
	{
		if (! is_effective_mode_create(effective_mode))
		{
			int apf_apt_size;

			/* get size of .tap file */
			apf_apt_size = mame_fsize(file);

			logerror("apf .apt size: %04x\n",apf_apt_size);

			if (apf_apt_size!=0)
			{
				UINT8 *apf_apt_data;

				/* allocate a temporary buffer to hold .apt image */
				/* this is used to calculate the number of samples that would be filled when this
				file is converted */
				apf_apt_data = (UINT8 *)malloc(apf_apt_size);

				if (apf_apt_data!=NULL)
				{
					/* number of samples to generate */
					int size_in_samples;

					/* read data into temporary buffer */
					mame_fread(file, apf_apt_data, apf_apt_size);

					/* calculate size in samples */
					size_in_samples = apf_cassette_calculate_size_in_samples(apf_apt_size, apf_apt_data);

					/* seek back to start */
					mame_fseek(file, 0, SEEK_SET);

					/* free temporary buffer */
					free(apf_apt_data);

					/* size of data in samples */
					logerror("size in samples: %d\n",size_in_samples);

					/* internal calculation used in wave.c:

					length =
						wa->header_samples +
						((mame_fsize(w->file) + wa->chunk_size - 1) / wa->chunk_size) * wa->chunk_samples +
						wa->trailer_samples;
					*/


					memset(&wa, 0, sizeof(&wa));
					wa.file = file;
					wa.chunk_size = apf_apt_size;
					wa.chunk_samples = size_in_samples;
					wa.smpfreq = APF_WAV_FREQUENCY;
					wa.fill_wave = apf_cassette_fill_wave;
					wa.header_samples = 0;
					wa.trailer_samples = 0;
					if( device_open(IO_CASSETTE,id,0,&wa) )
						return INIT_FAIL;

					return INIT_PASS;
				}

				return INIT_FAIL;
			}
		}
		/*else*/
		{
			memset(&wa, 0, sizeof(&wa));
			wa.file = file;
			wa.smpfreq = 22050;
			if( device_open(IO_CASSETTE,id,1,&wa) )
				return INIT_FAIL;

			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

/* 256 bytes per sector, single sided, single density, 40 track  */
int apfimag_floppy_init(mess_image *img, mame_file *fp, int open_mode)
{
	if (fp == NULL)
		return INIT_PASS;

	if (basicdsk_floppy_load(id, fp, open_mode)==INIT_PASS)
	{
		basicdsk_set_geometry(id, 40, 1, 8, 256, 1, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}
