/* */
#include "devices/cassette.h"
#include "image.h"

#if HAS_WAVE

int cassette_init(mess_image *img, mame_file *file, int open_mode, const struct cassette_args *args)
{
	struct wave_args_legacy wa;

	if (file == NULL)
	{	/* no cassette */
		memset(&wa, 0, sizeof(&wa));

		if (device_open(img, 0, &wa))
			return INIT_FAIL;

        device_status(img, args->initial_status);
		return INIT_PASS;
	}

	if (file)
	{
		if (! is_effective_mode_create(open_mode))
		{	/* existing file */
			memset(&wa, 0, sizeof(&wa));

			if (args->calc_chunk_info) {
				args->calc_chunk_info(file, &wa.chunk_size, &wa.chunk_samples);
			}
			else {
				wa.chunk_size = args->chunk_size;
				wa.chunk_samples = args->chunk_samples;
			}

			wa.file = file;
			wa.smpfreq = args->input_smpfreq ? args->input_smpfreq : Machine->sample_rate;
			wa.fill_wave = args->fill_wave;
			wa.header_samples = args->header_samples;
			wa.trailer_samples = args->trailer_samples;

			if (device_open(img, 0, &wa))
				return INIT_FAIL;

	        device_status(img, args->initial_status);
			return INIT_PASS;
		}
		else
		{	/* new file */
			memset(&wa, 0, sizeof(&wa));

			wa.file = file;
			wa.smpfreq = args->create_smpfreq ? args->create_smpfreq : Machine->sample_rate;
			if (device_open(img, 1, &wa))
				return INIT_FAIL;

	        device_status(img, args->initial_status);
			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

void cassette_exit(mess_image *img)
{
	device_close(img);
}

#endif /* HAS_WAVE */

