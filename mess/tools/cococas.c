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

static int cococas_nextfile(IMAGE *img, imgtool_dirent *ent)
{
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

