/*
 * .xsa decompression. Code stolen from :
 *
 * http://web.inter.nl.net/users/A.P.Wulms/
 *
 * note that this code is severly hacked to work with imgtool and mess.
 * basically all file handling and system-specific stuff is taken out,
 * just decompressing in memory (which is nice and fast anyways).
 *
 * Sean Young. For use with MSX driver (MSX specific format)
 */

#include "xsa.h"
#include <stdlib.h>
#include <string.h>

#ifdef LSB_FIRST
#define intelLong(x) (x)
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | \
                       (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif

/****************************************************************/
/* LZ77 data decompression					*/
/* Copyright (c) 1994 by XelaSoft				*/
/* version history:						*/
/*   version 0.9, start date: 11-27-1994			*/
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
/* global vars							*/
/****************************************************************/
static unsigned updhufcnt;
static unsigned cpdist[tblsize+1];
static unsigned cpdbmask[tblsize];
unsigned cpdext[] = { /* Extra bits for distance codes */
          0,  0,  0,  0,  1,  2,  3,  4,
          5,  6,  7,  8,  9, 10, 11, 12};

static SHORT tblsizes[tblsize];
static huf_node huftbl[2*tblsize-1];

/****************************************************************/
/* maak de huffman codeer informatie				*/
/****************************************************************/
static void mkhuftbl()
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
static void inithufinfo()
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
/* global vars							*/
/****************************************************************/
static UINT8 *inbuf;        /* the input buffer     */
static UINT8 *inbufpos;     /* pos in input buffer  */
static int inbufsize;      /* size of input buffer */
static int inbufcnt;  /* #elements in the input buffer */

static UINT8 *outbuf;       /* the output buffer     */
static UINT8 *outbufpos;    /* pos in output buffer  */
static int outbufsize;     /* size of output buffer */
static int outbufcnt;  /* #elements in the output buffer */
static int error_occ;

static UINT8 bitflg;  /* flag with the bits */
static UINT8 bitcnt;  /* #resterende bits   */

/****************************************************************/
/* The function prototypes					*/
/****************************************************************/
static void unlz77();       /* perform the real decompression       */
static void charout(UINT8);      /* put a character in the output stream */
static unsigned rdstrlen(); /* read string length                   */
static unsigned rdstrpos(); /* read string pos                      */
static UINT8 charin();       /* read a char                          */
static UINT8 bitin();        /* read a bit                           */

/****************************************************************/
/* de hoofdlus							*/
/****************************************************************/
int msx_xsa_extract (UINT8 *xsadata, int xsalen, UINT8 **dskdata, int *size)
    {
    UINT32 temp32;
    int len;

    if (xsalen < 14) return 1;

    if (memcmp (xsadata, "PCK\010", 4) ) return 1;

    temp32 = *((UINT32*)(xsadata+4));
    outbufsize = intelLong (temp32); /* decompressed length */

    temp32 = *((UINT32*)(xsadata+8));
    inbufsize = intelLong (temp32); /* compressed length */
    len = 0;
    while ( (12 + len) < xsalen)
        {
        if (!xsadata[12+len]) break;
        len++;
        }

    if (xsadata[12+len] || !len) return 1;

    inbufsize = (xsalen - (13 + len));
    inbuf = xsadata + 13 + len;

    outbuf = malloc (outbufsize);
    if (!outbuf) return 2;

	inithufinfo(); /* initialize the cpdist tables */

    inbufcnt = 0;       /* er zit nog niks in de input buffer  */
    outbufcnt = 0;

    outbufpos = outbuf; /* dadelijk vooraan in laden           */
    inbufpos = inbuf; /* dadelijk vooraan in laden           */
    bitcnt = 0;         /* nog geen bits gelezen               */

    error_occ = 0;
    unlz77();         /* decrunch de data echt               */
    if (error_occ)
		{
		free (outbuf);
		return 1;
		}
    
    *size = outbufsize;
    *dskdata = outbuf;

    return 0;
    }

/*
 * ID the file; check the header, extract size & filename;
 */
int msx_xsa_id (UINT8 *xsadata, int xsalen, char **fname, int *size)
    {
    UINT32 temp32;
    int len;

    if (xsalen < 14) return 1;

    if (memcmp (xsadata, "PCK\010", 4) ) return 1;

    temp32 = *((UINT32*)(xsadata+4));
    *size = intelLong (temp32); /* decompressed length */

    temp32 = *((UINT32*)(xsadata+8));
    temp32 = intelLong (temp32); /* compressed length */
	if (temp32 != xsalen) return 1;
    len = 0;
    while ( (12 + len) < xsalen)
		{
		if (!xsadata[12+len]) break;
		len++;
    	}
    if (xsadata[12+len] || !len) return 1;

    *fname = malloc (len+1);
    if (!*fname) return 2;
    strcpy (*fname, xsadata + 12);

    return 0;
    }

/****************************************************************/
/* the actual decompression algorithm itself			*/
/****************************************************************/
static void unlz77()
{
  UINT8 strlen = 0;
  unsigned strpos;

  do {
    switch (bitin()) {
      case 0 : charout(charin()); break;
      case 1 : strlen = rdstrlen();
	       if (strlen == (maxstrlen+1))
		 break;
	       strpos = rdstrpos();
	       while (strlen--)
		 charout(*(outbufpos-strpos));
	       strlen = 0;
	       break;
    }
  }
  while (strlen != (maxstrlen+1)&&!error_occ);
}

/****************************************************************/
/* Put the next character in the input buffer.                  */
/* Take care that minimal 'slwinsize' chars are before the      */
/* current position.                                            */
/****************************************************************/
static void charout (UINT8 ch)
	{
	if (error_occ || outbufcnt++ > outbufsize)
		error_occ = 1; /* problem!! ran out of data */
	else
		*(outbufpos++) = ch;
	}

/****************************************************************/
/* read string length						*/
/****************************************************************/
static unsigned rdstrlen()
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
/* read string pos						*/
/****************************************************************/
static unsigned rdstrpos()
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
/* read a bit from the input file				*/
/****************************************************************/
static UINT8 bitin()
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
static UINT8 charin()
	{
	if (error_occ || (inbufcnt++ > inbufsize) ) 
        { 
		error_occ = 1;
		return 0;
		}

    return *(inbufpos++);
	}
