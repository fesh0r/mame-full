/*
    SCSIBus.h

    Implementation of a raw SCSI/SASI bus for machines that don't use a SCSI
    controler chip such as the RM Nimbus, which implements it as a bunch of
    74LS series chips.

*/

#ifndef _SCSIBUS_H_
#define _SCSIBUS_H_

#include "emu.h"
#include "machine/scsi.h"


/***************************************************************************
    MACROS
***************************************************************************/

DECLARE_LEGACY_DEVICE(SCSIBUS, scsibus);

#define MCFG_SCSIBUS_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, SCSIBUS, 0) \
	MCFG_DEVICE_CONFIG(_intrf)


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define SCSI_LINE_SEL   0
#define SCSI_LINE_BSY   1
#define SCSI_LINE_REQ   2
#define SCSI_LINE_ACK   3
#define SCSI_LINE_CD    4
#define SCSI_LINE_IO    5
#define SCSI_LINE_MSG   6
#define SCSI_LINE_RESET 7

// Perhaps thse should be in scsi.h ?
#define SCSI_PHASE_BUS_FREE 8
#define SCSI_PHASE_SELECT   9

#define REQ_DELAY_NS    90
#define ACK_DELAY_NS    90
#define BSY_DELAY_NS	50

#define CMD_BUF_SIZE    			32
#define ADAPTEC_BUF_SIZE			1024

#define SCSI_CMD_TEST_READY			0x00
#define SCSI_CMD_RECALIBRATE		0x01
#define SCSI_CMD_REQUEST_SENSE		0x03
#define SCSI_CMD_FORMAT_UNIT    	0x04
#define SCSI_CMD_CHECK_TRACK_FORMAT	0x05
#define SCSI_CMD_INIT_DRIVE_PARAMS	0x0C
#define SCSI_CMD_FORMAT_ALT_TRACK	0x0E
#define SCSI_CMD_WRITE_SEC_BUFFER	0x0F
#define SCSI_CMD_READ_SEC_BUFFER	0x10
#define SCSI_COMMAND_INQUIRY    	0x12
#define SCSI_CMD_WRITE_DATA_BUFFER	0x13
#define SCSI_CMD_READ_DATA_BUFFER	0x14
#define SCSI_CMD_MODE_SELECT    	0x15
#define SCSI_CMD_SEND_DIAGNOSTIC	0x1D
#define SCSI_CMD_READ_CAPACITY  	0x25
#define SCSI_CMD_SEARCH_DATA_EQUAL	0x31
#define SCSI_CMD_READ_DEFECT    	0x37
#define SCSI_CMD_BUFFER_WRITE   	0x3B
#define SCSI_CMD_BUFFER_READ    	0x3C

// Xebec SASI
#define SCSI_CMD_RAM_DIAGS			0xE0
#define SCSI_CMD_DRIVE_DIAGS		0xE3
#define SCSI_CMD_CONTROLER_DIAGS	0xE4

// Commodore D9060/9090 SASI
#define SCSI_CMD_PHYSICAL_DEVICE_ID	0xc0

#define RW_BUFFER_HEAD_BYTES    	0x04

#define ADAPTEC_DATA_BUFFER_SIZE	0x0400
#define XEBEC_SECTOR_BUFFER_SIZE	0x0200

#define XEBEC_PARAMS_SIZE			0x08
#define XEBEC_ALT_TRACK_SIZE		0x03

#define IS_COMMAND(cmd)         	(bus->command[0]==cmd)
#define IS_READ_COMMAND()       	((bus->command[0]==0x08) || (bus->command[0]==0x28) || (bus->command[0]==0xa8))
#define IS_WRITE_COMMAND()      	((bus->command[0]==0x0a) || (bus->command[0]==0x2a))
#define SET_STATUS_SENSE(stat,sen)	{ bus->status=(stat); bus->sense=(sen); }

#define FORMAT_UNIT_TIMEOUT			5

//
// Status / Sense data taken from Adaptec ACB40x0 documentation.
//

#define SCSI_STATUS_OK				0x00
#define SCSI_STATUS_CHECK			0x02
#define SCSI_STATUS_EQUAL			0x04
#define SCSI_STATUS_BUSY			0x08

#define SCSI_SENSE_ADDR_VALID		0x80
#define SCSI_SENSE_NO_SENSE			0x00
#define SCSI_SENSE_NO_INDEX			0x01
#define SCSI_SENSE_SEEK_NOT_COMP	0x02
#define SCSI_SENSE_WRITE_FAULT		0x03
#define SCSI_SENSE_DRIVE_NOT_READY	0x04
#define SCSI_SENSE_NO_TRACK0		0x06
#define SCSI_SENSE_ID_CRC_ERROR		0x10
#define SCSI_SENSE_UNCORRECTABLE	0x11
#define SCSI_SENSE_ADDRESS_NF		0x12
#define SCSI_SENSE_RECORD_NOT_FOUND	0x14
#define SCSI_SENSE_SEEK_ERROR		0x15
#define SCSI_SENSE_DATA_CHECK_RETRY	0x18
#define SCSI_SENSE_ECC_VERIFY		0x19
#define SCSI_SENSE_INTERLEAVE_ERROR	0x1A
#define SCSI_SENSE_UNFORMATTED		0x1C
#define SCSI_SENSE_ILLEGAL_COMMAND	0x20
#define SCSI_SENSE_ILLEGAL_ADDRESS	0x21
#define SCSI_SENSE_VOLUME_OVERFLOW	0x23
#define SCSI_SENSE_BAD_ARGUMENT		0x24
#define SCSI_SENSE_INVALID_LUN		0x25
#define SCSI_SENSE_CART_CHANGED		0x28
#define SCSI_SENSE_ERROR_OVERFLOW	0x2C

#define SCSI_SENSE_SIZE				4

typedef struct
{
	// parameter list
	UINT8		reserved1[3];
	UINT8		length;

	// descriptor list
	UINT8		density;
	UINT8		reserved2[4];
	UINT8		block_size[3];

	// drive parameter list
	UINT8		format_code;
	UINT8		cylinder_count[2];
	UINT8		head_count;
	UINT8		reduced_write[2];
	UINT8		write_precomp[2];
	UINT8		landing_zone;
	UINT8		step_pulse_code;
	UINT8		bit_flags;
	UINT8		sectors_per_track;
} adaptec_sense_t;

/***************************************************************************
    INTERFACE
***************************************************************************/

typedef struct _SCSIBus_interface SCSIBus_interface;
struct _SCSIBus_interface
{
    const SCSIConfigTable *scsidevs;		/* SCSI devices */
    void (*line_change_cb)(device_t *device, UINT8 line, UINT8 state);

	devcb_write_line out_bsy_func;
	devcb_write_line out_sel_func;
	devcb_write_line out_cd_func;
	devcb_write_line out_io_func;
	devcb_write_line out_msg_func;
	devcb_write_line out_req_func;
	devcb_write_line out_rst_func;
};

/* SCSI Bus read/write */

UINT8 scsi_data_r(device_t *device);
void scsi_data_w(device_t *device, UINT8 data);

READ8_DEVICE_HANDLER( scsi_data_r );
WRITE8_DEVICE_HANDLER( scsi_data_w );

/* Get/Set lines */

UINT8 get_scsi_lines(device_t *device);
UINT8 get_scsi_line(device_t *device, UINT8 lineno);
void set_scsi_line(device_t *device, UINT8 line, UINT8 state);

READ_LINE_DEVICE_HANDLER( scsi_bsy_r );
READ_LINE_DEVICE_HANDLER( scsi_sel_r );
READ_LINE_DEVICE_HANDLER( scsi_cd_r );
READ_LINE_DEVICE_HANDLER( scsi_io_r );
READ_LINE_DEVICE_HANDLER( scsi_msg_r );
READ_LINE_DEVICE_HANDLER( scsi_req_r );
READ_LINE_DEVICE_HANDLER( scsi_ack_r );
READ_LINE_DEVICE_HANDLER( scsi_rst_r );

WRITE_LINE_DEVICE_HANDLER( scsi_bsy_w );
WRITE_LINE_DEVICE_HANDLER( scsi_sel_w );
WRITE_LINE_DEVICE_HANDLER( scsi_cd_w );
WRITE_LINE_DEVICE_HANDLER( scsi_io_w );
WRITE_LINE_DEVICE_HANDLER( scsi_msg_w );
WRITE_LINE_DEVICE_HANDLER( scsi_req_w );
WRITE_LINE_DEVICE_HANDLER( scsi_ack_w );
WRITE_LINE_DEVICE_HANDLER( scsi_rst_w );

/* Get current bus phase */

UINT8 get_scsi_phase(device_t *device);

/* utility functions */

/* get a drive's number from it's select line */
UINT8 scsibus_driveno(UINT8  drivesel);

/* get the number of bytes for a scsi command */
int get_scsi_cmd_len(int cbyte);

/* Initialisation at machine reset time */
void init_scsibus(device_t *device, int sectorbytes);

#endif
