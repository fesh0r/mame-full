/***************************************************************************

    machine/pc_hdc.c

	Functions to emulate a WD1004A-WXI hard disk controller

	Information was gathered from various places:
	Linux' /usr/src/linux/drivers/block/xd.c and /usr/include/linux/xd.h
	Original IBM PC-XT technical data book
	The WD1004A-WXI BIOS ROM image

    TODO:
	Still very much :)
	The format of the so called 'POD' (power on drive paramters?) area
	read from the MBR (master boot record) at offset 1AD to 1BD is wrong.

***************************************************************************/
#include "mess/machine/pc.h"

#define MAX_BOARD	2				/* two boards supported */
#define MAX_HARD	4				/* up to four had disks */
#define HDC_DMA 	3				/* DMA channel */

void *pc_hdc_file[MAX_HARD];        /* up to four hard disk images */

#define CMD_TESTREADY   0x00
#define CMD_RECALIBRATE 0x01
#define CMD_SENSE		0x03
#define CMD_FORMATDRV	0x04
#define CMD_VERIFY		0x05
#define CMD_FORMATTRK	0x06
#define CMD_FORMATBAD	0x07
#define CMD_READ		0x08
#define CMD_WRITE		0x0a
#define CMD_SEEK		0x0b

#define CMD_SETPARAM	0x0c
#define CMD_GETECC		0x0d

#define CMD_READSBUFF	0x0e
#define CMD_WRITESBUFF	0x0f

#define CMD_RAMDIAG 	0xe0
#define CMD_DRIVEDIAG   0xe3
#define CMD_INTERNDIAG	0xe4
#define CMD_READLONG	0xe5
#define CMD_WRITELONG	0xe6

/* Bits for command status byte */
#define CSB_ERROR       0x02
#define CSB_LUN 		0x20

/* XT hard disk controller status bits */
#define STA_READY		0x01
#define STA_INPUT		0x02
#define STA_COMMAND 	0x04
#define STA_SELECT		0x08
#define STA_REQUEST 	0x10
#define STA_INTERRUPT	0x20

/* XT hard disk controller control bits */
#define CTL_PIO 		0x00
#define CTL_DMA 		0x03

static int idx = 0; 							/* contoller * 2 + drive */
static int drv = 0; 							/* 0 master, 1 slave drive */
static int cylinders[MAX_HARD] = {612,612,};    /* number of cylinders */
static int rwc[MAX_HARD] = {613,613,};			/* recduced write current from cyl */
static int wp[MAX_HARD] = {613,613,};			/* write precompensation from cyl */
static int heads[MAX_HARD] = {4,4,};			/* heads */
static int spt[MAX_HARD] = {17,17,};			/* sectors per track */
static int ecc[MAX_HARD] = {11,11,};			/* ECC bytes */

static int cylinder[MAX_HARD] = {0,};			/* current cylinder */
static int head[MAX_HARD] = {0,};				/* current head */
static int sector[MAX_HARD] = {0,}; 			/* current sector */
static int sector_cnt[MAX_HARD] = {0,};         /* sector count */
static int control[MAX_HARD] = {0,};            /* control */
static int offset[MAX_HARD] = {0,};             /* offset into image file */

static int csb[MAX_BOARD] = {0,};				/* command status byte */
static int status[MAX_BOARD] = {0,};			/* drive status */
static int error[MAX_BOARD] = {0,}; 			/* error code */
static int dip[MAX_BOARD] = {0xff,0xff,};		/* dip switches */
static void *timer[MAX_BOARD] = {0,};


static int data_cnt = 0;                /* data count */
static UINT8 buffer[17*4*512];			/* data buffer */
static UINT8 *ptr = 0;					/* data pointer */

static void pc_hdc_result(int n)
{
	/* dip switch selected INT 5 or 2 */
	pc_PIC_issue_irq((dip[n] & 0x40) ? 5 : 2);

	HDC_LOG(1,"hdc_result",(errorlog, "$%02x to $%04x\n", csb[n], data_cnt));
	buffer[data_cnt++] = csb[n];

	if (csb[n] & CSB_ERROR) {
        buffer[data_cnt++] = error[n];
		if (error[n] & 0x80) {
			buffer[data_cnt++] = (drv << 5) | head[idx];
			buffer[data_cnt++] = ((cylinder[idx] >> 2) & 0xc0) | sector[idx];
			buffer[data_cnt++] = cylinder[idx] & 0xff;
			HDC_LOG(1,"hdc_result",(errorlog, "result [%02x %02x %02x %02x]\n",
				buffer[data_cnt-4],buffer[data_cnt-3],buffer[data_cnt-2],buffer[data_cnt-1]));
		} else {
			HDC_LOG(1,"hdc_result",(errorlog, "result [%02x]\n", buffer[data_cnt-1]));
        }
    }
	status[n] |= STA_INTERRUPT | STA_INPUT | STA_REQUEST | STA_COMMAND | STA_READY;
}

/********************************************************************
 *
 * Read a number of sectors to the address set up for DMA chan #3
 *
 ********************************************************************/

static void execute_read(void)
{
	void *f = pc_hdc_file[idx];
	UINT8 data[512], *src = data;
	int size = sector_cnt[idx] * 512;
	int read = 0, first = 1;
	int no_dma = pc_DMA_mask & (0x10 << HDC_DMA);

	if (f)
	{
		if (no_dma)
		{
			HDC_LOG(1,"hdc_PIO_read",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x, $%04x\n", cylinder[idx], head[idx], sector[idx], sector_cnt[idx], offset[idx], size));
			do
			{
				if( read == 0 )
				{
					osd_fseek(f, offset[idx], SEEK_SET);
					if( !first )
					{
						HDC_LOG(2,"hdc_PIO_read next",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x, $%04x\n",
							cylinder[idx], head[idx], sector[idx], sector_cnt[idx], offset[idx], size));
					}
					read = osd_fread(f, data, 512);
                    size -= 512;
					offset[idx] += read;
					src = data;
					first = 0;
					sector[idx]++;
				}
				/* copy data into the result buffer */
				buffer[data_cnt++] = *src++;
				if( --read == 0 )
				{
					/* end of cylinder ? */
					if( sector[idx] >= spt[idx] )
					{
						sector[idx] = 0;
						if (++head[idx] >= heads[idx])		/* beyond heads? */
						{
							head[idx] = 0;					/* reset head */
							cylinder[idx]++;				/* next cylinder */
						}
					}
				}
			} while (read || size);
		}
		else
		{
			HDC_LOG(1,"hdc_DMA_read",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x -> $%06x, $%04x\n", cylinder[idx], head[idx], sector[idx], sector_cnt[idx], offset[idx], pc_DMA_page[HDC_DMA] + pc_DMA_address[HDC_DMA], pc_DMA_count[HDC_DMA]+1));
			do
			{
				if (read == 0)
				{
					osd_fseek(f, offset[idx], SEEK_SET);
					if (!first)
					{
						HDC_LOG(2,"hdc_DMA_read next",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x -> $%06x, $%04x\n",
							cylinder[idx], head[idx], sector[idx], sector_cnt[idx], offset[idx],
							pc_DMA_page[HDC_DMA] + pc_DMA_address[HDC_DMA], pc_DMA_count[HDC_DMA]+1));
					}
					read = osd_fread(f, data, 512);
                    size -= 512;
					offset[idx] += read;
					src = data;
					first = 0;
					sector[idx]++;
				}
				if( pc_DMA_operation[HDC_DMA] == 1 )
				{
					/* now copy the buffer into PCs memory */
					cpu_writemem20(pc_DMA_page[HDC_DMA] + pc_DMA_address[HDC_DMA], *src++);
				}
				else
					src++;
				pc_DMA_address[HDC_DMA] += pc_DMA_direction[HDC_DMA];
				if( --read == 0 )
				{
					/* end of cylinder ? */
					if (sector[idx] >= spt[idx])
					{
						sector[idx] = 0;
						if (++head[idx] >= heads[idx])		/* beyond heads? */
						{
							head[idx] = 0;					/* reset head */
							cylinder[idx]++;				/* next cylinder */
                        }
                    }
				}
			} while( pc_DMA_count[HDC_DMA]-- );
			pc_DMA_status &= ~(0x10 << HDC_DMA);	/* reset DMA running flag */
			pc_DMA_status |= 0x01 << HDC_DMA;		/* set DMA terminal count flag */
		}
	}
}

static void execute_write(void)
{
	void *f = pc_hdc_file[idx];
	UINT8 data[512], *dst = data;
	int size = sector_cnt[idx] * 512;
	int write = 512, first = 1;
	int no_dma = pc_DMA_mask & (0x10 << HDC_DMA);

	if (f)
	{
		if (no_dma)
		{
			HDC_LOG(1,"hdc_PIO_write",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x, $%04x\n", cylinder[idx], head[idx], sector[idx], sector_cnt[idx], offset[idx], size));
			do
			{
				/* copy data into the result buffer */
				*dst++ = buffer[data_cnt++];
				if( --write == 0 )
				{
					osd_fseek(f, offset[idx], SEEK_SET);
					write = osd_fwrite(f, data, 512);
                    offset[idx] += write;
					size -= write;
					if (size <= 0) write = 0;
					dst = data;
                    /* end of cylinder ? */
					if( ++sector[idx] >= spt[idx] )
					{
						sector[idx] = 0;
						if( ++head[idx] >= heads[idx] ) 	/* beyond heads? */
						{
							head[idx] = 0;					/* reset head */
							cylinder[idx]++;				/* next cylinder */
                        }
                    }
				}
			} while (write || size);
		}
		else
		{
			HDC_LOG(1,"hdc_DMA_write",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x -> $%06x, $%04x\n", cylinder[idx], head[idx], sector[idx], sector_cnt[idx], offset[idx], pc_DMA_page[HDC_DMA] + pc_DMA_address[HDC_DMA], pc_DMA_count[HDC_DMA]+1));
			do
			{
				if( pc_DMA_operation[HDC_DMA] == 2 )
				{
					/* now copy the buffer into PCs memory */
					*dst++ = cpu_readmem20(pc_DMA_page[HDC_DMA] + pc_DMA_address[HDC_DMA]);
				};
				pc_DMA_address[HDC_DMA] += pc_DMA_direction[HDC_DMA];
				if( --write == 0 )
				{
					osd_fseek(f, offset[idx], SEEK_SET);
					write = osd_fwrite(f, data, 512);
                    size -= 512;
					offset[idx] += write;
                    /* end of cylinder ? */
					if( ++sector[idx] >= spt[idx] )
					{
						sector[idx] = 0;
						if (++head[idx] >= heads[idx])		/* beyond heads? */
						{
							head[idx] = 0;					/* reset head */
							cylinder[idx]++;				/* next cylinder */
                        }
                    }
					if( !first )
					{
						HDC_LOG(2,"hdc_DMA_write next",(errorlog, "C:%02d H:%d S:%02d N:%d $%08x -> $%06x, $%04x\n",
							cylinder[idx], head[idx], sector[idx], sector_cnt[idx], offset[idx],
							pc_DMA_page[HDC_DMA] + pc_DMA_address[HDC_DMA], pc_DMA_count[HDC_DMA]+1));
                    }
                    dst = data;
                    first = 0;
                }
			} while( pc_DMA_count[HDC_DMA]-- );
			pc_DMA_status &= ~(0x10 << HDC_DMA);	/* reset DMA running flag */
			pc_DMA_status |= 0x01 << HDC_DMA;		/* set DMA terminal count flag */
		}
	}
}

static void get_drive(int n)
{
	drv = (buffer[1] >> 5) & 1;
	csb[n] = (drv) ? CSB_LUN : 0x00;
    idx = n * 2 + drv;
}

static void get_chsn(int n)
{
	head[idx] = buffer[1] & 0x1f;
    sector[idx] = buffer[2] & 0x3f;
    cylinder[idx] = (buffer[2] & 0xc0) << 2;
    cylinder[idx] |= buffer[3];
    sector_cnt[idx] = buffer[4];
    control[idx] = buffer[5];   /* 7: no retry, 6: no ecc retry, 210: step rate */
    offset[idx] = ((cylinder[idx] * heads[idx] + head[idx]) * spt[idx] + sector[idx]) * 512;
	error[n] = 0x80;	/* a potential error has C/H/S/N info */
}

static int test_ready(int n)
{
	if( !pc_hdc_file[idx] )
	{
		csb[n] |= CSB_ERROR;
		error[n] |= 0x04;	/* drive not ready */
		return 0;
	}
	return 1;
}

static void pc_hdc_command(int n)
{
	UINT8 cmd;

	timer[n] = 0;
	csb[n] = 0x00;
	error[n] = 0;

    ptr = buffer;
    cmd = buffer[0];

	switch (cmd)
	{
		case CMD_TESTREADY:
			get_drive(n);
			HDC_LOG(1,"hdc test ready",(errorlog,"INDEX #%d D:%d\n", idx, drv));
			test_ready(n);
            break;
		case CMD_RECALIBRATE:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc recalibrate",(errorlog,"INDEX #%d D:%d CTL:$%02x\n", idx, drv, control[idx]));
//			test_ready(n);
            break;
		case CMD_SENSE:
			get_drive(n);
			HDC_LOG(1,"hdc sense",(errorlog,"INDEX #%d D:%d\n", idx, drv));
			test_ready(n);
            break;
		case CMD_FORMATDRV:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc format drive",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
		case CMD_VERIFY:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc verify",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
		case CMD_FORMATTRK:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc format track",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
		case CMD_FORMATBAD:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc format track (bad)",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
		case CMD_READ:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc read",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			if (test_ready(n))
				execute_read();
//			{ extern int debug_key_pressed; debug_key_pressed = 1; }
            break;
		case CMD_WRITE:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc write",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			if (test_ready(n))
				execute_write();
            break;
		case CMD_SEEK:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc seek",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
		case CMD_SETPARAM:
			get_drive(n);
			get_chsn(n);
			cylinders[idx] = ((buffer[6]&3)<<8) | buffer[7];
			heads[idx] = buffer[8] & 0x1f;
			rwc[idx] = ((buffer[9]&3)<<8) | buffer[10];
			wp[idx] = ((buffer[11]&3)<<8) | buffer[12];
			ecc[idx] = buffer[13];
			HDC_LOG(1,"hdc set param",(errorlog,"INDEX #%d D:%d C:%d H:%d RW:%d WP:%d ECC:%d\n",
				idx, drv, cylinders[idx], heads[idx], rwc[idx], wp[idx], ecc[idx]));
#if NEVERDEF
			if (pc_hdc_file[idx])
			{
                /* write the drive geometry to the image */
				buffer[ 0] = cylinders[idx]&0xff;		/* cylinders lsb */
				buffer[ 1] = (cylinders[idx]>>8)&3; 	/* cylinders msb */
				buffer[ 2] = heads[idx];				/* heads */
				buffer[ 3] = rwc[idx]&0xff; 			/* reduced write current lsb */
				buffer[ 4] = (rwc[idx]>>8)&3;			/* reduced write current msb */
				buffer[ 5] = wp[idx]&0xff;				/* write precompensation lsb */
				buffer[ 6] = (wp[idx]>>8)&3;			/* write precompensation msb */
				buffer[ 7] = ecc[idx];					/* ECC length */
				buffer[ 8] = 0x05;						/* control */
				buffer[11] = spt[idx];					/* no idea what this is, maybe SPT ? */
                buffer[12] = 0x00;
                buffer[11] = cylinders[idx]&0xff;       /* parking cylinder lsb */
				buffer[12] = (cylinders[idx]>>8)&3; 	/* parking cylinder msb */
				buffer[13] = 0x00;
				buffer[14] = 0x00;
				buffer[15] = 0x00;
				buffer[16] = dip[idx];					/* a non zero value is expected */
				osd_fseek(pc_hdc_file[idx], 0x1ad, SEEK_SET);
				osd_fwrite(pc_hdc_file[idx], buffer, 16);
            }
#endif
            break;
		case CMD_GETECC:
			HDC_LOG(1,"hdc get ECC",(errorlog,"controller #%d\n", n));
			buffer[data_cnt++] = ecc[idx];
            break;
		case CMD_READSBUFF:
			HDC_LOG(1,"hdc read sector buffer",(errorlog,"controller #%d\n", n));
            break;
		case CMD_WRITESBUFF:
			HDC_LOG(1,"hdc write sector buffer",(errorlog,"controller #%d\n", n));
            break;
		case CMD_RAMDIAG:
			HDC_LOG(1,"hdc RAM diag",(errorlog,"controller #%d", n));
            break;
        case CMD_DRIVEDIAG:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc drive diag",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
        case CMD_INTERNDIAG:
			HDC_LOG(1,"hdc internal diag",(errorlog,"BOARD #%d\n", n));
            break;
        case CMD_READLONG:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc read long",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			if (test_ready(n))
				execute_read();
            break;
        case CMD_WRITELONG:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc write long",(errorlog,"INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			if (test_ready(n))
				execute_write();
            break;
    }
	pc_hdc_result(n);
}


/*  Command format
 *	Bits	 Description
 *	7	   0
 *	xxxxxxxx command
 *	dddhhhhh drive / head
 *	ccssssss cylinder h / sector
 *	cccccccc cylinder l
 *	nnnnnnnn count
 *	xxxxxxxx control
 *
 *	Command format extra for set drive characteristics
 *	000000cc cylinders h
 *	cccccccc cylinders l
 *	000hhhhh heads
 *	000000cc reduced write h
 *	cccccccc reduced write l
 *	000000cc write precomp h
 *	cccccccc write precomp l
 *	eeeeeeee ecc
 */
void pc_hdc_data_w(int n, int data)
{
	if( data_cnt == 0 )
	{
		HDC_LOG(2,"hdc_data_w",(errorlog,"BOARD #%d $%02x: ", n, data));
        ptr = buffer;
		data_cnt = 6;	/* expect 6 bytes including this one */
		status[n] &= ~STA_READY;
		status[n] &= ~STA_INPUT;
		switch (data)
		{
			case CMD_TESTREADY:
				HDC_LOG(2,0,(errorlog,"test ready\n"));
				break;
			case CMD_RECALIBRATE:
				HDC_LOG(2,0,(errorlog,"recalibrate\n"));
                break;
			case CMD_SENSE:
				HDC_LOG(2,0,(errorlog,"sense\n"));
                break;
			case CMD_FORMATDRV:
				HDC_LOG(2,0,(errorlog,"format drive\n"));
                break;
			case CMD_VERIFY:
				HDC_LOG(2,0,(errorlog,"verify\n"));
                break;
			case CMD_FORMATTRK:
				HDC_LOG(2,0,(errorlog,"format track\n"));
                break;
			case CMD_FORMATBAD:
				HDC_LOG(2,0,(errorlog,"format track (bad)\n"));
                break;
			case CMD_READ:
				HDC_LOG(2,0,(errorlog,"read\n"));
                break;
			case CMD_WRITE:
				HDC_LOG(2,0,(errorlog,"write\n"));
                break;
			case CMD_SEEK:
				HDC_LOG(2,0,(errorlog,"seek\n"));
                break;
			case CMD_SETPARAM:
				HDC_LOG(2,0,(errorlog,"set param\n"));
				data_cnt += 8;
				break;
			case CMD_GETECC:
				HDC_LOG(2,0,(errorlog,"get ECC\n"));
                break;
            case CMD_READSBUFF:
				HDC_LOG(2,0,(errorlog,"read sector buffer\n"));
                break;
			case CMD_WRITESBUFF:
				HDC_LOG(2,0,(errorlog,"write sector buffer\n"));
                break;
			case CMD_RAMDIAG:
				HDC_LOG(2,0,(errorlog,"RAM diag\n"));
                break;
            case CMD_DRIVEDIAG:
				HDC_LOG(2,0,(errorlog,"drive diag\n"));
                break;
			case CMD_INTERNDIAG:
				HDC_LOG(2,0,(errorlog,"internal diag\n"));
                break;
			case CMD_READLONG:
				HDC_LOG(2,0,(errorlog,"read long\n"));
                break;
			case CMD_WRITELONG:
				HDC_LOG(2,0,(errorlog,"write long\n"));
                break;
            default:
				HDC_LOG(2,0,(errorlog,"unknown command!\n"));
				data_cnt = 0;
				status[n] |= STA_INPUT;
				csb[n] |= CSB_ERROR | 0x20; /* unknown command */
				pc_hdc_result(n);
		}
		if( data_cnt )
			status[n] |= STA_REQUEST;
	}

	if (data_cnt)
	{
		HDC_LOG(3,"hdc_data_w",(errorlog,"BOARD #%d $%02x\n", n, data));
        *ptr++ = data;
		status[n] |= STA_READY;
		if( --data_cnt == 0 )
		{
			if( buffer[0] == CMD_SETPARAM )
			{
				HDC_LOG(2,"hdc_command",(errorlog,"[%02x %02x %02x %02x %02x %02x] [%02x %02x %02x %02x %02x %02x %02x %02x]\n",
					buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5],
					buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11],buffer[12],buffer[13]));
			}
			else
			{
				HDC_LOG(2,"hdc_command",(errorlog,"[%02x %02x %02x %02x %02x %02x]\n",
					buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]));
			}
            status[n] &= ~STA_COMMAND;
			status[n] &= ~STA_REQUEST;
			status[n] &= ~STA_READY;
			status[n] |= STA_INPUT;
			if( timer[n] )
				timer_remove(timer[n]);
			timer[n] = timer_set(0.001, n, pc_hdc_command);
        }
	}
}

void pc_hdc_reset_w(int n, int data)
{
	HDC_LOG(1,"hdc_reset_w",(errorlog,"INDEX #%d $%02x\n", n, data));
	cylinder[n] = 0;
	head[n] = 0;
	sector[n] = 0;
	csb[n] = 0;
	status[n] = STA_COMMAND | STA_READY;
	memset(buffer, 0, sizeof(buffer));
    ptr = buffer;
	data_cnt = 0;
}

void pc_hdc_select_w(int n, int data)
{
	HDC_LOG(3,"hdc_select_w",(errorlog,"BOARD #%d $%02x\n", n, data));
	status[n] &= ~STA_INTERRUPT;
	status[n] |= STA_SELECT;
}

void pc_hdc_control_w(int n, int data)
{
	HDC_LOG(3,"hdc_control_w",(errorlog,"BOARD #%d $%02x\n", n, data));
}

int  pc_hdc_data_r(int n)
{
	int data = 0xff;
	if( data_cnt )
	{
		data = *ptr++;
		status[n] &= ~STA_INTERRUPT;
		if( --data_cnt == 0 )
		{
			status[n] &= ~STA_INPUT;
			status[n] &= ~STA_REQUEST;
			status[n] &= ~STA_SELECT;
			status[n] |= STA_COMMAND;
		}
	}
	HDC_LOG(3,"hdc_data_r",(errorlog,"BOARD #%d $%02x [$%04x]\n", n, data, (int)(ptr-buffer)-1));
    return data;
}

int  pc_hdc_status_r(int n)
{
	static int old_stat = 0;
	int data = status[n];
	if( old_stat != data )
	{
		old_stat = data;
		HDC_LOG(4,"hdc_status_r",(errorlog,"BOARD #%d $%02x: RDY:%d INP:%d CMD:%d SEL:%d REQ:%d INT:%d\n",
			n, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1, (data>>4)&1, (data>>5)&1));
	}
    return data;
}

int  pc_hdc_dipswitch_r(int n)
{
	int data = dip[n];
	HDC_LOG(4,"hdc_dipswitch_r",(errorlog,"BOARD #%d $%02x\n", n, data));
    return data;
}


