/***************************************************************************

  wd177x.c

  Functions to emulate a wd1770, wd1772 or wd1773 floppy disc controller

  LICENSE:
    Copyright held by code author. All rights reserved. Except
    redistribution for non-commercial purposes allowed.

***************************************************************************/

#ifndef wd177x_H
#define wd177x_H

#include "devices/flopdrv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* I would like to have more than one wd177x aaiable but MESS' floppy disk
   system will need to be enhanced to support more than one FDC. */
   
#define wd177x_MAX_wd177x 1

/* From the wd179x data sheet:
  Disk Size   Density   Unformatted Track Capacity
   5.25"       Double     6,250 bytes
   8"          Double    10,416 bytes
*/

#define wd177x_MAXTRACKLENGTH 10416

typedef enum wd177x_chip_type
{
	wd177x_wd1770,
	wd177x_wd1772,
	wd177x_wd1773
} wd177x_chip_type_t;

/* Here are some defines to specify 100% software compatibility */
#define wd177x_wd1793 wd177x_wd1773
#define wd177x_wd2793 wd177x_wd1773

typedef enum wd177x_mode
{
	wd177x_type_1,
	wd177x_type_2,
	wd177x_type_3,
	wd177x_type_4
} wd177x_mode_t;

typedef enum wd177x_event
{
	wd177x_IRQ_CLR,
	wd177x_IRQ_SET,
	wd177x_DRQ_CLR,
	wd177x_DRQ_SET
} wd177x_event_t;

typedef enum wd177x_Address_mark_flag
{
    wd177x_NoMark,
    wd177x_IDAddressMark,
    wd177x_GAPII,
    wd177x_DataAddressMark,
} wd177x_AddressMarkFlag_t;

typedef struct wd177x {
	int	wd177x_index;
	wd177x_chip_type_t	chip_type;
	DENSITY	density;
	UINT8	track_reg;
        UINT8	sector_reg;
	UINT8	data_reg;
	UINT8	data_shift_reg;
	UINT8	command_reg;
	UINT8	status_reg;
	UINT16	crc_reg;
	wd177x_mode_t	mode;
        int	ready;
	int	current_drive;
	int	current_side;
	int	step_direction;
	int	track_dirty;
        UINT8	track_data[ wd177x_MAXTRACKLENGTH ];
        unsigned int	track_length;
        unsigned int	track_position;
        wd177x_AddressMarkFlag_t	address_mark;
        unsigned int	address_mark_position;
        unsigned int	address_mark_detected;
        unsigned int	sector_length;
        unsigned int	motor_on;
	void   (*callback)(offs_t wd177x_index, wd177x_event_t event);
	void	*timer_Type1_1,
                *timer_Type1_2,
		*timer_INTRQ,
                *timer_index_pulse_reset;
} wd177x_t;

extern void wd177x_init( offs_t, wd177x_chip_type_t, void (*callback)(offs_t, wd177x_event_t) );
extern void wd177x_reset( offs_t);

extern void wd177x_set_drive( offs_t, UINT8 );
extern void wd177x_set_side( offs_t, UINT8 );
extern void wd177x_set_density( offs_t, DENSITY );


extern WRITE_HANDLER ( wd177x_command_w );
extern WRITE_HANDLER ( wd177x_track_w );
extern WRITE_HANDLER ( wd177x_sector_w );
extern WRITE_HANDLER ( wd177x_data_w );

extern READ_HANDLER ( wd177x_status_r );
extern READ_HANDLER ( wd177x_track_r );
extern READ_HANDLER ( wd177x_sector_r );
extern READ_HANDLER ( wd177x_data_r );

#ifdef __cplusplus
}
#endif

#endif
