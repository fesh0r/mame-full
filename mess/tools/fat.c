#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "imgtool.h"

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
#define GET_CYLINDER(word) (word>>6)
#define GET_SECTOR(word) (word&0x3f)
#define PACK_SECTOR_CYLINDER(sec,cyl) ((cyl<<6)|sec)
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
	{ 0 },
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
	buffer[0x1ad] = options->cylinders & 0xff;           /* cylinders */
	buffer[0x1ae] = (options->cylinders >> 8) & 3;
	buffer[0x1af] = options->heads;						/* heads */
	buffer[0x1b0] = (options->cylinders+1) & 0xff;		/* write precompensation */
	buffer[0x1b1] = ((options->cylinders+1) >> 8) & 3;
	buffer[0x1b2] = (options->cylinders+1) & 0xff;		/* reduced write current */
	buffer[0x1b3] = ((options->cylinders+1) >> 8) & 3;
	buffer[0x1b4] = ECC;						/* ECC length */
	buffer[0x1b5] = CONTROL;					/* control (step rate) */
	buffer[0x1b6] = options->cylinders & 0xff;			/* parking cylinder */
	buffer[0x1b7] = (options->cylinders >> 8) & 3;
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
	char jump[3];
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
	littleuword hidden_sectors;
	unsigned char reserved[0x1e0];
	unsigned char id[2];
} FAT_HEADER;

static FAT_HEADER fat_header={ 
	{ 0,0,0 },
	{ 'M', 'E', 'S', 'S', '0', '.', '1', '@' },
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
	{ 0 },
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
	IMAGE base;
	STREAM *file_handle;

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
	int offset=cluster+cluster*2;
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
	IMAGEENUM base;
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

static int fat_image_init(STREAM *f, IMAGE **outimg);
static void fat_image_exit(IMAGE *img);
static void fat_image_info(IMAGE *img, char *string, const int len);
static int fat_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int fat_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void fat_image_closeenum(IMAGEENUM *enumeration);
static size_t fat_image_freespace(IMAGE *img);
static int fat_image_readfile(IMAGE *img, const char *fname, STREAM *destf);
static int fat_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *options);
static int fat_image_deletefile(IMAGE *img, const char *fname);
static int fat_image_create(STREAM *f, const ResolvedOption *options);

static int fat_read_sector(IMAGE *img, int head, int track, int sector, char **buffer, int *size);
static int fat_write_sector(IMAGE *img, int head, int track, int sector, char *buffer, int size);

static struct OptionTemplate fat_createopts[] =
{
	{ "cylinders",	IMGOPTION_FLAG_TYPE_INTEGER,	1,		255,		NULL	},	/* [0] */
	{ "sectors",	IMGOPTION_FLAG_TYPE_INTEGER,	1,		255,		NULL	},	/* [1] */
	{ "heads",		IMGOPTION_FLAG_TYPE_INTEGER,	1,		2,			NULL	},	/* [2] */
	{ NULL, 0, 0, 0, 0 }
};

#define FAT_CREATEOPTIONS_CYLINDERS		0
#define FAT_CREATEOPTIONS_SECTORS		1
#define FAT_CREATEOPTIONS_HEADS			2

/* IMAGE_USES_LABEL, /* flags */
IMAGEMODULE(
	msdos,
	"MSDOS/PCDOS Diskette",			/* human readable name */
	"dsk",							/* file extension */
	NULL,							/* crcfile */
	NULL,							/* crc system name */
	EOLN_CRLF,						/* eoln */
	fat_image_init,					/* init function */
	fat_image_exit,					/* exit function */
	fat_image_info,					/* info function */
	fat_image_beginenum,			/* begin enumeration */
	fat_image_nextenum,				/* enumerate next */
	fat_image_closeenum,			/* close enumeration */
	fat_image_freespace,			/*  free space on image    */
	fat_image_readfile,				/* read file */
	fat_image_writefile,			/* write file */
	fat_image_deletefile,			/* delete file */
	fat_image_create,				/* create image */
	fat_read_sector,
	fat_write_sector,
	NULL,							/* file options */
	fat_createopts					/* create options */
)

static int fathd_image_init(STREAM *f, IMAGE **outimg);
static int fathd_image_create(STREAM *f, const ResolvedOption *options);

/*static geometry_ranges fathd_geometry = { {0x200,1,1,1}, {0x200,63,1024,16} };*/

static struct OptionTemplate fathd_createopts[] =
{
	{ "cylinders",	IMGOPTION_FLAG_TYPE_INTEGER,	1,		63,		NULL	},	/* [0] */
	{ "sectors",	IMGOPTION_FLAG_TYPE_INTEGER,	1,		1024,	NULL	},	/* [1] */
	{ "heads",		IMGOPTION_FLAG_TYPE_INTEGER,	1,		16,		NULL	},	/* [2] */
	{ NULL, 0, 0, 0, 0 }
};

IMAGEMODULE(
	msdoshd,
	"MSDOS/PCDOS Harddisk",	/* human readable name */
	"img",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	EOLN_CRLF,								/* eoln */
	fathd_image_init,				/* init function */
	fat_image_exit,				/* exit function */
	fat_image_info,		/* info function */
	fat_image_beginenum,			/* begin enumeration */
	fat_image_nextenum,			/* enumerate next */
	fat_image_closeenum,			/* close enumeration */
	fat_image_freespace,			 /*  free space on image    */
	fat_image_readfile,			/* read file */
	fat_image_writefile,			/* write file */
	fat_image_deletefile,			/* delete file */
	fathd_image_create,				/* create image */
	fat_read_sector,
	fat_write_sector,
	NULL,							/* file options */
	fathd_createopts				/* create options */
)

static int fat_image_init(STREAM *f, IMAGE **outimg)
{
	fat_image *image;
	int i;

	image=*(fat_image**)outimg=(fat_image *) malloc(sizeof(fat_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(fat_image));
	image->base.module = &imgmod_msdos;
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

static int fathd_image_init(STREAM *f, IMAGE **outimg)
{
	fat_image *image;
	int i;
	PARTITION_TABLE *table;

	image=*(fat_image**)outimg=(fat_image *) malloc(sizeof(fat_image));
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	memset(image, 0, sizeof(fat_image));
	image->base.module = &imgmod_msdoshd;
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

static void fat_image_exit(IMAGE *img)
{
	fat_image *image=(fat_image*)img;
	if (image->modified) {
		stream_seek(image->file_handle, 0, SEEK_SET);
		stream_write(image->file_handle, image->complete_image, image->size);
	}
	stream_close(image->file_handle);
	free(image->data);
	free(image);
}

static void fat_image_info(IMAGE *img, char *string, const int len)
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

static int fat_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
{
	fat_image *image=(fat_image*)img;
	fat_iterator *iter;

	iter=*(fat_iterator**)outenum = (fat_iterator *) malloc(sizeof(fat_iterator));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = &imgmod_msdos;

	iter->image=image;
	iter->level=0;
	iter->directory[iter->level].name[0]=0;
	iter->directory[iter->level].offset=image->root_offset;
	iter->directory[iter->level].index=0;
	iter->directory[iter->level].count=
		GET_UWORD(((FAT_HEADER*)image->data)->max_entries_in_root_directory);
	return 0;
}

static int fat_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
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
							entry->attribut&ATTRIBUT_HIDDEN?"Hidden ":"",
							entry->attribut&ATTRIBUT_SYSTEM?"System ":"",
							entry->attribut&ATTRIBUT_SUBDIRECTORY?"Dir ":"",
							entry->attribut&ATTRIBUT_LABEL?"Label ":"",
							entry->attribut&ATTRIBUT_ARCHIV?"Archiv ":"",
							entry->attribut&ATTRIBUT_READ_ONLY?"Readonly ":"");
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

static void fat_image_closeenum(IMAGEENUM *enumeration)
{
	free(enumeration);
}

static size_t fat_image_freespace(IMAGE *img)
{
	fat_image *image=(fat_image*)img;
	int i;
	size_t blocksfree = 0;

	for (i=0; i<image->cluster_count; i++) {
		if (image->read_fat(image, i)==0) blocksfree++;
	}

	return blocksfree*image->cluster_size;
}

static int fat_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
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

static int fat_image_writefile(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *options)
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

static int fat_image_deletefile(IMAGE *img, const char *fname)
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

static int fat_read_sector(IMAGE *img, int head, int track, int sector, 
						   char **buffer, int *size)
{
	fat_image *image=(fat_image*)img;
	int pos;

	if (*size<image->sector_size) *buffer=realloc(*buffer,image->sector_size);
	if (!*buffer) return IMGTOOLERR_OUTOFMEMORY;

	pos=fat_calc_pos(image, head, track, sector);

	memcpy(*buffer, image->data+pos, image->sector_size);
	*size=image->sector_size;
	
	return 0;
}

static int fat_write_sector(IMAGE *img, int head, int track, int sector, 
							char *buffer, int size)
{
	fat_image *image=(fat_image*)img;
	int pos;

	if (size!=image->sector_size) ; /* problem */

	pos=fat_calc_pos(image, head, track, sector);
	memcpy(image->data+pos, buffer, size);
	
	return 0;
}

typedef struct {
	int sectors, heads, tracks, sector_size;

	int fats;
	int cluster_size, directory_entries;
} FAT_FORMAT;

static FAT_FORMAT fat_formats[]={
	/* standard formats */
	{ 8, 1, 40, 0x200, 2, 0x400, 112 }, /* 160 */
	{ 8, 2, 40, 0x200, 2, 0x400, 112 }, /* 180 */
	{ 9, 1, 40, 0x200, 2, 0x400, 112 }, /* 320 */
	{ 9, 2, 40, 0x200, 2, 0x400, 112 }, /* 360 ok */
	{ 9, 2, 80, 0x200, 2, 0x400, 112 }, /* 720 ok */
	{ 15, 2, 80, 0x200, 2, 0x400, 112 }, /* 1.2 */
	{ 18, 2, 80, 0x200, 2, 0x400, 112 }, /* 1.44 */
	{ 36, 2, 80, 0x200, 2, 0x400, 112 } /* 2.88 */
	/* tons of extended xdf formats */
	/* use of larger and mixed sector sizes, more tracks */
};

static int fat_image_create(STREAM *f, const ResolvedOption *options)
{
	FAT_FORMAT *format=fat_formats+3;
	unsigned short fat_fat16[0x100]={ 0xfffd, 0xffff };
	unsigned char fat_fat12[0x200]={ 0xfd, 0xff, 0xff };
	unsigned char sector[0x200]={ 0 };
	int sectors=format->sectors*format->heads*format->tracks;
	int fat16=0;
	int sectors_in_fat=2, i, j, s;

/*	SET_UWORD(fat_header.sector_size, format->sector_size); */
	SET_UWORD(fat_header.heads, format->heads);
	SET_UWORD(fat_header.sectors_per_track, format->sectors);
	SET_UWORD(fat_header.sectors, sectors);

	fat_header.fat_count=format->fats;
	SET_UWORD(fat_header.max_entries_in_root_directory, format->directory_entries);
	fat_header.cluster_size=format->cluster_size/format->sector_size;

	SET_UWORD(fat_header.sectors_in_fat, 2);

	s=0;
	if (stream_write(f, &fat_header, sizeof(fat_header)) != sizeof(fat_header)) 
		return  IMGTOOLERR_WRITEERROR;
	s++;
	for (i=0; i<format->fats; i++) {
		if (fat16) {
			if (stream_write(f, fat_fat16, sizeof(fat_fat16)) != sizeof(fat_fat16)) 
				return  IMGTOOLERR_WRITEERROR;
		} else {
			if (stream_write(f, fat_fat12, sizeof(fat_fat12)) != sizeof(fat_fat12)) 
				return  IMGTOOLERR_WRITEERROR;
		}
		s++;
		for (j=1; j<sectors_in_fat; j++,s++) {
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

static int fathd_image_create(STREAM *f, const ResolvedOption *options)
{
	unsigned char sector[0x200]={ 0 };
	int s, h;
	PARTITION_TABLE *table = &partition_table;


	table->partition[0].bootflag=0x80;

	SET_UWORD(table->partition[0].begin.sector_cylinder, PACK_SECTOR_CYLINDER(1,0));
	table->partition[0].begin.head=1;

	SET_UWORD(table->partition[0].begin.sector_cylinder,
			  PACK_SECTOR_CYLINDER(options[FAT_CREATEOPTIONS_SECTORS].i,options[FAT_CREATEOPTIONS_CYLINDERS].i));
	table->partition[0].end.head=options[FAT_CREATEOPTIONS_HEADS].i;

	SET_ULONG(table->partition[0].start_sector,options[FAT_CREATEOPTIONS_SECTORS].i);
	SET_ULONG(table->partition[0].sectors,
			  options[FAT_CREATEOPTIONS_SECTORS].i*options[FAT_CREATEOPTIONS_HEADS].i*options[FAT_CREATEOPTIONS_CYLINDERS].i
			  -GET_ULONG(table->partition[0].start_sector));

	h=0;
	for (s=0; s<options[FAT_CREATEOPTIONS_SECTORS].i; s++) {
		if ((s==0)&&(h==0)) {
			if (stream_write(f, &table, sizeof(table)) != sizeof(table))
				return IMGTOOLERR_WRITEERROR;
		} else {
			if (stream_write(f, &sector, sizeof(sector)) != sizeof(sector))
				return IMGTOOLERR_WRITEERROR;
		}
	}

	/* write this fat partition */
	
	return 0;
}


