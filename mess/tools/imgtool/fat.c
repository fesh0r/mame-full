#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "osdepend.h"
#include "imgtoolx.h"

/*
  what to change/do for harddisk support?
  currently complete image is loaded into ram
  partition table is not read out


  sub directories not finished
  create not finished
  no code in bootsector (traps), should print a message and reboot or boot from harddisk
*/

/* all is loaded into memory */
/* #define ALL_IN_MEMORY */

/* binary words are stored in little endian order */

/* sorry nathan, but for unaligned words stored in little endian order */
typedef struct {
	unsigned char low, high;
} littleuword;

typedef struct {
	unsigned char low, mid, high, highest;
} littleulong;

#define GET_UWORD(a) ((a).low|((a).high<<8))
#define GET_ULONG(a) ((a).low|((a).mid<<8)|((a).high<<16)|((a).highest<<24))
#define SET_UWORD(a,v) (a).low=(v)&0xff;(a).high=((v)>>8)&0xff
#define SET_ULONG(a,v) (a).low=(v)&0xff;(a).mid=((v)>>8)&0xff;\
		(a).high=((v)>>16)&0xff;(a).highest=((v)>>24)&0xff

typedef struct {
	unsigned char code[0x1be];
	struct {
		unsigned char bootflag; /* 0x80 boot activ */
		struct {
			unsigned char head;
			littleuword sector_cylinder;
#define GET_CYLINDER(word) ( ((word&0xc0)<<2)|(word>>8))
#define GET_SECTOR(word) (word&0x3f)
#define PACK_SECTOR_CYLINDER(sec,cyl) ( (((cyl)>>2)&0xc0)|(sec)|((cyl)<<8) )
		} begin;
		unsigned char system;
#define FAT12 1
#define FAT16 4
		struct {
			unsigned char head;
			littleuword sector_cylinder;
		} end;
		littleulong start_sector, sectors;
	} partition[4];
	unsigned char id[2];
} PARTITION_TABLE;

static PARTITION_TABLE partition_table={
	{
		// program searches active partition in table and loads its bootsector
		0xfa ,0x2b, 0xc0 ,0x8e, 0xd0 ,0xb8, 0x00 ,0x7c,
		0x8b ,0xe0, 0xfb ,0xfc, 0x2b ,0xc0, 0x8e, 0xd8,
		0xbe ,0x00, 0x7c ,0xb8, 0x60 ,0x00, 0x8e ,0xc0,
		0x2b ,0xff, 0xb9 ,0x00, 0x01 ,0xf3, 0xa5, 0xea,
		0x24 ,0x00, 0x60 ,0x00, 0x8c ,0xc8, 0x8e ,0xd8,
		0xb9 ,0x04, 0x00 ,0xbf, 0xbe ,0x01, 0x8b, 0x15,
		0x80 ,0xfa, 0x80 ,0x74, 0x0c ,0x80, 0xfa ,0x00,
		0x75 ,0x50, 0x83 ,0xc7, 0x10 ,0xe2, 0xef, 0xcd,
		0x18 ,0x8b, 0xf7 ,0xeb, 0x0d ,0x83, 0xc7 ,0x10,
		0x8b ,0x05, 0x3c ,0x80, 0x74 ,0x3c, 0x3c, 0x00,
		0x75 ,0x38, 0xe2 ,0xf1, 0xb9 ,0x05, 0x00 ,0xb8,
		0x00 ,0x00, 0x8e ,0xc0, 0xbb ,0x00, 0x7c, 0xb4,
		0x02 ,0xb0, 0x01 ,0x51, 0x8b ,0x4c, 0x02 ,0xcd,
		0x13 ,0x59, 0x73 ,0x0b, 0xb4 ,0x00, 0xcd, 0x13,
		0xe2 ,0xe5, 0xbe ,0xc9, 0x00 ,0xeb, 0x16 ,0x26,
		0x81 ,0xbf, 0xfe ,0x01, 0x55 ,0xaa, 0x75, 0x05,
		0xea ,0x00, 0x7c ,0x00, 0x00 ,0xbe, 0xb5 ,0x00,
		0xeb ,0x03, 0xbe ,0x9d, 0x00 ,0xac, 0x3c, 0x24,
		0x74 ,0xfe, 0x56 ,0xbb, 0x07 ,0x00, 0xb4 ,0x0e,
		0xcd ,0x10, 0x5e ,0xeb, 0xf0 ,0x49, 0x6e, 0x76,
		0x61 ,0x6c, 0x69 ,0x64, 0x20 ,0x70, 0x61 ,0x72,
		0x74 ,0x69, 0x74 ,0x69, 0x6f ,0x6e, 0x20, 0x74,
		0x61 ,0x62, 0x6c ,0x65, 0x24 ,0x4e, 0x6f ,0x20,
		0x6f ,0x70, 0x65 ,0x72, 0x61 ,0x74, 0x69, 0x6e,
		0x67 ,0x20, 0x73 ,0x79, 0x73 ,0x74, 0x65 ,0x6d,
		0x24 ,0x4f, 0x70 ,0x65, 0x72 ,0x61, 0x74, 0x69,
		0x6e ,0x67, 0x20 ,0x73, 0x79 ,0x73, 0x74 ,0x65,
		0x6d ,0x20, 0x6c ,0x6f, 0x61 ,0x64, 0x20, 0x65,
		0x72 ,0x72, 0x6f ,0x72, 0x24, 0x00,
	},
	{
		{ 0 }
	},
	{ 0x55, 0xaa }
};

#if 0
/* are these values for the mess pc harddisk emulation? */

#define SECTORS     17
#define MAGIC       0xaa55
#define ECC 		11
#define CONTROL 	5

    /* fill in the drive geometry information */
	buffer[0x1ad] = options_->cylinders & 0xff;           /* cylinders */
	buffer[0x1ae] = (options_->cylinders >> 8) & 3;
	buffer[0x1af] = options_->heads;						/* heads */
	buffer[0x1b0] = (options_->cylinders+1) & 0xff;		/* write precompensation */
	buffer[0x1b1] = ((options_->cylinders+1) >> 8) & 3;
	buffer[0x1b2] = (options_->cylinders+1) & 0xff;		/* reduced write current */
	buffer[0x1b3] = ((options_->cylinders+1) >> 8) & 3;
	buffer[0x1b4] = ECC;						/* ECC length */
	buffer[0x1b5] = CONTROL;					/* control (step rate) */
	buffer[0x1b6] = options_->cylinders & 0xff;			/* parking cylinder */
	buffer[0x1b7] = (options_->cylinders >> 8) & 3;
	buffer[0x1b8] = 0x00;						/* no idea */
	buffer[0x1b9] = 0x00;
	buffer[0x1ba] = 0x00;
	buffer[0x1bb] = 0x00;
	buffer[0x1bc] = 0x00;
	buffer[0x1bd] = SECTORS;					/* some non zero value */
#endif

/* byte alignment ! */
/* I think not directly doable with every compiler */
typedef struct {
	unsigned char jump[3];
	char name[8];
	littleuword sector_size;
	unsigned char cluster_size;
	littleuword reserved_sectors;

	unsigned char fat_count;
	littleuword max_entries_in_root_directory;
	littleuword sectors;
	unsigned char media_descriptor; /* do not use */
	littleuword sectors_in_fat;
	littleuword sectors_per_track;
	littleuword heads;
	littleuword hidden_sectors; //littleulong?
	// littleulong total sectors largs
	// drive number
	// flags
	// signature 0x29
	// volumeid
	// db 11 volume lable
	// db 8 system id
	unsigned char reserved[0x1e0];
	unsigned char id[2];
} FAT_HEADER;

static FAT_HEADER fat_header={
//	{ 0xcd, 0x18,0 }, // int18 starts basic
	{ 0xeb, 0x56, 0x90 },
//	{ 'M', 'E', 'S', 'S', '0', '.', '1', '@' },
	{ ' ' },
	{ 0, 2 }, /* 0x200 */
	2,
	{ 1, 0 },
	2,
	{ 0xd0, 0 },
	{ 0, 0 },
	0,
	{ 2, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{
		0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
		// prints "cannot load dos press key to retry"
		// reboots after keypress
		0x4e, 0x41, 0x4d, 0x45, 0x20, 0x20, 0x20, 0x20,
		0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20,
		0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0xff, 0xff,
		0x49, 0x42, 0x4d, 0x42, 0x49, 0x4f, 0x20, 0x20,
		0x43, 0x4f, 0x4d, 0x00, 0x50, 0x00, 0x00, 0x08,
		0x00, 0x18, 0xfc, 0x33, 0xc0, 0x8e, 0xc0, 0xfa,
		0x8e, 0xd0, 0xbc, 0x00, 0x7c, 0xfb, 0x33, 0xd2,
		0xcd, 0x13, 0xbd, 0x78, 0x00, 0x8b, 0xfc, 0xc5,
		0x76, 0x00, 0x89, 0x7e, 0x00, 0x8c, 0x46, 0x02,
		0xb9, 0x0b, 0x00, 0xf3, 0xa4, 0x91, 0x8e, 0xd8,
		0x8b, 0xec, 0xc6, 0x46, 0x04, 0x24, 0x8a, 0x46,
		0x0d, 0x89, 0x46, 0x3e, 0x8a, 0x46, 0x10, 0xf7,
		0x66, 0x16, 0x03, 0x46, 0x0e, 0x83, 0xd2, 0x00,
		0x8b, 0x4e, 0x0b, 0x81, 0xf9, 0x00, 0x02, 0x74,
		0x11, 0x72, 0x54, 0xd1, 0xe9, 0x03, 0xc0, 0x83,
		0xd2, 0x00, 0xd1, 0x66, 0x3e, 0xd1, 0x66, 0x16,
		0xeb, 0xe9, 0x83, 0x7e, 0x16, 0x0c, 0x77, 0x05,
		0xc7, 0x46, 0x44, 0xff, 0x0f, 0x03, 0x46, 0x1c,
		0x13, 0x56, 0x1e, 0x50, 0x52, 0x8b, 0x5e, 0x11,
		0x53, 0x83, 0xc3, 0x0f, 0xb1, 0x04, 0xd3, 0xeb,
		0x88, 0x5e, 0x2b, 0x8e, 0x46, 0x54, 0x06, 0xe8,
		0xab, 0x00, 0x07, 0x89, 0x46, 0x2c, 0x89, 0x56,
		0x2e, 0x59, 0x2b, 0xff, 0x51, 0x57, 0x8d, 0x76,
		0x46, 0xb9, 0x0b, 0x00, 0xf3, 0xa6, 0x5f, 0x59,
		0x74, 0x13, 0x83, 0xc7, 0x20, 0xe2, 0xed, 0xbe,
		0xd5, 0x7d, 0xe8, 0xd4, 0x00, 0x98, 0xcd, 0x16,
		0xea, 0x00, 0x00, 0xff, 0xff, 0x26, 0x8b, 0x4d,
		0x1a, 0x26, 0x8b, 0x45, 0x1c, 0x05, 0xff, 0x01,
		0xd1, 0xe8, 0x88, 0x66, 0x25, 0x5a, 0x58, 0x51,
		0x8b, 0x4e, 0x16, 0x2b, 0xc1, 0x83, 0xda, 0x00,
		0x8e, 0x46, 0x54, 0x51, 0xc6, 0x46, 0x2b, 0x01,
		0xe8, 0x5a, 0x00, 0x59, 0xe2, 0xf5, 0x5b, 0x8e,
		0x46, 0x42, 0x8b, 0x46, 0x3e, 0x88, 0x46, 0x2b,
		0x28, 0x46, 0x25, 0x9c, 0x73, 0x06, 0x8a, 0x56,
		0x25, 0x00, 0x56, 0x2b, 0x53, 0x4b, 0x4b, 0xf7,
		0xe3, 0x03, 0x46, 0x2c, 0x13, 0x56, 0x2e, 0xe8,
		0x33, 0x00, 0x5b, 0x06, 0x8b, 0xc3, 0xd1, 0xe3,
		0xc4, 0x7e, 0x54, 0x72, 0x02, 0x8e, 0xc7, 0x81,
		0x7e, 0x44, 0xff, 0x0f, 0x75, 0x12, 0x03, 0xd8,
		0xd1, 0xeb, 0x26, 0x8b, 0x1f, 0x73, 0x04, 0xb1,
		0x04, 0xd3, 0xeb, 0x80, 0xe7, 0x0f, 0xeb, 0x03,
		0x26, 0x8b, 0x1f, 0x07, 0x9d, 0x77, 0xb3, 0x8a,
		0x56, 0x24, 0xff, 0x6e, 0x40, 0x33, 0xdb, 0x50,
		0x52, 0xe8, 0x15, 0x00, 0x8c, 0xc0, 0x05, 0x20,
		0x00, 0x8e, 0xc0, 0x5a, 0x58, 0x05, 0x01, 0x00,
		0x83, 0xd2, 0x00, 0xfe, 0x4e, 0x2b, 0x75, 0xe5,
		0xc3, 0xf7, 0x76, 0x18, 0x42, 0x8a, 0xca, 0x33,
		0xd2, 0xf7, 0x76, 0x1a, 0x8a, 0xf2, 0xd0, 0xcc,
		0xd0, 0xcc, 0x80, 0xe4, 0xc0, 0x0a, 0xcc, 0x8a,
		0xe8, 0x8a, 0x56, 0x24, 0xb8, 0x01, 0x02, 0xcd,
		0x13, 0x73, 0xdd, 0xfe, 0x4e, 0x26, 0x75, 0xf4,
		0xe9, 0x2c, 0xff, 0xb4, 0x0e, 0x2b, 0xdb, 0xcd,
		0x10, 0xac, 0x84, 0xc0, 0x75, 0xf5, 0xc3, 0x0d,
		0x0a, 0x43, 0x61, 0x6e, 0x6e, 0x6f, 0x74, 0x20,
		0x6c, 0x6f, 0x61, 0x64, 0x20, 0x44, 0x4f, 0x53,
		0x20, 0x70, 0x72, 0x65, 0x73, 0x73, 0x20, 0x6b,
		0x65, 0x79, 0x20, 0x74, 0x6f, 0x20, 0x72, 0x65,
		0x74, 0x72, 0x79, 0x0d, 0x0a, 0x00, 0x00, 0x00
	},
	{ 0x55, 0xaa }
};

typedef struct {
	char name[8], extension[3], attribut;
	char reserved[10];
	littleuword time, date, start_cluster;
	littleulong size;
} FAT_DIRECTORY;
#define ATTRIBUT_READ_ONLY 1
#define ATTRIBUT_HIDDEN 2
#define ATTRIBUT_SYSTEM 4
#define ATTRIBUT_LABEL 8
#define ATTRIBUT_SUBDIRECTORY 0x10
#define ATTRIBUT_ARCHIV 0x20

#define TIME_GET_SECOND(word) ((word&0x1f)<<1)
#define TIME_GET_MINUTE(word) ((word&0x7e0)>>5)
#define TIME_GET_HOUR(word) ((word&0xf800)>>11)
#define PACK_TIME(sec, min, hour) ((sec>>1)|(minutes<<5)|(hours<<11))

#define DATE_GET_DAY(word) (word&0x1f)
#define DATE_GET_MONTH(word) (((word&0x1e0)>>5)-1) /* 0..11 */
#define DATE_GET_YEAR(word) (((word&0xfe00)>>9)+1980)
#define PACK_DATE(day, month, year) ((day)|((month+1)<<5)|((year-1980)<<9))

typedef struct _fat_image{
	imgtool_image base;
	imgtool_stream *file_handle;

	int heads;
	int sectors;
	int sector_size;

	int cluster_size;
	int cluster_offset;
	int cluster_count;

	int root_offset;

	int fat_count;
	int fat_offset[2];
	int (*fat_is_special)(int cluster);
	int (*read_fat)(struct _fat_image* image, int cluster);
	void (*write_fat)(struct _fat_image* image, int cluster, int number);

	int size;
	int modified;
	unsigned char *complete_image;
	unsigned char *data; /* pointer to start of fat image */
} fat_image;

static int fat_calc_pos(fat_image *image, int head, int track, int sector)
{
	return (((sector*0x200)+head)*image->sectors+track)*image->heads;
}

static int fat_fat_is_special(int cluster)
{
	return (cluster&0xff0)==0xff0;
}

static int fat_read_fat_entry(fat_image *image, int cluster)
{
	int offset=image->fat_offset[0]+cluster+(cluster>>1);
	int data;
	if (cluster&1) data=(image->data[offset]|(image->data[offset+1]<<8))>>4;
	else data=(image->data[offset]|(image->data[offset+1]<<8))&0xfff;
	return data;
}

static void fat_write_fat_entry(fat_image *image, int cluster, int number)
{
	int offset=cluster+(cluster>>1);
	int i;
	if (cluster&1) {
		for (i=0; i<image->fat_count; i++) {
			image->data[image->fat_offset[i]+offset]=
				(image->data[image->fat_offset[i]+offset]&0xf)
				|((number&0xf)<<4);
			image->data[image->fat_offset[i]+offset+1]=number>>4;
		}
	} else {
		for (i=0; i<image->fat_count; i++) {
			image->data[image->fat_offset[i]+offset]=number&0xff;
			image->data[image->fat_offset[i]+offset+1]=
				(image->data[image->fat_offset[i]+offset+1]&0xf0)
				|((number>>8)&0xf);
		}
	}
}

static int fat_fat16_is_special(int cluster)
{
	return (cluster&0xfff0)==0xfff0;
}

static int fat_read_fat16_entry(fat_image *image, int cluster)
{
	int offset=image->fat_offset[0]+cluster*2;
	int data=image->data[offset]|(image->data[offset+1]<<8);
	return data;
}

static void fat_write_fat16_entry(fat_image *image, int cluster, int number)
{
	int offset=cluster*2;
	int i;
	for (i=0; i<image->fat_count; i++) {
		image->data[image->fat_offset[i]+offset]=number&0xff;
		image->data[image->fat_offset[i]+offset+1]=number>>8;
	}
}

static void fat_filename(FAT_DIRECTORY *entry, char *name)
{
	int i=7, j=2;
	while ((entry->name[i]==' ')&&(i>=0)) i--;
	i++;
	while ((entry->extension[j]==' ')&&(j>=0)) j--;
	j++;
	memcpy(name,entry->name, i);
	if (j>0) {
		name[i++]='.';
		memcpy(name+i, entry->extension, j);
		i+=j;
	}
	name[i]=0;
}

static int fat_calc_cluster_pos(fat_image *image,int cluster)
{
	return image->cluster_offset+cluster*image->cluster_size;
}

typedef struct {
	imgtool_imageenum base;
	fat_image *image;
	int level; /* directory level */
	struct {
		char name[200];
		int offset;
		int index;
		int count;
		int cluster;
	} directory[8]; /* I read somewhere, MSDOS does only support some directory levels */
} fat_iterator;

/* searches program with given name in directory
 * delivers -1 if not found
 * or pos in image of directory node */
static FAT_DIRECTORY *fat_image_findfile (fat_image *image,
										  const unsigned char *name)
{
	char tname[13];
	int offset=image->root_offset;
	int i, count=GET_UWORD(((FAT_HEADER*)image->data)->max_entries_in_root_directory);
	FAT_DIRECTORY *entry;

	for (i=0; i<count; i++) {
		entry=(FAT_DIRECTORY*)(image->data+offset)+i;
		if (entry->name[0]&&!(entry->attribut&ATTRIBUT_LABEL)) {
			fat_filename(entry, tname);
			if (strcmp(tname, (const char *)name)==0) return entry;
			if ((strncmp(tname, (const char *)name, strlen(tname))==0)
				&&(name[strlen(tname)]=='\\')
				&&(entry->attribut&ATTRIBUT_SUBDIRECTORY)) {
				name+=strlen(tname+1);
				/* sub directory to do */
			}
		}
	}

	return NULL;
}

static int fat_image_init(const struct ImageModule *mod, imgtool_stream *f, imgtool_image **outimg);
static void fat_image_exit(imgtool_image *img);
static void fat_image_info(imgtool_image *img, char *string, const int len);
static int fat_image_beginenum(imgtool_image *img, imgtool_imageenum **outenum);
static int fat_image_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent);
static void fat_image_closeenum(imgtool_imageenum *enumeration);
static size_t fat_image_freespace(imgtool_image *img);
static int fat_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf);
static int fat_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *options_);
static int fat_image_deletefile(imgtool_image *img, const char *fname);
static int fat_image_create(const struct ImageModule *mod, imgtool_stream *f, const ResolvedOption *options_);

static int fat_read_sector(imgtool_image *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length);
static int fat_write_sector(imgtool_image *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length);

#if 0
static struct OptionTemplate fat_createopts[] =
{
	{ "sectors",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,		255,		NULL	},	/* [0] */
	{ "heads",		NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,		2,			NULL	},	/* [1] */
	{ "cylinders",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,		255,		NULL	},	/* [2] */
	{ NULL, NULL, 0, 0, 0, 0 }
};
#define FAT_CREATEOPTIONS__SECTORS		0
#define FAT_CREATEOPTIONS__HEADS			1
#define FAT_CREATEOPTIONS_CYLINDERS		2
#else
static struct OptionTemplate fat_createopts[] =
{
	{ "type",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	0,		7,		NULL	},	/* [0] */
	{ NULL, NULL, 0, 0, 0, 0 }
};
#define FAT_CREATEOPTIONS_TYPE		0
#endif


IMAGEMODULE(
	msdos,
	"MSDOS/PCDOS Diskette",					/* human readable name */
	"dsk",									/* file extension */
	NULL,									/* crcfile */
	NULL,									/* crc system name */
	EOLN_CRLF,								/* eoln */
	IMGMODULE_FLAG_FILENAMES_PREFERUCASE,	/* flags */
	fat_image_init,							/* init function */
	fat_image_exit,							/* exit function */
	fat_image_info,							/* info function */
	fat_image_beginenum,					/* begin enumeration */
	fat_image_nextenum,						/* enumerate next */
	fat_image_closeenum,					/* close enumeration */
	fat_image_freespace,					/*  free space on image    */
	fat_image_readfile,						/* read file */
	fat_image_writefile,					/* write file */
	fat_image_deletefile,					/* delete file */
	fat_image_create,						/* create image */
	fat_read_sector,
	fat_write_sector,
	NULL,									/* file options */
	fat_createopts							/* create options */
)

static int fathd_image_init(const struct ImageModule *mod, imgtool_stream *f, imgtool_image **outimg);
static int fathd_image_create(const struct ImageModule *mod, imgtool_stream *f, const ResolvedOption *options_);

/*static geometry_ranges fathd_geometry = { {0x200,1,1,1}, {0x200,63,16,1024} };*/

#if 0
static struct OptionTemplate fathd_createopts[] =
{
	{ "sectors",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,		63,		NULL	},	/* [0] */
	{ "heads",		NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,		16,		NULL	},	/* [1] */
	{ "cylinders",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,		1024,	NULL	},	/* [2] */
	{ NULL, NULL, 0, 0, 0, 0 }
};
#else
static struct OptionTemplate fathd_createopts[] =
{
	{ "type",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	0,		0,		NULL	},	/* [0] */
	{ NULL, NULL, 0, 0, 0, 0 }
};
#endif

IMAGEMODULE(
	msdoshd,
	"MSDOS/PCDOS Harddisk",	/* human readable name */
	"img",									/* file extension */
	NULL,									/* crcfile */
	NULL,									/* crc system name */
	EOLN_CRLF,								/* eoln */
	IMGMODULE_FLAG_FILENAMES_PREFERUCASE,	/* flags */
	fathd_image_init,						/* init function */
	fat_image_exit,							/* exit function */
	fat_image_info,							/* info function */
	fat_image_beginenum,					/* begin enumeration */
	fat_image_nextenum,						/* enumerate next */
	fat_image_closeenum,					/* close enumeration */
	fat_image_freespace,					/*  free space on image    */
	fat_image_readfile,						/* read file */
	fat_image_writefile,					/* write file */
	fat_image_deletefile,					/* delete file */
	fathd_image_create,						/* create image */
	fat_read_sector,
	fat_write_sector,
	NULL,									/* file options */
	fathd_createopts						/* create options */
)

static int fat_image_init(const struct ImageModule *mod, imgtool_stream *f, imgtool_image **outimg)
{
	fat_image *image;
	int i;

	image=*(fat_image**)outimg=(fat_image *) malloc(sizeof(fat_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(fat_image));
	image->base.module = mod;
	image->size=stream_size(f);
	image->file_handle=f;

	image->complete_image=image->data = (unsigned char *) malloc(image->size);
	if ( (!image->data)
		 ||(stream_read(f, image->data, image->size)!=image->size) ) {
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}

	image->heads=GET_UWORD(((FAT_HEADER*)image->data)->heads);
	image->sector_size=GET_UWORD(((FAT_HEADER*)image->data)->sector_size);

	image->cluster_size=
		((FAT_HEADER*)image->data)->cluster_size*image->sector_size;
	image->fat_count=((FAT_HEADER*)image->data)->fat_count;

	image->fat_offset[0]=
		GET_UWORD(((FAT_HEADER*)image->data)->reserved_sectors)
		*image->sector_size;
	for (i=1; i<image->fat_count; i++) {
		image->fat_offset[i]=image->fat_offset[i-1]
			+GET_UWORD(((FAT_HEADER*)image->data)->sectors_in_fat)
			*image->sector_size;
	}
	image->root_offset=
		image->fat_offset[i-1]
		+GET_UWORD(((FAT_HEADER*)image->data)->sectors_in_fat)
		*image->sector_size;

	image->cluster_offset=image->root_offset
		+GET_UWORD(((FAT_HEADER*)image->data)->max_entries_in_root_directory)
		*sizeof(FAT_DIRECTORY)-2*image->cluster_size;

	image->cluster_count=(GET_UWORD(((FAT_HEADER*)image->data)->sectors)
		*image->sector_size-image->cluster_offset)/image->cluster_size;

	if (image->cluster_count <= 0xff0 ) {
		image->read_fat=fat_read_fat_entry;
		image->write_fat=fat_write_fat_entry;
		image->fat_is_special=fat_fat_is_special;
	} else {
		image->read_fat=fat_read_fat16_entry;
		image->write_fat=fat_write_fat16_entry;
		image->fat_is_special=fat_fat16_is_special;
	}

	return 0;
}

static int fathd_image_init(const struct ImageModule *mod, imgtool_stream *f, imgtool_image **outimg)
{
	fat_image *image;
	int sectors, heads;
	int i;
	PARTITION_TABLE *table;

	image=*(fat_image**)outimg=(fat_image *) malloc(sizeof(fat_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(fat_image));
	image->base.module = mod;
	image->size=stream_size(f);
	image->file_handle=f;

	image->complete_image= (unsigned char *) malloc(image->size);
	if ( (!image->complete_image)
		 ||(stream_read(f, image->complete_image, image->size)!=image->size) ) {
		free(image);
		*outimg=NULL;
		return IMGTOOLERR_OUTOFMEMORY;
	}

	table=(PARTITION_TABLE*)image->complete_image;

	/* currently fixed to the 4th partition */
	sectors=GET_SECTOR(GET_UWORD(table->partition[3].end.sector_cylinder));
	heads=table->partition[3].end.head;

	image->data=image->complete_image+0x200*sectors
		*(heads*GET_CYLINDER(GET_UWORD(table->partition[3].begin.sector_cylinder))
		  +table->partition[3].begin.head);

	image->heads=GET_UWORD(((FAT_HEADER*)image->data)->heads);
	image->sector_size=GET_UWORD(((FAT_HEADER*)image->data)->sector_size);

	image->cluster_size=
		((FAT_HEADER*)image->data)->cluster_size*image->sector_size;
	image->fat_count=((FAT_HEADER*)image->data)->fat_count;

	image->fat_offset[0]=
		GET_UWORD(((FAT_HEADER*)image->data)->reserved_sectors)
		*image->sector_size;
	for (i=1; i<image->fat_count; i++) {
		image->fat_offset[i]=image->fat_offset[i-1]
			+GET_UWORD(((FAT_HEADER*)image->data)->sectors_in_fat)
			*image->sector_size;
	}
	image->root_offset=
		image->fat_offset[i-1]
		+GET_UWORD(((FAT_HEADER*)image->data)->sectors_in_fat)
		*image->sector_size;

	image->cluster_offset=image->root_offset
		+GET_UWORD(((FAT_HEADER*)image->data)->max_entries_in_root_directory)
		*sizeof(FAT_DIRECTORY)-2*image->cluster_size;

	image->cluster_count=(GET_UWORD(((FAT_HEADER*)image->data)->sectors)
		*image->sector_size-image->cluster_offset)/image->cluster_size;

	image->read_fat=fat_read_fat16_entry;
	image->write_fat=fat_write_fat16_entry;
	image->fat_is_special=fat_fat16_is_special;

	return 0;
}

static void fat_image_exit(imgtool_image *img)
{
	fat_image *image=(fat_image*)img;
	if (image->modified) {
		stream_seek(image->file_handle, 0, SEEK_SET);
		stream_write(image->file_handle, image->complete_image, image->size);
	}
	stream_close(image->file_handle);
	free(image->complete_image);
	free(image);
}

static void fat_image_info(imgtool_image *img, char *string, const int len)
{
	fat_image *image=(fat_image*)img;
	char name[12]="          ";
	int offset=image->root_offset;
	int i, count=GET_UWORD(((FAT_HEADER*)image->data)->max_entries_in_root_directory);
	FAT_DIRECTORY *entry;

	for (i=0; i<count; i++) {
		entry=(FAT_DIRECTORY*)(image->data+offset)+i;
		if (entry->attribut&ATTRIBUT_LABEL) {
			memcpy(name,entry->name,11);
			sprintf(string, "%s",name);
			return;
		}
	}

	string[0]=0;
}

static int fat_image_beginenum(imgtool_image *img, imgtool_imageenum **outenum)
{
	fat_image *image=(fat_image*)img;
	fat_iterator *iter;

	iter=*(fat_iterator**)outenum = (fat_iterator *) malloc(sizeof(fat_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = img->module;

	iter->image=image;
	iter->level=0;
	iter->directory[iter->level].name[0]=0;
	iter->directory[iter->level].offset=image->root_offset;
	iter->directory[iter->level].index=0;
	iter->directory[iter->level].count=
		GET_UWORD(((FAT_HEADER*)image->data)->max_entries_in_root_directory);
	return 0;
}

static int fat_image_nextenum(imgtool_imageenum *enumeration, imgtool_dirent *ent)
{
	fat_iterator *iter=(fat_iterator*)enumeration;
	FAT_DIRECTORY *entry;
	char name[13];

	ent->corrupt=0;
	ent->eof=0;

	for (; iter->level>=0; iter->level--) {
		while (iter->directory[iter->level].index
				 <iter->directory[iter->level].count) {

			entry=(FAT_DIRECTORY*)
				(iter->image->data+iter->directory[iter->level].offset)
				+iter->directory[iter->level].index;

			iter->directory[iter->level].index++;
			if ((iter->level!=0)&& (iter->directory[iter->level].index==16)) {
				if (iter->image->fat_is_special(iter->directory[iter->level].cluster)) {
					iter->level--;
				} else {
					iter->directory[iter->level].offset=
						fat_calc_cluster_pos(iter->image, iter->directory[iter->level].cluster);
					iter->directory[iter->level].index=0;
					iter->directory[iter->level].cluster=
						iter->image->read_fat(iter->image, iter->directory[iter->level].cluster);
				}
			}

			if (entry->name[0]&&!(entry->attribut&ATTRIBUT_LABEL)) {
				fat_filename(entry, name);
				if (iter->level==0) {
					sprintf(ent->fname,"%s",name);
				} else {
					sprintf(ent->fname,"%s\\%s",
							iter->directory[iter->level].name,name);
				}
				ent->filesize=GET_ULONG(entry->size);
				if (ent->attr) {
					int date=GET_UWORD(entry->date), _time=GET_UWORD(entry->date);
					sprintf(ent->attr,"%.4d-%.2d-%.2d %.2d:%.2d:%.2d %s%s%s%s%s%s",
							DATE_GET_YEAR(date), DATE_GET_MONTH(date)+1, DATE_GET_DAY(date),
							TIME_GET_HOUR(_time), TIME_GET_MINUTE(_time), TIME_GET_SECOND(_time),
							entry->attribut&ATTRIBUT_HIDDEN?"H":"",
							entry->attribut&ATTRIBUT_SYSTEM?"S":"",
							entry->attribut&ATTRIBUT_SUBDIRECTORY?"D":"",
							entry->attribut&ATTRIBUT_LABEL?"L":"",
							entry->attribut&ATTRIBUT_ARCHIV?"A":"",
							entry->attribut&ATTRIBUT_READ_ONLY?"R":"");
				}
				if (entry->attribut&ATTRIBUT_SUBDIRECTORY) {
					if (iter->level==0) {
						strcpy(iter->directory[iter->level+1].name, name);
					} else {
						sprintf(iter->directory[iter->level+1].name,"%s\\%s",
								iter->directory[iter->level].name,name);
					}
					iter->level++;
					iter->directory[iter->level].offset=
						fat_calc_cluster_pos(iter->image,
											 GET_UWORD(entry->start_cluster));
					iter->directory[iter->level].count=16;
					iter->directory[iter->level].index=2;
					iter->directory[iter->level].cluster=
						iter->image->read_fat(iter->image,
											  GET_UWORD(entry->start_cluster));
				}
				return 0;
			}
		}
	}

	ent->eof=1;
	return 0;
}

static void fat_image_closeenum(imgtool_imageenum *enumeration)
{
	free(enumeration);
}

static size_t fat_image_freespace(imgtool_image *img)
{
	fat_image *image=(fat_image*)img;
	int i;
	size_t blocksfree = 0;

	for (i=0; i<image->cluster_count; i++) {
		if (image->read_fat(image, i)==0) blocksfree++;
	}

	return blocksfree*image->cluster_size;
}

static int fat_image_readfile(imgtool_image *img, const char *fname, imgtool_stream *destf)
{
	fat_image *image=(fat_image*)img;
	int size;
	int cluster, i;
	FAT_DIRECTORY *entry;

	if ((entry=fat_image_findfile(image, (const unsigned char *)fname))==NULL )
		return IMGTOOLERR_MODULENOTFOUND;

	cluster = GET_UWORD(entry->start_cluster);
	size=GET_ULONG(entry->size);

	for (i = 0; i < size; i += image->cluster_size)
	{
		if (i+image->cluster_size<size) {
			if (stream_write(destf, image->data+fat_calc_cluster_pos(image,cluster),
							 image->cluster_size)!=image->cluster_size)
				return IMGTOOLERR_WRITEERROR;
		} else {
			if (stream_write(destf, image->data+fat_calc_cluster_pos(image,cluster),
							 size-i)!=size-i)
				return IMGTOOLERR_WRITEERROR;
		}
		cluster = image->read_fat(image, cluster);
	}
	return 0;
}

static int fat_image_writefile(imgtool_image *img, const char *fname, imgtool_stream *sourcef, const ResolvedOption *options_)
{
	fat_image *image=(fat_image*)img;
    FAT_DIRECTORY *entry;
	int fsize;
	int cluster;
	int i;

	fsize=stream_size(sourcef);
	if ((entry=fat_image_findfile(image, (const unsigned char *)fname))!=NULL ) {
		/* override file !!! */
		if (fat_image_freespace(img)+(GET_ULONG(entry->size)|(image->cluster_size-1))<fsize)
			return IMGTOOLERR_NOSPACE;

		cluster=GET_UWORD(entry->start_cluster);

		while (!image->fat_is_special(cluster)) {
			int new=image->read_fat(image,cluster);
			image->write_fat(image, cluster, 0);
			cluster=new;
		}
	} else {
		int count=GET_UWORD(((FAT_HEADER*)image->data)->max_entries_in_root_directory);
		if (fat_image_freespace(img)<fsize) return IMGTOOLERR_NOSPACE;
		/* find free entry for directory */
		entry=(FAT_DIRECTORY*)(image->data+image->root_offset);
		for (i=0; i<count; i++, entry++) {
			if (entry->name[0]==0) break;
		}
		if (count==i) return IMGTOOLERR_NOSPACE; /* in root directory */
	}

	for ( i=0; (i<9)&&(fname[i]!='.')&&(fname[i]!=0); i++) ;

	memset(entry->name, ' ', 11);
	memcpy(entry->name, fname, i);
	if ((i<9)&&(fname[i]!=0)) { /* with extension */
		memcpy(entry->extension, fname+i+1, strlen(fname+i+1));
	}
	SET_ULONG(entry->size, fsize);
	entry->attribut=0;
	SET_UWORD(entry->time,0);
	SET_UWORD(entry->date,0);

	i=0; cluster=0;
	while (fsize>0) {
		while (image->read_fat(image, i)!=0) i++;
		if (fsize<image->cluster_size) {
			if (stream_read(sourcef, image->data+fat_calc_cluster_pos(image, i), fsize)!=fsize)
				return IMGTOOLERR_READERROR;
		} else {
			if (stream_read(sourcef, image->data+fat_calc_cluster_pos(image, i), image->cluster_size)
				!=image->cluster_size) return IMGTOOLERR_READERROR;
		}
		if (cluster==0) { SET_UWORD(entry->start_cluster,i); }
		else image->write_fat(image, cluster, i);
		cluster=i;
		i++;
		fsize-=image->cluster_size;
	}
	image->write_fat(image, cluster, 0xffffffff);

	image->modified=1;
	return 0;
}

static int fat_image_deletefile(imgtool_image *img, const char *fname)
{
	fat_image *image=(fat_image*)img;
    FAT_DIRECTORY *entry;
	int cluster;

	if ((entry=fat_image_findfile(image, (const unsigned char *)fname))==NULL ) {
		return IMGTOOLERR_MODULENOTFOUND;
	}
	cluster=GET_UWORD(entry->start_cluster);
	while (!image->fat_is_special(cluster)) {
		int new=image->read_fat(image,cluster);
		image->write_fat(image, cluster, 0);
		cluster=new;
	}

	entry->name[0]=0;
	image->modified=1;
	return 0;
}

static int fat_read_sector(imgtool_image *img, UINT8 head, UINT8 track, UINT8 sector, int offset, void *buffer, int length)
{
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}

static int fat_write_sector(imgtool_image *img, UINT8 head, UINT8 track, UINT8 sector, int offset, const void *buffer, int length)
{
	/* not yet implemented */
	assert(0);
	return IMGTOOLERR_UNEXPECTED;
}

typedef struct {
	int sectors, heads, tracks, sector_size;

	int fats;
	int cluster_size, directory_entries;
	int fat16, sectors_in_fat;
} FAT_FORMAT;

static FAT_FORMAT fat_formats[]={
	/* standard formats */
	{ 8, 1, 40, 0x200, 2, 0x400, 112, 0, 2}, /* 160 */
	{ 8, 2, 40, 0x200, 2, 0x400, 112, 0, 2 }, /* 180 */
	{ 9, 1, 40, 0x200, 2, 0x400, 112, 0, 2 }, /* 320 */
	{ 9, 2, 40, 0x200, 2, 0x400, 112, 0, 2 }, /* 360 ok */
	{ 9, 2, 80, 0x200, 2, 0x400, 112, 0, 2 }, /* 720 ok */
	{ 15, 2, 80, 0x200, 2, 0x400, 112, 0, 2 }, /* 1.2 */
	{ 18, 2, 80, 0x200, 2, 0x400, 112, 0, 2 }, /* 1.44 */
	{ 36, 2, 80, 0x200, 2, 0x400, 112, 0, 2 }, /* 2.88 */
	/* tons of extended xdf formats */
	/* use of larger and mixed sector sizes, more tracks */
};

static FAT_FORMAT hd_formats[]={
	{ 17, 4, 615, 0x200, 2, 0x800, 112, 1, 40 } // standard pc 20mb harddisk
};

static int fat_create(imgtool_stream *f, FAT_FORMAT *format)
{
	unsigned short fat_fat16[0x100]={ 0xfffd, 0xffff };
	unsigned char fat_fat12[0x200]={ 0xfd, 0xff, 0xff };
	unsigned char sector[0x200]={ 0 };
	int sectors=format->sectors*format->heads*format->tracks;
	int i, j, s;

/*	SET_UWORD(fat_header.sector_size, format->sector_size); */
	SET_UWORD(fat_header.heads, format->heads);
	SET_UWORD(fat_header.sectors_per_track, format->sectors);
	SET_UWORD(fat_header.sectors, sectors);

	fat_header.fat_count=format->fats;
	SET_UWORD(fat_header.max_entries_in_root_directory, format->directory_entries);
	fat_header.cluster_size=format->cluster_size/format->sector_size;

	SET_UWORD(fat_header.sectors_in_fat, format->sectors_in_fat);

	s=0;
	if (stream_write(f, &fat_header, sizeof(fat_header)) != sizeof(fat_header))
		return  IMGTOOLERR_WRITEERROR;
	s++;
	for (i=0; i<format->fats; i++) {
		if (format->fat16) {
			if (stream_write(f, fat_fat16, sizeof(fat_fat16)) != sizeof(fat_fat16))
				return  IMGTOOLERR_WRITEERROR;
		} else {
			if (stream_write(f, fat_fat12, sizeof(fat_fat12)) != sizeof(fat_fat12))
				return  IMGTOOLERR_WRITEERROR;
		}
		s++;
		for (j=1; j<format->sectors_in_fat; j++,s++) {
			if (stream_write(f, sector, sizeof(sector)) != sizeof(sector))
				return  IMGTOOLERR_WRITEERROR;
		}
	}
	for (i=0; i<format->directory_entries; i+=32, s++) {
		if (stream_write(f, sector, sizeof(sector)) != sizeof(sector))
			return  IMGTOOLERR_WRITEERROR;
	}
	for (; s<sectors; s++) {
		if (stream_write(f, sector, sizeof(sector)) != sizeof(sector))
			return  IMGTOOLERR_WRITEERROR;
	}
	return 0;
}

static int fat_image_create(const struct ImageModule *mod, imgtool_stream *f, const ResolvedOption *options_)
{
	FAT_FORMAT format;
#if 0
	// maybe later (as option), when calculation of parameter available
	format=fat_formats[3];
	format.sectors=options_[FAT_CREATEOPTIONS_SECTORS].i;
	format.heads=options_[FAT_CREATEOPTIONS_HEADS].i;
	format.tracks=options_[FAT_CREATEOPTIONS_CYLINDERS].i;
#else
	format=fat_formats[options_[FAT_CREATEOPTIONS_TYPE].i];
#endif
	return fat_create(f, &format);
}

static int fathd_image_create(const struct ImageModule *mod, imgtool_stream *f, const ResolvedOption *options_)
{
	FAT_FORMAT format;
	unsigned char sector[0x200]={ 0 };
	int c, s, h;
	PARTITION_TABLE *table = &partition_table;
	int head=1, cylinder=0;

#if 0
	format=hd_formats[0];
	format.sectors=options_[FAT_CREATEOPTIONS_SECTORS].i;
	format.heads=options_[FAT_CREATEOPTIONS_HEADS].i;
	format.tracks=options_[FAT_CREATEOPTIONS_CYLINDERS].i;
#else
	format=hd_formats[options_[FAT_CREATEOPTIONS_TYPE].i];
#endif

	table->partition[3].bootflag=0x80;

	SET_UWORD(table->partition[3].begin.sector_cylinder, PACK_SECTOR_CYLINDER(1,cylinder));
	table->partition[3].begin.head=head;

	SET_UWORD(table->partition[3].end.sector_cylinder,
			  PACK_SECTOR_CYLINDER(format.sectors,format.tracks-1));
	table->partition[3].end.head=format.heads-1;

	table->partition[3].system=FAT16;

	SET_ULONG(table->partition[3].start_sector,
			  format.sectors*head+(format.sectors*format.heads*cylinder));
	SET_ULONG(table->partition[3].sectors,
			  format.sectors*format.heads*(format.tracks-1)
			  -GET_ULONG(table->partition[3].start_sector));

	for (c=0; c<=cylinder; c++) {
		for (h=0; ((c<cylinder)&&(h<format.heads))||(h<head); h++) {
			for (s=0; s<format.sectors; s++) {
				if ((c==0)&&(h==0)&&(s==0)) {
					if (stream_write(f, table, sizeof(*table)) != sizeof(*table))
						return IMGTOOLERR_WRITEERROR;
				} else {
					if (stream_write(f, &sector, sizeof(sector)) != sizeof(sector))
						return IMGTOOLERR_WRITEERROR;
				}
			}
		}
	}

	return fat_create(f, &format);
}


