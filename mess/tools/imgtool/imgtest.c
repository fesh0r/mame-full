#include <assert.h>
#include <string.h>
#include "imgtool.h"

#ifdef MAME_DEBUG

static const char *testimage = "test.img";
/* static const char *testfile = "test.txt"; */

/* ----------------------------------------------------------------------- */

static void *create_buffer(size_t len)
{
	char *buf;
	size_t testlen, i;
	static const char *teststring = "All your base are belong to us";

	testlen = strlen(teststring);
	buf = malloc(len);
	if (!buf)
		return NULL;

	for (i = 0; i < len; i++) {
		buf[i] = teststring[i % testlen];
	}
	return buf;
}

static imgtoolerr_t get_freespace(const struct ImageModule *module, const char *imagename, UINT64 *freespace)
{
	imgtoolerr_t err;
	imgtool_image *img = NULL;

	err = img_open(module, imagename, OSD_FOPEN_RW, &img);
	if (err)
		goto done;

	err = img_freespace(img, freespace);
	if (err)
		goto done;

done:
	if (img)
		img_close(img);
	return err;
}

static imgtoolerr_t create_file_in_image(const struct ImageModule *module, const char *imagename, const char *fname, void *testbuf, size_t filesize)
{
	imgtoolerr_t err;
	imgtool_image *img = NULL;
	imgtool_stream *s = NULL;

	/* Create a test stream */
	s = stream_open_mem(testbuf, filesize);
	if (!s)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	err = img_open(module, imagename, OSD_FOPEN_RW, &img);
	if (err)
		goto done;

	err = img_writefile(img, fname, s, NULL, NULL);
	if (err)
		goto done;

done:
	if (img)
		img_close(img);
	return err;
}


static int verify_file_in_image(const struct ImageModule *module, const char *imagename, const char *fname, void *testbuf, size_t filesize)
{
	int err;
	imgtool_image *img = NULL;
	imgtool_stream *s = NULL;
	void *readbuf;
	int results;

	readbuf = malloc(filesize);
	if (!readbuf)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	/* Create a test stream */
	s = stream_open_mem(readbuf, filesize);
	if (!s)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	err = img_open(module, imagename, OSD_FOPEN_READ, &img);
	if (err)
		goto done;

	err = img_readfile(img, fname, s, NULL);
	if (err)
		goto done;

	results = memcmp(readbuf, testbuf, filesize);
	assert(!results);
	if (results)
	{
		err = -1;
		goto done;
	}

done:
	if (img)
		img_close(img);
	if (s)
		stream_close(s);
	return err;
}


static int delete_file_in_image(const struct ImageModule *module, const char *imagename, const char *fname)
{
	int err;
	imgtool_image *img = NULL;

	err = img_open(module, imagename, OSD_FOPEN_RW, &img);
	if (err)
		goto done;

	err = img_deletefile(img, fname);
	if (err)
		goto done;

done:
	if (img)
		img_close(img);
	return err;
}

static imgtoolerr_t count_files(const struct ImageModule *module, const char *imagename, int *totalfiles)
{
	imgtoolerr_t err;
	imgtool_image *img = NULL;

	err = img_open(module, imagename, OSD_FOPEN_RW, &img);
	if (err)
		goto done;

	err = img_countfiles(img, totalfiles);
	if (err)
		goto done;

done:
	if (img)
		img_close(img);
	return err;
}

static imgtoolerr_t assert_file_count(const struct ImageModule *module, const char *imagename, int filecount)
{
	imgtoolerr_t err;
	int totalfiles;

	err = count_files(module, imagename, &totalfiles);
	if (err)
		return err;

	assert(filecount == totalfiles);
	return 0;
}

static int assert_file_size(const struct ImageModule *module, const char *imagename, const char *fname, int assertedsize)
{
	UINT64 filesize;
	int err;
	imgtool_image *img = NULL;

	err = img_open(module, imagename, OSD_FOPEN_RW, &img);
	if (err)
		goto done;

	err = img_filesize(img, fname, &filesize);
	if (err)
		goto done;

	assert(filesize == assertedsize);

done:
	if (img)
		img_close(img);
	return err;
}

#define NUM_FILES	10

static int calculate_filesize(int i)
{
	return 5*(i+1)*(i+1)*(i+1)*(i+1);
}

static imgtoolerr_t internal_test(const struct ImageModule *module)
{
	imgtoolerr_t err;
	int i;
	UINT64 freespace, freespace2;
	int filesize;
	void *testbuf;
	char buf[2048];

	/* Create the buffer */
	testbuf = create_buffer(calculate_filesize(NUM_FILES-1));
	if (!testbuf)
	{
		err = IMGTOOLERR_OUTOFMEMORY;
		goto done;
	}

	/* Create the image */
	err = img_create(module, testimage, NULL);
	if (err)
		goto done;

	/* Calculate free space */
	err = get_freespace(module, testimage, &freespace);
	if (err)
		return err;

	/* Add files */
	for (i = 0; i < NUM_FILES; i++)
	{
		/* Must be n files */
		err = assert_file_count(module, testimage, i);
		if (err)
			goto done;

		/* Put a file on the image */
		sprintf(buf, "test%i.txt", i);
		filesize = calculate_filesize(i);
		err = create_file_in_image(module, testimage, buf, testbuf, filesize);
		if (err)
			goto done;

		/* Must be n+1 files */
		err = assert_file_count(module, testimage, i+1);
		if (err)
			goto done;

		/* Verify the file size */
		err = assert_file_size(module, testimage, buf, filesize);
		if (err)
			goto done;
	}

	/* Verify that the files are intact */
	for (i = 0; i < NUM_FILES; i++)
	{
		sprintf(buf, "test%i.txt", i);
		filesize = calculate_filesize(i);
		err = verify_file_in_image(module, testimage, buf, testbuf, filesize);
		if (err)
			goto done;
	}

	/* Delete files */
	for (i = NUM_FILES-1; i >= 0; i--)
	{
		/* Must be n+1 files */
		err = assert_file_count(module, testimage, i+1);
		if (err)
			goto done;

		/* Delete a file on the image */
		sprintf(buf, "test%i.txt", i);
		err = delete_file_in_image(module, testimage, buf);
		if (err)
			goto done;

		/* Must be n files */
		err = assert_file_count(module, testimage, i);
		if (err)
			goto done;
	}

	/* Verify that we freed all allocated blocks */
	err = get_freespace(module, testimage, &freespace2);
	if (err)
		goto done;
	assert(freespace == freespace2);

done:
	if (testbuf)
		free(testbuf);
	return err;
}

/* ----------------------------------------------------------------------- */

imgtoolerr_t imgtool_test(imgtool_library *library, const struct ImageModule *module)
{
	imgtoolerr_t err;

	if (module)
	{
		err = internal_test(module);
		if (err)
			return err;
	}
	else
	{
		module = imgtool_library_findmodule(library, NULL);
		while(module)
		{
			err = internal_test(module);
			if (err)
				return err;
			module = module->next;
		}
	}
	return 0;
}

imgtoolerr_t imgtool_test_byname(imgtool_library *library, const char *modulename)
{
	const struct ImageModule *module;

	if (modulename)
	{
		module = imgtool_library_findmodule(library, modulename);
		if (!module)
			return IMGTOOLERR_MODULENOTFOUND | IMGTOOLERR_SRC_MODULE;
	}
	else
	{
		module = NULL;
	}
	return imgtool_test(library, module);
}


#endif /* MAME_DEBUG */
