#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "imgtool.h"

static int cococas_init(STREAM *f, IMAGE **outimg);
static void cococas_exit(IMAGE *img);
static int cococas_nextfile(IMAGE *img, imgtool_dirent *ent);
static int cococas_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int cococas_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const file_options *options);
static int cococas_deletefile(IMAGE *img, const char *fname);

static UINT8 blockheader[] = { 0x55, 0x3C };

WAVEMODULE(
	cococas,
	"Tandy CoCo Cassette",
	"wav",
	1200, 1800, 2400,		/* 0=1200 Hz, 1=2400 Hz, threshold=1800 Hz */
	WAVEIMAGE_LSB_FIRST,
	blockheader, sizeof(blockheader) / sizeof(blockheader[0]),
	cococas_nextfile,				/* enumerate next */
	cococas_readfile,				/* read file */
	cococas_writefile,				/* write file */
	cococas_deletefile,				/* delete file */
)

typedef struct {
	UINT8 type;
	UINT8 length;
	UINT8 data[255];
} casblock;

/* from rsdos.c */
extern void rtrim(char *buf);

static int readblock(IMAGE *img, casblock *blk)
{
	int err;
	int i;
	UINT8 sum = 0;
	UINT8 actualsum;

	err = imgwave_forward(img);
	if (err)
		return err;

	err = imgwave_read(img, &blk->type, 2);
	if (err)
		return err;
	sum += blk->type;
	sum += blk->length;

	err = imgwave_read(img, blk->data, blk->length);
	if (err)
		return err;

	for (i = 0; i < blk->length; i++)
		sum += blk->data[i];

	err = imgwave_read(img, &actualsum, 1);
	if (err)
		return err;

	if (sum != actualsum)
		return IMGTOOLERR_CORRUPTIMAGE;

	return 0;
}

static int cococas_nextfile(IMAGE *img, imgtool_dirent *ent)
{
	int err;
	casblock blk;
	char fname[9];

	do {
		err = readblock(img, &blk);
		if (err)
			return err;
	}
	while(blk.type != 0);

	fname[8] = '\0';
	memcpy(fname, blk.data, 8);
	rtrim(fname);



	return IMGTOOLERR_UNIMPLEMENTED;
}

static int cococas_readfile(IMAGE *img, const char *fname, STREAM *destf)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

static int cococas_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const file_options *options)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

static int cococas_deletefile(IMAGE *img, const char *fname)
{
	return IMGTOOLERR_UNIMPLEMENTED;
}

