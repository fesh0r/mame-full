#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "imgtool.h"

static int cococas_nextfile(IMAGE *img, imgtool_dirent *ent);
static int cococas_readfile(IMAGE *img, STREAM *destf);

static UINT8 blockheader[] = { 0x55, 0x3C };

WAVEMODULE(
	cococas,
	"Tandy CoCo Cassette",
	"wav",
	1200, 1600, 2400,		/* 0=1200 Hz, 1=2400 Hz, threshold=1600 Hz */
	WAVEIMAGE_LSB_FIRST,
	blockheader, sizeof(blockheader) / sizeof(blockheader[0]),
	cococas_nextfile,				/* enumerate next */
	cococas_readfile				/* read file */
)

typedef struct {
	UINT8 type;
	UINT8 length;
	UINT8 data[255];
} casblock;

enum {
	COCOCAS_FILETYPE_BASIC = 0,
	COCOCAS_FILETYPE_DATA = 1,
	COCOCAS_FILETYPE_BIN = 2
};

enum {
	COCOCAS_BLOCKTYPE_FILENAME = 0,
	COCOCAS_BLOCKTYPE_DATA = 1,
	COCOCAS_BLOCKTYPE_EOF = 0xff
};

/* from rsdos.c */
extern void rtrim(char *buf);

static int readblock(IMAGE *img, casblock *blk)
{
	int err;
	int i;
	UINT8 sum = 0;
	UINT8 actualsum;

	/* Move forward until we hit header */
	err = imgwave_forward(img);
	if (err)
		return err;

	/* Read in the block type and length */
	err = imgwave_read(img, &blk->type, 2);
	if (err)
		return err;
	sum += blk->type;
	sum += blk->length;

	/* Read in the block data */
	err = imgwave_read(img, blk->data, blk->length);
	if (err)
		return err;

	/* Calculate checksum */
	for (i = 0; i < blk->length; i++)
		sum += blk->data[i];

	/* Read the checksum */
	err = imgwave_read(img, &actualsum, 1);
	if (err)
		return err;

	if (sum != actualsum)
		return IMGTOOLERR_CORRUPTIMAGE;

	return 0;
}

static int cococas_nextfile(IMAGE *img, imgtool_dirent *ent)
{
	int err, filesize;
	casblock blk;
	char fname[9];

	/* Read directory block */
	err = readblock(img, &blk);
	if (err == IMGTOOLERR_INPUTPASTEND) {
		ent->eof = 1;
		return 0;
	}
	if (err)
		return err;
	
	/* If block is not a filename block, fail */
	if (blk.type != COCOCAS_BLOCKTYPE_FILENAME)
		return IMGTOOLERR_CORRUPTIMAGE;

	fname[8] = '\0';
	memcpy(fname, blk.data, 8);
	rtrim(fname);

	if (strlen(fname) >= ent->fname_len)
		return IMGTOOLERR_BUFFERTOOSMALL;
	strcpy(ent->fname, fname);

	filesize = 0;

	do {
		err = readblock(img, &blk);
		if (err)
			return err;

		if (blk.type == COCOCAS_BLOCKTYPE_DATA) {
			filesize += blk.length;
		}
	}
	while(blk.type == COCOCAS_BLOCKTYPE_DATA);

	if (blk.type != COCOCAS_BLOCKTYPE_EOF)
		return IMGTOOLERR_CORRUPTIMAGE;
	
	ent->filesize = filesize;

	return 0;
}

static int cococas_readfile(IMAGE *img, STREAM *destf)
{
	int err;
	casblock blk;

	/* Read directory block */
	err = readblock(img, &blk);
	if (err)
		return err;
	
	/* If block is not a filename block, fail */
	if (blk.type != COCOCAS_BLOCKTYPE_FILENAME)
		return IMGTOOLERR_CORRUPTIMAGE;

	do {
		err = readblock(img, &blk);
		if (err)
			return err;

		if (blk.type == COCOCAS_BLOCKTYPE_DATA)
			stream_write(destf, blk.data, blk.length);
	}
	while(blk.type == COCOCAS_BLOCKTYPE_DATA);

	if (blk.type != COCOCAS_BLOCKTYPE_EOF)
		return IMGTOOLERR_CORRUPTIMAGE;
	
	return 0;
}

