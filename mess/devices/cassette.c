/*********************************************************************

	cassette.c

	MESS interface to the cassette image abstraction code

*********************************************************************/

#include "devices/cassette.h"
#include "formats/cassimg.h"

#define CASSETTE_TAG		"cassette"
#define ANIMATION_FPS		4
#define ANIMATION_FRAMES	4

#define VERBOSE				0


/* from devices/mflopimg.c */
extern struct io_procs mess_ioprocs;
extern void specify_extension(char *extbuf, size_t extbuflen, const char *extension);


struct mess_cassetteimg
{
	cassette_image *cassette;
	cassette_state state;
	double position;
	double position_time;
	INT32 value;
};


static struct mess_cassetteimg *get_cassimg(mess_image *image)
{
	return image_lookuptag(image, CASSETTE_TAG);
}



static cassette_state get_default_state(const struct IODevice *dev)
{
	assert(dev->type == IO_CASSETTE);
	return (cassette_state) dev->user2;
}



/*********************************************************************
	cassette IO
*********************************************************************/

static int cassette_is_motor_on(mess_image *cassette)
{
	cassette_state state;
	state = cassette_get_state(cassette);
	if ((state & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED)
		return FALSE;
	if ((state & CASSETTE_MASK_MOTOR) != CASSETTE_MOTOR_ENABLED)
		return FALSE;
	return TRUE;
}



static void cassette_update(mess_image *cassette)
{
	struct mess_cassetteimg *tag;
	double cur_time;
	double new_position;

	cur_time = timer_get_time();
	tag = get_cassimg(cassette);

	if (cassette_is_motor_on(cassette) && tag->cassette)
	{
		new_position = tag->position + (cur_time - tag->position_time);

		switch(tag->state & CASSETTE_MASK_UISTATE) {
		case CASSETTE_RECORD:
			cassette_put_sample(tag->cassette, 0, tag->position, new_position - tag->position, tag->value);
			break;

		case CASSETTE_PLAY:
			cassette_get_sample(tag->cassette, 0, new_position, 0.0, &tag->value);
			break;
		}
		tag->position = new_position;
	}
	tag->position_time = cur_time;
}



cassette_state cassette_get_state(mess_image *cassette)
{
	return get_cassimg(cassette)->state;
}



void cassette_change_state(mess_image *cassette, cassette_state state, cassette_state mask)
{
	struct mess_cassetteimg *tag;
	cassette_state new_state;

	tag = get_cassimg(cassette);
	new_state = tag->state;
	new_state &= ~mask;
	new_state |= (state & mask);

	if (new_state != tag->state)
	{
		cassette_update(cassette);
		tag->state = new_state;
	}
}



void cassette_set_state(mess_image *cassette, cassette_state state)
{
	cassette_change_state(cassette, state, ~0);
}



double cassette_input(mess_image *cassette)
{
	INT32 sample;
	double double_value;
	struct mess_cassetteimg *tag;

	cassette_update(cassette);
	tag = get_cassimg(cassette);
	sample = tag->value;
	double_value = sample / ((double) 0x7FFFFFFF);

#if VERBOSE
	logerror("cassette_input(): time_index=%g value=%g\n", tag->position, double_value);
#endif

	return double_value;
}



void cassette_output(mess_image *cassette, double value)
{
	struct mess_cassetteimg *tag;
	tag = get_cassimg(cassette);
	if (((tag->state & CASSETTE_MASK_UISTATE) == CASSETTE_RECORD) && (tag->value != value))
	{
		cassette_update(cassette);

		value = MIN(value, 1.0);
		value = MAX(value, -1.0);

		tag->value = (INT32) (value * 0x7FFFFFFF);
	}
}



cassette_image *cassette_get_image(mess_image *cassette)
{
	struct mess_cassetteimg *tag;
	tag = get_cassimg(cassette);
	return tag->cassette;
}



double cassette_get_position(mess_image *cassette)
{
	double position;
	struct mess_cassetteimg *tag;

	tag = get_cassimg(cassette);
	position = tag->position;

	if (cassette_is_motor_on(cassette))
		position += timer_get_time() - tag->position_time;
	return position;
}



double cassette_get_length(mess_image *cassette)
{
	struct mess_cassetteimg *tag;
	struct CassetteInfo info;

	tag = get_cassimg(cassette);
	cassette_get_info(tag->cassette, &info);
	return ((double) info.sample_count) / info.sample_frequency;
}



void cassette_seek(mess_image *cassette, double time, int origin)
{
	struct mess_cassetteimg *tag;

	cassette_update(cassette);

	switch(origin) {
	case SEEK_SET:
		break;

	case SEEK_END:
		time += cassette_get_length(cassette);
		break;

	case SEEK_CUR:
		time += cassette_get_position(cassette);
		break;
	}

	/* clip position into legal bounds */
	tag = get_cassimg(cassette);
	tag->position = time;
}



/*********************************************************************
	cassette device init/load/unload/specify
*********************************************************************/

static DEVICE_INIT(cassette)
{
	const struct IODevice *dev;

	if (!image_alloctag(image, CASSETTE_TAG, sizeof(struct mess_cassetteimg)))
		return INIT_FAIL;

	/* set to default state */
	dev = device_find(Machine->gamedrv, IO_CASSETTE);
	get_cassimg(image)->state = get_default_state(dev);

	return INIT_PASS;
}



static DEVICE_LOAD(cassette)
{
	casserr_t err;
	int cassette_flags;
	struct mess_cassetteimg *tag;
	const struct IODevice *dev;
	const struct CassetteFormat **formats;
	const struct CassetteOptions *create_opts;
	const char *extension;
	int is_writable;

	tag = get_cassimg(image);

	/* figure out the cassette format */
	dev = device_find(Machine->gamedrv, IO_CASSETTE);
	assert(dev);
	formats = (const struct CassetteFormat **) dev->user1;

	if (image_has_been_created(image))
	{
		/* creating an image */
		create_opts = (const struct CassetteOptions *) dev->user3;
		err = cassette_create(file, &mess_ioprocs, &wavfile_format, create_opts, CASSETTE_FLAG_READWRITE|CASSETTE_FLAG_SAVEONEXIT, &tag->cassette);
		if (err)
			goto error;
	}
	else
	{
		/* opening an image */
		do
		{
			is_writable = image_is_writable(image); 
			cassette_flags = is_writable ? (CASSETTE_FLAG_READWRITE|CASSETTE_FLAG_SAVEONEXIT) : CASSETTE_FLAG_READONLY;
			extension = image_filetype(image);
			err = cassette_open_choices(file, &mess_ioprocs, extension, formats, cassette_flags, &tag->cassette);

			/* this is kind of a hack */
			if (err && is_writable)
				image_make_readonly(image);
		}
		while(err && is_writable);

		if (err)
			goto error;
	}

	/* set to default state, but only change the UI state */
	cassette_change_state(image, get_default_state(dev), CASSETTE_MASK_UISTATE);

	/* reset the position */
	tag->position = 0.0;
	tag->position_time = timer_get_time();

	return INIT_PASS;

error:
	return INIT_FAIL;
}



static DEVICE_UNLOAD(cassette)
{
	struct mess_cassetteimg *tag;
	
	tag = get_cassimg(image);

	/* if we are recording, write the value to the image */
	if ((tag->state & CASSETTE_MASK_UISTATE) == CASSETTE_RECORD)
		cassette_update(image);

	/* close out the cassette */
	cassette_close(tag->cassette);
	tag->cassette = NULL;

	/* set to default state, but only change the UI state */
	cassette_change_state(image, CASSETTE_STOPPED, CASSETTE_MASK_UISTATE);
}



/*
	display a small tape icon, with the current position in the tape image
*/
static void device_display_cassette(mess_image *image, struct mame_bitmap *bitmap)
{
	char buf[65];
	int x, y, n;
	double position, length;
	cassette_state uistate;

	/* abort if we should not be showing the image */
	if (!image_exists(image))
		return;
	if (!cassette_is_motor_on(image))
		return;

	/* figure out where we are in the cassette */
	position = cassette_get_position(image);
	length = cassette_get_length(image);
	uistate = cassette_get_state(image) & CASSETTE_MASK_UISTATE;

	/* choose a location on the screen */
	x = image_index_in_devtype(image) * Machine->uifontwidth * 16 + 1;
	y = Machine->uiheight - 9;

	/* choose which frame of the animation we are at */
	n = ((int) position / ANIMATION_FPS) % ANIMATION_FRAMES;

	/* character pairs 2-3, 4-5, 6-7, 8-9 form little tape cassette images */
	snprintf(buf, sizeof(buf) / sizeof(buf[0]), "%c%c %c %02d:%02d (%04d) [%02d:%02d (%04d)]",
		n * 2 + 2,								/* cassette icon left */
		n * 2 + 3,								/* cassette icon right */
		(uistate == CASSETTE_PLAY) ? 16 : 14,	/* play or record icon */
		((int) position / 60),
		((int) position % 60),
		(int) position,
		((int) length / 60),
		((int) length % 60),
		(int) length);

	ui_text(bitmap, buf, x, y);
}


const struct IODevice *cassette_device_specify(
	struct IODevice *iodev,
	char *extbuf,
	size_t extbuflen,
	int count,
	const struct CassetteFormat **formats,
	const struct CassetteOptions *casopts,
	cassette_state default_state)
{
	int i;

	assert(count);
	if (iodev->count == 0)
	{
		/* specify if default */
		if (default_state == (cassette_state) -1)
			default_state = CASSETTE_PLAY;

		if (formats == NULL)
			formats = cassette_default_formats;

		for (i = 0; formats[i]; i++)
			specify_extension(extbuf, extbuflen, formats[i]->extensions);

		memset(iodev, 0, sizeof(*iodev));
		iodev->type = IO_CASSETTE;
		iodev->count = count;
		iodev->file_extensions = extbuf;
		iodev->flags = DEVICE_LOAD_RESETS_NONE;
		iodev->open_mode = OSD_FOPEN_RW_CREATE_OR_READ;
		iodev->init = device_init_cassette;
		iodev->load = device_load_cassette;
		iodev->unload = device_unload_cassette;
		iodev->display = device_display_cassette;
		iodev->user1 = (void *) formats;
		iodev->user2 = (void *) default_state;
		iodev->user3 = (void *) casopts;
	}
	return iodev;
}


