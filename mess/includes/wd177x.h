/***************************************************************************

  WD177X.c

  Functions to emulate a wd1770, wd1772 or wd1773 floppy disc controller

  LICENSE:
    Copyright help by code author. All rights reserved. Except
    redistribution for non-commercial purposes allowed.

***************************************************************************/

#ifndef WD177X_H
#define WD177X_H

#include "devices/flopdrv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define wd177x_MAX_WD177X 2

/* From the wd179x data sheet:
  Disk Size   Density   Unformatted Track Capacity
   5.25"       Double     6,250
   8"          Double    10,416
*/

#define WD177X_MAXTRACKLENGTH 10416

typedef enum WD177X_chipType
{
	wd177x_wd1770,
	wd177x_wd1772,
	wd177x_wd1773
} WD177X_chipType_t;

/* wd1793 is 100% software compaiible with the wd1773 */
#define wd177x_wd1793 wd177x_wd1773

typedef enum WD177X_mode
{
	WD177X_idle,
	WD177X_type_1,
	WD177X_type_2,
	WD177X_type_3,
	WD177X_type_4
} WD177X_mode_t;

typedef enum WD177X_event
{
	WD177X_IRQ_CLR,
	WD177X_IRQ_SET,
	WD177X_DRQ_CLR,
	WD177X_DRQ_SET
} WD177X_event_t;

typedef enum WD177X_AddressMarkFlag
{
    WD177X_NoMark,
    WD177X_IDAddressMark,
    WD177X_GAPII,
    WD177X_DataAddressMark,
} WD177X_AddressMarkFlag_t;

typedef struct WD177X {
	int	WD177X_index;
	WD177X_chipType_t	chipType;
	DENSITY	density;
	UINT8	trackReg;
        UINT8	sectorReg;
	UINT8	dataReg;
	UINT8	dataShiftReg;
	UINT8	commandReg;
	UINT8	statusReg;
	UINT16	crcReg;
	WD177X_mode_t	mode;
	int	currentDrive;
	int	currentSide;
	int	stepDirection;
	int	trackDirty;
        UINT8	trackData[ WD177X_MAXTRACKLENGTH ];
        unsigned int	trackLength;
        unsigned int	trackPosition;
        WD177X_AddressMarkFlag_t	addressMark;
        unsigned int	addressMarkPosition;
        unsigned int	addressMarkDetected;
        unsigned int	sectorLength;
	void   (* callback)(offs_t WD177X_index, WD177X_event_t event);
	void	*timer_Type1_1,
                *timer_Type1_2,
		*timer_INTRQ;
} WD177X_t;

extern void WD177X_init( offs_t, WD177X_chipType_t, void (*callback)(offs_t, WD177X_event_t) );
extern void WD177X_reset( offs_t);

extern void WD177X_setDrive( offs_t, UINT8 );
extern void WD177X_setSide( offs_t, UINT8 );
extern void WD177X_setDensity( offs_t, DENSITY );


extern WRITE_HANDLER ( WD177X_command_w );
extern WRITE_HANDLER ( WD177X_track_w );
extern WRITE_HANDLER ( WD177X_sector_w );
extern WRITE_HANDLER ( WD177X_data_w );

extern READ_HANDLER ( WD177X_status_r );
extern READ_HANDLER ( WD177X_track_r );
extern READ_HANDLER ( WD177X_sector_r );
extern READ_HANDLER ( WD177X_data_r );

#ifdef __cplusplus
}
#endif

#endif
