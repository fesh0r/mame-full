/* */
#include "cassette.h"
#include "image.h"

int cassette_init(int id, const struct cassette_args *args)
{
	void *file;
	struct wave_args wa;

	if (!image_exists(IO_CASSETTE, id))
	{	/* no cassette */
		memset(&wa, 0, sizeof(&wa));

		if (device_open(IO_CASSETTE, id, 0, &wa))
			return INIT_FAIL;

        device_status(IO_CASSETTE, id, args->initial_status);
		return INIT_PASS;
	}

	/* Try to open existing file */
	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);
	if (file)
	{
		memset(&wa, 0, sizeof(&wa));

		if (args->calc_chunk_info) {
			args->calc_chunk_info(file, &wa.chunk_size, &wa.chunk_samples);
		}
		else {
			wa.chunk_size = args->chunk_size;
			wa.chunk_samples = args->chunk_samples;
		}

		wa.file = file;
		wa.smpfreq = args->input_smpfreq;
		wa.fill_wave = args->fill_wave;
		wa.header_samples = args->header_samples;
		wa.trailer_samples = args->trailer_samples;
		wa.display = 1;

		if (device_open(IO_CASSETTE, id, 0, &wa))
			return INIT_FAIL;

        device_status(IO_CASSETTE, id, args->initial_status);
		return INIT_PASS;
	}

	/* No file?  Can I create it? */
	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_RW_CREATE);
	if (file) {
		memset(&wa, 0, sizeof(&wa));

		wa.file = file;
		wa.display = 1;
		wa.smpfreq = args->create_smpfreq
			;
		if (device_open(IO_CASSETTE, id, 1, &wa))
			return INIT_FAIL;

        device_status(IO_CASSETTE, id, args->initial_status);
		return INIT_PASS;
    }
	return INIT_FAIL;
}

void cassette_exit(int id)
{
	if (image_exists(IO_CASSETTE, id))
		device_close(IO_CASSETTE, id);
}

