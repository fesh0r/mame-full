/*********************************************************************

	testimgt.c

	Imgtool testing code

*********************************************************************/

#include "testimgt.h"
#include "../imgtool/imgtool.h"
#include "../imgtool/modules.h"

#define VERBOSE_FILECHAIN	0

struct expected_dirent
{
	char filename[256];
	int size;
};

static imgtool_library *library;
static imgtool_image *image;
static struct expected_dirent entries[256];
static char filename_buffer[256];
static char driver_buffer[256];
static int entry_count;
static int failed;
static UINT64 recorded_freespace;
static option_resolution *opts;



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



static void createimage_start_handler(const char **attributes)
{
	const char *driver;
	const struct ImageModule *module;

	driver = find_attribute(attributes, "driver");
	if (!driver)
	{
		error_missingattribute("driver");
		return;
	}

	strncpy(driver_buffer, driver, sizeof(driver_buffer) / sizeof(driver_buffer[0]));
	
	/* does image creation support options? */
	module = imgtool_library_findmodule(library, driver);
	if (module && module->createimage_optguide && module->createimage_optspec)
		opts = option_resolution_create(module->createimage_optguide, module->createimage_optspec);
	else
		opts = NULL;
}



static void createimage_end_handler(const void *buffer, size_t size)
{
	imgtoolerr_t err;

	report_message(MSG_INFO, "Creating image (module '%s')", driver_buffer);

	err = img_create_byname(library, driver_buffer, tempfile_name(), opts, &image);
	if (opts)
	{
		option_resolution_close(opts);
		opts = NULL;
	}
	if (err)
	{
		report_imgtoolerr(err);
		return;
	}
}



static void createimage_param_handler(const char **attributes)
{
	const char *param_name;
	const char *param_value;

	if (!opts)
	{
		report_message(MSG_FAILURE, "Cannot specify creation options with this module");
		return;
	}

	param_name = find_attribute(attributes, "name");
	if (!param_name)
	{
		error_missingattribute("name");
		return;
	}

	param_value = find_attribute(attributes, "value");
	if (!param_value)
	{
		error_missingattribute("value");
		return;
	}

	option_resolution_add_param(opts, param_name, param_value);
}



static void file_start_handler(const char **attributes)
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
	
	if (!image)
		return;

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

	if (VERBOSE_FILECHAIN)
	{
		char buf[1024];
		err = img_getchain_string(image, filename_buffer, buf, sizeof(buf) / sizeof(buf[0]));
		if (err == IMGTOOLERR_SUCCESS)
			report_message(MSG_INFO, "Filechain '%s': %s", filename_buffer, buf);
	}
}



static void checkfile_end_handler(const void *buffer, size_t size)
{
	imgtoolerr_t err;
	imgtool_stream *stream;
	UINT64 stream_sz;
	const void *stream_ptr;

	stream = stream_open_mem(NULL, 0);
	if (!stream)
	{
		error_outofmemory();
		return;
	}

	err = img_readfile(image, filename_buffer, stream, NULL);
	if (err)
	{
		report_imgtoolerr(err);
		return;
	}

	stream_ptr = stream_getptr(stream);
	stream_sz = stream_size(stream);

	if ((size != stream_sz) || (memcmp(stream_ptr, buffer, size)))
	{
		failed = TRUE;
		report_message(MSG_FAILURE, "Failed file verification");
		return;
	}
}



static void deletefile_start_handler(const char **attributes)
{
	imgtoolerr_t err;
	const char *filename;

	filename = find_attribute(attributes, "name");
	if (!filename)
	{
		error_missingattribute("name");
		return;
	}

	err = img_deletefile(image, filename);
	if (err)
	{
		report_imgtoolerr(err);
		return;
	}
}



static void checkdirectory_start_handler(const char **attributes)
{
	const char *filename;

	filename = find_attribute(attributes, "path");
	if (filename)
		snprintf(filename_buffer, sizeof(filename_buffer) / sizeof(filename_buffer[0]), "%s", filename);
	else
		filename_buffer[0] = '\0';

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

	err = img_beginenum(image, filename_buffer, &imageenum);
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



static void createdirectory_start_handler(const char **attributes)
{
	imgtoolerr_t err;
	const char *dirname;

	dirname = find_attribute(attributes, "path");
	if (!dirname)
	{
		error_missingattribute("path");
		return;
	}

	err = img_createdir(image, dirname);
	if (err)
	{
		report_imgtoolerr(err);
		return;
	}
}



static void deletedirectory_start_handler(const char **attributes)
{
	imgtoolerr_t err;
	const char *dirname;

	dirname = find_attribute(attributes, "path");
	if (!dirname)
	{
		error_missingattribute("path");
		return;
	}

	err = img_deletedir(image, dirname);
	if (err)
	{
		report_imgtoolerr(err);
		return;
	}
}



static void recordfreespace_start_handler(const char **attributes)
{
	imgtoolerr_t err;

	err = img_freespace(image, &recorded_freespace);
	if (err)
	{
		report_imgtoolerr(err);
		return;
	}
}



static void checkfreespace_start_handler(const char **attributes)
{
	imgtoolerr_t err;
	UINT64 current_freespace;
	INT64 leaked_space;
	const char *verb;

	err = img_freespace(image, &current_freespace);
	if (err)
	{
		report_imgtoolerr(err);
		return;
	}

	if (recorded_freespace != current_freespace)
	{
		leaked_space = recorded_freespace - current_freespace;
		if (leaked_space > 0)
		{
			verb = "Leaked";
		}
		else
		{
			leaked_space = -leaked_space;
			verb = "Reverse leaked";
		}

		failed = TRUE;
		report_message(MSG_FAILURE, "%s %u bytes of space", verb, (unsigned int) leaked_space);
	}
}



void testimgtool_start_handler(const char **attributes)
{
	imgtoolerr_t err;

	failed = FALSE;
	recorded_freespace = ~0;

	if (!library)
	{
		err = imgtool_create_cannonical_library(FALSE, &library);
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

static const struct messtest_tagdispatch createimage_dispatch[] =
{
	{ "param",			DATA_NONE,		createimage_param_handler },
	{ NULL }
};

const struct messtest_tagdispatch testimgtool_dispatch[] =
{
	{ "createimage",		DATA_NONE,		createimage_start_handler,		createimage_end_handler, createimage_dispatch },
	{ "checkfile",			DATA_BINARY,	file_start_handler,				checkfile_end_handler },
	{ "checkdirectory",		DATA_NONE,		checkdirectory_start_handler,	checkdirectory_end_handler, checkdirectory_dispatch },
	{ "putfile",			DATA_BINARY,	file_start_handler,				putfile_end_handler },
	{ "deletefile",			DATA_NONE,		deletefile_start_handler,		NULL },
	{ "createdirectory",	DATA_NONE,		createdirectory_start_handler,	NULL },
	{ "deletedirectory",	DATA_NONE,		deletedirectory_start_handler,	NULL },
	{ "recordfreespace",	DATA_NONE,		recordfreespace_start_handler,	NULL },
	{ "checkfreespace",		DATA_NONE,		checkfreespace_start_handler,	NULL },
	{ NULL }
};



