#ifndef WD179X_H
#define WD179X_H

#define MAX_DRIVES		4		/* we support 'only' four drives in MESS */

#define WD179X_IRQ_CLR  0
#define WD179X_IRQ_SET  1
/* R Nabet : added events for the DRQ pin... */
#define WD179X_DRQ_CLR  2
#define WD179X_DRQ_SET  3

#define DEN_FM_LO		0		/* this is used by TRS-80 (but not working) */
#define DEN_FM_HI		1
#define DEN_MFM_LO		2		/* and this one is the one that works */
#define DEN_MFM_HI		3		/* There were no HD disks back then ;) */

#define REAL_FDD		((void*)-1)

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

typedef struct {
	UINT8	 track;
	UINT8	 sector;
	UINT8	 status;
}	SECMAP;

typedef struct {
	void   (* callback)(int event);   /* callback for IRQ status */
	UINT8	unit;					/* unit number if image_file == REAL_FDD */
	UINT8	tracks; 				/* maximum # of tracks */
	UINT8	heads;					/* maximum # of heads */
	UINT8	density;				/* FM/MFM, single / double density */
	UINT16	offset; 				/* track 0 offset */
	UINT8	first_sector_id;		/* id of first sector */
	UINT8	sec_per_track;			/* sectors per track */
	UINT16	sector_length;			/* sector length (byte) */

	UINT8	head;					/* current head # */
	UINT8	track;					/* current track # */
	UINT8	track_reg;				/* value of track register */
	UINT8	direction;				/* last step direction */
	UINT8	sector; 				/* current sector # */
	UINT8	sector_dam; 			/* current sector # to fake read DAM command */
	UINT8	data;					/* value of data register */
	UINT8	command;				/* last command written */

	UINT8	read_cmd;				/* last read command issued */
	UINT8	write_cmd;				/* last write command issued */

	UINT8	status; 				/* status register */
	UINT8	status_drq; 			/* status register data request bit */
	UINT8	status_ipl; 			/* status register toggle index pulse bit */
	UINT8	busy_count; 			/* how long to keep busy bit set */

	UINT8	buffer[6144];			/* I/O buffer (holds up to a whole track) */
	UINT8	dam_list[256][4];		/* list of data address marks while formatting */
	int 	dam_data[256];			/* offset to data inside buffer while formatting */
	int 	dam_cnt;				/* valid number of entries in the dam_list */
	UINT8	*fmt_sector_data[256];	/* pointer to data after formatting a track */
    int     data_offset;            /* offset into I/O buffer */
	int 	data_count; 			/* transfer count from/into I/O buffer */

	const char *image_name; 		/* file name for disc image */
	void	*image_file;			/* file handle for disc image */
	int 	mode;					/* open mode == 0 read only, != 0 read/write */
	unsigned long image_size;		/* size of image file */

	UINT16	dir_sector; 			/* directory track for deleted DAM */
	UINT16	dir_length; 			/* directory length for deleted DAM */

	SECMAP	*secmap;

}	WD179X;

extern void wd179x_init(int active);

extern void *wd179x_select_drive(UINT8 drive, UINT8 head, void (*callback)(int), const char *name);
extern void wd179x_stop_drive(void);

extern void wd179x_read_sectormap(UINT8 drive, UINT8 *tracks, UINT8 *heads, UINT8 *sec_per_track);
extern void wd179x_set_geometry(UINT8 drive, UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT16 dir_sector, UINT16 dir_length, UINT8 first_sector_id);

extern WRITE_HANDLER ( wd179x_command_w );
extern WRITE_HANDLER ( wd179x_track_w );
extern WRITE_HANDLER ( wd179x_sector_w );
extern WRITE_HANDLER ( wd179x_data_w );

extern READ_HANDLER ( wd179x_status_r );
extern READ_HANDLER ( wd179x_track_r );
extern READ_HANDLER ( wd179x_sector_r );
extern READ_HANDLER ( wd179x_data_r );

#endif
