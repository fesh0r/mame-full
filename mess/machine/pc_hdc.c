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

#include <stdio.h>
#include "snprintf.h"

#include "mscommon.h"
#include "includes/pic8259.h"
#include "machine/8237dma.h"
#include "machine/pc_hdc.h"
#include "devices/harddriv.h"

#define VERBOSE_HDC 0		/* HDC (hard disk controller) */

#if VERBOSE_HDC
#define HDC_LOG(n,m,a) LOG(VERBOSE_HDC,n,m,a)
#else
#define HDC_LOG(n,m,a)
#endif


#define MAX_HARD	2
#define MAX_BOARD	2				/* two boards supported */
#define HDC_DMA 	3				/* DMA channel */

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

/* indexes */
static int cylinder[MAX_HARD] = {0,};			/* current cylinder */
static int head[MAX_HARD] = {0,};				/* current head */
static int sector[MAX_HARD] = {0,}; 			/* current sector */
static int sector_cnt[MAX_HARD] = {0,};         /* sector count */
static int control[MAX_HARD] = {0,};            /* control */
static int lbasector[MAX_HARD] = {0,};          /* offset into image file */

static int csb[MAX_BOARD];				/* command status byte */
static int status[MAX_BOARD];			/* drive status */
static int error[MAX_BOARD]; 			/* error code */
static int dip[MAX_BOARD];				/* dip switches */
static mame_timer *timer[MAX_BOARD];


static int data_cnt = 0;                /* data count */
static UINT8 *buffer;					/* data buffer */
static UINT8 *ptr = 0;					/* data pointer */


static void pc_hdc_command(int n);

int pc_hdc_setup(void)
{
	int i;

	buffer = auto_malloc(17*4*512);
	if (!buffer)
		return -1;

	/* init for all boards */
	for (i = 0; i < MAX_BOARD; i++)
	{
		csb[i] = 0;
		status[i] = 0;
		error[i] = 0;
		dip[i] = 0xff;
		timer[i] = timer_alloc(pc_hdc_command);
		if (!timer[i])
			return -1;
	}
	return 0;
}

static struct hard_disk_file *pc_hdc_file(int id)
{
	mess_image *img;
	
	img = image_from_devtype_and_index(IO_HARDDISK, id);
	if (!image_exists(img))
		return NULL;

	return mess_hd_get_hard_disk_file(img);
}

static void pc_hdc_result(int n)
{
	/* dip switch selected INT 5 or 2 */
	pic8259_0_issue_irq((dip[n] & 0x40) ? 5 : 2);

	HDC_LOG(1,"hdc_result",("$%02x to $%04x\n", csb[n], data_cnt));
	buffer[data_cnt++] = csb[n];

	if (csb[n] & CSB_ERROR) {
        buffer[data_cnt++] = error[n];
		if (error[n] & 0x80) {
			buffer[data_cnt++] = (drv << 5) | head[idx];
			buffer[data_cnt++] = ((cylinder[idx] >> 2) & 0xc0) | sector[idx];
			buffer[data_cnt++] = cylinder[idx] & 0xff;
			HDC_LOG(1,"hdc_result",("result [%02x %02x %02x %02x]\n",
				buffer[data_cnt-4],buffer[data_cnt-3],buffer[data_cnt-2],buffer[data_cnt-1]));
		} else {
			HDC_LOG(1,"hdc_result",("result [%02x]\n", buffer[data_cnt-1]));
        }
    }
	status[n] |= STA_INTERRUPT | STA_INPUT | STA_REQUEST | STA_COMMAND | STA_READY;
}



static int no_dma(void)
{
	return dma8237_0_r(10) & (0x10 << HDC_DMA);
}



/********************************************************************
 *
 * Read a number of sectors to the address set up for DMA chan #3
 *
 ********************************************************************/

/* the following crap is an abomination; it is a relic of the old crappy DMA
 * implementation that threw the idea of "emulating the hardware" to the wind
 */
static data8_t *hdcdma_data;
static data8_t *hdcdma_src;
static data8_t *hdcdma_dst;
static int hdcdma_read;
static int hdcdma_write;
static int hdcdma_size;

int pc_hdc_dack_r(void)
{
	data8_t result;
	struct hard_disk_info *info;
	struct hard_disk_file *file;
	
	file = pc_hdc_file(idx);
	if (!file)
		return 0;
	info = hard_disk_get_info(file);

	if (hdcdma_read == 0)
	{
		hard_disk_read(file, lbasector[idx], 1, hdcdma_data);
		hdcdma_read = 512;
		hdcdma_size -= 512;
		lbasector[idx]++;
		hdcdma_src = hdcdma_data;
		sector[idx]++;
	}

	result = *(hdcdma_src++);

	if( --hdcdma_read == 0 )
	{
		/* end of cylinder ? */
		if (sector[idx] >= info->sectors)
		{
			sector[idx] = 0;
			if (++head[idx] >= info->heads)		/* beyond heads? */
			{
				head[idx] = 0;					/* reset head */
				cylinder[idx]++;				/* next cylinder */
            }
        }
	}

	return result;
}



void pc_hdc_dack_w(int data)
{
	struct hard_disk_info *info;
	struct hard_disk_file *file;
	
	file = pc_hdc_file(idx);
	if (!file)
		return;
	info = hard_disk_get_info(file);	

	*(hdcdma_dst++) = data;

	if( --hdcdma_write == 0 )
	{
		hard_disk_write(file, lbasector[idx], 1, hdcdma_data);
		hdcdma_write = 512;
		hdcdma_size -= 512;
		lbasector[idx]++;

        /* end of cylinder ? */
		if( ++sector[idx] >= info->sectors )
		{
			sector[idx] = 0;
			if (++head[idx] >= info->heads)		/* beyond heads? */
			{
				head[idx] = 0;					/* reset head */
				cylinder[idx]++;				/* next cylinder */
            }
        }
        hdcdma_dst = hdcdma_data;
    }
}



static void execute_read(void)
{
	struct hard_disk_file *disk = pc_hdc_file(idx);
	UINT8 data[512], *src = data;
	int size = sector_cnt[idx] * 512;
	int read_ = 0;

	disk = pc_hdc_file(idx);
	if (!disk)
		return;

	hdcdma_data = data;
	hdcdma_src = src;
	hdcdma_read = read_;
	hdcdma_size = size;

	if (no_dma())
	{
		do
		{
			buffer[data_cnt++] = pc_hdc_dack_r();
		} while (hdcdma_read || hdcdma_size);
	}
	else
	{
		dma8237_run_transfer(0, HDC_DMA);
	}
}



static void execute_write(void)
{
	struct hard_disk_file *disk = pc_hdc_file(idx);
	UINT8 data[512], *dst = data;
	int size = sector_cnt[idx] * 512;
	int write_ = 512;

	disk = pc_hdc_file(idx);
	if (!disk)
		return;

	hdcdma_data = data;
	hdcdma_dst = dst;
	hdcdma_write = write_;
	hdcdma_size = size;

	if (no_dma())
	{
		do
		{
			pc_hdc_dack_w(buffer[data_cnt++]);
		} while (hdcdma_write || hdcdma_size);
	}
	else
	{
		dma8237_run_transfer(0, HDC_DMA);
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

	lbasector[idx] = ((cylinder[idx] * heads[idx] + head[idx]) * spt[idx] + sector[idx]);
	error[n] = 0x80;	/* a potential error has C/H/S/N info */
}

static int test_ready(int n)
{
	if( !pc_hdc_file(idx) )
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

	csb[n] = 0x00;
	error[n] = 0;

	ptr = buffer;
	cmd = buffer[0];

	switch (cmd)
	{
		case CMD_TESTREADY:
			get_drive(n);
			HDC_LOG(1,"hdc test ready",("INDEX #%d D:%d\n", idx, drv));
			test_ready(n);
            break;
		case CMD_RECALIBRATE:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc recalibrate",("INDEX #%d D:%d CTL:$%02x\n", idx, drv, control[idx]));
/*			test_ready(n); */
            break;
		case CMD_SENSE:
			get_drive(n);
			HDC_LOG(1,"hdc sense",("INDEX #%d D:%d\n", idx, drv));
			test_ready(n);
            break;
		case CMD_FORMATDRV:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc format drive",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
		case CMD_VERIFY:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc verify",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
		case CMD_FORMATTRK:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc format track",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
		case CMD_FORMATBAD:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc format track (bad)",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
            break;
		case CMD_READ:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc read",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			if (test_ready(n))
				execute_read();
/*			{ extern int debug_key_pressed; debug_key_pressed = 1; } */
			break;
		case CMD_WRITE:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc write",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			if (test_ready(n))
				execute_write();
			break;
		case CMD_SEEK:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc seek",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
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
			HDC_LOG(1,"hdc set param",("INDEX #%d D:%d C:%d H:%d RW:%d WP:%d ECC:%d\n",
				idx, drv, cylinders[idx], heads[idx], rwc[idx], wp[idx], ecc[idx]));
			break;
		case CMD_GETECC:
			HDC_LOG(1,"hdc get ECC",("controller #%d\n", n));
			buffer[data_cnt++] = ecc[idx];
			break;
		case CMD_READSBUFF:
			HDC_LOG(1,"hdc read sector buffer",("controller #%d\n", n));
			break;
		case CMD_WRITESBUFF:
			HDC_LOG(1,"hdc write sector buffer",("controller #%d\n", n));
			break;
		case CMD_RAMDIAG:
			HDC_LOG(1,"hdc RAM diag",("controller #%d", n));
			break;
		case CMD_DRIVEDIAG:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc drive diag",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			test_ready(n);
			break;
		case CMD_INTERNDIAG:
			HDC_LOG(1,"hdc internal diag",("BOARD #%d\n", n));
			break;
		case CMD_READLONG:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc read long",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
				idx, drv, cylinder[idx], head[idx], sector[idx], sector_cnt[idx], control[idx]));
			if (test_ready(n))
				execute_read();
			break;
		case CMD_WRITELONG:
			get_drive(n);
			get_chsn(n);
			HDC_LOG(1,"hdc write long",("INDEX #%d D:%d C:%d H:%d S:%d N:%d CTL:$%02x\n",
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
static void pc_hdc_data_w(int n, int data)
{
	if( data_cnt == 0 )
	{
		HDC_LOG(2,"hdc_data_w",("BOARD #%d $%02x: ", n, data));
        ptr = buffer;
		data_cnt = 6;	/* expect 6 bytes including this one */
		status[n] &= ~STA_READY;
		status[n] &= ~STA_INPUT;
		switch (data)
		{
			case CMD_TESTREADY:
				HDC_LOG(2,0,("test ready\n"));
				break;
			case CMD_RECALIBRATE:
				HDC_LOG(2,0,("recalibrate\n"));
                break;
			case CMD_SENSE:
				HDC_LOG(2,0,("sense\n"));
                break;
			case CMD_FORMATDRV:
				HDC_LOG(2,0,("format drive\n"));
                break;
			case CMD_VERIFY:
				HDC_LOG(2,0,("verify\n"));
                break;
			case CMD_FORMATTRK:
				HDC_LOG(2,0,("format track\n"));
                break;
			case CMD_FORMATBAD:
				HDC_LOG(2,0,("format track (bad)\n"));
                break;
			case CMD_READ:
				HDC_LOG(2,0,("read\n"));
                break;
			case CMD_WRITE:
				HDC_LOG(2,0,("write\n"));
                break;
			case CMD_SEEK:
				HDC_LOG(2,0,("seek\n"));
                break;
			case CMD_SETPARAM:
				HDC_LOG(2,0,("set param\n"));
				data_cnt += 8;
				break;
			case CMD_GETECC:
				HDC_LOG(2,0,("get ECC\n"));
                break;
            case CMD_READSBUFF:
				HDC_LOG(2,0,("read sector buffer\n"));
                break;
			case CMD_WRITESBUFF:
				HDC_LOG(2,0,("write sector buffer\n"));
                break;
			case CMD_RAMDIAG:
				HDC_LOG(2,0,("RAM diag\n"));
                break;
            case CMD_DRIVEDIAG:
				HDC_LOG(2,0,("drive diag\n"));
                break;
			case CMD_INTERNDIAG:
				HDC_LOG(2,0,("internal diag\n"));
                break;
			case CMD_READLONG:
				HDC_LOG(2,0,("read long\n"));
                break;
			case CMD_WRITELONG:
				HDC_LOG(2,0,("write long\n"));
                break;
            default:
				HDC_LOG(2,0,("unknown command!\n"));
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
		HDC_LOG(3,"hdc_data_w",("BOARD #%d $%02x\n", n, data));
		*ptr++ = data;
		status[n] |= STA_READY;
		if( --data_cnt == 0 )
		{
			if( buffer[0] == CMD_SETPARAM )
			{
				HDC_LOG(2,"hdc_command",("[%02x %02x %02x %02x %02x %02x] [%02x %02x %02x %02x %02x %02x %02x %02x]\n",
					buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5],
					buffer[6],buffer[7],buffer[8],buffer[9],buffer[10],buffer[11],buffer[12],buffer[13]));
			}
			else
			{
				HDC_LOG(2,"hdc_command",("[%02x %02x %02x %02x %02x %02x]\n",
					buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]));
			}
            status[n] &= ~STA_COMMAND;
			status[n] &= ~STA_REQUEST;
			status[n] &= ~STA_READY;
			status[n] |= STA_INPUT;
			
			assert(timer[n]);
			timer_adjust(timer[n], 0.001, n, 0);
        }
	}
}



static void pc_hdc_reset_w(int n, int data)
{
	HDC_LOG(1,"hdc_reset_w",("INDEX #%d $%02x\n", n, data));
	cylinder[n] = 0;
	head[n] = 0;
	sector[n] = 0;
	csb[n] = 0;
	status[n] = STA_COMMAND | STA_READY;
	memset(buffer, 0, sizeof(buffer));
	ptr = buffer;
	data_cnt = 0;
}



static void pc_hdc_select_w(int n, int data)
{
	HDC_LOG(3,"hdc_select_w",("BOARD #%d $%02x\n", n, data));
	status[n] &= ~STA_INTERRUPT;
	status[n] |= STA_SELECT;
}



static void pc_hdc_control_w(int n, int data)
{
	HDC_LOG(3,"hdc_control_w",("BOARD #%d $%02x\n", n, data));
}



static int  pc_hdc_data_r(int n)
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
	HDC_LOG(3,"hdc_data_r",("BOARD #%d $%02x [$%04x]\n", n, data, (int)(ptr-buffer)-1));
    return data;
}



static int  pc_hdc_status_r(int n)
{
	static int old_stat = 0;
	int data = status[n];
	if( old_stat != data )
	{
		old_stat = data;
		HDC_LOG(4,"hdc_status_r",("BOARD #%d $%02x: RDY:%d INP:%d CMD:%d SEL:%d REQ:%d INT:%d\n",
			n, data, data&1, (data>>1)&1, (data>>2)&1, (data>>3)&1, (data>>4)&1, (data>>5)&1));
	}
	return data;
}



static int  pc_hdc_dipswitch_r(int n)
{
	int data = dip[n];
	HDC_LOG(4,"hdc_dipswitch_r",("BOARD #%d $%02x\n", n, data));
	return data;
}



/*************************************************************************
 *
 *		HDC
 *		hard disk controller
 *
 *************************************************************************/

static int pc_HDC_r(int chip, int offs)
{
	int data = 0xff;
	if( !(input_port_3_r(0) & (0x08>>chip)) || !pc_hdc_file(chip<<1) )
		return data;
	switch( offs )
	{
		case 0: data = pc_hdc_data_r(chip); 	 break;
		case 1: data = pc_hdc_status_r(chip);	 break;
		case 2: data = pc_hdc_dipswitch_r(chip); break;
		case 3: break;
	}
	return data;
}



static void pc_HDC_w(int chip, int offs, int data)
{
	if( !(input_port_3_r(0) & (0x08>>chip)) || !pc_hdc_file(chip<<1) )
		return;

	switch( offs )
	{
		case 0: pc_hdc_data_w(chip,data);	 break;
		case 1: pc_hdc_reset_w(chip,data);	 break;
		case 2: pc_hdc_select_w(chip,data);  break;
		case 3: pc_hdc_control_w(chip,data); break;
	}
}


/*************************************
 *
 *		port handlers.
 *
 *************************************/

READ_HANDLER ( pc_HDC1_r ) { return pc_HDC_r(0, offset); }
READ_HANDLER ( pc_HDC2_r ) { return pc_HDC_r(1, offset); }

WRITE_HANDLER ( pc_HDC1_w ) { pc_HDC_w(0, offset, data); }
WRITE_HANDLER ( pc_HDC2_w ) { pc_HDC_w(1, offset, data); }
