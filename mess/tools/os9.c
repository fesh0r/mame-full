#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "imgtool.h"

enum {
	OS9ATTR_D	= 0x80,	/* directory (0=file, 1=directory) */
	OS9ATTR_S	= 0x40,	/* shared (0=sharable, 1=nonsharable) */
	OS9ATTR_PE	= 0x20,	/* public executable */
	OS9ATTR_PW	= 0x10,	/* public write */
	OS9ATTR_PR	= 0x08,	/* public read */
	OS9ATTR_E	= 0x04,	/* executable */
	OS9ATTR_W	= 0x02,	/* write */
	OS9ATTR_R	= 0x01	/* read */
};

typedef struct {
	UINT8 lsnstart[3];
	UINT8 sectorcount[2];
} segment_entry;

typedef struct {
	UINT8 attributes;
	UINT8 user[2];
	UINT8 datemodified_yy;
	UINT8 datemodified_mm;
	UINT8 datemodified_dd;
	UINT8 datemodified_hh;
	UINT8 datemodified_nn;
	UINT8 linkcount;
	UINT8 filesize[4];
	UINT8 datecreated_yy;
	UINT8 datecreated_mm;
	UINT8 datecreated_dd;
	segment_entry segs[48];
} file_descriptor;

typedef struct {
	UINT8 total_sectors[3];
	UINT8 total_tracks;
	UINT8 allocation_map_bytes[2];
	UINT8 sectors_per_cluster[2];
	UINT8 rootdirectory_sector[3];
	UINT8 owners_uid[2];
	UINT8 attributes;
	UINT8 id[2];
	UINT8 format;
	UINT8 sectors_per_track[2];
	UINT8 reserved[2];
	UINT8 bootstrap_sector[3];
	UINT8 bootstrap_size[2];
	UINT8 datecreated_yy;
	UINT8 datecreated_mm;
	UINT8 datecreated_dd;
	UINT8 datecreated_hh;
	UINT8 datecreated_nn;
	char name[32]; /* last char has sign bit set */
} boot_sector;