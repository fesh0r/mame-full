/***************************************************************************

  WD179X.c

  Functions to emulate a WD179x floppy disc controller

***************************************************************************/

#include "driver.h"
#include "machine/wd179x.h"

#undef  VERBOSE
#define VERBOSE

#define FDC_STEP_RATE   0x03    /* Type I additional flags */
#define FDC_STEP_VERIFY 0x04    /* verify track number */
#define FDC_STEP_HDLOAD 0x08    /* load head */
#define FDC_STEP_UPDATE 0x10    /* update track register */

#define FDC_RESTORE     0x00    /* Type I commands */
#define FDC_SEEK        0x10
#define FDC_STEP        0x20
#define FDC_STEP_IN     0x40
#define FDC_STEP_OUT    0x60

#define FDC_MASK_TYPE_I         (FDC_STEP_HDLOAD|FDC_STEP_VERIFY|FDC_STEP_RATE)

/* Type I commands status */
#define STA_1_BUSY      0x01    /* controller is busy */
#define STA_1_IPL       0x02    /* index pulse */
#define STA_1_TRACK0    0x04    /* track 0 detected */
#define STA_1_CRC_ERR   0x08    /* CRC error */
#define STA_1_SEEK_ERR  0x10    /* seek error */
#define STA_1_HD_LOADED 0x20    /* head loaded */
#define STA_1_WRITE_PRO 0x40    /* floppy is write protected */
#define STA_1_NOT_READY 0x80    /* controller not ready */

/* Type II and III additional flags */
#define FDC_DELETED_AM  0x01    /* read/write deleted address mark */
#define FDC_SIDE_CMP_T  0x02    /* side compare track data */
#define FDC_15MS_DELAY  0x04    /* delay 15ms before command */
#define FDC_SIDE_CMP_S  0x08    /* side compare sector data */
#define FDC_MULTI_REC   0x10    /* only for type II commands */

/* Type II commands */
#define FDC_READ_SEC    0x80    /* read sector */
#define FDC_WRITE_SEC   0xA0    /* write sector */

#define FDC_MASK_TYPE_II        (FDC_MULTI_REC|FDC_SIDE_CMP_S|FDC_15MS_DELAY|FDC_SIDE_CMP_T|FDC_DELETED_AM)

/* Type II commands status */
#define STA_2_BUSY      0x01    
#define STA_2_DRQ       0x02
#define STA_2_LOST_DAT  0x04
#define STA_2_CRC_ERR   0x08
#define STA_2_REC_N_FND 0x10
#define STA_2_REC_TYPE  0x20
#define STA_2_WRITE_PRO 0x40
#define STA_2_NOT_READY 0x80

#define FDC_MASK_TYPE_III       (FDC_SIDE_CMP_S|FDC_15MS_DELAY|FDC_SIDE_CMP_T|FDC_DELETED_AM)

/* Type III commands */
#define FDC_READ_DAM    0xc0    /* read data address mark */
#define FDC_READ_TRK    0xe0    /* read track */
#define FDC_WRITE_TRK   0xf0    /* write track (format) */

/* Type IV additional flags */
#define FDC_IM0         0x01    /* interrupt mode 0 */
#define FDC_IM1         0x02    /* interrupt mode 1 */
#define FDC_IM2         0x04    /* interrupt mode 2 */
#define FDC_IM3         0x08    /* interrupt mode 3 */

#define FDC_MASK_TYPE_IV        (FDC_IM3|FDC_IM2|FDC_IM1|FDC_IM0)

/* Type IV commands */
#define FDC_FORCE_INT   0xd0    /* force interrupt */

/* structure describing a double density track */
#define TRKSIZE_DD      6144
static  byte track_DD[][2] = {
        {16, 0x4e},     /* 16 * 4E (track lead in)               */
        {8,  0x00},     /*  8 * 00 (pre DAM)                     */
        {3,  0xf5},     /*  3 * F5 (clear CRC)                   */
        {1,  0xfe},     /* *** sector *** FE (DAM)               */
        {1,  0x80},     /*  4 bytes track,head,sector,seclen     */
        {1,  0xf7},     /*  1 * F7 (CRC)                         */
        {22, 0x4e},     /* 22 * 4E (sector lead in)              */
        {12, 0x00},     /* 12 * 00 (pre AM)                      */
        {3,  0xf5},     /*  3 * F5 (clear CRC)                   */
        {1,  0xfb},     /*  1 * FB (AM)                          */
        {1,  0x81},     /*  x bytes sector data                  */
        {1,  0xf7},     /*  1 * F7 (CRC)                         */
        {16, 0x4e},     /* 16 * 4E (sector lead out)             */
        {8,  0x00},     /*  8 * 00 (post sector)                 */
        {0,  0x00},     /* end of data                           */
};

/* structure describing a single density track */
#define TRKSIZE_SD      3172
static  byte track_SD[][2] = {
        {16, 0xff},     /* 16 * FF (track lead in)               */
        {8,  0x00},     /*  8 * 00 (pre DAM)                     */
        {1,  0xfc},     /*  1 * FC (clear CRC)                   */
        {11, 0xff},     /* *** sector *** 11 * FF                */
        {6,  0x00},     /*  6 * 00 (pre DAM)                     */
        {1,  0xfe},     /*  1 * FE (DAM)                         */
        {1,  0x80},     /*  4 bytes track,head,sector,seclen     */
        {1,  0xf7},     /*  1 * F7 (CRC)                         */
        {10, 0xff},     /* 10 * FF (sector lead in)              */
        {4,  0x00},     /*  4 * 00 (pre AM)                      */
        {1,  0xfb},     /*  1 * FB (AM)                          */
        {1,  0x81},     /*  x bytes sector data                  */
        {1,  0xf7},     /*  1 * F7 (CRC)                         */
        {0,  0x00},     /* end of data                           */
};

static  WD179X  wd[MAX_DRIVES] = {{0,}, };
static  byte    drv = 0;

void    wd179x_init(int active)
{
int     i;
        for (i = 0; i < MAX_DRIVES; i++)
        {
                wd[i].tracks            = 40;
                wd[i].heads             = 1;
                wd[i].density           = 1;
                wd[i].offset            = 0;
                wd[i].sec_per_track     = 18;
                wd[i].sector_length     = 256;
                wd[i].head              = 0;
                wd[i].track             = 0;
                wd[i].track_reg         = 0;
                wd[i].direction         = 1;
                wd[i].sector            = 0;
                wd[i].data              = 0;
                wd[i].status            = (active) ? STA_1_TRACK0 : 0;
                wd[i].status_drq        = 0;
                wd[i].status_ipl        = 0;
                wd[i].busy_count        = 0;
                wd[i].data_offset       = 0;
                wd[i].data_count        = 0;
                wd[i].image_name        = 0;
                wd[i].image_size        = 0;
                wd[i].dir_sector        = 0;
                wd[i].dir_length        = 0;
                wd[i].secmap            = 0;

        }
}

void    *wd179x_select_drive(byte drive, byte head, void (*callback)(int), char * name)
{
WD179X  *w = &wd[drive];

        if (drive < MAX_DRIVES)
        {
#ifdef  VERBOSE
                if (errorlog)
                        fprintf(errorlog, "WD179X select #%d head %d\n", drive, head);
#endif
                drv = drive;
                w->head = head;
                w->status_ipl = STA_1_IPL;
                w->callback = callback;

                if (w->image_name && strcmp(w->image_name, name))
                {
                        if (w->image_file)
                        {
#ifdef  VERBOSE
                                if (errorlog)
                                        fprintf(errorlog, "WD179X close image #%d %s\n",
                                                drv, w->image_name);
#endif
                                osd_fclose(w->image_file);
                                w->image_file = 0;
                        }
                }
                if (!strlen(name))
                {
                        w->status = STA_1_NOT_READY;
                        return 0;
                }

                w->image_name = name;

                if (w->image_file)
                        return w->image_file;

#ifdef  VERBOSE
                if (errorlog)
                        fprintf(errorlog, "WD179X open image #%d %s\n",
                                drv, w->image_name);
#endif
                w->image_file = osd_fopen(Machine->gamedrv->name, w->image_name, OSD_FILETYPE_IMAGE, 1);

                if (w->image_file)
                {
                        w->track  = 0;
                        w->head   = 0;
                        w->sector = 0;
                }

                return w->image_file;
        }
        return 0;
}

void    wd179x_stop_drive(void)
{
int     i;
        for (i = 0; i < MAX_DRIVES; i++)
        {
                wd[i].busy_count = 0;
                wd[i].status     = 0;
                wd[i].status_drq = 0;
                wd[i].status_ipl = 0;
                if (wd[i].image_file)
                {
                	osd_fclose (wd[i].image_file);
                	wd[i].image_file = 0;
#ifdef  VERBOSE
        			if (errorlog)
						fprintf(errorlog, "WD179X drive #%d image %s closed\n", i, wd[i].image_name);
#endif
                }
        }
}

void    wd179x_read_sectormap(byte drive, byte * tracks, byte * heads, byte * sec_per_track)
{
WD179X  *w = &wd[drive];
SECMAP  *p;
byte    head;
        if (!w->secmap)
                w->secmap = malloc(0x2200);
        osd_fseek(w->image_file, 0, SEEK_SET);
        osd_fread(w->image_file, w->secmap, 0x2200);
        w->offset = 0x2200;
        w->tracks = 0;
        w->heads = 0;
        w->sec_per_track = 0;
        for (p = w->secmap; p->track != 0xff; p++)
        {
                if (p->track > w->tracks)
                        w->tracks = p->track;
                if (p->sector > w->sec_per_track)
                        w->sec_per_track = p->sector;
                head = (p->status >> 4) & 1;
                if (head > w->heads)
                        w->heads = head;
        }
        *tracks = w->tracks++;
        *heads = w->heads++;
        *sec_per_track = w->sec_per_track++;
#ifdef  VERBOSE
        if (errorlog)
        {       fprintf(errorlog, "WD179X geometry for drive #%d is %d tracks, %d heads, %d sec/track\n",
                        drive, w->tracks, w->heads, w->sec_per_track);
        }
#endif
}

void    wd179x_set_geometry(byte drive, byte tracks, byte heads, byte sec_per_track, word sector_length, word dir_sector, word dir_length)
{
WD179X  *w = &wd[drive];

        if (drive >= MAX_DRIVES)
        {
                if (errorlog)
                        fprintf(errorlog, "WD179X drive #%d not supported!\n", drive);
                return;
        }

#ifdef  VERBOSE
        if (errorlog)
        {
                fprintf(errorlog, "WD179X geometry for drive #%d is %d tracks, %d heads, %d sec/track\n",
                        drive, tracks, heads, sec_per_track);
                if (dir_length)
                {
                        fprintf(errorlog, "WD179X directory at sector # %d, %d sectors\n",
                                dir_sector, dir_length);
                }
        }
#endif

        w->tracks        = tracks;
        w->heads         = heads;
        w->sec_per_track = sec_per_track;
        w->sector_length = sector_length;
        w->dir_sector    = dir_sector;
        w->dir_length    = dir_length;

        w->image_size = w->tracks * w->heads * w->sec_per_track * w->sector_length;
}

/* seek to track/head/sector relative position in image file */
static  int     seek(WD179X * w, byte t, byte h, byte s)
{
unsigned long offset;
SECMAP  *p;
byte    head;

        if (w->secmap)
        {
                offset = 0x2200;
                for (p = w->secmap; p->track != 0xff; p++)
                {
                        if (p->track == t && p->sector == s)
                        {
                                head = (p->status & 0x10) >> 4;
                                if (head == h)
                                {
#ifdef  VERBOSE
                                        if (errorlog) fprintf(errorlog, "WD179X seek #%d track:%d head:%d sector:%d-> offset #0x%08lX\n",
                                                drv, t, h, s, offset);
#endif
                                        if (osd_fseek(w->image_file, offset, SEEK_SET) < 0)
                                        {
                                                if (errorlog) fprintf(errorlog, "WD179X seek failed\n");
                                                return STA_1_SEEK_ERR;
                                        }
                                        return 0;
                                }
                        }
                        offset += 0x100;
                }
#ifdef  VERBOSE
                if (errorlog) fprintf(errorlog, "WD179X seek #%d track:%d head:%d sector:%d : seek err\n",
                        drv, t, h, s);
#endif
                return STA_1_SEEK_ERR;
        }
        if (t >= w->tracks)
        {
                if (errorlog) fprintf(errorlog, "WD179X track %d >= %d\n", t, w->tracks);
                return STA_1_SEEK_ERR;
        }
        if (h >= w->heads)
        {
                if (errorlog) fprintf(errorlog, "WD179X head %d >= %d\n", h, w->heads);
                return STA_1_SEEK_ERR;
        }
        if (s >= w->sec_per_track)
        {
                if (errorlog) fprintf(errorlog, "WD179X sector %d >= %d\n", w->sector, w->sec_per_track);
                return STA_2_REC_N_FND;
        }

        offset = t;
        offset *= w->heads;
        offset += h;
        offset *= w->sec_per_track;
        offset += s;
        offset *= w->sector_length;

#ifdef  VERBOSE
        if (errorlog) fprintf(errorlog, "WD179X seek #%d track:%d head:%d sector:%d-> offset #0x%08lX\n",
                drv, t, h, s, offset);
#endif

        if (offset > w->image_size)
        {
                if (errorlog) fprintf(errorlog, "WD179X seek offset %ld >= %ld\n", offset, w->image_size);
                return STA_1_SEEK_ERR;
        }

        if (osd_fseek(w->image_file, offset, SEEK_SET) < 0)
        {
                if (errorlog) fprintf(errorlog, "WD179X seek failed\n");
                return STA_1_SEEK_ERR;
        }

        return 0;
}

/* return STA_2_REC_TYPE depending on relative sector */
static  int     deleted_dam(WD179X * w)
{
unsigned rel_sector = (w->track * w->heads + w->head) * w->sec_per_track + w->sector;
SECMAP  *p;
byte    head;
        if (w->secmap)
        {
                for (p = w->secmap; p->track != 0xff; p++)
                {
                        if (p->track == w->track && p->sector == w->sector)
                        {
                                head = (p->status >> 4) & 1;
                                if (w->head == head)
                                        return p->status & STA_2_REC_TYPE;
                        }
                }
                return STA_2_REC_N_FND;
        }
        if (rel_sector >= w->dir_sector && rel_sector < w->dir_sector + w->dir_length)
        {
#ifdef  VERBOSE
                if (errorlog) fprintf(errorlog, "WD179X deleted DAM at sector #%d\n", rel_sector);
#endif
                return STA_2_REC_TYPE;
        }
        return 0;
}

/* calculate CRC for data address marks or sector data */
static  void    calc_crc(word * crc, byte value)
{
byte    l, h;
        l = value ^ (*crc >> 8);
        *crc = (*crc & 0xff) | (l << 8);
        l >>= 4;
        l ^= (*crc >> 8);
        *crc <<= 8;
        *crc = (*crc & 0xff00) | l;
        l = (l << 4) | (l >> 4);
        h = l;
        l = (l << 2) | (l >> 6);
        l &= 0x1f;
        *crc = *crc ^ (l << 8);
        l = h & 0xf0;
        *crc = *crc ^ (l << 8);
        l = (h << 1) | (h >> 7);
        l &= 0xe0;
        *crc = *crc ^ l;
}

/* read the next data address mark */
static  void    read_dam(WD179X * w)
{
word    crc = 0xffff;
        w->data_offset = 0;
        w->data_count = 6;
        w->buffer[0] = w->track;
        w->buffer[1] = w->head;
        w->buffer[2] = w->sector_dam;
        w->buffer[3] = w->sector_length >> 8;
        calc_crc(&crc, w->buffer[0]);
        calc_crc(&crc, w->buffer[1]);
        calc_crc(&crc, w->buffer[2]);
        calc_crc(&crc, w->buffer[3]);
        w->buffer[4] = crc & 255;
        w->buffer[5] = crc / 256;
        if (++w->sector_dam == w->sec_per_track)
                w->sector_dam = 0;
        w->status_drq = STA_2_DRQ;
        w->status = STA_2_DRQ | STA_2_BUSY;
        w->busy_count = 0;
}

/* read a sector */
static  void    read_sector(WD179X * w)
{
        w->data_offset = 0;
        w->data_count = w->sector_length;
        if (osd_fread(w->image_file, w->buffer, w->sector_length) != w->sector_length)
        {
                w->status = STA_2_LOST_DAT;
                return;
        }
        w->status_drq = STA_2_DRQ;
        w->status = STA_2_DRQ | STA_2_BUSY;
        w->busy_count = 0;
}

/* read an entire track */
static  void    read_track(WD179X * w)
{
byte    dam_list[256][4];       /* buffer for DAM list */
byte    * psrc;                 /* pointer to track format structure */
byte    * pdst;                 /* pointer to track buffer */
int     cnt;                    /* number of bytes to fill in */
word    crc;                    /* id or data CRC */
byte    d;                      /* data */
byte    t = w->track;           /* track of DAM */
byte    h = w->head;            /* head of DAM */
byte    s = w->sector_dam;      /* sector of DAM */
word    l = w->sector_length;   /* sector length of DAM */
int     dam_cnt = 0;
int     i;

        for (i = 0; i < w->sec_per_track; i++)
        {
                dam_list[i][0] = t;
                dam_list[i][1] = h;
                dam_list[i][2] = i;
                dam_list[i][3] = l >> 8;
        }

        pdst = w->buffer;

        if (w->density)
        {
                psrc  = track_DD[0];    /* double density track format */
                cnt = TRKSIZE_DD;
        }
        else
        {
                psrc  = track_SD[0];    /* single density track format */
                cnt = TRKSIZE_SD;
        }

        while (cnt > 0)
        {
                if (psrc[0] == 0)                       /* no more track format info ? */
                {
                        if (dam_cnt < w->sec_per_track) /* but more DAM info ? */
                        {
                                if (w->density)         /* DD track ? */
                                        psrc = track_DD[3];
                                else
                                        psrc = track_SD[3];
                        }
                }

                if (psrc[0] != 0)                       /* more track format info ? */
                {
                        cnt -= psrc[0];               /* subtract size */
                        d = psrc[1];

                        if (d == 0xf5)                  /* clear CRC ? */
                        {
                                crc = 0xffff;
                                d = 0xa1;               /* store A1 */
                        }

                        for (i = 0; i < *psrc; i++)
                                *pdst++ = d;            /* fill data */

                        if (d == 0xf7)                  /* store CRC ? */
                        {
                                pdst--;                 /* go back one byte */
                                *pdst++ = crc & 255;    /* put CRC low */
                                *pdst++ = crc / 256;    /* put CRC high */
                                cnt -= 1;               /* count one more byte */
                        }
                        else
                        if (d == 0xfe)                  /* address mark ? */
                        {
                                crc = 0xffff;           /* reset CRC */
                        }
                        else
                        if (d == 0x80)                  /* sector ID ? */
                        {
                                pdst--;                 /* go back one byte */
                                t = *pdst++ = dam_list[dam_cnt][0];  /* track number */
                                h = *pdst++ = dam_list[dam_cnt][1];  /* head number */
                                s = *pdst++ = dam_list[dam_cnt][2];  /* sector number */
                                l = *pdst++ = dam_list[dam_cnt][3];  /* sector length code */
                                dam_cnt++;
                                calc_crc(&crc,t);       /* build CRC */
                                calc_crc(&crc,h);       /* build CRC */
                                calc_crc(&crc,s);       /* build CRC */
                                calc_crc(&crc,l);       /* build CRC */
                                l = (l == 0) ? 128 : l << 8;
                        }
                        else
                        if (d == 0xfb)                  // data address mark ?
                        {
                                crc = 0xffff;           // reset CRC
                        }
                        else
                        if (d == 0x81)                  // sector DATA ?
                        {
                                pdst--;                 /* go back one byte */
                                if (seek(w, t, h, s) == 0)
                                {
                                        if (osd_fread(w->image_file, pdst, l) != l)
                                        {
                                                w->status = STA_2_CRC_ERR;
                                                return;
                                        }
                                }
                                else
                                {
                                        w->status = STA_2_REC_N_FND;
                                        return;
                                }
                                for (i = 0; i < l; i++) // build CRC of all data
                                        calc_crc(&crc, *pdst++);
                                cnt -= l;
                        }
                        psrc += 2;
                }
                else
                {
                        *pdst++ = 0xff;                 /* fill track */
                        cnt--;                          /* until end */
                }
        }

        w->data_offset = 0;
        w->data_count = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;

        w->status_drq = STA_2_DRQ;
        w->status |= STA_2_DRQ | STA_2_BUSY;
        w->busy_count = 0;
}

/* write a sector */
static  void    write_sector(WD179X * w)
{
        w->status = seek(w, w->track, w->head, w->sector);
        if (w->status == 0)
        {
                if (osd_fwrite(w->image_file, w->buffer, w->data_offset) != w->data_offset)
                        w->status = STA_2_LOST_DAT;
        }
}

/* write an entire track by extracting the sectors */
static  void    write_track(WD179X * w)
{
byte    *f;
byte    dam_list[256][4];
byte    *dam_data[256];
int     dam_cnt = 0;
int     cnt;

        memset(dam_list, 0xff, sizeof(dam_list));

        f = w->buffer;
        dam_cnt = 0;
        cnt = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;

        do
        {
                while ((--cnt > 0) && (*f != 0xfe))     /* start of DAM ?? */
                        f++;

                if (cnt > 0)
                {
                        f++;                            /* skip FE */
                        dam_list[dam_cnt][0] = *f++;    /* copy track number */
                        dam_list[dam_cnt][1] = *f++;    /* copy head number */
                        dam_list[dam_cnt][2] = *f++;    /* copy sector number */
                        dam_list[dam_cnt][3] = *f++;    /* copy sector length */
                        while ((--cnt > 0) && (*f != 0xfb))     /* start of DATA ? */
                                f++;
                        f++;                            /* skip FB */
                        dam_data[dam_cnt] = f;          /* set pointer to DATA */
                        dam_cnt++;
                }
        } while (cnt > 0);

        /* now put all sectors contained in the format buffer */
        for (cnt = 0; cnt < dam_cnt; cnt++)
        {
                w->status = seek(w, w->track, w->head, dam_list[cnt][2]);
                if (w->status == 0)
                {
                        if (osd_fwrite(w->image_file, dam_data[cnt], w->sector_length) != w->sector_length)
                        {
                                w->status = STA_2_LOST_DAT;
                                return;
                        }
                }
        }
}


/* read the FDC status register. This clears IRQ line too */
int     wd179x_status_r(int offset)
{
WD179X  *w = &wd[drv];
int     result = w->status;
        if (w->callback)
                (*w->callback)(WD179X_IRQ_CLR);
        if (w->busy_count)
        {
                if (!--w->busy_count)
                        w->status &= ~STA_1_BUSY;
        }
        /* eventually toggle index pulse bit */
        w->status ^= w->status_ipl;
        /* eventually set data request bit */
        w->status |= w->status_drq;
#ifdef VERBOSE
		if (errorlog) fprintf (errorlog, "wd179x_status_r: %02x\n", result);
#endif
        return result;
}

/* read the FDC track register */
int     wd179x_track_r(int offset)
{
WD179X  *w = &wd[drv];
#ifdef VERBOSE
		if (errorlog) fprintf (errorlog, "wd179x_track_r: %02x\n", w->track_reg);
#endif
        return w->track_reg;
}

/* read the FDC sector register */
int     wd179x_sector_r(int offset)
{
WD179X  *w = &wd[drv];
#ifdef VERBOSE
		if (errorlog) fprintf (errorlog, "wd179x_sector_r: %02x\n", w->sector);
#endif
        return w->sector;
}

/* read the FDC data register */
int     wd179x_data_r(int offset)
{
WD179X  *w = &wd[drv];
        if (w->data_count > 0)
        {
                w->status &= ~STA_2_DRQ;
                if (--w->data_count <= 0)
                {
                        /* clear busy bit */
                        w->status &= ~STA_2_BUSY;
                        /* no more setting of data request bit */
                        w->status_drq = 0;
                        /* read normal or deleted data address mark ? */
                        w->status |= deleted_dam(w);
                        /* generate an IRQ */
                        if (w->callback) (*w->callback)(WD179X_IRQ_SET);
                }
                w->data = w->buffer[w->data_offset++];
        }
        return w->data;
}

/* write the FDC command register */
void    wd179x_command_w(int offset, int data)
{
WD179X  *w = &wd[drv];

        if ((data | 1) == 0xff)         /* change single/double density ? */
        {
                w->density = data & 1;
                return;
        }

        if ((data & ~FDC_MASK_TYPE_IV) == FDC_FORCE_INT)
        {
#ifdef  VERBOSE
                if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X FORCE_INT\n", drv, data);
#endif
                w->data_count = 0;
                w->data_offset = 0;
                w->status &= ~(STA_2_DRQ | STA_2_BUSY);
                w->status_drq = 0;
                w->status_ipl = STA_1_IPL;
                if (w->callback) (*w->callback)(WD179X_IRQ_CLR);
                w->busy_count = 0;
                return;
        }

        if (data & 0x80)
        {
                w->status_ipl = 0;

                if ((data & ~FDC_MASK_TYPE_II) == FDC_READ_SEC)
                {
#ifdef  VERBOSE
                        if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X READ_SEC\n", drv, data);
#endif
                        w->command = data & ~FDC_MASK_TYPE_II;
                        w->status = seek(w, w->track, w->head, w->sector);
                        if (w->status == 0)
                                read_sector(w);
                        return;
                }

                if ((data & ~FDC_MASK_TYPE_II) == FDC_WRITE_SEC)
                {
#ifdef  VERBOSE
                        if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X WRITE_SEC\n", drv, data);
#endif
                        w->command = data & ~FDC_MASK_TYPE_II;
                        w->data_offset = 0;
                        w->data_count = w->sector_length;
                        w->status_drq = STA_2_DRQ;
                        w->status = STA_2_DRQ | STA_2_BUSY;
                        w->busy_count = 0;
                        return;
                }

                if ((data & ~FDC_MASK_TYPE_III) == FDC_READ_TRK)
                {
#ifdef  VERBOSE
                        if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X READ_TRK\n", drv, data);
#endif
                        w->command = data & ~FDC_MASK_TYPE_III;
                        w->status = seek(w, w->track, w->head, w->sector);
                        if (w->status == 0)
                                read_track(w);
                        return;
                }

                if ((data & ~FDC_MASK_TYPE_III) == FDC_WRITE_TRK)
                {
#ifdef  VERBOSE
                        if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X WRITE_TRK\n", drv, data);
#endif
                        w->command = data & ~FDC_MASK_TYPE_III;
                        w->data_offset = 0;
                        w->data_count = (w->density) ? TRKSIZE_DD : TRKSIZE_SD;
                        w->status_drq = STA_2_DRQ;
                        w->status = STA_2_DRQ | STA_2_BUSY;
                        w->busy_count = 0;
                        return;
                }

                if ((data & ~FDC_MASK_TYPE_III) == FDC_READ_DAM)
                {
#ifdef  VERBOSE
                        if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X READ_DAM\n", drv, data);
#endif
                        w->status = seek(w, w->track, w->head, w->sector);
                        if (w->status == 0)
                                read_dam(w);
                        return;
                }

                if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X unknown\n", drv, data);
                return;
        }


        if ((data & ~FDC_MASK_TYPE_I) == FDC_RESTORE)
        {
#ifdef  VERBOSE
                if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X RESTORE\n", drv, data);
#endif
                /* simulate seek time busy signal */
                w->busy_count = w->track * ((data & FDC_STEP_RATE) + 1);
                w->track_reg = w->track = 0;    /* set track number 0 */
        }

        if ((data & ~FDC_MASK_TYPE_I) == FDC_SEEK)
        {
#ifdef  VERBOSE
                if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X SEEK $%02X\n", drv, data, w->data);
#endif
                /* simulate seek time busy signal */
                w->busy_count = abs(data - w->track) * ((data & FDC_STEP_RATE) + 1);
                w->track = w->data;     /* get track number from data register */
        }

        if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP)
        {
#ifdef  VERBOSE
                if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X STEP dir $%02X\n", drv, data, w->direction);
#endif
                /* simulate seek time busy signal */
                w->busy_count = ((data & FDC_STEP_RATE) + 1);
                w->track += w->direction;       /* adjust track number */
        }

        if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP_IN)
        {
#ifdef  VERBOSE
                if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X STEP_IN\n", drv, data);
#endif
                /* simulate seek time busy signal */
                w->busy_count = ((data & FDC_STEP_RATE) + 1);
                w->direction = +1;
                w->track += w->direction;       /* adjust track number */
        }

        if ((data & ~(FDC_STEP_UPDATE | FDC_MASK_TYPE_I)) == FDC_STEP_OUT)
        {
#ifdef  VERBOSE
                if (errorlog) fprintf(errorlog, "WD179X #%d cmd $%02X STEP_OUT\n", drv, data);
#endif
                /* simulate seek time busy signal */
                w->busy_count = ((data & FDC_STEP_RATE) + 1);
                w->direction = -1;
                w->track += w->direction;       /* adjust track number */
        }

        if (w->busy_count)
                w->status = STA_1_BUSY;

        /* toggle index pulse at read */
        w->status_ipl = STA_1_IPL;

        if (w->track >= w->tracks)
                w->status |= STA_1_SEEK_ERR;

        if (w->track == 0)
                w->status |= STA_1_TRACK0;

        if (data & FDC_STEP_UPDATE)
                w->track_reg = w->track;

        if (data & FDC_STEP_HDLOAD)
                w->status |= STA_1_HD_LOADED;

        if (data & FDC_STEP_VERIFY)
                if (w->track_reg != w->track)
                        w->status |= STA_1_SEEK_ERR;
}

/* write the FDC track register */
void    wd179x_track_w(int offset, int data)
{
WD179X  *w = &wd[drv];
        w->track = w->track_reg = data;
#ifdef  VERBOSE
        if (errorlog) fprintf(errorlog, "WD179X #%d trk $%02X\n", drv, data);
#endif
}

/* write the FDC sector register */
void    wd179x_sector_w(int offset, int data)
{
WD179X  *w = &wd[drv];
        w->sector = data;
#ifdef  VERBOSE
        if (errorlog) fprintf(errorlog, "WD179X #%d sec $%02X\n", drv, data);
#endif
}

/* write the FDC data register */
void    wd179x_data_w(int offset, int data)
{
WD179X  *w = &wd[drv];
        if (w->data_count > 0)
        {
                w->buffer[w->data_offset++] = data;
                if (--w->data_count <= 0)
                {
#ifdef  VERBOSE
                        if (errorlog) fprintf(errorlog, "WD179X buffered %d byte\n", w->data_offset);
#endif
                        w->status_drq = 0;
                        if (w->command == FDC_WRITE_TRK)
                                write_track(w);
                        else
                                write_sector(w);
                        w->data_offset = 0;
                        if (w->callback) (*w->callback)(WD179X_IRQ_SET);
                }
        }
#ifdef  VERBOSE
        else
        {
                if (errorlog) fprintf(errorlog, "WD179X #%d data $%02X\n", drv, data);
        }
#endif
        w->data = data;
}
