/***************************************************************************

	machine/pc_ide.c

	Functions to emulate a IDE hard disk controller
	Not (currently) used, since an XT does not have IDE support in BIOS :(

***************************************************************************/
#include "includes/pc_ide.h"

static int drv = 0; 					/* 0 master, 1 slave drive */
static int lba[2] = {0,0};				/* 0 CHS mode, 1 LBA mode */
static int cylinder[2] = {0,0}; 		/* current cylinder (lba = 0) */
static int head[2] = {0,0}; 			/* current head (lba = 0) */
static int sector[2] = {0,0};			/* current sector (or LBA if lba = 1) */
static int sector_cnt[2] = {0,0};		/* sector count */
static int error[2] = {0,0};			/* error code */
static int status[2] = {0,0};			/* drive status */

static int data_cnt = 0;                /* data count */
static UINT8 *buffer = 0;				/* data buffer */
static UINT8 *ptr = 0;					/* data pointer */

void pc_ide_data_w(int data)
{
	if (data_cnt) {
		*ptr++ = data;
		if (--data_cnt == 0) {
		}
	}
}

int pc_ide_data_r(void)
{
	int data = 0xff;

	if (data_cnt) {
		data = *ptr++;
		if (--data_cnt == 0) {
		}
	}
	return data;
}

void pc_ide_write_precomp_w(int data)
{

}

/*
 * ---diagnostic mode errors---
 * 7   which drive failed (0 = master, 1 = slave)
 * 6-3 reserved
 * 2-0 error code
 *	   001 no error detected
 *	   010 formatter device error
 *	   011 sector buffer error
 *	   100 ECC circuitry error
 *	   101 controlling microprocessor error
 * ---operation mode---
 * 7   bad block detected
 * 6   uncorrectable ECC error
 * 5   reserved
 * 4   ID found
 * 3   reserved
 * 2   command aborted prematurely
 * 1   track 000 not found
 * 0   DAM not found (always 0 for CP-3022)
 */
int pc_ide_error_r(void)
{
	int data = error[drv];
	return data;
}

void pc_ide_sector_count_w(int data)
{
	sector_cnt[drv] = data;
}

int pc_ide_sector_count_r(void)
{
	int data = sector_cnt[drv];
	return data;
}

void pc_ide_sector_number_w(int data)
{
	if (lba[drv])
		sector[drv] = (sector[drv] & 0xfffff00) | (data & 0xff);
	else
		sector[drv] = data;
}

int pc_ide_sector_number_r(void)
{
	int data = sector[drv] & 0xff;
	return data;
}

void pc_ide_cylinder_number_l_w(int data)
{
	if (lba[drv])
		sector[drv] = (sector[drv] & 0xfff00ff) | ((data & 0xff) << 8);
	else
		cylinder[drv] = (cylinder[drv] & 0xff00) | (data & 0xff);
}

int pc_ide_cylinder_number_l_r(void)
{
	int data;
    if (lba[drv])
		data = (sector[drv] >> 8) & 0xff;
	else
        data = cylinder[drv] & 0xff;
	return data;
}

void pc_ide_cylinder_number_h_w(int data)
{
	if (lba[drv])
		sector[drv] = (sector[drv] & 0xf00ffff) | ((data & 0xff) << 16);
	else
        cylinder[drv] = (cylinder[drv] & 0x00ff) | ((data & 0xff) << 8);
}

int pc_ide_cylinder_number_h_r(void)
{
	int data;
    if (lba[drv])
		data = (sector[drv] >> 16) & 0xff;
	else
		data = (cylinder[drv] >> 8) & 0xff;
	return data;
}

void pc_ide_drive_head_w(int data)
{
	drv = (data >> 4) & 1;
    lba[drv] = (data >> 6) & 1;
	if (lba[drv])
		sector[drv] = (sector[drv] & 0x0ffffff) | ((data & 0x0f) << 24);
	else
		head[drv] = data & 0x0f;
}

int pc_ide_drive_head_r(void)
{
	int data;
	if (lba[drv])
		data = 0xe0 | (drv << 4) | ((sector[drv] >> 24) & 0x0f);
	else
		data = 0xa0 | (drv << 4) | head[drv];
	return data;
}

void pc_ide_command_w(int data)
{
	switch (data) {
		case 0x00:
			/* nop */
			break;
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: case 0x19: case 0x1a: case 0x1b:
		case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			/* recalibrate */
			cylinder[drv] = 0;
            break;
		case 0x20: case 0x21:
			/* read sectors (with or w/o retry) */
			ptr = buffer;
			data_cnt = sector_cnt[drv] * 512;
			break;
		case 0x22: case 0x23:
			/* read long (with or w/o retry) */
            break;
		case 0x30: case 0x31:
            /* write sectors (with or w/o retry) */
			ptr = buffer;
			data_cnt = sector_cnt[drv] * 512;
			break;
		case 0x32: case 0x33:
			/* write long (with or w/o retry) */
            break;
		case 0x40: case 0x41:
			/* read verify sectors (with or w/o retry) */
            break;
		case 0x50:
			/* format track */
			break;
		case 0x70: case 0x71: case 0x72: case 0x73:
		case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7a: case 0x7b:
		case 0x7c: case 0x7d: case 0x7e: case 0x7f:
			/* seek */
            break;
		case 0x90:
			/* execute diagnostics */
            break;
		case 0x91:
			/* initialize drive parameters */
            break;
	}
}

/*
 * Bit(s) Description
 * 7	  controller is executing a command
 * 6	  drive is ready
 * 5	  write fault
 * 4	  seek complete
 * 3	  sector buffer requires servicing
 * 2	  disk data read successfully corrected
 * 1	  index - set to 1 each disk revolution
 * 0	  previous command ended in an error
 */
int pc_ide_status_r(void)
{
	int data = status[drv];
	return data;
}


/*************************************************************************
 *
 *		ATHD
 *		AT hard disk
 *
 *************************************************************************/
WRITE_HANDLER(at_mfm_0_w)
{
	switch (offset) {
	case 0: pc_ide_data_w(data);				break;
	case 1: pc_ide_write_precomp_w(data);		break;
	case 2: pc_ide_sector_count_w(data);		break;
	case 3: pc_ide_sector_number_w(data);		break;
	case 4: pc_ide_cylinder_number_l_w(data);	break;
	case 5: pc_ide_cylinder_number_h_w(data);	break;
	case 6: pc_ide_drive_head_w(data);			break;
	case 7: pc_ide_command_w(data); 			break;
	}
}

READ_HANDLER(at_mfm_0_r)
{
	int data=0;

	switch (offset) {
	case 0: data = pc_ide_data_r(); 			break;
	case 1: data = pc_ide_error_r();			break;
	case 2: data = pc_ide_sector_count_r(); 	break;
	case 3: data = pc_ide_sector_number_r();	break;
	case 4: data = pc_ide_cylinder_number_l_r();break;
	case 5: data = pc_ide_cylinder_number_h_r();break;
	case 6: data = pc_ide_drive_head_r();		break;
	case 7: data = pc_ide_status_r();			break;
	}
	return data;
}

