#include "devices/snapquik.h"
#include "mess.h"

/* ----------------------------------------------------------------------- */

struct snapquick_info
{
	const struct IODevice *dev;
	mess_image *img;
	mame_file *fp;
	int file_size;
	struct snapquick_info *next;
};

static struct snapquick_info *snapquick_infolist;



static void snapquick_processsnapshot(int arg)
{
	struct snapquick_info *si;
	snapquick_loadproc loadproc;
	const char *file_type;
	
	si = (struct snapquick_info *) arg;
	loadproc = (snapquick_loadproc) si->dev->genf1;
	file_type = image_filetype(si->img);
	loadproc(si->fp, file_type, si->file_size);
	image_unload(si->img);
}



static DEVICE_LOAD(snapquick)
{
	const struct IODevice *dev;
	struct snapquick_info *si;
	double delay;
	int file_size;

	file_size = mame_fsize(file);
	if (file_size <= 0)
		return INIT_FAIL;

	si = (struct snapquick_info *) image_malloc(image, sizeof(struct snapquick_info));
	if (!si)
		return INIT_FAIL;

	dev = image_device(image);
	assert(dev);

	si->dev = dev;
	si->img = image;
	si->fp = file;
	si->file_size = file_size;
	si->next = snapquick_infolist;
	snapquick_infolist = si;

	delay = ((double) (UINT32) dev->user2) / 65536.0;

	timer_set(delay, (int) si, snapquick_processsnapshot);
	return INIT_PASS;
}



static DEVICE_UNLOAD(snapquick)
{
	struct snapquick_info **si = &snapquick_infolist;

	while(*si && ((*si)->img != image))
		si = &(*si)->next;
	if (*si)
		*si = (*si)->next;
}



/* ----------------------------------------------------------------------- */

static void snapquick_device_getinfo(struct IODevice *iodev, int type,
	snapquick_loadproc loadproc, double delay)
{
	iodev->type = type;
	iodev->count = 1;
	iodev->readable = 1;
	iodev->writeable = 0;
	iodev->creatable = 0;
	iodev->load = device_load_snapquick;
	iodev->unload = device_unload_snapquick;
	iodev->genf1 = (genf *) loadproc;
	iodev->user2 = (void *) (UINT32) (delay * 65536);
}



void snapshot_device_getinfo(struct IODevice *iodev,
	snapquick_loadproc loadproc, double delay)
{
	snapquick_device_getinfo(iodev, IO_SNAPSHOT, loadproc, delay);
}



void quickload_device_getinfo(struct IODevice *iodev,
	snapquick_loadproc loadproc, double delay)
{
	snapquick_device_getinfo(iodev, IO_QUICKLOAD, loadproc, delay);
}


