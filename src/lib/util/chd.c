/***************************************************************************

    MAME Compressed Hunks of Data file format

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "chd.h"
#include "md5.h"
#include "sha1.h"
#include <zlib.h>
#include <time.h>



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define PRINTF_MAX_HUNK				(0)



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAP_STACK_ENTRIES			512			/* max number of entries to use on the stack */
#define MAP_ENTRY_SIZE				16			/* V3 and later */
#define OLD_MAP_ENTRY_SIZE			8			/* V1-V2 */
#define METADATA_HEADER_SIZE		16			/* metadata header size */
#define CRCMAP_HASH_SIZE			4095		/* number of CRC hashtable entries */

#define MAP_ENTRY_FLAG_TYPE_MASK	0x0f		/* what type of hunk */
#define MAP_ENTRY_FLAG_NO_CRC		0x10		/* no CRC is present */

#define MAP_ENTRY_TYPE_INVALID		0x00		/* invalid type */
#define MAP_ENTRY_TYPE_COMPRESSED	0x01		/* standard compression */
#define MAP_ENTRY_TYPE_UNCOMPRESSED	0x02		/* uncompressed data */
#define MAP_ENTRY_TYPE_MINI			0x03		/* mini: use offset as raw data */
#define MAP_ENTRY_TYPE_SELF_HUNK	0x04		/* same as another hunk in this file */
#define MAP_ENTRY_TYPE_PARENT_HUNK	0x05		/* same as a hunk in the parent file */

#define CHD_V1_SECTOR_SIZE			512			/* size of a "sector" in the V1 header */

#define COOKIE_VALUE				0xbaadf00d
#define MAX_ZLIB_ALLOCS				64

#define END_OF_LIST_COOKIE			"EndOfListCookie"

#define NO_MATCH					(~0)



/***************************************************************************
    MACROS
***************************************************************************/

#define EARLY_EXIT(x)				do { (void)(x); goto cleanup; } while (0)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* interface to a codec */
typedef struct _codec_interface codec_interface;
struct _codec_interface
{
	UINT32		compression;				/* type of compression */
	const char *compname;					/* name of the algorithm */
	UINT8		lossy;						/* is this a lossy algorithm? */
	chd_error	(*init)(chd_file *chd);		/* codec initialize */
	void 		(*free)(chd_file *chd);		/* codec free */
	chd_error	(*compress)(chd_file *chd, const void *src, UINT32 *complen); /* compress data */
	chd_error	(*decompress)(chd_file *chd, UINT32 complen, void *dst); /* decompress data */
	chd_error	(*config)(chd_file *chd, int param, void *config); /* configure */
};


/* a single map entry */
typedef struct _map_entry map_entry;
struct _map_entry
{
	UINT64					offset;			/* offset within the file of the data */
	UINT32					crc;			/* 32-bit CRC of the data */
	UINT32					length;			/* length of the data */
	UINT8					flags;			/* misc flags */
};


/* simple linked-list of hunks used for our CRC map */
typedef struct _crcmap_entry crcmap_entry;
struct _crcmap_entry
{
	UINT32					hunknum;		/* hunk number */
	crcmap_entry *			next;			/* next entry in list */
};


/* a single metadata entry */
typedef struct _metadata_entry metadata_entry;
struct _metadata_entry
{
	UINT64					offset;			/* offset within the file of the header */
	UINT64					next;			/* offset within the file of the next header */
	UINT64					prev;			/* offset within the file of the previous header */
	UINT32					length;			/* length of the metadata */
	UINT32					metatag;		/* metadata tag */
};


/* multifile object */
typedef struct _multi_file multi_file;
struct _multi_file
{
	UINT32					numfiles;		/* number of files */
	chd_interface_file **	files;			/* array of open file handles */
	UINT64 *				length;			/* array of file lengths */
};


/* internal representation of an open CHD file */
struct _chd_file
{
	UINT32					cookie;			/* cookie, should equal COOKIE_VALUE */
	chd_file *				next;			/* pointer to next file in the global list */

	multi_file *			file;			/* handle to the open multi file */
	chd_header 				header;			/* header, extracted from file */

	chd_file *				parent;			/* pointer to parent file, or NULL */

	map_entry *				map;			/* array of map entries */

	UINT8 *					cache;			/* hunk cache pointer */
	UINT32					cachehunk;		/* index of currently cached hunk */

	UINT8 *					compare;		/* hunk compare pointer */
	UINT32					comparehunk;	/* index of current compare data */

	UINT8 *					compressed;		/* pointer to buffer for compressed data */
	const codec_interface *	codecintf;		/* interface to the codec */
	void *					codecdata;		/* opaque pointer to codec data */

	crcmap_entry *			crcmap;			/* CRC map entries */
	crcmap_entry *			crcfree;		/* free list CRC entries */
	crcmap_entry **			crctable;		/* table of CRC entries */

	UINT32					maxhunk;		/* maximum hunk accessed */

	UINT8					compressing;	/* are we compressing? */
	struct MD5Context		compmd5; 		/* running MD5 during compression */
	struct sha1_ctx			compsha1; 		/* running SHA1 during compression */
	UINT32					comphunk;		/* next hunk we will compress */

	UINT8					verifying;		/* are we verifying? */
	struct MD5Context		vermd5; 		/* running MD5 during verification */
	struct sha1_ctx			versha1; 		/* running SHA1 during verification */
	UINT32					verhunk;		/* next hunk we will verify */

	osd_work_queue *		workqueue;		/* pointer to work queue for async operations */
	osd_work_item *			workitem;		/* active work item, or NULL if none */
	UINT32					async_hunknum;	/* hunk index for asynchronous operations */
	void *					async_buffer;	/* buffer pointer for asynchronous operations */
};


/* codec-private data for the ZLIB codec */
typedef struct _zlib_codec_data zlib_codec_data;
struct _zlib_codec_data
{
	z_stream				inflater;
	z_stream				deflater;
	UINT32 *				allocptr[MAX_ZLIB_ALLOCS];
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static chd_interface cur_interface;
static chd_file *first_file;

static const UINT8 nullmd5[CHD_MD5_BYTES] = { 0 };
static const UINT8 nullsha1[CHD_SHA1_BYTES] = { 0 };



/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* internal async operations */
static void *async_read_callback(void *param);
static void *async_write_callback(void *param);

/* internal header operations */
static chd_error header_validate(const chd_header *header);
static chd_error header_read(multi_file *file, chd_header *header);
static chd_error header_write(multi_file *file, const chd_header *header);

/* internal hunk read/write */
static chd_error hunk_read_into_cache(chd_file *chd, UINT32 hunknum);
static chd_error hunk_read_into_memory(chd_file *chd, UINT32 hunknum, UINT8 *dest);
static chd_error hunk_write_from_memory(chd_file *chd, UINT32 hunknum, const UINT8 *src);

/* internal map access */
static chd_error map_write_initial(multi_file *file, chd_file *parent, const chd_header *header);
static chd_error map_read(chd_file *chd);

/* internal CRC map access */
static void crcmap_init(chd_file *chd, int prepopulate);
static void crcmap_add_entry(chd_file *chd, UINT32 hunknum);
static UINT32 crcmap_find_hunk(chd_file *chd, UINT32 hunknum, UINT32 crc, const UINT8 *rawdata);

/* metadata management */
static chd_error metadata_find_entry(chd_file *chd, UINT32 metatag, UINT32 metaindex, metadata_entry *metaentry);
static chd_error metadata_set_previous_next(chd_file *chd, UINT64 prevoffset, UINT64 nextoffset);
static chd_error metadata_set_length(chd_file *chd, UINT64 offset, UINT32 length);

/* zlib compression codec */
static chd_error zlib_codec_init(chd_file *chd);
static void zlib_codec_free(chd_file *chd);
static chd_error zlib_codec_compress(chd_file *chd, const void *src, UINT32 *length);
static chd_error zlib_codec_decompress(chd_file *chd, UINT32 srclength, void *dest);
static voidpf zlib_fast_alloc(voidpf opaque, uInt items, uInt size);
static void zlib_fast_free(voidpf opaque, voidpf address);

/* multifile routines */
static multi_file *multi_open(const char *filename, const char *mode);
static void multi_close(multi_file *file);
static UINT32 multi_read(multi_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 multi_write(multi_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 multi_length(multi_file *file);



/***************************************************************************
    CODEC INTERFACES
***************************************************************************/

static const codec_interface codec_interfaces[] =
{
	/* "none" or no compression */
	{
		CHDCOMPRESSION_NONE,
		"none",
		FALSE,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	},

	/* standard zlib compression */
	{
		CHDCOMPRESSION_ZLIB,
		"zlib",
		FALSE,
		zlib_codec_init,
		zlib_codec_free,
		zlib_codec_compress,
		zlib_codec_decompress,
		NULL
	},

	/* zlib+ compression */
	{
		CHDCOMPRESSION_ZLIB_PLUS,
		"zlib+",
		FALSE,
		zlib_codec_init,
		zlib_codec_free,
		zlib_codec_compress,
		zlib_codec_decompress,
		NULL
	}
};



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    get_bigendian_uint64 - fetch a UINT64 from
    the data stream in bigendian order
-------------------------------------------------*/

INLINE UINT64 get_bigendian_uint64(const UINT8 *base)
{
	return ((UINT64)base[0] << 56) | ((UINT64)base[1] << 48) | ((UINT64)base[2] << 40) | ((UINT64)base[3] << 32) |
			((UINT64)base[4] << 24) | ((UINT64)base[5] << 16) | ((UINT64)base[6] << 8) | (UINT64)base[7];
}


/*-------------------------------------------------
    put_bigendian_uint64 - write a UINT64 to
    the data stream in bigendian order
-------------------------------------------------*/

INLINE void put_bigendian_uint64(UINT8 *base, UINT64 value)
{
	base[0] = value >> 56;
	base[1] = value >> 48;
	base[2] = value >> 40;
	base[3] = value >> 32;
	base[4] = value >> 24;
	base[5] = value >> 16;
	base[6] = value >> 8;
	base[7] = value;
}


/*-------------------------------------------------
    get_bigendian_uint32 - fetch a UINT32 from
    the data stream in bigendian order
-------------------------------------------------*/

INLINE UINT32 get_bigendian_uint32(const UINT8 *base)
{
	return (base[0] << 24) | (base[1] << 16) | (base[2] << 8) | base[3];
}


/*-------------------------------------------------
    put_bigendian_uint32 - write a UINT32 to
    the data stream in bigendian order
-------------------------------------------------*/

INLINE void put_bigendian_uint32(UINT8 *base, UINT32 value)
{
	base[0] = value >> 24;
	base[1] = value >> 16;
	base[2] = value >> 8;
	base[3] = value;
}


/*-------------------------------------------------
    get_bigendian_uint16 - fetch a UINT16 from
    the data stream in bigendian order
-------------------------------------------------*/

INLINE UINT16 get_bigendian_uint16(const UINT8 *base)
{
	return (base[0] << 8) | base[1];
}


/*-------------------------------------------------
    put_bigendian_uint16 - write a UINT16 to
    the data stream in bigendian order
-------------------------------------------------*/

INLINE void put_bigendian_uint16(UINT8 *base, UINT16 value)
{
	base[0] = value >> 8;
	base[1] = value;
}


/*-------------------------------------------------
    map_extract - extract a single map
    entry from the datastream
-------------------------------------------------*/

INLINE void map_extract(const UINT8 *base, map_entry *entry)
{
	entry->offset = get_bigendian_uint64(&base[0]);
	entry->crc = get_bigendian_uint32(&base[8]);
	entry->length = get_bigendian_uint16(&base[12]) | (base[14] << 16);
	entry->flags = base[15];
}


/*-------------------------------------------------
    map_assemble - write a single map
    entry to the datastream
-------------------------------------------------*/

INLINE void map_assemble(UINT8 *base, map_entry *entry)
{
	put_bigendian_uint64(&base[0], entry->offset);
	put_bigendian_uint32(&base[8], entry->crc);
	put_bigendian_uint16(&base[12], entry->length);
	base[14] = entry->length >> 16;
	base[15] = entry->flags;
}


/*-------------------------------------------------
    map_extract_old - extract a single map
    entry in old format from the datastream
-------------------------------------------------*/

INLINE void map_extract_old(const UINT8 *base, map_entry *entry, UINT32 hunkbytes)
{
	entry->offset = get_bigendian_uint64(&base[0]);
	entry->crc = 0;
	entry->length = entry->offset >> 44;
	entry->flags = MAP_ENTRY_FLAG_NO_CRC | ((entry->length == hunkbytes) ? MAP_ENTRY_TYPE_UNCOMPRESSED : MAP_ENTRY_TYPE_COMPRESSED);
#ifdef __MWERKS__
	entry->offset = entry->offset & 0x00000FFFFFFFFFFFLL;
#else
	entry->offset = (entry->offset << 20) >> 20;
#endif
}


/*-------------------------------------------------
    queue_async_operation - queue a new work
    item
-------------------------------------------------*/

INLINE int queue_async_operation(chd_file *chd, osd_work_callback callback)
{
	/* if no queue yet, create one on the fly */
	if (chd->workqueue == NULL)
	{
		chd->workqueue = osd_work_queue_alloc(WORK_QUEUE_FLAG_IO);
		if (chd->workqueue == NULL)
			return FALSE;
	}

	/* make sure we cleared out the previous item */
	if (chd->workitem != NULL)
		return FALSE;

	/* create a new work item to run the job */
	chd->workitem = osd_work_item_queue(chd->workqueue, callback, chd);
	if (chd->workitem == NULL)
		return FALSE;

	return TRUE;
}


/*-------------------------------------------------
    wait_for_pending_async - wait for any pending
    async
-------------------------------------------------*/

INLINE void wait_for_pending_async(chd_file *chd)
{
	/* if something is pending, wait for it */
	if (chd->workitem != NULL)
	{
		/* 10 seconds should be enough for anything! */
		int wait_successful = osd_work_item_wait(chd->workitem, 10 * osd_ticks_per_second());
		if (!wait_successful)
			osd_break_into_debugger("Pending async operation never completed!");
	}
}



/***************************************************************************
    EXTERNAL INTERFACING
***************************************************************************/

/*-------------------------------------------------
    chd_set_interface - set the interface for
    CHD accesses
-------------------------------------------------*/

void chd_set_interface(chd_interface *new_interface)
{
	if (new_interface != NULL)
		cur_interface = *new_interface;
	else
		memset(&cur_interface, 0, sizeof(cur_interface));
}



/***************************************************************************
    CHD FILE MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    chd_create - create a new CHD file
-------------------------------------------------*/

chd_error chd_create(const char *filename, UINT64 logicalbytes, UINT32 hunkbytes, UINT32 compression, chd_file *parent)
{
	multi_file *file = NULL;
	chd_file *newchd = NULL;
	chd_header header;
	chd_error err;
	int intfnum;

	/* punt if no interface */
	if (cur_interface.open == NULL)
		EARLY_EXIT(err = CHDERR_NO_INTERFACE);

	/* verify parameters */
	if (filename == NULL)
		EARLY_EXIT(err = CHDERR_FILE_NOT_FOUND);
	if (parent == NULL && (logicalbytes == 0 || hunkbytes == 0))
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);

	/* verify the compression type */
	for (intfnum = 0; intfnum < ARRAY_LENGTH(codec_interfaces); intfnum++)
		if (codec_interfaces[intfnum].compression == compression)
			break;
	if (intfnum == ARRAY_LENGTH(codec_interfaces))
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);

	/* if we have a parent, the sizes come from there */
	if (parent != NULL)
	{
		logicalbytes = parent->header.logicalbytes;
		hunkbytes = parent->header.hunkbytes;
	}

	/* if we have a parent, it must be V3 or later */
	if (parent != NULL && parent->header.version < 3)
		EARLY_EXIT(err = CHDERR_UNSUPPORTED_VERSION);

	/* build the header */
	memset(&header, 0, sizeof(header));
	header.length = CHD_V3_HEADER_SIZE;
	header.version = CHD_HEADER_VERSION;
	header.flags = CHDFLAGS_IS_WRITEABLE;
	header.compression = compression;
	header.hunkbytes = hunkbytes;
	header.totalhunks = (logicalbytes + hunkbytes - 1) / hunkbytes;
	header.logicalbytes = logicalbytes;

	/* tweaks if there is a parent */
	if (parent != NULL)
	{
		header.flags |= CHDFLAGS_HAS_PARENT;
		memcpy(&header.parentmd5[0], &parent->header.md5[0], sizeof(header.parentmd5));
		memcpy(&header.parentsha1[0], &parent->header.sha1[0], sizeof(header.parentsha1));
	}

	/* validate it */
	err = header_validate(&header);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* attempt to create the file */
	file = multi_open(filename, "wb");
	if (file == NULL)
		EARLY_EXIT(err = CHDERR_CANT_CREATE_FILE);

	/* write the resulting header */
	err = header_write(file, &header);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* create an initial map */
	err = map_write_initial(file, parent, &header);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* all done */
	multi_close(file);
	file = NULL;

	/* if we have a parent, clone the metadata */
	if (parent != NULL)
	{
		/* open the new CHD via the standard mechanism */
		err = chd_open(filename, CHD_OPEN_READWRITE, parent, &newchd);
		if (err != CHDERR_NONE)
			EARLY_EXIT(err);

		/* close the metadata */
		err = chd_clone_metadata(parent, newchd);
		if (err != CHDERR_NONE)
			EARLY_EXIT(err);

		/* close the CHD */
		chd_close(newchd);
	}

	return CHDERR_NONE;

cleanup:
	if (file != NULL)
		multi_close(file);
	if (newchd != NULL)
		chd_close(newchd);
	return err;
}


/*-------------------------------------------------
    chd_open - open a CHD file for access
-------------------------------------------------*/

chd_error chd_open(const char *filename, int mode, chd_file *parent, chd_file **chd)
{
	chd_file *newchd = NULL;
	chd_file **currptr;
	chd_error err;
	int intfnum;

	/* punt if no interface */
	if (cur_interface.open == NULL)
		EARLY_EXIT(err = CHDERR_NO_INTERFACE);

	/* verify parameters */
	if (filename == NULL)
		EARLY_EXIT(err = CHDERR_FILE_NOT_FOUND);

	/* punt if invalid parent */
	if (parent != NULL && parent->cookie != COOKIE_VALUE)
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);

	/* allocate memory for the final result */
	newchd = malloc(sizeof(**chd));
	if (newchd == NULL)
		EARLY_EXIT(err = CHDERR_OUT_OF_MEMORY);
	memset(newchd, 0, sizeof(*newchd));
	newchd->cookie = COOKIE_VALUE;
	newchd->parent = parent;

	/* first attempt to open the file */
	newchd->file = multi_open(filename, (mode == CHD_OPEN_READWRITE) ? "rb+" : "rb");
	if (newchd->file == NULL)
		EARLY_EXIT(err = CHDERR_FILE_NOT_FOUND);

	/* now attempt to read the header */
	err = header_read(newchd->file, &newchd->header);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* validate the header */
	err = header_validate(&newchd->header);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* make sure we don't open a read-only file writeable */
	if (mode == CHD_OPEN_READWRITE && !(newchd->header.flags & CHDFLAGS_IS_WRITEABLE))
		EARLY_EXIT(err = CHDERR_FILE_NOT_WRITEABLE);

	/* also, never open an older version writeable */
	if (mode == CHD_OPEN_READWRITE && newchd->header.version < CHD_HEADER_VERSION)
		EARLY_EXIT(err = CHDERR_UNSUPPORTED_VERSION);

	/* if we need a parent, make sure we have one */
	if (parent == NULL && (newchd->header.flags & CHDFLAGS_HAS_PARENT))
		EARLY_EXIT(err = CHDERR_REQUIRES_PARENT);

	/* make sure we have a valid parent */
	if (parent != NULL)
	{
		/* check MD5 if it isn't empty */
		if (memcmp(nullmd5, newchd->header.parentmd5, sizeof(newchd->header.parentmd5)) != 0 &&
			memcmp(nullmd5, newchd->parent->header.md5, sizeof(newchd->parent->header.md5)) != 0 &&
			memcmp(newchd->parent->header.md5, newchd->header.parentmd5, sizeof(newchd->header.parentmd5)) != 0)
			EARLY_EXIT(err = CHDERR_INVALID_PARENT);

		/* check SHA1 if it isn't empty */
		if (memcmp(nullsha1, newchd->header.parentsha1, sizeof(newchd->header.parentsha1)) != 0 &&
			memcmp(nullsha1, newchd->parent->header.sha1, sizeof(newchd->parent->header.sha1)) != 0 &&
			memcmp(newchd->parent->header.sha1, newchd->header.parentsha1, sizeof(newchd->header.parentsha1)) != 0)
			EARLY_EXIT(err = CHDERR_INVALID_PARENT);
	}

	/* now read the hunk map */
	err = map_read(newchd);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* allocate and init the hunk cache */
	newchd->cache = malloc(newchd->header.hunkbytes);
	newchd->compare = malloc(newchd->header.hunkbytes);
	if (newchd->cache == NULL || newchd->compare == NULL)
		EARLY_EXIT(err = CHDERR_OUT_OF_MEMORY);
	newchd->cachehunk = ~0;
	newchd->comparehunk = ~0;

	/* allocate the temporary compressed buffer */
	newchd->compressed = malloc(newchd->header.hunkbytes);
	if (newchd->compressed == NULL)
		EARLY_EXIT(err = CHDERR_OUT_OF_MEMORY);

	/* find the codec interface */
	for (intfnum = 0; intfnum < ARRAY_LENGTH(codec_interfaces); intfnum++)
		if (codec_interfaces[intfnum].compression == newchd->header.compression)
		{
			newchd->codecintf = &codec_interfaces[intfnum];
			break;
		}
	if (intfnum == ARRAY_LENGTH(codec_interfaces))
		EARLY_EXIT(err = CHDERR_UNSUPPORTED_FORMAT);

	/* initialize the codec */
	if (newchd->codecintf->init != NULL)
		err = (*newchd->codecintf->init)(newchd);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* hook us to the end of the global list */
	for (currptr = &first_file; *currptr != NULL; currptr = &(*currptr)->next) ;
	*chd = *currptr = newchd;

	/* all done */
	return CHDERR_NONE;

cleanup:
	if (newchd != NULL)
		chd_close(newchd);
	return err;
}


/*-------------------------------------------------
    chd_close - close a CHD file for access
-------------------------------------------------*/

void chd_close(chd_file *chd)
{
	chd_file **currptr;

	/* punt if NULL or invalid */
	if (chd == NULL || chd->cookie != COOKIE_VALUE)
		return;

	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* kill the work queue and any work item */
	if (chd->workitem != NULL)
		osd_work_item_release(chd->workitem);
	if (chd->workqueue != NULL)
		osd_work_queue_free(chd->workqueue);

	/* deinit the codec */
	if (chd->codecintf != NULL && chd->codecintf->free != NULL)
		(*chd->codecintf->free)(chd);

	/* free the compressed data buffer */
	if (chd->compressed != NULL)
		free(chd->compressed);

	/* free the hunk cache and compare data */
	if (chd->compare != NULL)
		free(chd->compare);
	if (chd->cache != NULL)
		free(chd->cache);

	/* free the hunk map */
	if (chd->map != NULL)
		free(chd->map);

	/* free the CRC table */
	if (chd->crctable != NULL)
		free(chd->crctable);

	/* free the CRC map */
	if (chd->crcmap != NULL)
		free(chd->crcmap);

	/* close the file */
	if (chd->file != NULL)
		multi_close(chd->file);

	/* unlink ourselves */
	for (currptr = &first_file; *currptr != NULL; currptr = &(*currptr)->next)
		if (*currptr == chd)
		{
			*currptr = (*currptr)->next;
			break;
		}

#if PRINTF_MAX_HUNK
	printf("Max hunk = %d/%d\n", chd->maxhunk, chd->header.totalhunks);
#endif

	/* free our memory */
	free(chd);
}


/*-------------------------------------------------
    chd_close_all - close all open CHD files
-------------------------------------------------*/

void chd_close_all(void)
{
	while (first_file != NULL)
		chd_close(first_file);
}


/*-------------------------------------------------
    chd_multi_filename - compute the indexed CHD
    filename
-------------------------------------------------*/

void chd_multi_filename(const char *origname, char *finalname, int index)
{
	char *extension;
	char findex[5];

	/* copy the original name */
	strcpy(finalname, origname);

	/* determine the offset of the extension period */
	extension = strchr(finalname, '.');
	if (extension == NULL)
		extension = finalname + strlen(finalname);

	/* compute our extension */
	sprintf(findex, ".%3d", index);
	if (findex[1] == ' ')
		findex[1] = 'c';
	if (findex[2] == ' ')
		findex[2] = 'h';
	strcpy(extension, findex);
}



/***************************************************************************
    CHD HEADER MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    chd_get_header - return a pointer to the
    extracted header data
-------------------------------------------------*/

const chd_header *chd_get_header(chd_file *chd)
{
	/* punt if NULL or invalid */
	if (chd == NULL || chd->cookie != COOKIE_VALUE)
		return NULL;

	return &chd->header;
}


/*-------------------------------------------------
    chd_set_header - write the current header to
    the file
-------------------------------------------------*/

chd_error chd_set_header(const char *filename, const chd_header *header)
{
	multi_file *file = NULL;
	chd_header oldheader;
	chd_error err;

	/* punt if no interface */
	if (cur_interface.open == NULL)
		EARLY_EXIT(err = CHDERR_NO_INTERFACE);

	/* punt if NULL or invalid */
	if (filename == NULL || header == NULL)
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);

	/* validate the header */
	err = header_validate(header);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* attempt to open the file */
	file = multi_open(filename, "rb+");
	if (file == NULL)
		EARLY_EXIT(err = CHDERR_FILE_NOT_FOUND);

	/* read the old header */
	err = header_read(file, &oldheader);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* make sure we're only making valid changes */
	if (header->length != oldheader.length)
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);
	if (header->version != oldheader.version)
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);
	if (header->compression != oldheader.compression)
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);
	if (header->hunkbytes != oldheader.hunkbytes)
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);
	if (header->totalhunks != oldheader.totalhunks)
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);
	if (header->metaoffset != oldheader.metaoffset)
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);
	if (header->obsolete_hunksize != oldheader.obsolete_hunksize)
		EARLY_EXIT(err = CHDERR_INVALID_PARAMETER);

	/* write the new header */
	err = header_write(file, header);
	if (err != CHDERR_NONE)
		EARLY_EXIT(err);

	/* close the file and return */
	multi_close(file);
	return CHDERR_NONE;

cleanup:
	if (file)
		multi_close(file);
	return err;
}



/***************************************************************************
    CORE DATA READ/WRITE
***************************************************************************/

/*-------------------------------------------------
    chd_read - read a single hunk from the CHD
    file
-------------------------------------------------*/

chd_error chd_read(chd_file *chd, UINT32 hunknum, void *buffer)
{
	/* punt if NULL or invalid */
	if (chd == NULL || chd->cookie != COOKIE_VALUE)
		return CHDERR_INVALID_PARAMETER;

	/* if we're past the end, fail */
	if (hunknum >= chd->header.totalhunks)
		return CHDERR_HUNK_OUT_OF_RANGE;

	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* perform the read */
	return hunk_read_into_memory(chd, hunknum, buffer);
}


/*-------------------------------------------------
    chd_read_async - read a single hunk from the
    CHD file asynchronously
-------------------------------------------------*/

chd_error chd_read_async(chd_file *chd, UINT32 hunknum, void *buffer)
{
	/* punt if NULL or invalid */
	if (chd == NULL || chd->cookie != COOKIE_VALUE)
		return CHDERR_INVALID_PARAMETER;

	/* if we're past the end, fail */
	if (hunknum >= chd->header.totalhunks)
		return CHDERR_HUNK_OUT_OF_RANGE;

	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* set the async parameters */
	chd->async_hunknum = hunknum;
	chd->async_buffer = buffer;

	/* queue the work item */
	if (queue_async_operation(chd, async_read_callback))
		return CHDERR_OPERATION_PENDING;

	/* if we fail, fall back on the sync version */
	return chd_read(chd, hunknum, buffer);
}


/*-------------------------------------------------
    chd_write - write a single hunk to the CHD
    file
-------------------------------------------------*/

chd_error chd_write(chd_file *chd, UINT32 hunknum, const void *buffer)
{
	/* punt if NULL or invalid */
	if (chd == NULL || chd->cookie != COOKIE_VALUE)
		return CHDERR_INVALID_PARAMETER;

	/* if we're past the end, fail */
	if (hunknum >= chd->header.totalhunks)
		return CHDERR_HUNK_OUT_OF_RANGE;

	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* then write out the hunk */
	return hunk_write_from_memory(chd, hunknum, buffer);
}


/*-------------------------------------------------
    chd_write_async - write a single hunk to the
    CHD file asynchronously
-------------------------------------------------*/

chd_error chd_write_async(chd_file *chd, UINT32 hunknum, const void *buffer)
{
	/* punt if NULL or invalid */
	if (chd == NULL || chd->cookie != COOKIE_VALUE)
		return CHDERR_INVALID_PARAMETER;

	/* if we're past the end, fail */
	if (hunknum >= chd->header.totalhunks)
		return CHDERR_HUNK_OUT_OF_RANGE;

	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* set the async parameters */
	chd->async_hunknum = hunknum;
	chd->async_buffer = (void *)buffer;

	/* queue the work item */
	if (queue_async_operation(chd, async_write_callback))
		return CHDERR_OPERATION_PENDING;

	/* if we fail, fall back on the sync version */
	return chd_write(chd, hunknum, buffer);
}


/*-------------------------------------------------
    chd_async_complete - get the result of a
    completed work item and clear it out of the
    system
-------------------------------------------------*/

chd_error chd_async_complete(chd_file *chd)
{
	void *result;

	/* if nothing present, return an error */
	if (chd->workitem == NULL)
		return CHDERR_NO_ASYNC_OPERATION;

	/* wait for the work to complete */
	wait_for_pending_async(chd);

	/* get the result and free the work item */
	result = osd_work_item_result(chd->workitem);
	osd_work_item_release(chd->workitem);
	chd->workitem = NULL;

	return (chd_error)result;
}



/***************************************************************************
    METADATA MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    chd_get_metadata - get the indexed metadata
    of the given type
-------------------------------------------------*/

chd_error chd_get_metadata(chd_file *chd, UINT32 searchtag, UINT32 searchindex, void *output, UINT32 outputlen, UINT32 *resultlen, UINT32 *resulttag)
{
	metadata_entry metaentry;
	chd_error err;
	UINT32 count;

	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* if we didn't find it, just return */
	err = metadata_find_entry(chd, searchtag, searchindex, &metaentry);
	if (err != CHDERR_NONE)
	{
		/* unless we're an old version and they are requesting hard disk metadata */
		if (chd->header.version < 3 && (searchtag == HARD_DISK_METADATA_TAG || searchtag == CHDMETATAG_WILDCARD) && searchindex == 0)
		{
			char faux_metadata[256];
			UINT32 faux_length;

			/* fill in the faux metadata */
			sprintf(faux_metadata, HARD_DISK_METADATA_FORMAT, chd->header.obsolete_cylinders, chd->header.obsolete_heads, chd->header.obsolete_sectors, chd->header.hunkbytes / chd->header.obsolete_hunksize);
			faux_length = (UINT32)strlen(faux_metadata) + 1;

			/* copy the metadata itself */
			memcpy(output, faux_metadata, MIN(outputlen, faux_length));

			/* return the length of the data and the tag */
			if (resultlen != NULL)
				*resultlen = faux_length;
			if (resulttag != NULL)
				*resulttag = HARD_DISK_METADATA_TAG;
			return CHDERR_NONE;
		}
		return err;
	}

	/* read the metadata */
	outputlen = MIN(outputlen, metaentry.length);
	count = multi_read(chd->file, metaentry.offset + METADATA_HEADER_SIZE, outputlen, output);
	if (count != outputlen)
		return CHDERR_READ_ERROR;

	/* return the length of the data and the tag */
	if (resultlen != NULL)
		*resultlen = metaentry.length;
	if (resulttag != NULL)
		*resulttag = metaentry.metatag;
	return CHDERR_NONE;
}


/*-------------------------------------------------
    chd_set_metadata - write the indexed metadata
    of the given type
-------------------------------------------------*/

chd_error chd_set_metadata(chd_file *chd, UINT32 metatag, UINT32 metaindex, const void *inputbuf, UINT32 inputlen)
{
	UINT8 raw_meta_header[METADATA_HEADER_SIZE];
	metadata_entry metaentry;
	chd_error err;
	UINT64 offset;
	UINT32 count;

	/* if the disk is an old version, punt */
	if (chd->header.version < 3)
		return CHDERR_NOT_SUPPORTED;

	/* if the disk isn't writeable, punt */
	if (!(chd->header.flags & CHDFLAGS_IS_WRITEABLE))
		return CHDERR_FILE_NOT_WRITEABLE;

	/* must write at least 1 byte */
	if (inputlen < 1)
		return CHDERR_INVALID_PARAMETER;

	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* find the entry if it already exists */
	err = metadata_find_entry(chd, metatag, metaindex, &metaentry);

	/* if it's there and it fits, just overwrite */
	if (err == CHDERR_NONE && inputlen <= metaentry.length)
	{
		/* overwrite the original data with our new input data */
		count = multi_write(chd->file, metaentry.offset + METADATA_HEADER_SIZE, inputlen, inputbuf);
		if (count != inputlen)
			return CHDERR_WRITE_ERROR;

		/* if the lengths don't match, we need to update the length in our header */
		if (inputlen != metaentry.length)
		{
			err = metadata_set_length(chd, metaentry.offset, inputlen);
			if (err != CHDERR_NONE)
				return err;
		}
		return CHDERR_NONE;
	}

	/* if we already have an entry, unlink it */
	if (err == CHDERR_NONE)
	{
		err = metadata_set_previous_next(chd, metaentry.prev, metaentry.next);
		if (err != CHDERR_NONE)
			return err;
	}

	/* now build us a new entry */
	put_bigendian_uint32(&raw_meta_header[0], metatag);
	put_bigendian_uint32(&raw_meta_header[4], inputlen);
	put_bigendian_uint64(&raw_meta_header[8], (err == CHDERR_NONE) ? metaentry.next : 0);

	/* write out the new header */
	offset = multi_length(chd->file);
	count = multi_write(chd->file, offset, sizeof(raw_meta_header), raw_meta_header);
	if (count != sizeof(raw_meta_header))
		return CHDERR_WRITE_ERROR;

	/* follow that with the data */
	count = multi_write(chd->file, offset + METADATA_HEADER_SIZE, inputlen, inputbuf);
	if (count != inputlen)
		return CHDERR_WRITE_ERROR;

	/* set the previous entry to point to us */
	err = metadata_set_previous_next(chd, metaentry.prev, offset);
	if (err != CHDERR_NONE)
		return err;

	return CHDERR_NONE;
}


/*-------------------------------------------------
    chd_clone_metadata - clone the metadata from
    one CHD to a second
-------------------------------------------------*/

chd_error chd_clone_metadata(chd_file *source, chd_file *dest)
{
	UINT32 metatag, metasize, metaindex;
	UINT8 metabuffer[1024];
	chd_error err;

	/* clone the metadata */
	for (metaindex = 0; ; metaindex++)
	{
		/* fetch the next piece of metadata */
		err = chd_get_metadata(source, CHDMETATAG_WILDCARD, metaindex, metabuffer, sizeof(metabuffer), &metasize, &metatag);
		if (err != CHDERR_NONE)
		{
			if (err == CHDERR_METADATA_NOT_FOUND)
				err = CHDERR_NONE;
			break;
		}

		/* if that fit, just write it back from the temporary buffer */
		if (metasize <= sizeof(metabuffer))
		{
			/* write it to the target */
			err = chd_set_metadata(dest, metatag, CHD_METAINDEX_APPEND, metabuffer, metasize);
			if (err != CHDERR_NONE)
				break;
		}

		/* otherwise, allocate a bigger temporary buffer */
		else
		{
			UINT8 *allocbuffer = malloc(metasize);
			if (allocbuffer == NULL)
			{
				err = CHDERR_OUT_OF_MEMORY;
				break;
			}

			/* re-read the whole thing */
			err = chd_get_metadata(source, CHDMETATAG_WILDCARD, metaindex, allocbuffer, metasize, &metasize, &metatag);
			if (err != CHDERR_NONE)
			{
				free(allocbuffer);
				break;
			}

			/* write it to the target */
			err = chd_set_metadata(dest, metatag, CHD_METAINDEX_APPEND, allocbuffer, metasize);
			free(allocbuffer);
			if (err != CHDERR_NONE)
				break;
		}
	}
	return err;
}



/***************************************************************************
    COMPRESSION MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    chd_compress_begin - begin compressing data
    into a CHD
-------------------------------------------------*/

chd_error chd_compress_begin(chd_file *chd)
{
	chd_error err;

	/* punt if no interface */
	if (cur_interface.open == NULL)
		return CHDERR_NO_INTERFACE;

	/* verify parameters */
	if (chd == NULL)
		return CHDERR_INVALID_PARAMETER;

	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* mark the CHD writeable and write the updated header */
	chd->header.flags |= CHDFLAGS_IS_WRITEABLE;
	err = header_write(chd->file, &chd->header);
	if (err != CHDERR_NONE)
		return err;

	/* create CRC maps for the new CHD and the parent */
	crcmap_init(chd, FALSE);
	if (chd->parent != NULL)
		crcmap_init(chd->parent, TRUE);

	/* init the MD5/SHA1 computations */
	MD5Init(&chd->compmd5);
	sha1_init(&chd->compsha1);
	chd->compressing = TRUE;
	chd->comphunk = 0;

	return CHDERR_NONE;
}


/*-------------------------------------------------
    chd_compress_hunk - append data to a CHD
    that is being compressed
-------------------------------------------------*/

chd_error chd_compress_hunk(chd_file *chd, const void *data, double *curratio)
{
	UINT32 thishunk = chd->comphunk++;
	UINT64 sourceoffset = (UINT64)thishunk * (UINT64)chd->header.hunkbytes;
	UINT32 bytestochecksum;
	const void *crcdata;
	chd_error err;

	/* error if in the wrong state */
	if (!chd->compressing)
		return CHDERR_INVALID_STATE;

	/* write out the hunk */
	err = hunk_write_from_memory(chd, thishunk, data);
	if (err != CHDERR_NONE)
		return err;

	/* if we are lossy, then we need to use the decompressed version in */
	/* the cache as our MD5/SHA1 source */
	crcdata = chd->codecintf->lossy ? chd->cache : data;

	/* update the MD5/SHA1 */
	bytestochecksum = chd->header.hunkbytes;
	if (sourceoffset + chd->header.hunkbytes > chd->header.logicalbytes)
	{
		if (sourceoffset >= chd->header.logicalbytes)
			bytestochecksum = 0;
		else
			bytestochecksum = chd->header.logicalbytes - sourceoffset;
	}
	if (bytestochecksum > 0)
	{
		MD5Update(&chd->compmd5, crcdata, bytestochecksum);
		sha1_update(&chd->compsha1, bytestochecksum, crcdata);
	}

	/* update our CRC map */
	if ((chd->map[thishunk].flags & MAP_ENTRY_FLAG_TYPE_MASK) != MAP_ENTRY_TYPE_SELF_HUNK &&
		(chd->map[thishunk].flags & MAP_ENTRY_FLAG_TYPE_MASK) != MAP_ENTRY_TYPE_PARENT_HUNK)
		crcmap_add_entry(chd, thishunk);

	/* update the ratio */
	if (curratio != NULL)
	{
		UINT64 curlength = multi_length(chd->file);
		*curratio = 1.0 - (double)curlength / (double)((UINT64)chd->comphunk * (UINT64)chd->header.hunkbytes);
	}

	return CHDERR_NONE;
}


/*-------------------------------------------------
    chd_compress_finish - complete compression of
    a CHD
-------------------------------------------------*/

chd_error chd_compress_finish(chd_file *chd)
{
	/* error if in the wrong state */
	if (!chd->compressing)
		return CHDERR_INVALID_STATE;

	/* compute the final MD5/SHA1 values */
	MD5Final(chd->header.md5, &chd->compmd5);
	sha1_final(&chd->compsha1);
	sha1_digest(&chd->compsha1, SHA1_DIGEST_SIZE, chd->header.sha1);

	/* turn off the writeable flag and re-write the header */
	chd->header.flags &= ~CHDFLAGS_IS_WRITEABLE;
	chd->compressing = FALSE;
	return header_write(chd->file, &chd->header);
}



/***************************************************************************
    VERIFICATION
***************************************************************************/

/*-------------------------------------------------
    chd_verify_begin - begin compressing data
    into a CHD
-------------------------------------------------*/

chd_error chd_verify_begin(chd_file *chd)
{
	/* punt if no interface */
	if (cur_interface.open == NULL)
		return CHDERR_NO_INTERFACE;

	/* verify parameters */
	if (chd == NULL)
		return CHDERR_INVALID_PARAMETER;

	/* if this is a writeable file image, we can't verify */
	if (chd->header.flags & CHDFLAGS_IS_WRITEABLE)
		return CHDERR_CANT_VERIFY;

	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* init the MD5/SHA1 computations */
	MD5Init(&chd->vermd5);
	sha1_init(&chd->versha1);
	chd->verifying = TRUE;
	chd->verhunk = 0;

	return CHDERR_NONE;
}


/*-------------------------------------------------
    chd_verify_hunk - verify the next hunk in
    the CHD
-------------------------------------------------*/

chd_error chd_verify_hunk(chd_file *chd)
{
	UINT32 thishunk = chd->verhunk++;
	UINT64 hunkoffset = (UINT64)thishunk * (UINT64)chd->header.hunkbytes;
	map_entry *entry;
	chd_error err;

	/* error if in the wrong state */
	if (!chd->verifying)
		return CHDERR_INVALID_STATE;

	/* read the hunk into the cache */
	err = hunk_read_into_cache(chd, thishunk);
	if (err != CHDERR_NONE)
		return err;

	/* update the MD5/SHA1 */
	if (hunkoffset < chd->header.logicalbytes)
	{
		UINT64 bytestochecksum = MIN(chd->header.hunkbytes, chd->header.logicalbytes - hunkoffset);
		if (bytestochecksum > 0)
		{
			MD5Update(&chd->vermd5, chd->cache, bytestochecksum);
			sha1_update(&chd->versha1, bytestochecksum, chd->cache);
		}
	}

	/* validate the CRC if we have one */
	entry = &chd->map[thishunk];
	if (!(entry->flags & MAP_ENTRY_FLAG_NO_CRC) && entry->crc != crc32(0, chd->cache, chd->header.hunkbytes))
		return CHDERR_DECOMPRESSION_ERROR;

	return CHDERR_NONE;
}


/*-------------------------------------------------
    chd_verify_finish - finish verification of
    the CHD
-------------------------------------------------*/

chd_error chd_verify_finish(chd_file *chd, UINT8 *finalmd5, UINT8 *finalsha1)
{
	/* error if in the wrong state */
	if (!chd->verifying)
		return CHDERR_INVALID_STATE;

	/* compute the final MD5 */
	if (finalmd5 != NULL)
		MD5Final(finalmd5, &chd->vermd5);

	/* compute the final SHA1 */
	if (finalsha1 != NULL)
	{
		sha1_final(&chd->versha1);
		sha1_digest(&chd->versha1, SHA1_DIGEST_SIZE, finalsha1);
	}

	/* return an error */
	chd->verifying = FALSE;
	return (chd->verhunk < chd->header.totalhunks) ? CHDERR_VERIFY_INCOMPLETE : CHDERR_NONE;
}



/***************************************************************************
    CODEC INTERFACES
***************************************************************************/

/*-------------------------------------------------
    chd_codec_config - set internal codec
    parameters
-------------------------------------------------*/

chd_error chd_codec_config(chd_file *chd, int param, void *config)
{
	/* wait for any pending async operations */
	wait_for_pending_async(chd);

	/* if the codec has a configuration callback, call through to it */
	if (chd->codecintf->config != NULL)
		return (*chd->codecintf->config)(chd, param, config);

	return CHDERR_INVALID_PARAMETER;
}


/*-------------------------------------------------
    chd_get_codec_name - get the name of a
    particular codec
-------------------------------------------------*/

const char *chd_get_codec_name(UINT32 codec)
{
	int intfnum;

	/* look for a matching codec and return its string */
	for (intfnum = 0; intfnum < ARRAY_LENGTH(codec_interfaces); intfnum++)
		if (codec_interfaces[intfnum].compression == codec)
			return codec_interfaces[intfnum].compname;

	return "Unknown";
}



/***************************************************************************
    INTERNAL ASYNC OPERATIONS
***************************************************************************/

/*-------------------------------------------------
    async_read_callback - asynchronous reading
    callback
-------------------------------------------------*/

static void *async_read_callback(void *param)
{
	chd_file *chd = param;
	chd_error err;

	/* read the hunk into the cache */
	err = hunk_read_into_memory(chd, chd->async_hunknum, chd->async_buffer);

	/* return the error */
	return (void *)err;
}


/*-------------------------------------------------
    async_write_callback - asynchronous writing
    callback
-------------------------------------------------*/

static void *async_write_callback(void *param)
{
	chd_file *chd = param;
	chd_error err;

	/* write the hunk from memory */
	err = hunk_write_from_memory(chd, chd->async_hunknum, chd->async_buffer);

	/* return the error */
	return (void *)err;
}



/***************************************************************************
    INTERNAL HEADER OPERATIONS
***************************************************************************/

/*-------------------------------------------------
    header_validate - check the validity of a
    CHD header
-------------------------------------------------*/

static chd_error header_validate(const chd_header *header)
{
	int intfnum;

	/* require a valid version */
	if (header->version == 0 || header->version > CHD_HEADER_VERSION)
		return CHDERR_UNSUPPORTED_VERSION;

	/* require a valid length */
	if ((header->version == 1 && header->length != CHD_V1_HEADER_SIZE) ||
		(header->version == 2 && header->length != CHD_V2_HEADER_SIZE) ||
		(header->version == 3 && header->length != CHD_V3_HEADER_SIZE))
		return CHDERR_INVALID_PARAMETER;

	/* require valid flags */
	if (header->flags & CHDFLAGS_UNDEFINED)
		return CHDERR_INVALID_PARAMETER;

	/* require a supported compression mechanism */
	for (intfnum = 0; intfnum < ARRAY_LENGTH(codec_interfaces); intfnum++)
		if (codec_interfaces[intfnum].compression == header->compression)
			break;
	if (intfnum == ARRAY_LENGTH(codec_interfaces))
		return CHDERR_INVALID_PARAMETER;

	/* require a valid hunksize */
	if (header->hunkbytes == 0 || header->hunkbytes >= 65536 * 256)
		return CHDERR_INVALID_PARAMETER;

	/* require a valid hunk count */
	if (header->totalhunks == 0)
		return CHDERR_INVALID_PARAMETER;

	/* require a valid MD5 and/or SHA1 if we're using a parent */
	if ((header->flags & CHDFLAGS_HAS_PARENT) && memcmp(header->parentmd5, nullmd5, sizeof(nullmd5)) == 0 && memcmp(header->parentsha1, nullsha1, sizeof(nullsha1)) == 0)
		return CHDERR_INVALID_PARAMETER;

	/* if we're V3 or later, the obsolete fields must be 0 */
	if (header->version >= 3 &&
		(header->obsolete_cylinders != 0 || header->obsolete_sectors != 0 ||
		 header->obsolete_heads != 0 || header->obsolete_hunksize != 0))
		return CHDERR_INVALID_PARAMETER;

	/* if we're pre-V3, the obsolete fields must NOT be 0 */
	if (header->version < 3 &&
		(header->obsolete_cylinders == 0 || header->obsolete_sectors == 0 ||
		 header->obsolete_heads == 0 || header->obsolete_hunksize == 0))
		return CHDERR_INVALID_PARAMETER;

	return CHDERR_NONE;
}


/*-------------------------------------------------
    header_read - read a CHD header into the
    internal data structure
-------------------------------------------------*/

static chd_error header_read(multi_file *file, chd_header *header)
{
	UINT8 rawheader[CHD_MAX_HEADER_SIZE];
	UINT32 count;

	/* punt if NULL */
	if (!header)
		return CHDERR_INVALID_PARAMETER;

	/* punt if invalid file */
	if (!file)
		return CHDERR_INVALID_FILE;

	/* punt if no interface */
	if (!cur_interface.read)
		return CHDERR_NO_INTERFACE;

	/* seek and read */
	count = multi_read(file, 0, sizeof(rawheader), rawheader);
	if (count != sizeof(rawheader))
		return CHDERR_READ_ERROR;

	/* verify the tag */
	if (strncmp((char *)rawheader, "MComprHD", 8) != 0)
		return CHDERR_INVALID_DATA;

	/* extract the direct data */
	memset(header, 0, sizeof(*header));
	header->length        = get_bigendian_uint32(&rawheader[8]);
	header->version       = get_bigendian_uint32(&rawheader[12]);

	/* make sure it's a version we understand */
	if (header->version == 0 || header->version > CHD_HEADER_VERSION)
		return CHDERR_UNSUPPORTED_VERSION;

	/* make sure the length is expected */
	if ((header->version == 1 && header->length != CHD_V1_HEADER_SIZE) ||
		(header->version == 2 && header->length != CHD_V2_HEADER_SIZE) ||
		(header->version == 3 && header->length != CHD_V3_HEADER_SIZE))
		return CHDERR_INVALID_DATA;

	/* extract the common data */
	header->flags         = get_bigendian_uint32(&rawheader[16]);
	header->compression   = get_bigendian_uint32(&rawheader[20]);
	memcpy(header->md5, &rawheader[44], CHD_MD5_BYTES);
	memcpy(header->parentmd5, &rawheader[60], CHD_MD5_BYTES);

	/* extract the V1/V2-specific data */
	if (header->version < 3)
	{
		int seclen = (header->version == 1) ? CHD_V1_SECTOR_SIZE : get_bigendian_uint32(&rawheader[76]);
		header->obsolete_hunksize  = get_bigendian_uint32(&rawheader[24]);
		header->totalhunks         = get_bigendian_uint32(&rawheader[28]);
		header->obsolete_cylinders = get_bigendian_uint32(&rawheader[32]);
		header->obsolete_heads     = get_bigendian_uint32(&rawheader[36]);
		header->obsolete_sectors   = get_bigendian_uint32(&rawheader[40]);
		header->logicalbytes = (UINT64)header->obsolete_cylinders * (UINT64)header->obsolete_heads * (UINT64)header->obsolete_sectors * (UINT64)seclen;
		header->hunkbytes = seclen * header->obsolete_hunksize;
		header->metaoffset = 0;
	}

	/* extract the V3-specific data */
	else
	{
		header->totalhunks   = get_bigendian_uint32(&rawheader[24]);
		header->logicalbytes = get_bigendian_uint64(&rawheader[28]);
		header->metaoffset   = get_bigendian_uint64(&rawheader[36]);
		header->hunkbytes    = get_bigendian_uint32(&rawheader[76]);
		memcpy(header->sha1, &rawheader[80], CHD_SHA1_BYTES);
		memcpy(header->parentsha1, &rawheader[100], CHD_SHA1_BYTES);
	}

	/* guess it worked */
	return CHDERR_NONE;
}


/*-------------------------------------------------
    header_write - write a CHD header from the
    internal data structure
-------------------------------------------------*/

static chd_error header_write(multi_file *file, const chd_header *header)
{
	UINT8 rawheader[CHD_MAX_HEADER_SIZE];
	UINT32 count;

	/* punt if NULL */
	if (!header)
		return CHDERR_INVALID_PARAMETER;

	/* punt if invalid file */
	if (!file)
		return CHDERR_INVALID_FILE;

	/* punt if no interface */
	if (!cur_interface.write)
		return CHDERR_NO_INTERFACE;

	/* only support writing modern headers */
	if (header->version != 3)
		return CHDERR_INVALID_PARAMETER;

	/* assemble the data */
	memset(rawheader, 0, sizeof(rawheader));
	memcpy(rawheader, "MComprHD", 8);

	put_bigendian_uint32(&rawheader[8],  CHD_V3_HEADER_SIZE);
	put_bigendian_uint32(&rawheader[12], header->version);
	put_bigendian_uint32(&rawheader[16], header->flags);
	put_bigendian_uint32(&rawheader[20], header->compression);
	put_bigendian_uint32(&rawheader[24], header->totalhunks);
	put_bigendian_uint64(&rawheader[28], header->logicalbytes);
	put_bigendian_uint64(&rawheader[36], header->metaoffset);
	memcpy(&rawheader[44], header->md5, CHD_MD5_BYTES);
	memcpy(&rawheader[60], header->parentmd5, CHD_MD5_BYTES);
	put_bigendian_uint32(&rawheader[76], header->hunkbytes);
	memcpy(&rawheader[80], header->sha1, CHD_SHA1_BYTES);
	memcpy(&rawheader[100], header->parentsha1, CHD_SHA1_BYTES);

	/* seek and write */
	count = multi_write(file, 0, CHD_V3_HEADER_SIZE, rawheader);
	if (count != CHD_V3_HEADER_SIZE)
		return CHDERR_WRITE_ERROR;

	return CHDERR_NONE;
}



/***************************************************************************
    INTERNAL HUNK READ/WRITE
***************************************************************************/

/*-------------------------------------------------
    hunk_read_into_cache - read a hunk into
    the CHD's hunk cache
-------------------------------------------------*/

static chd_error hunk_read_into_cache(chd_file *chd, UINT32 hunknum)
{
	chd_error err;

	/* track the max */
	if (hunknum > chd->maxhunk)
		chd->maxhunk = hunknum;

	/* if we're already in the cache, we're done */
	if (chd->cachehunk == hunknum)
		return CHDERR_NONE;
	chd->cachehunk = ~0;

	/* otherwise, read the data */
	err = hunk_read_into_memory(chd, hunknum, chd->cache);
	if (err != CHDERR_NONE)
		return err;

	/* mark the hunk successfully cached in */
	chd->cachehunk = hunknum;
	return CHDERR_NONE;
}


/*-------------------------------------------------
    hunk_read_into_memory - read a hunk into
    memory at the given location
-------------------------------------------------*/

static chd_error hunk_read_into_memory(chd_file *chd, UINT32 hunknum, UINT8 *dest)
{
	map_entry *entry = &chd->map[hunknum];
	chd_error err;
	UINT32 bytes;

	/* switch off the entry type */
	switch (entry->flags & MAP_ENTRY_FLAG_TYPE_MASK)
	{
		/* compressed data */
		case MAP_ENTRY_TYPE_COMPRESSED:

			/* read it into the decompression buffer */
			bytes = multi_read(chd->file, entry->offset, entry->length, chd->compressed);
			if (bytes != entry->length)
				return CHDERR_READ_ERROR;

			/* now decompress using the codec */
			err = CHDERR_NONE;
			if (chd->codecintf->decompress != NULL)
				err = (*chd->codecintf->decompress)(chd, entry->length, dest);
			if (err != CHDERR_NONE)
				return err;
			break;

		/* uncompressed data */
		case MAP_ENTRY_TYPE_UNCOMPRESSED:
			bytes = multi_read(chd->file, entry->offset, chd->header.hunkbytes, dest);
			if (bytes != chd->header.hunkbytes)
				return CHDERR_READ_ERROR;
			break;

		/* mini-compressed data */
		case MAP_ENTRY_TYPE_MINI:
			put_bigendian_uint64(&dest[0], entry->offset);
			for (bytes = 8; bytes < chd->header.hunkbytes; bytes++)
				dest[bytes] = dest[bytes - 8];
			break;

		/* self-referenced data */
		case MAP_ENTRY_TYPE_SELF_HUNK:
			if (chd->cachehunk == entry->offset && dest == chd->cache)
				break;
			return hunk_read_into_memory(chd, entry->offset, dest);

		/* parent-referenced data */
		case MAP_ENTRY_TYPE_PARENT_HUNK:
			err = hunk_read_into_memory(chd->parent, entry->offset, dest);
			if (err != CHDERR_NONE)
				return err;
			break;
	}
	return CHDERR_NONE;
}


/*-------------------------------------------------
    hunk_write_from_memory - write a hunk from
    memory into a CHD
-------------------------------------------------*/

static chd_error hunk_write_from_memory(chd_file *chd, UINT32 hunknum, const UINT8 *src)
{
	map_entry *entry = &chd->map[hunknum];
	map_entry newentry;
	UINT8 fileentry[MAP_ENTRY_SIZE];
	const void *data = src;
	UINT32 bytes = 0, match;
	chd_error err;

	/* track the max */
	if (hunknum > chd->maxhunk)
		chd->maxhunk = hunknum;

	/* first compute the CRC of the original data */
	newentry.crc = crc32(0, &src[0], chd->header.hunkbytes);

	/* if we're not a lossy codec, compute the CRC and look for matches */
	if (!chd->codecintf->lossy)
	{
		/* some extra stuff for zlib+ compression */
		if (chd->header.compression >= CHDCOMPRESSION_ZLIB_PLUS)
		{
			/* see if we can mini-compress first */
			for (bytes = 8; bytes < chd->header.hunkbytes; bytes++)
				if (src[bytes] != src[bytes - 8])
					break;

			/* if so, we don't need to write any data */
			if (bytes == chd->header.hunkbytes)
			{
				newentry.offset = get_bigendian_uint64(&src[0]);
				newentry.length = 0;
				newentry.flags = MAP_ENTRY_TYPE_MINI;
				goto write_entry;
			}

			/* otherwise, see if we can find a match in the current file */
			match = crcmap_find_hunk(chd, hunknum, newentry.crc, &src[0]);
			if (match != NO_MATCH)
			{
				newentry.offset = match;
				newentry.length = 0;
				newentry.flags = MAP_ENTRY_TYPE_SELF_HUNK;
				goto write_entry;
			}

			/* if we have a parent, see if we can find a match in there */
			if (chd->header.flags & CHDFLAGS_HAS_PARENT)
			{
				match = crcmap_find_hunk(chd->parent, ~0, newentry.crc, &src[0]);
				if (match != NO_MATCH)
				{
					newentry.offset = match;
					newentry.length = 0;
					newentry.flags = MAP_ENTRY_TYPE_PARENT_HUNK;
					goto write_entry;
				}
			}
		}
	}

	/* now try compressing the data */
	err = CHDERR_COMPRESSION_ERROR;
	if (chd->codecintf->compress != NULL)
		err = (*chd->codecintf->compress)(chd, src, &bytes);

	/* if that worked, and we're lossy, decompress and CRC the result */
	if (err == CHDERR_NONE && chd->codecintf->lossy)
	{
		err = (*chd->codecintf->decompress)(chd, bytes, chd->cache);
		if (err == CHDERR_NONE)
			newentry.crc = crc32(0, chd->cache, chd->header.hunkbytes);
	}

	/* if we succeeded in compressing the data, replace our data pointer and mark it so */
	if (err == CHDERR_NONE)
	{
		data = chd->compressed;
		newentry.length = bytes;
		newentry.flags = MAP_ENTRY_TYPE_COMPRESSED;
	}

	/* otherwise, mark it uncompressed and use the original data */
	else
	{
		newentry.length = chd->header.hunkbytes;
		newentry.flags = MAP_ENTRY_TYPE_UNCOMPRESSED;
	}

	/* if the data doesn't fit into the previous entry, make a new one at the eof */
	newentry.offset = entry->offset;
	if (newentry.offset == 0 || newentry.length > entry->length)
		newentry.offset = multi_length(chd->file);

	/* write the data */
	bytes = multi_write(chd->file, newentry.offset, newentry.length, data);
	if (bytes != newentry.length)
		return CHDERR_WRITE_ERROR;

	/* update the entry in memory */
write_entry:
	*entry = newentry;

	/* update the map on file */
	map_assemble(&fileentry[0], &chd->map[hunknum]);
	bytes = multi_write(chd->file, chd->header.length + hunknum * sizeof(fileentry), sizeof(fileentry), &fileentry[0]);
	if (bytes != sizeof(fileentry))
		return CHDERR_WRITE_ERROR;

	return CHDERR_NONE;
}



/***************************************************************************
    INTERNAL MAP ACCESS
***************************************************************************/

/*-------------------------------------------------
    map_write_initial - write an initial map to
    a new CHD file
-------------------------------------------------*/

static chd_error map_write_initial(multi_file *file, chd_file *parent, const chd_header *header)
{
	UINT8 blank_map_entries[MAP_STACK_ENTRIES * MAP_ENTRY_SIZE];
	int fullchunks, remainder, count, i, j;
	map_entry mapentry;
	UINT64 fileoffset;

	/* create a mini hunk of 0's */
	mapentry.offset = 0;
	mapentry.crc = 0;
	mapentry.length = 0;
	mapentry.flags = MAP_ENTRY_TYPE_MINI | MAP_ENTRY_FLAG_NO_CRC;
	for (i = 0; i < MAP_STACK_ENTRIES; i++)
		map_assemble(&blank_map_entries[i * MAP_ENTRY_SIZE], &mapentry);

	/* prepare to write a blank hunk map immediately following */
	fileoffset = header->length;
	fullchunks = header->totalhunks / MAP_STACK_ENTRIES;
	remainder = header->totalhunks % MAP_STACK_ENTRIES;

	/* first write full chunks of blank entries */
	for (i = 0; i < fullchunks; i++)
	{
		/* parent drives need to be mapped through */
		if (parent != NULL)
			for (j = 0; j < MAP_STACK_ENTRIES; j++)
			{
				mapentry.offset = i * MAP_STACK_ENTRIES + j;
				mapentry.crc = parent->map[i * MAP_STACK_ENTRIES + j].crc;
				mapentry.flags = MAP_ENTRY_TYPE_PARENT_HUNK;
				map_assemble(&blank_map_entries[j * MAP_ENTRY_SIZE], &mapentry);
			}

		/* write the chunks */
		count = multi_write(file, fileoffset, sizeof(blank_map_entries), blank_map_entries);
		if (count != sizeof(blank_map_entries))
			return CHDERR_WRITE_ERROR;
		fileoffset += sizeof(blank_map_entries);
	}

	/* then write the remainder */
	if (remainder > 0)
	{
		/* parent drives need to be mapped through */
		if (parent != NULL)
			for (j = 0; j < remainder; j++)
			{
				mapentry.offset = i * MAP_STACK_ENTRIES + j;
				mapentry.crc = parent->map[i * MAP_STACK_ENTRIES + j].crc;
				mapentry.flags = MAP_ENTRY_TYPE_PARENT_HUNK;
				map_assemble(&blank_map_entries[j * MAP_ENTRY_SIZE], &mapentry);
			}

		/* write the chunks */
		count = multi_write(file, fileoffset, remainder * MAP_ENTRY_SIZE, blank_map_entries);
		if (count != remainder * MAP_ENTRY_SIZE)
			return CHDERR_WRITE_ERROR;
		fileoffset += remainder * MAP_ENTRY_SIZE;
	}

	/* then write a special end-of-list cookie */
	memcpy(&blank_map_entries[0], END_OF_LIST_COOKIE, MAP_ENTRY_SIZE);
	count = multi_write(file, fileoffset, MAP_ENTRY_SIZE, blank_map_entries);
	if (count != MAP_ENTRY_SIZE)
		return CHDERR_WRITE_ERROR;

	return CHDERR_NONE;
}


/*-------------------------------------------------
    map_read - read the initial sector map
-------------------------------------------------*/

static chd_error map_read(chd_file *chd)
{
	UINT32 entrysize = (chd->header.version < 3) ? OLD_MAP_ENTRY_SIZE : MAP_ENTRY_SIZE;
	UINT8 raw_map_entries[MAP_STACK_ENTRIES * MAP_ENTRY_SIZE];
	UINT64 fileoffset, maxoffset = 0;
	UINT8 cookie[MAP_ENTRY_SIZE];
	UINT32 count;
	int i, err;

	/* first allocate memory */
	chd->map = malloc(sizeof(chd->map[0]) * chd->header.totalhunks);
	if (!chd->map)
		return CHDERR_OUT_OF_MEMORY;

	/* read the map entries in in chunks and extract to the map list */
	fileoffset = chd->header.length;
	for (i = 0; i < chd->header.totalhunks; i += MAP_STACK_ENTRIES)
	{
		/* compute how many entries this time */
		int entries = chd->header.totalhunks - i, j;
		if (entries > MAP_STACK_ENTRIES)
			entries = MAP_STACK_ENTRIES;

		/* read that many */
		count = multi_read(chd->file, fileoffset, entries * entrysize, raw_map_entries);
		if (count != entries * entrysize)
		{
			err = CHDERR_READ_ERROR;
			goto cleanup;
		}
		fileoffset += entries * entrysize;

		/* process that many */
		if (entrysize == MAP_ENTRY_SIZE)
		{
			for (j = 0; j < entries; j++)
				map_extract(&raw_map_entries[j * MAP_ENTRY_SIZE], &chd->map[i + j]);
		}
		else
		{
			for (j = 0; j < entries; j++)
				map_extract_old(&raw_map_entries[j * OLD_MAP_ENTRY_SIZE], &chd->map[i + j], chd->header.hunkbytes);
		}

		/* track the maximum offset */
		for (j = 0; j < entries; j++)
			if ((chd->map[i + j].flags & MAP_ENTRY_FLAG_TYPE_MASK) == MAP_ENTRY_TYPE_COMPRESSED ||
				(chd->map[i + j].flags & MAP_ENTRY_FLAG_TYPE_MASK) == MAP_ENTRY_TYPE_UNCOMPRESSED)
				maxoffset = MAX(maxoffset, chd->map[i + j].offset + chd->map[i + j].length);
	}

	/* verify the cookie */
	count = multi_read(chd->file, fileoffset, entrysize, &cookie);
	if (count != entrysize || memcmp(&cookie, END_OF_LIST_COOKIE, entrysize))
	{
		err = CHDERR_INVALID_FILE;
		goto cleanup;
	}

	/* verify the length */
	if (maxoffset > multi_length(chd->file))
	{
		err = CHDERR_INVALID_FILE;
		goto cleanup;
	}
	return CHDERR_NONE;

cleanup:
	if (chd->map)
		free(chd->map);
	chd->map = NULL;
	return err;
}



/***************************************************************************
    INTERNAL CRC MAP ACCESS
***************************************************************************/

/*-------------------------------------------------
    crcmap_init - initialize the CRC map
-------------------------------------------------*/

static void crcmap_init(chd_file *chd, int prepopulate)
{
	int i;

	/* if we already have one, bail */
	if (chd->crcmap != NULL)
		return;

	/* reset all pointers */
	chd->crcmap = NULL;
	chd->crcfree = NULL;
	chd->crctable = NULL;

	/* allocate a list; one for each hunk */
	chd->crcmap = malloc(chd->header.totalhunks * sizeof(chd->crcmap[0]));
	if (chd->crcmap == NULL)
		return;

	/* allocate a CRC map table */
	chd->crctable = malloc(CRCMAP_HASH_SIZE * sizeof(chd->crctable[0]));
	if (chd->crctable == NULL)
	{
		free(chd->crcmap);
		chd->crcmap = NULL;
		return;
	}

	/* initialize the free list */
	for (i = 0; i < chd->header.totalhunks; i++)
	{
		chd->crcmap[i].next = chd->crcfree;
		chd->crcfree = &chd->crcmap[i];
	}

	/* initialize the table */
	memset(chd->crctable, 0, CRCMAP_HASH_SIZE * sizeof(chd->crctable[0]));

	/* if we're to prepopulate, go for it */
	if (prepopulate)
		for (i = 0; i < chd->header.totalhunks; i++)
			crcmap_add_entry(chd, i);
}


/*-------------------------------------------------
    crcmap_add_entry - log a CRC entry
-------------------------------------------------*/

static void crcmap_add_entry(chd_file *chd, UINT32 hunknum)
{
	UINT32 hash = chd->map[hunknum].crc % CRCMAP_HASH_SIZE;
	crcmap_entry *crcmap;

	/* pull a free entry off the list */
	crcmap = chd->crcfree;
	chd->crcfree = crcmap->next;

	/* set up the entry and link it into the hash table */
	crcmap->hunknum = hunknum;
	crcmap->next = chd->crctable[hash];
	chd->crctable[hash] = crcmap;
}


/*-------------------------------------------------
    crcmap_verify_hunk_match - verify that a
    hunk really matches by doing a byte-for-byte
    compare
-------------------------------------------------*/

static int crcmap_verify_hunk_match(chd_file *chd, UINT32 hunknum, const UINT8 *rawdata)
{
	/* we have a potential match -- better be sure */
	/* read the hunk from disk and compare byte-for-byte */
	if (hunknum != chd->comparehunk)
	{
		chd->comparehunk = ~0;
		if (hunk_read_into_memory(chd, hunknum, chd->compare) == CHDERR_NONE)
			chd->comparehunk = hunknum;
	}
	return (hunknum == chd->comparehunk && memcmp(rawdata, chd->compare, chd->header.hunkbytes) == 0);
}


/*-------------------------------------------------
    crcmap_find_hunk - find a hunk with a matching
    CRC in the map
-------------------------------------------------*/

static UINT32 crcmap_find_hunk(chd_file *chd, UINT32 hunknum, UINT32 crc, const UINT8 *rawdata)
{
	UINT32 lasthunk = (hunknum < chd->header.totalhunks) ? hunknum : chd->header.totalhunks;
	int curhunk;

	/* if we have a CRC map, use that */
	if (chd->crctable)
	{
		crcmap_entry *curentry;
		for (curentry = chd->crctable[crc % CRCMAP_HASH_SIZE]; curentry; curentry = curentry->next)
		{
			curhunk = curentry->hunknum;
			if (chd->map[curhunk].crc == crc && !(chd->map[curhunk].flags & MAP_ENTRY_FLAG_NO_CRC) && crcmap_verify_hunk_match(chd, curhunk, rawdata))
				return curhunk;
		}
		return NO_MATCH;
	}

	/* first see if the last match is a valid one */
	if (chd->comparehunk < chd->header.totalhunks && chd->map[chd->comparehunk].crc == crc && !(chd->map[chd->comparehunk].flags & MAP_ENTRY_FLAG_NO_CRC) &&
		memcmp(rawdata, chd->compare, chd->header.hunkbytes) == 0)
		return chd->comparehunk;

	/* scan through the CHD's hunk map looking for a match */
	for (curhunk = 0; curhunk < lasthunk; curhunk++)
		if (chd->map[curhunk].crc == crc && !(chd->map[curhunk].flags & MAP_ENTRY_FLAG_NO_CRC) && crcmap_verify_hunk_match(chd, curhunk, rawdata))
			return curhunk;

	return NO_MATCH;
}



/***************************************************************************
    INTERNAL METADATA ACCESS
***************************************************************************/

/*-------------------------------------------------
    metadata_find_entry - find a metadata entry
-------------------------------------------------*/

static chd_error metadata_find_entry(chd_file *chd, UINT32 metatag, UINT32 metaindex, metadata_entry *metaentry)
{
	/* start at the beginning */
	metaentry->offset = chd->header.metaoffset;
	metaentry->prev = 0;

	/* loop until we run out of options */
	while (metaentry->offset != 0)
	{
		UINT8	raw_meta_header[METADATA_HEADER_SIZE];
		UINT32	count;

		/* read the raw header */
		count = multi_read(chd->file, metaentry->offset, sizeof(raw_meta_header), raw_meta_header);
		if (count != sizeof(raw_meta_header))
			break;

		/* extract the data */
		metaentry->metatag = get_bigendian_uint32(&raw_meta_header[0]);
		metaentry->length = get_bigendian_uint32(&raw_meta_header[4]);
		metaentry->next = get_bigendian_uint64(&raw_meta_header[8]);

		/* if we got a match, proceed */
		if (metatag == CHDMETATAG_WILDCARD || metaentry->metatag == metatag)
			if (metaindex-- == 0)
				return CHDERR_NONE;

		/* no match, fetch the next link */
		metaentry->prev = metaentry->offset;
		metaentry->offset = metaentry->next;
	}

	/* if we get here, we didn't find it */
	return CHDERR_METADATA_NOT_FOUND;
}


/*-------------------------------------------------
    metadata_set_previous_next - set the 'next'
    offset of a piece of metadata
-------------------------------------------------*/

static chd_error metadata_set_previous_next(chd_file *chd, UINT64 prevoffset, UINT64 nextoffset)
{
	UINT8 raw_meta_header[METADATA_HEADER_SIZE];
	chd_error err;
	UINT32 count;

	/* if we were the first entry, make the next entry the first */
	if (prevoffset == 0)
	{
		chd->header.metaoffset = nextoffset;
		err = header_write(chd->file, &chd->header);
		if (err != CHDERR_NONE)
			return err;
	}

	/* otherwise, update the link in the previous pointer */
	else
	{
		/* read the previous raw header */
		count = multi_read(chd->file, prevoffset, sizeof(raw_meta_header), raw_meta_header);
		if (count != sizeof(raw_meta_header))
			return CHDERR_READ_ERROR;

		/* copy our next pointer into the previous->next offset */
		put_bigendian_uint64(&raw_meta_header[8], nextoffset);

		/* write the previous raw header */
		count = multi_write(chd->file, prevoffset, sizeof(raw_meta_header), raw_meta_header);
		if (count != sizeof(raw_meta_header))
			return CHDERR_WRITE_ERROR;
	}

	return CHDERR_NONE;
}


/*-------------------------------------------------
    metadata_set_length - set the length field of
    a piece of metadata
-------------------------------------------------*/

static chd_error metadata_set_length(chd_file *chd, UINT64 offset, UINT32 length)
{
	UINT8 raw_meta_header[METADATA_HEADER_SIZE];
	UINT32 count;

	/* read the raw header */
	count = multi_read(chd->file, offset, sizeof(raw_meta_header), raw_meta_header);
	if (count != sizeof(raw_meta_header))
		return CHDERR_READ_ERROR;

	/* update the length at offset 4 */
	put_bigendian_uint32(&raw_meta_header[4], length);

	/* write the raw header */
	count = multi_write(chd->file, offset, sizeof(raw_meta_header), raw_meta_header);
	if (count != sizeof(raw_meta_header))
		return CHDERR_WRITE_ERROR;

	return CHDERR_NONE;
}



/***************************************************************************
    ZLIB COMPRESSION CODEC
***************************************************************************/

/*-------------------------------------------------
    zlib_codec_init - initialize the ZLIB codec
-------------------------------------------------*/

static chd_error zlib_codec_init(chd_file *chd)
{
	zlib_codec_data *data;
	chd_error err;
	int zerr;

	/* allocate memory for the 2 stream buffers */
	data = malloc(sizeof(*data));
	if (data == NULL)
		return CHDERR_OUT_OF_MEMORY;

	/* clear the buffers */
	memset(data, 0, sizeof(*data));

	/* init the inflater first */
	data->inflater.next_in = (Bytef *)data;	/* bogus, but that's ok */
	data->inflater.avail_in = 0;
	data->inflater.zalloc = zlib_fast_alloc;
	data->inflater.zfree = zlib_fast_free;
	data->inflater.opaque = data;
	zerr = inflateInit2(&data->inflater, -MAX_WBITS);

	/* if that worked, initialize the deflater */
	if (zerr == Z_OK)
	{
		data->deflater.next_in = (Bytef *)data;	/* bogus, but that's ok */
		data->deflater.avail_in = 0;
		data->deflater.zalloc = zlib_fast_alloc;
		data->deflater.zfree = zlib_fast_free;
		data->deflater.opaque = data;
		zerr = deflateInit2(&data->deflater, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
	}

	/* convert errors */
	if (zerr == Z_MEM_ERROR)
		err = CHDERR_OUT_OF_MEMORY;
	else if (zerr != Z_OK)
		err = CHDERR_CODEC_ERROR;
	else
		err = CHDERR_NONE;

	/* handle an error */
	if (err == CHDERR_NONE)
		chd->codecdata = data;
	else
		free(data);

	return err;
}


/*-------------------------------------------------
    zlib_codec_free - free data for the ZLIB
    codec
-------------------------------------------------*/

static void zlib_codec_free(chd_file *chd)
{
	zlib_codec_data *data = chd->codecdata;

	/* deinit the streams */
	if (data != NULL)
	{
		int i;

		inflateEnd(&data->inflater);
		deflateEnd(&data->deflater);

		/* free our fast memory */
		for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
			if (data->allocptr[i])
				free(data->allocptr[i]);
		free(data);
	}
}


/*-------------------------------------------------
    zlib_codec_compress - compress data using the
    ZLIB codec
-------------------------------------------------*/

static chd_error zlib_codec_compress(chd_file *chd, const void *src, UINT32 *length)
{
	zlib_codec_data *data = chd->codecdata;
	int zerr;

	/* reset the decompressor */
	data->deflater.next_in = (void *)src;
	data->deflater.avail_in = chd->header.hunkbytes;
	data->deflater.total_in = 0;
	data->deflater.next_out = chd->compressed;
	data->deflater.avail_out = chd->header.hunkbytes;
	data->deflater.total_out = 0;
	zerr = deflateReset(&data->deflater);
	if (zerr != Z_OK)
		return CHDERR_COMPRESSION_ERROR;

	/* do it */
	zerr = deflate(&data->deflater, Z_FINISH);

	/* if we ended up with more data than we started with, return an error */
	if (zerr != Z_STREAM_END || data->deflater.total_out >= chd->header.hunkbytes)
		return CHDERR_COMPRESSION_ERROR;

	/* otherwise, fill in the length and return success */
	*length = data->deflater.total_out;
	return CHDERR_NONE;
}


/*-------------------------------------------------
    zlib_codec_decompress - decomrpess data using
    the ZLIB codec
-------------------------------------------------*/

static chd_error zlib_codec_decompress(chd_file *chd, UINT32 srclength, void *dest)
{
	zlib_codec_data *data = chd->codecdata;
	int zerr;

	/* reset the decompressor */
	data->inflater.next_in = chd->compressed;
	data->inflater.avail_in = srclength;
	data->inflater.total_in = 0;
	data->inflater.next_out = dest;
	data->inflater.avail_out = chd->header.hunkbytes;
	data->inflater.total_out = 0;
	zerr = inflateReset(&data->inflater);
	if (zerr != Z_OK)
		return CHDERR_DECOMPRESSION_ERROR;

	/* do it */
	zerr = inflate(&data->inflater, Z_FINISH);
	if (data->inflater.total_out != chd->header.hunkbytes)
		return CHDERR_DECOMPRESSION_ERROR;

	return CHDERR_NONE;
}


/*-------------------------------------------------
    zlib_fast_alloc - fast malloc for ZLIB, which
    allocates and frees memory frequently
-------------------------------------------------*/

static voidpf zlib_fast_alloc(voidpf opaque, uInt items, uInt size)
{
	zlib_codec_data *data = opaque;
	UINT32 *ptr;
	int i;

	/* compute the size, rounding to the nearest 1k */
	size = (size * items + 0x3ff) & ~0x3ff;

	/* reuse a hunk if we can */
	for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
	{
		ptr = data->allocptr[i];
		if (ptr && size == *ptr)
		{
			/* set the low bit of the size so we don't match next time */
			*ptr |= 1;
			return ptr + 1;
		}
	}

	/* alloc a new one */
	ptr = malloc(size + sizeof(UINT32));
	if (!ptr)
		return NULL;

	/* put it into the list */
	for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
		if (!data->allocptr[i])
		{
			data->allocptr[i] = ptr;
			break;
		}

	/* set the low bit of the size so we don't match next time */
	*ptr = size | 1;
	return ptr + 1;
}


/*-------------------------------------------------
    zlib_fast_free - fast free for ZLIB, which
    allocates and frees memory frequently
-------------------------------------------------*/

static void zlib_fast_free(voidpf opaque, voidpf address)
{
	zlib_codec_data *data = opaque;
	UINT32 *ptr = (UINT32 *)address - 1;
	int i;

	/* find the hunk */
	for (i = 0; i < MAX_ZLIB_ALLOCS; i++)
		if (ptr == data->allocptr[i])
		{
			/* clear the low bit of the size to allow matches */
			*ptr &= ~1;
			return;
		}
}



/***************************************************************************
    MULTIFILE ROUTINES
***************************************************************************/

/*-------------------------------------------------
    multi_open - open one or more files that
    make up a CHD
-------------------------------------------------*/

static multi_file *multi_open(const char *filename, const char *mode)
{
	chd_interface_file *chdfile = NULL;
	multi_file *file = NULL;
	char *curname = NULL;
	int numfiles = 1;
	int i;

	/* open the first file */
	chdfile = (*cur_interface.open)(filename, mode);
	if (chdfile == NULL)
		return NULL;

	/* allocate a new multi_file object */
	file = malloc(sizeof(*file));
	if (file == NULL)
		goto error;
	memset(file, 0, sizeof(*file));

	/* if we're not writing, look for additional files */
	if (strchr(mode, 'w') == NULL && strchr(mode, '+') == NULL)
	{
		/* allocate a copy of the filename */
		curname = malloc(strlen(filename) + 1 + 4);
		if (curname == NULL)
			goto error;

		/* count how many files are available */
		for ( ; numfiles <= 1000; numfiles++)
		{
			chd_interface_file *tempfile;

			chd_multi_filename(filename, curname, numfiles - 1);
			tempfile = (*cur_interface.open)(curname, mode);
			if (tempfile == NULL)
				break;
			(*cur_interface.close)(tempfile);
		}
	}

	/* allocate memory for the file and length arrays */
	file->files = malloc(numfiles * sizeof(file->files[0]));
	file->length = malloc(numfiles * sizeof(file->length[0]));
	if (file->files == NULL || file->length == NULL)
		goto error;

	/* fill in the first entry */
	file->files[0] = chdfile;
	file->length[0] = (*cur_interface.length)(chdfile);
	file->numfiles = numfiles;

	/* now open the remaining files for real */
	for (i = 1; i < file->numfiles; i++)
	{
		chd_multi_filename(filename, curname, i - 1);
		file->files[i] = (*cur_interface.open)(curname, mode);
		if (file->files[i] == NULL)
			goto error;
		file->length[i] = (*cur_interface.length)(file->files[i]);
	}

	/* free memory */
	if (curname != NULL)
		free(curname);

	return file;

error:
	if (file != NULL)
	{
		if (file->files != NULL)
			free(file->files);
		if (file->length != NULL)
			free(file->length);
		free(file);
	}
	if (curname != NULL)
		free(curname);
	if (chdfile != NULL)
		(*cur_interface.close)(chdfile);
	return NULL;
}


/*-------------------------------------------------
    multi_close - close all open files associated
    with a CHD
-------------------------------------------------*/

static void multi_close(multi_file *file)
{
	int i;

	/* close all files */
	for (i = 0; i < file->numfiles; i++)
		(*cur_interface.close)(file->files[i]);

	/* free memory */
	free(file->files);
	free(file->length);
	free(file);
}


/*-------------------------------------------------
    multi_read - read from potentially multiple
    files that make up a CHD
-------------------------------------------------*/

static UINT32 multi_read(multi_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	UINT64 baseoffset = 0;
	UINT32 firstcount;
	int i;

	/* find the file where the starting offset lives */
	for (i = 0; i < file->numfiles; i++)
	{
		if (offset >= baseoffset && offset < baseoffset + file->length[i])
			break;
		baseoffset += file->length[i];
	}

	/* if past the total end, return nothing */
	if (i == file->numfiles)
		return 0;

	/* if we don't cross the end of the file, just do the read */
	if (offset + count <= baseoffset + file->length[i])
		return (*cur_interface.read)(file->files[i], offset - baseoffset, count, buffer);

	/* otherwise, break it into two reads */
	firstcount = baseoffset + file->length[i] - offset;
	return multi_read(file, offset, firstcount, buffer) +
		   multi_read(file, offset + firstcount, count - firstcount, (UINT8 *)buffer + firstcount);
}


/*-------------------------------------------------
    multi_write - write to a CHD file; writes
    past the end always go to the last file, no
    matter how large it grows
-------------------------------------------------*/

static UINT32 multi_write(multi_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	UINT64 baseoffset = 0;
	UINT32 firstcount;
	int i;

	/* find the file where the starting offset lives */
	for (i = 0; i < file->numfiles; i++)
	{
		if (offset >= baseoffset && offset < baseoffset + file->length[i])
			break;
		baseoffset += file->length[i];
	}

	/* if past the total end, append to the last file */
	if (i == file->numfiles)
	{
		baseoffset -= file->length[--i];
		file->length[i] = (offset + count) - baseoffset;
	}

	/* if we don't cross the end of the file, just do the write */
	if (offset + count <= baseoffset + file->length[i])
		return (*cur_interface.write)(file->files[i], offset - baseoffset, count, buffer);

	/* otherwise, break it into two writes */
	firstcount = baseoffset + file->length[i] - offset;
	return multi_write(file, offset, firstcount, buffer) +
		   multi_write(file, offset + firstcount, count - firstcount, (UINT8 *)buffer + firstcount);
}


/*-------------------------------------------------
    multi_length - return the total length of a
    CHD file
-------------------------------------------------*/

static UINT64 multi_length(multi_file *file)
{
	UINT64 totalsize = 0;
	int i;

	/* sum the sizes of all the files */
	for (i = 0; i < file->numfiles; i++)
		totalsize += file->length[i];
	return totalsize;
}
