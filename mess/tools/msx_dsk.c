/*
 * msx_dsk.c : converts .ddi/.img/.msx disk images to .dsk image
 *
 * Sean Young
 */

#include "osdepend.h"
#include "imgtool.h"
#include "utils.h"


static int xsa_extract (STREAM *in, STREAM *out);

#ifdef LSB_FIRST
#define intelLong(x) (x)
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | \
                       (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif

#define 	IMG_1DD		(1)
#define 	IMG_2DD		(2)
#define 	DDI_2DD		(3)
#define 	MSX_2DD		(4)
#define 	XSA_2DD		(5)

typedef struct {
	IMAGE			base;
	char			*file_name;
	STREAM 			*file_handle;
	int 			size, format;
	} DSK_IMAGE;

typedef struct
	{
	IMAGEENUM 	base;
	DSK_IMAGE	*image;
	int			index;
	} DSK_ITERATOR;

static int msx_dsk_image_init(STREAM *f, IMAGE **outimg);
static void msx_dsk_image_exit(IMAGE *img);
static int msx_dsk_image_beginenum(IMAGE *img, IMAGEENUM **outenum);
static int msx_dsk_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
static void msx_dsk_image_closeenum(IMAGEENUM *enumeration);
static int msx_dsk_image_readfile(IMAGE *img, const char *fname, STREAM *destf);

IMAGEMODULE(
	msx_dsk,
	"Various bogus MSX disk images (xsa/img/ddi/msx)",		/* human readable name */
	"xsa\0img\0ddi\0msx\0",								/* file extension */
	NULL,								/* crcfile */
	NULL,								/* crc system name */
	NULL,								/* eoln */
	0,									/* flags */
	msx_dsk_image_init,					/* init function */
	msx_dsk_image_exit,					/* exit function */
	NULL,								/* info function */
	msx_dsk_image_beginenum,			/* begin enumeration */
	msx_dsk_image_nextenum,				/* enumerate next */
	msx_dsk_image_closeenum,			/* close enumeration */
	NULL,								/* free space on image */
	msx_dsk_image_readfile,				/* read file */
	NULL,/* write file */
	NULL,/* delete file */
	NULL,/* create image */
	NULL,								/* read sector */
	NULL,								/* write sector */
	NULL,					/* file options */
	NULL					/* create options */
)

static int msx_dsk_image_init(STREAM *f, IMAGE **outimg)
	{
	DSK_IMAGE *image;
	int pos, len, format /*,name_len */;
    UINT8 header[4], byt;
	char *pbase, default_name[] = "msxdisk";

	format = 0;
	len=stream_size(f);
	if (len < 5) return IMGTOOLERR_MODULENOTFOUND;
	if (4 != stream_read (f, header, 4) ) return IMGTOOLERR_READERROR;

    if (!memcmp (header, "PCK\010", 4) )
		{
		format = XSA_2DD;
		if (4 != stream_read (f, &len, 4) ) return IMGTOOLERR_READERROR;
		len = intelLong (len);
		if (4 != stream_read (f, header, 4) ) return IMGTOOLERR_READERROR;
		}
	else switch (len)
		{
		case 720*1024+1:
		case 360*1024+1:
			if ( (header[0] * 360 * 1024 + 1) == len)
				format = header[0]; /* bit evil, but works */

			len--;
			break;
		case 720*1024:
/*			if (f->name)
				{
				name_len = strlen (f->name);
				if (name_len > 4 || !strcmpi (f->name + name_len - 4, ".msx") )
					format = MSX_2DD;
				}
*/			break;
		case 720*1024+0x1800:
			len -= 0x1800;
			format = DDI_2DD;
		}
	
	if (!format) return IMGTOOLERR_MODULENOTFOUND;

	image = (DSK_IMAGE*)malloc (sizeof (DSK_IMAGE) );
	if (!image) return IMGTOOLERR_OUTOFMEMORY;

	*outimg = (IMAGE*)image;

	memset(image, 0, sizeof(DSK_IMAGE));
	image->base.module = &imgmod_msx_dsk;
	image->file_handle = f;
	image->size = len;
	image->format = format;

	if (format != XSA_2DD)
		{
	    /*if (f->name) pbase = (char*)osd_basename (f->name);
		else */pbase = default_name;

   		len = strlen (pbase);

    	image->file_name = malloc (len + 5);
		if (!image->file_name)
			{
			free(image);
			*outimg=NULL;
			return IMGTOOLERR_CORRUPTIMAGE;
			}

		strcpy (image->file_name, pbase);
        if (len > 4 || image->file_name[len -4] == '.') len -= 4;

		strcpy (image->file_name + len, ".dsk");
		}
	else
		{
		/* get name from XSA header, can't be longer than 8.3 really */
		/* but could be, it's zero-terminated */
		pos = len = 0;
		image->file_name = NULL;
		while (1)
			{
			if (1 != stream_read (f, &byt, 1) )
				{
				if (image->file_name) free (image->file_name);
				free(image);
				*outimg=NULL;
				return IMGTOOLERR_READERROR;
				}
			if (len <= pos)
				{
				len += 8; /* why 8? */
				pbase = realloc (image->file_name, len);
				if (!pbase)
					{
					if (image->file_name) free (image->file_name);
					free(image);
					*outimg=NULL;
					return IMGTOOLERR_OUTOFMEMORY;
					}
				image->file_name = pbase;
				}
			image->file_name[pos++] = byt;
			if (!byt) break;
			}
		}

	return 0;
	}

static void msx_dsk_image_exit(IMAGE *img)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	stream_close(image->file_handle);
	free(image->file_name);
	free(image);
	}

static int msx_dsk_image_beginenum(IMAGE *img, IMAGEENUM **outenum)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	DSK_ITERATOR *iter;

	iter=*(DSK_ITERATOR**)outenum = (DSK_ITERATOR*) malloc(sizeof(DSK_ITERATOR));
	if (!iter) return IMGTOOLERR_OUTOFMEMORY;

	iter->base.module = &imgmod_msx_dsk;

	iter->image=image;
	iter->index = 0;
	return 0;
	}

static int msx_dsk_image_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent)
	{
	DSK_ITERATOR *iter=(DSK_ITERATOR*)enumeration;

	ent->eof=iter->index;
	if (!ent->eof)
		{
		strcpy (ent->fname, iter->image->file_name);
		ent->corrupt=0;
		ent->filesize = iter->image->size;
		iter->index++;
		}

	return 0;
	}

static void msx_dsk_image_closeenum(IMAGEENUM *enumeration)
	{
	free(enumeration);
	}

static int msx_dsk_image_readfile(IMAGE *img, const char *fname, STREAM *destf)
	{
	DSK_IMAGE *image=(DSK_IMAGE*)img;
	UINT8	buf[0x1200];
	int 	i, offset, n1, n2;

	if (stricmp (fname, image->file_name) )
		return IMGTOOLERR_MODULENOTFOUND;

	switch (image->format)
		{
		case IMG_2DD: offset = 1; i = 160; break;
		case IMG_1DD: offset = 1; i = 80; break;
		case DDI_2DD: offset = 0x1800; i = 160; break;
		case MSX_2DD:
			{
			i = 80; n1 = 0; n2 = 80;
			while (i--)
				{
				stream_seek (image->file_handle, n1++ * 0x1200, SEEK_SET);
				if (0x1200 != stream_read (image->file_handle, buf, 0x1200) )
					return IMGTOOLERR_READERROR;

				if (0x1200 != stream_write (destf, buf, 0x1200) )
					return IMGTOOLERR_WRITEERROR;

				stream_seek (image->file_handle, n2++ * 0x1200, SEEK_SET);
				if (0x1200 != stream_read (image->file_handle, buf, 0x1200) )
					return IMGTOOLERR_READERROR;

				if (0x1200 != stream_write (destf, buf, 0x1200) )
					return IMGTOOLERR_WRITEERROR;
				}

			return 0;
			}
		case XSA_2DD:	/* XSA .. special case */
			return xsa_extract (image->file_handle, destf);
		default:
			return IMGTOOLERR_UNEXPECTED;
		}

	stream_seek (image->file_handle, offset, SEEK_SET);

	while (i--)
		{
		if (0x1200 != stream_read (image->file_handle, buf, 0x1200) )
			return IMGTOOLERR_READERROR;

		if (0x1200 != stream_write (destf, buf, 0x1200) )
			return IMGTOOLERR_WRITEERROR;
		}

	return 0;
	}


/*
 * .xsa decompression. Code stolen from :
 *
 * http://web.inter.nl.net/users/A.P.Wulms/
 */


/****************************************************************/
/* LZ77 data decompression										*/
/* Copyright (c) 1994 by XelaSoft								*/
/* version history:												*/
/*   version 0.9, start date: 11-27-1994						*/
/****************************************************************/

#define SHORT unsigned

#define slwinsnrbits (13)
#define maxstrlen (254)

#define maxhufcnt (127)
#define logtblsize (4)


#define tblsize (1<<logtblsize)

typedef struct huf_node {
  SHORT weight;
  struct huf_node *child1, *child2;
} huf_node;

/****************************************************************/
/* global vars													*/
/****************************************************************/
static unsigned updhufcnt;
static unsigned cpdist[tblsize+1];
static unsigned cpdbmask[tblsize];
static unsigned cpdext[] = { /* Extra bits for distance codes */
          0,  0,  0,  0,  1,  2,  3,  4,
          5,  6,  7,  8,  9, 10, 11, 12};

static SHORT tblsizes[tblsize];
static huf_node huftbl[2*tblsize-1];

/****************************************************************/
/* maak de huffman codeer informatie							*/
/****************************************************************/
static void mkhuftbl( void )
{
  unsigned count;
  huf_node  *hufpos;
  huf_node  *l1pos, *l2pos;
  SHORT tempw;

  /* Initialize the huffman tree */
  for (count=0, hufpos=huftbl; count != tblsize; count++) {
    (hufpos++)->weight=1+(tblsizes[count] >>= 1);
  }
  for (count=tblsize; count != 2*tblsize-1; count++)
    (hufpos++)->weight=-1;

  /* Place the nodes in the correct manner in the tree */
  while (huftbl[2*tblsize-2].weight == -1) {
    for (hufpos=huftbl; !(hufpos->weight); hufpos++)
      ;
    l1pos = hufpos++;
    while (!(hufpos->weight))
      hufpos++;
    if (hufpos->weight < l1pos->weight) {
      l2pos = l1pos;
      l1pos = hufpos++;
    }
    else
      l2pos = hufpos++;
    while ((tempw=(hufpos)->weight) != -1) {
      if (tempw) {
        if (tempw < l1pos->weight) {
          l2pos = l1pos;
          l1pos = hufpos;
        }
        else if (tempw < l2pos->weight)
          l2pos = hufpos;
      }
      hufpos++;
    }
    hufpos->weight = l1pos->weight+l2pos->weight;
    (hufpos->child1 = l1pos)->weight = 0;
    (hufpos->child2 = l2pos)->weight = 0;
  }
  updhufcnt = maxhufcnt;
}

/****************************************************************/
/* initialize the huffman info tables                           */
/****************************************************************/
static void inithufinfo( void )
{
  unsigned offs, count;

  for (offs=1, count=0; count != tblsize; count++) {
    cpdist[count] = offs;
    offs += (cpdbmask[count] = 1<<cpdext[count]);
  }
  cpdist[count] = offs;

  for (count = 0; count != tblsize; count++) {
    tblsizes[count] = 0; /* reset the table counters */
    huftbl[count].child1 = 0;  /* mark the leave nodes */
  }
  mkhuftbl(); /* make the huffman table */
}

/****************************************************************/
/* global vars													*/
/****************************************************************/
static STREAM *in_stream, *out_stream;

static UINT8 *outbuf;       /* the output buffer     */
static UINT8 *outbufpos;    /* pos in output buffer  */
static int outbufcnt;  /* #elements in the output buffer */
static int error_occ;

static UINT8 bitflg;  /* flag with the bits */
static UINT8 bitcnt;  /* #resterende bits   */

#define slwinsize (1 << slwinsnrbits)
#define outbufsize (slwinsize+4096)

/****************************************************************/
/* The function prototypes										*/
/****************************************************************/
static void unlz77( void );       /* perform the real decompression       */
static void charout(UINT8);      /* put a character in the output stream */
static unsigned rdstrlen( void ); /* read string length                   */
static unsigned rdstrpos( void ); /* read string pos                      */
static void flushoutbuf( void );
static UINT8 charin( void );       /* read a char                          */
static UINT8 bitin( void );        /* read a bit                           */

/****************************************************************/
/* de hoofdlus													*/
/****************************************************************/
static int xsa_extract (STREAM *in, STREAM *out)
    {
    UINT8 byt;

	/* setup the in-stream */
	stream_seek (in, 12, SEEK_SET);
    do  {
		if (1 != stream_read (in, &byt, 1) )
			return IMGTOOLERR_READERROR;
		} while (byt);
	in_stream = in;
    bitcnt = 0;         /* nog geen bits gelezen               */

	/* setup the out buffer */
    outbuf = malloc (outbufsize);
    if (!outbuf) return IMGTOOLERR_OUTOFMEMORY;
    outbufcnt = 0;
    outbufpos = outbuf; /* dadelijk vooraan in laden           */
	out_stream = out;

	/* let's do it .. */
	inithufinfo(); /* initialize the cpdist tables */

    error_occ = 0;
    unlz77();         /* decrunch de data echt               */

	free (outbuf);

	return error_occ;
    }

/****************************************************************/
/* the actual decompression algorithm itself					*/
/****************************************************************/
static void unlz77( void )
{
  UINT8 strl = 0;
  unsigned strpos;

  do {
    switch (bitin()) {
      case 0 : charout(charin()); break;
      case 1 : strl = rdstrlen();
	       if (strl == (maxstrlen+1))
		 break;
	       strpos = rdstrpos();
	       while (strl--)
		 charout(*(outbufpos-strpos));
	       strl = 0;
	       break;
    }
  }
  while (strl != (maxstrlen+1)&&!error_occ);
  flushoutbuf ();
}

/****************************************************************/
/* Put the next character in the input buffer.                  */
/* Take care that minimal 'slwinsize' chars are before the      */
/* current position.                                            */
/****************************************************************/
static void charout(UINT8 ch)
{
  if (error_occ) return;

  if ((outbufcnt++) == outbufsize) {
    if ( (outbufsize-slwinsize) != stream_write (out_stream, outbuf, outbufsize-slwinsize) )
	 	error_occ = IMGTOOLERR_WRITEERROR;

    memcpy(outbuf, outbuf+outbufsize-slwinsize, slwinsize);
    outbufpos = outbuf+slwinsize;
    outbufcnt = slwinsize+1;
  }
  *(outbufpos++) = ch;
}

/****************************************************************/
/* flush the output buffer                                      */
/****************************************************************/
static void flushoutbuf( void )
{
  if (!error_occ && outbufcnt) {
    if (outbufcnt != stream_write (out_stream, outbuf, outbufcnt) )
		error_occ = IMGTOOLERR_WRITEERROR;

    outbufcnt = 0;
  }
}

/****************************************************************/
/* read string length											*/
/****************************************************************/
static unsigned rdstrlen( void )
{
  UINT8 len;
  UINT8 nrbits;

  if (!bitin())
    return 2;
  if (!bitin())
    return 3;
  if (!bitin())
    return 4;

  for (nrbits = 2; (nrbits != 7) && bitin(); nrbits++)
    ;

  len = 1;
  while (nrbits--)
    len = (len << 1) | bitin();

  return (unsigned)(len+1);
}

/****************************************************************/
/* read string pos												*/
/****************************************************************/
static unsigned rdstrpos( void )
{
  UINT8 nrbits;
  UINT8 cpdindex;
  unsigned strpos;
  huf_node *hufpos;

  hufpos = huftbl+2*tblsize-2;

  while (hufpos->child1)
    if (bitin()) {
      hufpos = hufpos->child2;
    }
    else {
      hufpos = hufpos->child1;
    }
  cpdindex = hufpos-huftbl;
  tblsizes[cpdindex]++;

  if (cpdbmask[cpdindex] >= 256) {
    UINT8 strposmsb, strposlsb;

    strposlsb = (unsigned)charin();  /* lees LSB van strpos */
    strposmsb = 0;
    for (nrbits = cpdext[cpdindex]-8; nrbits--; strposmsb |= bitin())
      strposmsb <<= 1;
    strpos = (unsigned)strposlsb | (((unsigned)strposmsb)<<8);
  }
  else {
    strpos=0;
    for (nrbits = cpdext[cpdindex]; nrbits--; strpos |= bitin())
      strpos <<= 1;
  }
  if (!(updhufcnt--))
    mkhuftbl(); /* make the huffman table */

  return strpos+cpdist[cpdindex];
}

/****************************************************************/
/* read a bit from the input file								*/
/****************************************************************/
static UINT8 bitin( void )
{
  UINT8 temp;

  if (!bitcnt) {
    bitflg = charin();  /* lees de bitflg in */
    bitcnt = 8;         /* nog 8 bits te verwerken */
  }
  temp = bitflg & 1;
  bitcnt--;  /* weer 1 bit verwerkt */
  bitflg >>= 1;

  return temp;
}

/****************************************************************/
/* Get the next character from the input buffer.                */
/****************************************************************/
static UINT8 charin( void )
	{
	UINT8 byte;

	if (error_occ)
		return 0;
	else
        {
		if (1 != stream_read (in_stream, &byte, 1) )
			{
			error_occ = IMGTOOLERR_READERROR;
			return 0;
			}
		else
			return byte;
		}
	}
