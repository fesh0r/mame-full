/***************************************************************************

  WD177X.c

  Functions to emulate a wd1770, wd1772 or wd1773 floppy disc controller

 ADDITIONS:
   date         who   what
   12/26/2003   tjl   Type I commands implemented
   
   tjl = tim lindner (tlindner@ix.netcom.com)
 
 NEEDS TESTING:
   Type I commands

 TODO:
    Type II commands
    Type III commands
    Type IV commands

  LICENSE:
    Copyright help by code author. All rights reserved. Except
    redistribution for non-commercial purposes allowed.

***************************************************************************/

#include "driver.h"
#include "includes/WD177x.h"
#include "utils.h"
#include <assert.h>

#define VERBOSE 1 /* General logging */

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

WD177X_t WD177X_state[ wd177x_MAX_WD177X ];

void WD177X_TypeICommand( WD177X_t *wd );
void WD177X_Type1_Step1( int param );
void WD177X_Type1_Step2( int param );
void WD177X_INTRQ( int param );
void WD177X_DoStep( WD177X_t *wd );
double WD177X_StepRate( WD177X_t *wd );
void WD177X_WriteBackCurrentTrack( WD177X_t *wd );
void WD177X_ReadCurrentTrack( WD177X_t *wd );
void WD177X_ReadByte( WD177X_t *wd );

void WD177X_init( offs_t WD177X_index, WD177X_chipType_t chip_type, void (*callback)(offs_t, WD177X_event_t) )
{
    WD177X_t *wd = &(WD177X_state[ WD177X_index ]);
    
    assert( WD177X_index < wd177x_MAX_WD177X );
    assert( callback != NULL );
    
    memset(wd, 0, sizeof(WD177X_t));
    wd->WD177X_index = WD177X_index;
    wd->chipType = chip_type;
    wd->crcReg = 0xffff;
    wd->mode = WD177X_idle;
    wd->density = DEN_FM_LO;
    wd->stepDirection = 1;
    wd->callback = callback;
    wd->addressMark = WD177X_NoMark;
    wd->timer_Type1_1 = timer_alloc(WD177X_Type1_Step1);
    wd->timer_Type1_2 = timer_alloc(WD177X_Type1_Step2);
    wd->timer_INTRQ = timer_alloc(WD177X_INTRQ);
    
    WD177X_ReadCurrentTrack( wd );
}

void WD177X_reset( offs_t WD177X_index)
{

}

void WD177X_setDrive( offs_t WD177X_index, UINT8 drive)
{
    WD177X_t *wd = &(WD177X_state[ WD177X_index ]);
    
    assert( WD177X_index < wd177x_MAX_WD177X );
    
    if( wd->currentDrive != drive )
    {
        WD177X_WriteBackCurrentTrack( wd );
        wd->currentDrive = drive;
        WD177X_ReadCurrentTrack( wd );
    }

}

void WD177X_setSide( offs_t WD177X_index, UINT8 side )
{
    WD177X_t *wd = &(WD177X_state[ WD177X_index ]);
    
    assert( WD177X_index < wd177x_MAX_WD177X );
    
    if( wd->currentSide != side )
    {
        WD177X_WriteBackCurrentTrack( wd );
        wd->currentSide = side;
        WD177X_ReadCurrentTrack( wd );
    }

}

void WD177X_setDensity( offs_t WD177X_index, DENSITY density )
{
    WD177X_t *wd = &(WD177X_state[ WD177X_index ]);
    
    assert( WD177X_index < wd177x_MAX_WD177X );
    wd->density = density;

}

WRITE_HANDLER ( WD177X_command_w )
{
    WD177X_t *wd = &(WD177X_state[ offset ]);
    
    assert( offset < wd177x_MAX_WD177X );
    LOG(( "wd177x: Command Write: %x, chip #%d", data, offset ));
    
    /* Check for type IV command - todo */
    
    /* Check if busy -- todo */
    
    /* Clear DRQ and INTRQ - todo */
    
    wd->commandReg = data;

    if ( (data & 0x80) == 0x80 ) /* Is Type 1 command? */
        WD177X_TypeICommand( wd );

    /* Check for type II command - todo */
    
    /* Check for type III command - todo */

}

WRITE_HANDLER ( WD177X_track_w )
{
    WD177X_t *wd = &(WD177X_state[ offset ]);
    
    assert( offset < wd177x_MAX_WD177X );
    LOG(( "wd177x: Track Write: %x, chip #%d", data, offset ));

    wd->trackReg = data;
}

WRITE_HANDLER ( WD177X_sector_w )
{
    WD177X_t *wd = &(WD177X_state[ offset ]);
    
    assert( offset < wd177x_MAX_WD177X );
    LOG(( "wd177x: Sector Write: %x, chip #%d", data, offset ));
    
    wd->sectorReg = data;

}

WRITE_HANDLER ( WD177X_data_w )
{
    WD177X_t *wd = &(WD177X_state[ offset ]);
    
    assert( offset < wd177x_MAX_WD177X );
    LOG(( "wd177x: Data Write: %x, chip #%d", data, offset ));
    
    wd->dataReg = data;
    
    /* Clear DRQ - todo */

}

READ_HANDLER ( WD177X_status_r )
{
    WD177X_t *wd = &(WD177X_state[ offset ]);
    
    assert( offset < wd177x_MAX_WD177X );

    LOG(( "wd177x: Status Read: %x, chip #%d", wd->statusReg, offset ));
    return wd->statusReg;
}

READ_HANDLER ( WD177X_track_r )
{
    WD177X_t *wd = &(WD177X_state[ offset ]);
    
    assert( offset < wd177x_MAX_WD177X );

    LOG(( "wd177x: Track Read: %x, chip #%d", wd->trackReg, offset ));
    return wd->trackReg;
}

READ_HANDLER ( WD177X_sector_r )
{
    WD177X_t *wd = &(WD177X_state[ offset ]);
    
    assert( offset < wd177x_MAX_WD177X );

    LOG(( "wd177x: Sector Read: %x, chip #%d", wd->sectorReg, offset ));
    return wd->sectorReg;
}

READ_HANDLER ( WD177X_data_r )
{
    WD177X_t *wd = &(WD177X_state[ offset ]);
    
    assert( offset < wd177x_MAX_WD177X );

    /* Clear DRQ - todo */
    
    LOG(( "wd177x: Data Read: %x, chip #%d", wd->dataReg, offset ));
    return wd->dataReg;
}

void WD177X_TypeICommand( WD177X_t *wd )
{
    double wait = TIME_IN_USEC(5);
    
    wd->mode = WD177X_type_1;
    
    wd->statusReg |= 0x01; /* Set busy */
    wd->statusReg &= ~0x38; /* Reset seek, crc, spin-up/head loaded */
    wd->callback( wd->WD177X_index, WD177X_IRQ_CLR ); /* Reset INTRQ */
    wd->callback( wd->WD177X_index, WD177X_DRQ_CLR ); /* Reset DRQ */
    
    /* Check if spin-up is requested */
    if( (wd->commandReg & 0x08) == 0x08 )
    {
        if( wd->chipType == wd177x_wd1770 || wd->chipType == wd177x_wd1772 )
        {
                /* Wait an additional 6 disk revolutions */
                wait += TIME_IN_HZ(300) * 6;
        }
    }
    
    /* Now pause untill the specified time */
    timer_adjust(wd->timer_Type1_1, wait, (int)wd, 0);
}

void WD177X_Type1_Step1( int param )
{
    double wait = TIME_IN_USEC(5);
    WD177X_t *wd = (WD177X_t *)param;
    
    timer_reset(wd->timer_Type1_1, TIME_NEVER);
    
    if( wd->chipType == wd177x_wd1770 || wd->chipType == wd177x_wd1772 )
        wd->statusReg |= 0x20;	/* Set spin-up flag */
    
    if( (wd->commandReg & 0x60) == 0x40 )	/* Test for step in command */
    {
        wd->stepDirection = 1;
        WD177X_DoStep( wd );
        wait += WD177X_StepRate( wd ); /* Extend wait per Step Rate */
    }
    else if( (wd->commandReg & 0x60) == 0x60 ) /* Test for step out command */
    {
        wd->stepDirection = -1;
        WD177X_DoStep( wd );
        wait += WD177X_StepRate( wd ); /* Extend wait per Step Rate */
    }
    else if( (wd->commandReg & 0x60) == 0x20 ) /* Test for step command */
    {
        WD177X_DoStep( wd );
        wait += WD177X_StepRate( wd ); /* Extend wait per Step Rate */
    }
    else /* command is seek or restore */
    {
        if( (wd->commandReg & 0xf0) == 0 ) /* Test for restore command */
        {
            wd->trackReg = 0xff;
            wd->dataReg = 0;
        }
        
        wd->dataShiftReg = wd->dataReg;
        
        WD177X_WriteBackCurrentTrack( wd );

        do
        {
            if( wd->trackReg == wd->dataShiftReg )
                exit;
            
            if( wd->dataShiftReg > wd->trackReg )
                wd->stepDirection = -1;
            else
                wd->stepDirection = 1;
                    
            floppy_drive_seek(image_from_devtype_and_index(IO_FLOPPY, wd->currentDrive), wd->stepDirection);
            wd->trackReg += wd->stepDirection;
            
            wait += WD177X_StepRate( wd ); /* Extend wait per Step Rate */
            
            if( floppy_drive_get_flag_state(image_from_devtype_and_index(IO_FLOPPY, wd->currentDrive), FLOPPY_DRIVE_HEAD_AT_TRACK_0) )
            {
                if( wd->stepDirection == -1 )
                {
                    wd->trackReg = 0;
                    exit;
                }
            }
        } while( 1 );
        
        WD177X_ReadCurrentTrack( wd );
    }
    
    timer_adjust(wd->timer_Type1_2, wait, (int)wd, 0);
}

void WD177X_Type1_Step2( int param )
{
    double wait = TIME_IN_USEC(5);
    WD177X_t *wd = (WD177X_t *)param;
    int	found_sector = 0;
    UINT16 crc;
    
    timer_reset(wd->timer_Type1_2, TIME_NEVER);

    if( floppy_drive_get_flag_state(image_from_devtype_and_index(IO_FLOPPY, wd->currentDrive), FLOPPY_DRIVE_HEAD_AT_TRACK_0) )
        wd->statusReg |= 0x04;	/* Set Track Zero flag */
            
    if( (wd->commandReg & 0x04) == 0x04 )	/* Test for V bit */
    {
        /* Verify new track */
        int i;
        
        for( i=0; i<wd->trackLength; i++ )
        {
            WD177X_ReadByte( wd );
            
            if( wd->addressMark == WD177X_IDAddressMark )
            {
                /* Found ID Field */
                WD177X_ReadByte( wd ); i++; /* Track number? */
                
                if( wd->trackReg == wd->dataShiftReg )
                {
                    /* Track nuber matches, read thru remaining ID field */
                    WD177X_ReadByte( wd ); i++; /* Side number */
                    WD177X_ReadByte( wd ); i++; /* Sector number */
                    WD177X_ReadByte( wd ); i++; /* Sector length */
                    WD177X_ReadByte( wd ); i++; /* MSB CRC byte */
                    crc = wd->dataShiftReg << 8;
                    WD177X_ReadByte( wd ); i++; /* LSB CRC byte */
                    crc &= wd->dataShiftReg;
                    
                    /* Does CRCs match? */
                    if ( crc == wd->crcReg )
                    {
                        /* Yes */
                        wd->statusReg &= ~0x08; /* Reset CRC error */
                        found_sector = 1;
                        
                        /* Add time it took to find this sector */
                        wait += TIME_IN_HZ(300) * ( (double)i / (double)wd->trackLength );
                        
                        exit;
                    }
                    else
                        wd->statusReg |= 0x08; /* Set CRC error */
                }
            }
        }
        
        if( found_sector == 0 )
        {
            /* Sector not found, add four revloutions to the time */
            wait += TIME_IN_HZ(300) * 5;
        }
        
    }
    
    timer_adjust(wd->timer_INTRQ, wait, (int)wd, 0);
}

void WD177X_INTRQ( int param )
{
    WD177X_t *wd = (WD177X_t *)param;

    timer_reset(wd->timer_INTRQ, TIME_NEVER);
    
    wd->mode = WD177X_idle;
    wd->statusReg &= ~0x01; /* Reset busy */
    wd->callback( wd->WD177X_index, WD177X_IRQ_SET ); /* Set INTRQ */
}

void WD177X_DoStep( WD177X_t *wd )
{
    int	trackReg;
    
    WD177X_WriteBackCurrentTrack( wd );
    floppy_drive_seek(image_from_devtype_and_index(IO_FLOPPY, wd->currentDrive), wd->stepDirection);
    WD177X_ReadCurrentTrack( wd );

    /* Update Track Register if U = 1 */
    if( (wd->commandReg & 0x10) == 0x10 )
    {
        trackReg = wd->trackReg;
        trackReg += wd->stepDirection;
        
        if( trackReg < 0 )
            trackReg = 0;
                
        if( trackReg > 0xff )
            trackReg = 0xff;
                
        wd->trackReg = trackReg;
    }
}

double WD177X_StepRate( WD177X_t *wd )
{
    const int WD1772_stepRates[] = { 6, 12, 2, 3 };
    const int WD177X_stepRates[] = { 6, 12, 20, 30 };
    
    if( wd->chipType == wd177x_wd1772 )
        return TIME_IN_USEC( WD1772_stepRates[ wd->commandReg & 0x03 ] );
    else
        return TIME_IN_USEC( WD177X_stepRates[ wd->commandReg & 0x03 ] );
}
void WD177X_WriteBackCurrentTrack( WD177X_t *wd )
{
    if( wd->trackDirty )
    {
        floppy_drive_write_track_data_info_buffer( image_from_devtype_and_index(IO_FLOPPY, wd->currentDrive), wd->currentSide, wd->trackData, &(wd->trackLength));
        wd->trackDirty = 0;
    }
    
}

void WD177X_ReadCurrentTrack( WD177X_t *wd )
{
    wd->trackLength = WD177X_MAXTRACKLENGTH;
    floppy_drive_read_track_data_info_buffer( image_from_devtype_and_index(IO_FLOPPY, wd->currentDrive), wd->currentSide, wd->trackData, &(wd->trackLength));
    wd->trackDirty = 0;
}

/* Used for pattern matching */

void WD177X_ReadByte( WD177X_t *wd )
{
    /* This implements ID/DATA address mark detection and CRC logic for FM and MFM */

    if( wd->density == DEN_FM_LO ) /* Read byte off disk (Single density) */
    {
        wd->trackPosition += 2;	/* In low density mode: read every other byte */
    
        /* Wrap around if necessary */
        if( wd->trackPosition > wd->trackLength )
            wd->trackPosition = 0;
        
        wd->dataShiftReg = wd->trackData[ wd->trackPosition ];
        
        /* Process gap 2 */
        if( wd->addressMarkDetected > 0 && wd->addressMark == WD177X_GAPII )
        {
            wd->addressMarkDetected--;
            if( wd->addressMarkDetected == 0 )
                wd->addressMark = WD177X_NoMark;
        }
        
        /* Process data inside the address mark */
        if( wd->addressMarkDetected > 0 && wd->addressMark != WD177X_GAPII)
        {
            if( wd->addressMarkDetected > 2 ) /* Don't CRC the CRC bytes */
                wd->crcReg = ccitt_crc16_one( wd->crcReg, wd->dataShiftReg );
            
            wd->addressMarkDetected--;
            
            /*Save sector length in internal register */
            if( wd->addressMark == WD177X_IDAddressMark && wd->addressMarkDetected == 2 )
                wd->sectorLength = wd->dataShiftReg;
            
            if( wd->addressMarkDetected == 0 ) /* Are we done with the data fields? */
            {
                if( wd->addressMark == WD177X_IDAddressMark)
                {
                    wd->addressMark = WD177X_GAPII; /*Yes, done with Index data fields */
                    wd->addressMarkDetected = 30;  /* Switch two GAP II */
                }
                else
                    wd->addressMark = WD177X_NoMark; /* Yes, done with data field */
            }
        }
        /* Looing for ID Address mark */
        else if( wd->addressMark == WD177X_NoMark && wd->addressMarkPosition > 2 && wd->dataShiftReg == 0xfe )
        {
            wd->addressMark = WD177X_IDAddressMark;
            wd->addressMarkDetected = 6;

            wd->crcReg = ccitt_crc16_one( 0xffff, wd->dataShiftReg );
            wd->addressMarkPosition = 0;
        }
        /* Looking for Data Address mark */
        else if( wd->addressMark == WD177X_GAPII && wd->addressMarkPosition > 2 && ( wd->dataShiftReg > 0xf7 && wd->dataShiftReg < 0xfc ) )
        {
            /* We found a data address mark */

            wd->addressMark = WD177X_DataAddressMark;

            wd->addressMarkDetected = (128 << wd->sectorLength) + 2;

            wd->crcReg = ccitt_crc16_one( 0xffff, wd->dataShiftReg );
            wd->addressMarkPosition = 0;
        }
        /* Synching to address mark */
        else if( wd->dataShiftReg == 0x00 )
            wd->addressMarkPosition++;
        /* Reset sync */
        else
            wd->addressMarkPosition = 0;
    }
    else   /* Read byte off disk (double density) */
    {
        wd->trackPosition++;

        if( wd->trackPosition > wd->trackLength )
            wd->trackPosition = 0;
    
        wd->dataShiftReg = wd->trackData[ wd->trackPosition ];
        
        /* Process gap 2 */
        if( wd->addressMarkDetected > 0 && wd->addressMark == WD177X_GAPII )
        {
            wd->addressMarkDetected--;
            if( wd->addressMarkDetected == 0 )
                wd->addressMark = WD177X_NoMark;
        }
        
        /* Process data inside the address mark */
        if( wd->addressMarkDetected > 0 && wd->addressMark != WD177X_GAPII)
        {
            if( wd->addressMarkDetected > 2 ) /* Don't CRC the CRC bytes */
                wd->crcReg = ccitt_crc16_one( wd->crcReg, wd->dataShiftReg );
            
            wd->addressMarkDetected--;
            
            /*Save sector length in internal register */
            if( wd->addressMark == WD177X_IDAddressMark && wd->addressMarkDetected == 2 )
                wd->sectorLength = wd->dataShiftReg;
            
            if( wd->addressMarkDetected == 0 )
            {
                if( wd->addressMark == WD177X_IDAddressMark)
                {
                    wd->addressMark = WD177X_GAPII; /* We are now done with the address mark */
                    wd->addressMarkDetected = 43;
                }
                else
                    wd->addressMark = WD177X_NoMark;
            }
        }
        /* Looing for ID Address mark */
        else if( wd->addressMark == WD177X_NoMark && wd->addressMarkPosition > 2 && wd->dataShiftReg == 0xfe )
        {
            wd->addressMark = WD177X_IDAddressMark;
            wd->addressMarkDetected = 6;

            wd->crcReg = ccitt_crc16_one( 0xffff, 0xa1 );
            wd->crcReg = ccitt_crc16_one( wd->crcReg, 0xa1 );
            wd->crcReg = ccitt_crc16_one( wd->crcReg, 0xa1 );
            wd->crcReg = ccitt_crc16_one( wd->crcReg, wd->dataShiftReg );
            wd->addressMarkPosition = 0;
        }
        /* Looking for Data Address mark */
        else if( wd->addressMark == WD177X_GAPII && wd->addressMarkPosition > 2 && wd->dataShiftReg > 0xf7 )
        {
            /* We found a data address mark */
            wd->addressMark = WD177X_DataAddressMark;

            wd->addressMarkDetected = (128 << wd->sectorLength) + 2;

            wd->crcReg = ccitt_crc16_one( 0xffff, 0xa1 );
            wd->crcReg = ccitt_crc16_one( wd->crcReg, 0xa1 );
            wd->crcReg = ccitt_crc16_one( wd->crcReg, 0xa1 );
            wd->crcReg = ccitt_crc16_one( wd->crcReg, wd->dataShiftReg );
            wd->addressMarkPosition = 0;
        }
        /* Synching to address mark */
        else if( wd->dataShiftReg == 0xa1 )
            wd->addressMarkPosition++;
        /* Reset sync */
        else
            wd->addressMarkPosition = 0;
    }
}
