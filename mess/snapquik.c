#include "snapquik.h"
#include "mess.h"

/* ----------------------------------------------------------------------- */

struct snapquick_info
{
	const struct IODevice *dev;
	int id;
	void *fp;
	struct snapquick_info *next;
};

static struct snapquick_info *snapquick_infolist;

static void snapquick_load(int arg)
{
	struct snapquick_info *si;
	snapquick_loadproc loadproc;
	
	si = (struct snapquick_info *) arg;
	loadproc = (snapquick_loadproc) si->dev->user1;
	loadproc(si->fp);
	image_unload(si->dev->type, si->id);
}

static int snapquick_init(int type, int id, void *fp, int open_mode)
{
	const struct IODevice *dev;
	struct snapquick_info *si;
	double delay;

	if (fp)
	{
		si = (struct snapquick_info *) image_malloc(type, id, sizeof(struct snapquick_info));
		if (!si)
			return INIT_FAIL;

		dev = device_find(Machine->gamedrv, type);
		assert(dev);

		si->dev = dev;
		si->id = id;
		si->fp = fp;
		si->next = snapquick_infolist;
		snapquick_infolist = si;

		delay = ((double) (UINT32) dev->user2) / 65536.0;

		timer_set(delay, (int) si, snapquick_load);
	}
	return INIT_PASS;
}

static void snapquick_exit(int type, int id)
{
	struct snapquick_info **si = &snapquick_infolist;

	while(*si && ((*si)->dev->type != type) && ((*si)->id != id))
		si = &(*si)->next;
	if (*si)
		*si = (*si)->next;
}

/* ----------------------------------------------------------------------- */

static int snapshot_init(int id, void *fp, int open_mode)
{
	return snapquick_init(IO_SNAPSHOT, id, fp, open_mode);
}

static void snapshot_exit(int id)
{
	snapquick_exit(IO_SNAPSHOT, id);
}

static int quickload_init(int id, void *fp, int open_mode)
{
	return snapquick_init(IO_QUICKLOAD, id, fp, open_mode);
}

static void quickload_exit(int id)
{
	snapquick_exit(IO_QUICKLOAD, id);
}

const struct IODevice *snapquick_specify(struct IODevice *iodev, int type,
	const char *file_extensions, snapquick_loadproc loadproc, double delay)
{
	memset(iodev, 0, sizeof(*iodev));
	iodev->type = type;
	iodev->count = 1;
	iodev->file_extensions = file_extensions;
	iodev->reset_depth = IO_RESET_CPU;
	iodev->open_mode = OSD_FOPEN_READ;
	iodev->init = (type == IO_SNAPSHOT) ? snapshot_init : quickload_init;
	iodev->exit = (type == IO_SNAPSHOT) ? snapshot_exit : quickload_exit;
	iodev->user1 = (void *) loadproc;
	iodev->user2 = (void *) (UINT32) (delay * 65536);
	return iodev;
}

