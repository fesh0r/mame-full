/*********************************************************************

	testimgt.c

	Imgtool testing code

*********************************************************************/

#include "testimgt.h"
#include "../imgtool/imgtool.h"
#include "../imgtool/modules.h"

struct expected_dirent
{
	char filename[256];
	int size;
};

static imgtool_library *library;
static imgtool_image *image;
static struct expected_dirent entries[256];
static char filename_buffer[256];
static int entry_count;
static int failed;



static const char *tempfile_name(void)
{
	static char buffer[256];
	strcpy(buffer, "imgtool.tmp");
	make_filename_temporary(buffer, sizeof(buffer));
	return buffer;
}



static void report_imgtoolerr(imgtoolerr_t err)
{
	const char *msg;
	failed = TRUE;
	msg = imgtool_error(err);
	report_message(MSG_FAILURE, msg);
}



static void createimage_handler(const char **attributes)
{
	imgtoolerr_t err;
	const char *driver;

	driver = find_attribute(attributes, "driver");
	if (!driver)
	{
		error_missingattribute("driver");
		return;
	}

	report_message(MSG_INFO, "Creating image (module '%s')", driver);

	err = img_create_byname(library, driver, tempfile_name(), NULL, &image);
	if (err)
	{
		report_imgtoolerr(err);
		return;
	}
}



static void putfile_start_handler(const char **attributes)
{
	const char *filename;

	filename = find_attribute(attributes, "name");
	if (!filename)
	{
		error_missingattribute("name");
		return;
	}

	snprintf(filename_buffer, sizeof(filename_buffer) / sizeof(filename_buffer[0]), "%s", filename);
}



static void putfile_end_handler(const void *buffer, size_t size)
{
	imgtoolerr_t err;
	imgtool_stream *stream;

	stream = stream_open_mem((void *) buffer, size);
	if (!stream)
	{
		error_outofmemory();
		return;
	}

	err = img_writefile(image, filename_buffer, stream, NULL, NULL);
	if (err)
	{
		report_imgtoolerr(err);
		return;
	}
}



static void checkdirectory_start_handler(const char **attributes)
{
	memset(&entries, 0, sizeof(entries));
	entry_count = 0;
}



static void checkdirectory_entry_handler(const char **attributes)
{
	const char *name;
	const char *size;
	struct expected_dirent *entry;

	if (entry_count >= sizeof(entries) / sizeof(entries[0]))
	{
		failed = TRUE;
		report_message(MSG_FAILURE, "Too many directory entries");
		return;
	}

	name = find_attribute(attributes, "name");
	size = find_attribute(attributes, "size");

	entry = &entries[entry_count++];

	if (name)
		snprintf(entry->filename, sizeof(entry->filename) / sizeof(entry->filename[0]), "%s", name);
	entry->size = size ? atoi(size) : -1;
}



static void append_to_list(char *buf, size_t buflen, const char *entry)
{
	size_t pos;
	pos = strlen(buf);
	snprintf(buf + pos, buflen - pos,
		(pos > 0) ? ", %s" : "%s", entry);
}



static void checkdirectory_end_handler(const void *buffer, size_t size)
{
	imgtoolerr_t err = IMGTOOLERR_SUCCESS;
	imgtool_imageenum *imageenum;
	imgtool_dirent ent;
	char filename_buffer[1024];
	char expected_listing[128];
	char actual_listing[128];
	int i, actual_count;
	int mismatch;

	if (!image)
	{
		failed = TRUE;
		report_message(MSG_FAILURE, "Image not loaded");
		return;
	}

	/* build expected listing string */
	expected_listing[0] = '\0';
	for (i = 0; i < entry_count; i++)
	{
		append_to_list(expected_listing,
			sizeof(expected_listing) / sizeof(expected_listing[0]),
			entries[i].filename);
	}

	/* now enumerate though listing */
	actual_count = 0;
	actual_listing[0] = '\0';
	mismatch = FALSE;

	memset(&ent, 0, sizeof(ent));
	ent.filename = filename_buffer;
	ent.filename_len = sizeof(filename_buffer) / sizeof(filename_buffer[0]);

	err = img_beginenum(image, NULL, &imageenum);
	if (err)
		goto done;

	i = 0;
	do
	{
		err = img_nextenum(imageenum, &ent);
		if (err)
			goto done;

		if (!ent.eof)
		{
			append_to_list(actual_listing,
				sizeof(actual_listing) / sizeof(actual_listing[0]),
				ent.filename);

			if (i < entry_count && (strcmp(ent.filename, entries[i].filename)))
				mismatch = TRUE;
			i++;
		}
	}
	while(!ent.eof);

	if (i != entry_count)
		mismatch = TRUE;

	if (mismatch)
	{
		failed = TRUE;
		report_message(MSG_FAILURE, "File listing mismatch: {%s} expected {%s}",
			actual_listing, expected_listing);
		goto done;
	}

done:
	if (imageenum)
		img_closeenum(imageenum);
	if (err)
		report_imgtoolerr(err);
}



void testimgtool_start_handler(const char **attributes)
{
	imgtoolerr_t err;

	failed = FALSE;

	if (!library)
	{
		err = imgtool_create_cannonical_library(&library);
		if (err)
		{
			report_imgtoolerr(err);
			return;
		}
	}

	report_testcase_begin(find_attribute(attributes, "name"));
}



void testimgtool_end_handler(const void *buffer, size_t size)
{
	report_testcase_ran(failed);

	if (image)
	{
		img_close(image);
		image = NULL;
	}
}



static const struct messtest_tagdispatch checkdirectory_dispatch[] =
{
	{ "entry",			DATA_NONE,		checkdirectory_entry_handler },
	{ NULL }
};

const struct messtest_tagdispatch testimgtool_dispatch[] =
{
	{ "createimage",	DATA_NONE,		createimage_handler,			NULL },
	{ "checkdirectory",	DATA_NONE,		checkdirectory_start_handler,	checkdirectory_end_handler, checkdirectory_dispatch },
	{ "putfile",		DATA_BINARY,	putfile_start_handler,			putfile_end_handler },
	{ NULL }
};



