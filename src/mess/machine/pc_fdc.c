/***************************************************************************

	machine/pc_fdc.c

	Functions to emulate a NEC765 compatible floppy disk controller

	TODO:
	implement more commands: eg. read ID, read/write deleted
	handle images containing some sort of geometry information
	(ie. support non constant sector lengths / sizes per disk).

***************************************************************************/
#include "mess/machine/pc.h"

#define TWO_SIDE		0x08
#define READY			0x20
#define WRITE_PROTECT	0x40
#define FAULT			0x80

#define FDC_DMA 	2						/* DMA channel number for the FDC */

void *pc_fdc_file[MAX_FLOPPY];				/* up to four floppy disk images */
UINT8 pc_fdc_heads[MAX_FLOPPY] = {2,2,2,2}; 	/* 2 heads */
UINT8 pc_fdc_spt[MAX_FLOPPY] = {9,9,9,9};	/* 9 sectors per track */
UINT8 pc_gpl[MAX_FLOPPY] = {42,42,42,42};	/* gap III length */
UINT8 pc_fill[MAX_FLOPPY] = {0xf6,0xf6,0xf6,0xf6}; /* filler byte */
UINT8 pc_fdc_scl[MAX_FLOPPY] = {2,2,2,2};	/* 512 bytes per sector */

static void *FDC_timer = 0;

static UINT8 drv;							/* drive number (0..3) */
static UINT8 FDC_enable;					/* enable/reset flag */
static UINT8 FDC_dma;						/* DMA mode flag */
static UINT8 FDC_motor; 					/* motor running flags */
static UINT8 track[MAX_FLOPPY] = {0,0,0,0}; /* floppy current track numbers */
static UINT8 head[MAX_FLOPPY] = {0,0,0,0};	/* floppy current head numbers */
static UINT8 sector[MAX_FLOPPY] = {0,0,0,0};/* floppy current sector numbers */
static int offset[MAX_FLOPPY];              /* current seek() offset into disk images */

static int FDC_floppy[MAX_FLOPPY] = {		/* floppy drive status flags */
	READY, FAULT, FAULT, FAULT };

static int FDC_drq;                     /* data request count */

static UINT8 FDC_main = 0x00;           /* main status */

static UINT8 FDC_cmd_file[16];          /* command file */
static UINT8 *FDC_cmd = FDC_cmd_file;	/* command pointer */
static UINT8 cmd_size[32] = {
	1,1,3,3,1,9,9,2,1,9,2,1,9,6,1,3,
	1,9,1,1,1,1,9,1,1,9,1,1,1,9,1,1
};
#define FDC_CMD 		FDC_cmd_file[0]
#define FDC_CMD_UNIT	FDC_cmd_file[1]
#define FDC_CMD_C		FDC_cmd_file[2]
#define FDC_CMD_H		FDC_cmd_file[3]
#define FDC_CMD_S		FDC_cmd_file[4]
#define FDC_CMD_N		FDC_cmd_file[5]
#define FDC_CMD_EOT 	FDC_cmd_file[6]
#define FDC_CMD_GPL 	FDC_cmd_file[7]
#define FDC_CMD_DTL 	FDC_cmd_file[8]

#define FDC_FMT_N		FDC_cmd_file[2]
#define FDC_FMT_SC		FDC_cmd_file[3]
#define FDC_FMT_GPL 	FDC_cmd_file[4]
#define FDC_FMT_D		FDC_cmd_file[5]

static UINT8 FDC_sta_file[16];          /* status file */
static UINT8 *FDC_sta = FDC_sta_file;   /* status pointer */
static UINT8 sta_size[32] = {
	1,1,7,0,1,7,7,0,2,7,7,1,7,7,1,0,
	1,7,1,1,1,1,7,1,1,7,1,1,1,7,1,1
};
#define FDC_ST0 		FDC_sta_file[0]
#define FDC_ST1 		FDC_sta_file[1]
#define FDC_ST2 		FDC_sta_file[2]
#define FDC_STA_C		FDC_sta_file[3] 	/* cylinder  */
#define FDC_STA_H		FDC_sta_file[4] 	/* head 	 */
#define FDC_STA_S		FDC_sta_file[5] 	/* sector	 */
#define FDC_STA_N		FDC_sta_file[6] 	/* track #	 */
#define FDC_ST3 		FDC_sta_file[7]
#define FDC_PCN 		FDC_sta_file[8] 	/* cylinder new */

static void FDC_irq(int num)
{
	FDC_timer = 0;
	pc_PIC_issue_irq(num);
}

#if VERBOSE_FDC
char *dump_FDC_cmd(int n)
{
	static char buff[80];
	char *dst = buff;
	int i;
	dst += sprintf(dst, "[%02x", FDC_cmd_file[0]);
	for (i = 1; i < n; i++)
		dst += sprintf(dst, " %02x", FDC_cmd_file[i]);
	sprintf(dst, "]");
    return buff;
}
char *dump_FDC_sta(int n)
{
	static char buff[80];
	char *dst = buff;
	int i;

	dst += sprintf(dst, "[%02x", FDC_sta_file[0]);
	for (i = 1; i < n; i++)
		dst += sprintf(dst, " %02x", FDC_sta_file[i]);
	sprintf(dst, "]");
    return buff;
}
#endif

/********************************************************************
 *
 * execute a seek command, ie. fseek in disk image
 *
 ********************************************************************/
static void FDC_seek_execute(void)
{
	void *f = pc_fdc_file[drv];
	offset[drv] = (track[drv] * pc_fdc_heads[drv] + head[drv]) * pc_fdc_spt[drv] * pc_fdc_scl[drv] * 256;
	FDC_LOG(1,"FDC_seek_execute",(errorlog, "T:%02d H:%d $%08x\n", track[drv], head[drv], offset[drv]));
	if (f) {
		osd_fseek(f, offset[drv], SEEK_SET);
		FDC_ST0 |= 0x10;	/* seek completed */
	} else {
		FDC_ST0 |= 0x08;	/* not ready */
	}
}

/********************************************************************
 *
 * Format a track according to C/H/S/N info supplied by the PC
 *
 ********************************************************************/
static void FDC_DMA_format(void)
{
	void *f = pc_fdc_file[drv];
	UINT8 data[8192];
	int size = FDC_FMT_SC, write, s;

	head[drv] = (FDC_CMD_UNIT >> 2) & 1;
	pc_fdc_scl[drv] = FDC_FMT_N;
	pc_fdc_spt[drv] = FDC_FMT_SC;	/* sector count */
	pc_gpl[drv] = FDC_FMT_GPL;	/* GAP III */
	pc_fill[drv] = FDC_FMT_D;	/* filler */

    if (pc_DMA_mask & (0x10 << FDC_DMA)) {
		FDC_LOG(1,"FDC_DMA_format",(errorlog, "DMA %d is masked\n", FDC_DMA));
		return;
	}
	if (f) {
		FDC_LOG(1,"FDC_DMA_format",(errorlog, "N:%d SC:%d GPL:%02d D:$%02x $%08x <- $%06x, $%04x\n", FDC_FMT_N, FDC_FMT_SC, FDC_FMT_GPL, FDC_FMT_D, offset[drv], pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA], pc_DMA_count[FDC_DMA]+1));
		s = 0;
        do {
			offset[drv] = ((track[drv] * pc_fdc_heads[drv] + head[drv]) * pc_fdc_spt[drv] + s) * pc_fdc_scl[drv] * 256;

            FDC_STA_C = cpu_readmem20(pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA]);
			pc_DMA_address[FDC_DMA]++;
			if (!--pc_DMA_count[FDC_DMA]) break;

            FDC_STA_H = cpu_readmem20(pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA]);
			pc_DMA_address[FDC_DMA]++;
			if (!--pc_DMA_count[FDC_DMA]) break;

            FDC_STA_S = cpu_readmem20(pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA]);
			pc_DMA_address[FDC_DMA]++;
			if (!--pc_DMA_count[FDC_DMA]) break;

            FDC_STA_N = cpu_readmem20(pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA]);
			pc_DMA_address[FDC_DMA]++;
            if (!--pc_DMA_count[FDC_DMA]) break;

			FDC_LOG(2,"FDC_DMA_format sector",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x <- $%06x, $%04x\n", FDC_STA_C, FDC_STA_H, FDC_STA_S, FDC_STA_N, offset[drv], pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA], pc_DMA_count[FDC_DMA]+1));

            write = (FDC_FMT_N) ? 256 * FDC_FMT_N : 128;
			memset(data, FDC_FMT_D, write);
			osd_fseek(f, offset[drv], SEEK_SET);
			osd_fwrite(f, data, write);
			s++;
		} while (pc_DMA_count[FDC_DMA] && --size);
	}
	pc_DMA_status &= ~(0x10 << FDC_DMA);	/* reset DMA running flag */
	pc_DMA_status |= 0x01 << FDC_DMA;		/* set DMA terminal count flag */
}

/********************************************************************
 *
 * Write a number of sectors from the address set up at DMA chan #2
 *
 ********************************************************************/
static void FDC_DMA_write(int deleted_dam)
{
	void *f = pc_fdc_file[drv];
	UINT8 data[8192], *dst = data;
	int size = FDC_CMD_N * 256;
	int write = 0, multi = 1, first = 1;

	head[drv] = (FDC_CMD_UNIT >> 2) & 1;
	sector[drv] = FDC_CMD_S;  /* get sector */
//	pc_fdc_spt[drv] = FDC_CMD_EOT;	 /* end of track -> sectors per track */

    if (pc_DMA_mask & (0x10 << FDC_DMA)) {
		FDC_LOG(1,"FDC_DMA_write",(errorlog, "DMA %d is masked\n", FDC_DMA));
		return;
	}
	offset[drv] = ((track[drv] * pc_fdc_heads[drv] + head[drv]) * pc_fdc_spt[drv] + (sector[drv] - 1)) * pc_fdc_scl[drv] * 256;
	if (f) {
		FDC_LOG(1,"FDC_DMA_write",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x <- $%06x, $%04x\n", track[drv], head[drv], sector[drv], FDC_CMD_N, offset[drv], pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA], pc_DMA_count[FDC_DMA]+1));
		do {
			if (write == 0) {
				if (!first) {
					if (multi) {
						FDC_LOG(2,"FDC_DMA_write next",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x <- $%06x, $%04x\n", track[drv], head[drv], sector[drv], FDC_CMD_N, offset[drv], pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA], pc_DMA_count[FDC_DMA]+1));
					} else {
						FDC_LOG(2,"FDC_DMA_write next",(errorlog, "C:%02d H:%d S:%02d N:%d; no MT command \n", track[drv], head[drv], sector[drv], FDC_CMD_N));
					}
				}
				write = size;
				dst = data;
				first = 0;
			}
			if (pc_DMA_operation[FDC_DMA] == 2) {
				/* now copy the buffer from PCs memory */
				*dst++ = cpu_readmem20(pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA]);
			} else dst++;
			pc_DMA_address[FDC_DMA] += pc_DMA_direction[FDC_DMA];
			if (--write == 0) {
				osd_fseek(f, offset[drv], SEEK_SET);
                osd_fwrite(f, data, size);
				/* end of track ? */
				if (sector[drv] > pc_fdc_spt[drv]) {
					/* multi track command ? */
					if (FDC_CMD & 0x80) {
						sector[drv] = 1;
						if (++head[drv] > 1)	/* beyond head 1? */
							multi = 0;			/* abort the read */
					} else multi = 0; /* abort the read if MT was not set */
				}
				offset[drv] += size;
				sector[drv]++;
            }
		} while (pc_DMA_count[FDC_DMA]-- && multi);
	}
	pc_DMA_status &= ~(0x10 << FDC_DMA);	/* reset DMA running flag */
	pc_DMA_status |= 0x01 << FDC_DMA;		/* set DMA terminal count flag */
}

/********************************************************************
 *
 * Read a number of sectors to the address set up for DMA chan #2
 *
 ********************************************************************/
static void FDC_DMA_read(int deleted_dam)
{
	void *f = pc_fdc_file[drv];
	UINT8 data[8192], *src = data;
	int size = FDC_CMD_N * 256;
	int read = 0, multi = 1, first = 1;

	head[drv] = (FDC_CMD_UNIT >> 2) & 1;
	sector[drv] = FDC_CMD_S;  /* get sector */
//	pc_fdc_spt[drv] = FDC_CMD_EOT;	 /* end of track -> sectors per track */

    if (pc_DMA_mask & (0x10 << FDC_DMA)) {
		FDC_LOG(1,"FDC_DMA_read",(errorlog, "DMA %d is masked\n", FDC_DMA));
		return;
	}
	offset[drv] = ((track[drv] * pc_fdc_heads[drv] + head[drv]) * pc_fdc_spt[drv] + (sector[drv] - 1)) * pc_fdc_scl[drv] * 256;
	if (f) {
		FDC_LOG(1,"FDC_DMA_read",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x -> $%06x, $%04x\n", 
			track[drv], head[drv], sector[drv], FDC_CMD_N, offset[drv], pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA], pc_DMA_count[FDC_DMA]+1));
		do {
			if (read == 0) {
				osd_fseek(f, offset[drv], SEEK_SET);
				if (!first) {
					if (multi) {
						FDC_LOG(2,"FDC_DMA_read next",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x -> $%06x, $%04x\n", track[drv], head[drv], sector[drv], FDC_CMD_N, offset[drv], pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA], pc_DMA_count[FDC_DMA]+1));
					} else {
						FDC_LOG(2,"FDC_DMA_read next",(errorlog, "C:%02d H:%d S:%02d N:%d; no MT command \n", track[drv], head[drv], sector[drv], FDC_CMD_N));
					}
				}
				read = osd_fread(f, data, size);
				offset[drv] += read;
				src = data;
				first = 0;
				sector[drv]++;
			}
			if (pc_DMA_operation[FDC_DMA] == 1) {
				/* now copy the buffer into PCs memory */
				cpu_writemem20(pc_DMA_page[FDC_DMA] + pc_DMA_address[FDC_DMA], *src++);
			} else src++;
			pc_DMA_address[FDC_DMA] += pc_DMA_direction[FDC_DMA];
			if (--read == 0) {
				/* end of track ? */
				if (sector[drv] > pc_fdc_spt[drv]) {
					/* multi track command ? */
					if (FDC_CMD & 0x80) {
						sector[drv] = 1;
						if (++head[drv] > 1)	/* beyond head 1? */
							multi = 0;			/* abort the read */
					} else multi = 0; /* abort the read if MT was not set */
				}
			}
		} while (pc_DMA_count[FDC_DMA]-- && multi);
	}
	pc_DMA_status &= ~(0x10 << FDC_DMA);	/* reset DMA running flag */
	pc_DMA_status |= 0x01 << FDC_DMA;		/* set DMA terminal count flag */
}

int pc_fdc_status_r(void)
{
	static int old_data = 0x00;
	int data = FDC_main;
	if (data != old_data) {
		old_data = data;
		FDC_LOG(3,"FDC_status_r",(errorlog, "$%02x\n", data));
	}
	return data;
}

void pc_fdc_data_rate_w(int data)
{
	FDC_LOG(1,"FDC_data_rate_w",(errorlog, "$%02x\n", data));
}

int pc_fdc_data_r(void)
{
    int data = 0x00;

    if (FDC_drq) {
		data = *FDC_sta++;
		if (--FDC_drq == 0) {
			FDC_main &= ~0x80;	/* clear DRQ */
			FDC_main &= ~0x40;	/* direction FDC <- PC */
            FDC_main &= ~0x10;  /* clear BUSY */

			FDC_main |= 0x80;	/* set DRQ (accept commands) again */
        }
	}

	FDC_LOG(1,"FDC_data_r",(errorlog, "$%02x\n", data));
    return data;
}

void FDC_command_execute(int arg)
{
	FDC_main |= 0x80;		/* set DRQ */
    FDC_sta = FDC_sta_file;
	FDC_drq = sta_size[FDC_CMD & 0x1f];
    if (FDC_drq) {          /* if the command has a result phase */
		FDC_main |= 0x40;	/* direction FDC -> PC */
		FDC_main |= 0x10;	/* set BUSY */
		/* if a timer is running remove it now */
	} else {
		FDC_main &= ~0x40;	/* direction PC -> FDC */
        FDC_main &= ~0x10;  /* clear BUSY */
    }
	if (FDC_timer) timer_remove(FDC_timer);
	FDC_timer = 0;
	/* and issue the command completed IRQ */
	FDC_irq(6);

	switch (FDC_CMD & 0x1f) {
		case 0x02:	/* read track */
			FDC_LOG(2,"FDC read track",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
			break;

		case 0x03:	/* specify */
			FDC_LOG(2,"FDC specify",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            break;

		case 0x04:	/* sense drive status */
			FDC_LOG(2,"FDC sense drive status",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			/* head + selected drive */
            FDC_ST3 = head[drv] << 2 | (drv & 3);  
			if (FDC_floppy[drv] & TWO_SIDE)
				FDC_ST3 |= 0x08;	/* two sided status signal */
			if (track[drv] == 0)
				FDC_ST3 |= 0x10;	/* track zero status */
			if (FDC_floppy[drv] & READY)
				FDC_ST3 |= 0x20;	/* ready status */
			if (FDC_floppy[drv] & WRITE_PROTECT)
				FDC_ST3 |= 0x40;	/* write protect status */
			if (FDC_floppy[drv] & FAULT)
				FDC_ST3 |= 0x80;	/* fault status */
			FDC_PCN = track[drv];	/* set the cylinder number */
			FDC_sta = &FDC_ST3; 	/* result comes from ST3, PCN */
			break;

		case 0x05:	/* write data */
			FDC_LOG(2,"FDC write data",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
			FDC_ST2 = 0x00;
			FDC_DMA_write(0);
            break;

		case 0x06:	/* read data */
			FDC_LOG(2,"FDC read data",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
			FDC_ST2 = 0x00;
			FDC_DMA_read(0);
			break;

		case 0x07:	/* recalibrate */
			FDC_LOG(2,"FDC recalibrate",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
            track[drv] = 0;       /* set track */
			break;

		case 0x08:	/* sense interrupt status */
			FDC_LOG(2,"FDC sense int status",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST1 = track[drv];
			break;

		case 0x09:	/* write deleted data */
			FDC_LOG(2,"FDC write deleted data",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            FDC_ST2 = 0x00;
            FDC_DMA_write(1);
            break;

		case 0x0a:	/* read id */
			FDC_LOG(2,"FDC read ID",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            FDC_ST2 = 0x00;
            break;

		case 0x0c:	/* read deleted data */
			FDC_LOG(2,"FDC read deleted data",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            FDC_ST2 = 0x00;
            FDC_DMA_read(0);
            break;

		case 0x0d:	/* format track */
			FDC_LOG(2,"FDC format track",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            FDC_ST2 = 0x00;
			FDC_DMA_format();
            break;

		case 0x0f:	/* seek */
			FDC_LOG(2,"FDC seek",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            head[drv] = (FDC_CMD_UNIT >> 2) & 1;
			track[drv] = FDC_CMD_C;  /* get track */
			FDC_seek_execute();
			break;

		case 0x11:	/* scan equal */
			FDC_LOG(2,"FDC scan equal",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            FDC_ST2 |= 0x08;    /* scan satisfied */
			break;

		case 0x16:	/* verify */
			FDC_LOG(2,"FDC verify",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            break;

		case 0x19:	/* scan low or equal */
			FDC_LOG(2,"FDC scan low or equal",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            FDC_ST2 |= 0x08;    /* scan satisfied */
			break;

		case 0x1d:	/* scan high or equal */
			FDC_LOG(2,"FDC scan high or equal",(errorlog, "%s\n", dump_FDC_cmd(cmd_size[FDC_CMD & 0x1f])));
			FDC_ST0 &= ~(0x80 | 0x40);	/* normal command end */
			FDC_ST1 = 0x00;
            FDC_ST2 |= 0x08;    /* scan satisfied */
			break;

	}
	if (!pc_fdc_file[drv]) FDC_ST0 |= 0x08; /* not ready */

	FDC_STA_C = track[drv];
	FDC_STA_H = head[drv];
	FDC_STA_S = sector[drv];
	FDC_STA_N = FDC_CMD_N;
}

void pc_fdc_command_w(int data)
{
	double reaction = 0.0000001;

    if (FDC_drq == 0) {
        FDC_cmd = FDC_cmd_file;     /* reset command file ptr */
		FDC_drq = cmd_size[data & 0x1f];
        switch (data & 0x1f) {
            case 0x02:  /* read track */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: read track\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.0005;
                break;

            case 0x03:  /* specify */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: specify\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.0;
                break;

            case 0x04:  /* sense drive status */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: sense drive status\n", data));
				FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.0000001;
                break;

            case 0x05:  /* write data */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: write data\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.000001;
                break;

            case 0x06:  /* read data */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: read data\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				FDC_ST1 = 0x00;
				FDC_ST2 = 0x00;
				reaction = 0.000001;
                break;

            case 0x07:  /* recalibrate */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: recalibrate\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.000001;
                break;

            case 0x08:  /* sense interrupt status */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: sense int status\n", data));
				reaction = 0.0000001;
                break;

            case 0x09:  /* write deleted data */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: write deleted data\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.000001;
                break;

            case 0x0a:  /* read id */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: read id\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.000001;
                break;

            case 0x0c:  /* read deleted data */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: read deleted data\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.000001;
                break;

            case 0x0d:  /* format track */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: format track\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.001;
                break;

            case 0x0f:  /* seek */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: seek\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.00001;
                break;

            case 0x11:  /* scan equal */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: scan equal\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.000001;
                break;

            case 0x16:  /* verify */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: verify\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.000001;
                break;

            case 0x19:  /* scan low or equal */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: scan low or equal\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.000001;
                break;

            case 0x1d:  /* scan low or equal */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: scan low or equal\n", data));
                FDC_ST0 = 0x00; /* assume successful operation */
				reaction = 0.000001;
                break;

			default:	/* ???? */
				FDC_LOG(1,"FDC_command_w",(errorlog, "$%02x: invalid\n", data));
				FDC_ST0 = 0x80; /* illegal command */
				reaction = 0.0000001;
                break;
        }
	}

	*FDC_cmd++ = data;			/* store command byte */
	FDC_main |= 0x80;			/* set main status DRQ bit */
	FDC_main &= ~0x40;			/* direction PC -> FDC */
	FDC_main &= ~0x10;			/* clear BUSY */
	if (--FDC_drq == 0) {		/* command done now? */
		if (reaction > 0.0) {
			FDC_main &= ~0x80;		/* main status clear DRQ */
			FDC_main |= 0x40;		/* direction FDC -> PC */
			FDC_main |= 0x10;		/* set BUSY */
			/* start a one shot timer to execute the command */
			timer_set(reaction, 0, FDC_command_execute);
		}
	}
}

int pc_fdc_DIR_r(void)
{
	int data = 0x00;
	FDC_LOG(1,"FDC_DIR_r",(errorlog, "$%02x\n", data));
	return data;
}

void pc_fdc_DOR_w(int data)
{
	if (!FDC_enable && (data & 0x04)) {
        /* start a one shot timer to issue an IRQ6 */
		if (FDC_timer) timer_remove(FDC_timer);
		FDC_timer = timer_set(0.001, 6, FDC_irq);
	}

	FDC_motor = (data >> 4) & 0x0f;
    FDC_dma = (data >> 3) & 1;
    FDC_enable = (data >> 2) & 1;
	drv = data & 3;

	FDC_LOG(1,"FDC_DOR_w",(errorlog, "$%02x: drive %d, enable %d, DMA %d\n", data, drv, FDC_enable, FDC_dma));

    if (FDC_enable) {
		if (FDC_dma)
			FDC_main &= ~0x10;	/* reset 'non DMA' flag */
		else
			FDC_main |= 0x10;	/* set 'non DMA' flag */
	} else {
		/* resetting controller */
		FDC_main = 0x80;		/* DRQ, FDC <- FDC */
		memset(FDC_sta_file, 0, sizeof(FDC_sta_file));
		FDC_drq   = 0;
		FDC_cmd   = FDC_cmd_file;
		FDC_sta   = FDC_sta_file;
		FDC_ST0   = 0x80 | 0x40;  /* seek/recal aborted */
		FDC_ST1   = 0x00;
		FDC_ST2   = 0x00;
		FDC_STA_C = 0x00;
		FDC_STA_H = 0x00;
		FDC_STA_S = 0x00;
		FDC_STA_N = 0x00;
		FDC_ST3   = 0x00;
		pc_DMA_status &= ~(0x10 << FDC_DMA);	/* reset DMA running flag */
		pc_DMA_status &= ~(0x01 << FDC_DMA);	/* set DMA terminal count flag */
    }
}
