/***************************************************************************

  wd177x.c

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
    Copyright held by code author. All rights reserved. Except
    redistribution for non-commercial purposes allowed.

***************************************************************************/

#include "driver.h"
#include "includes/wd177x.h"
#include "utils.h"
#include <assert.h>

#define VERBOSE 1 /* General logging */

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

wd177x_t wd177x_state[ wd177x_MAX_wd177x ];

void wd177x_type_1_command( wd177x_t *wd );
void wd177x_type_1_step_1( int param );
void wd177x_type_1_step_2( int param );
void wd177x_intrq( int param );
void wd177x_step_read_write_head( wd177x_t *wd );
double wd177x_get_step_rate( wd177x_t *wd );
void wd177x_write_back_current_track( wd177x_t *wd );
void wd177x_read_current_track( wd177x_t *wd );
void wd177x_read_byte_from_track( wd177x_t *wd );
void wd177x_index_pulse_callback(mess_image *img);
void wd177x_timer_index_pulse_reset( int param );
void wd177x_ready_state_change( mess_image *img, int state );

void wd177x_init( offs_t wd177x_index, wd177x_chip_type_t chip_type, void (*callback)(offs_t, wd177x_event_t) )
{
    wd177x_t *wd = &(wd177x_state[ wd177x_index ]);
    
    assert( wd177x_index < wd177x_MAX_wd177x );
    assert( callback != NULL );
    
    memset(wd, 0, sizeof(wd177x_t));
    wd->wd177x_index = wd177x_index;
    wd->chip_type = chip_type;
    wd->crc_reg = 0xffff;
    wd->mode = wd177x_type_1;
    wd->density = DEN_FM_LO;
    wd->step_direction = 1;
    wd->callback = callback;
    wd->address_mark = wd177x_NoMark;
    wd->motor_on = 0;
    wd->timer_Type1_1 = timer_alloc(wd177x_type_1_step_1);
    wd->timer_Type1_2 = timer_alloc(wd177x_type_1_step_2);
    wd->timer_INTRQ = timer_alloc(wd177x_intrq);
    wd->timer_index_pulse_reset = timer_alloc(wd177x_timer_index_pulse_reset);
    
    wd177x_read_current_track( wd );
}

void wd177x_reset( offs_t wd177x_index)
{

}

void wd177x_set_drive( offs_t wd177x_index, UINT8 drive)
{
    wd177x_t *wd = &(wd177x_state[ wd177x_index ]);
    
    assert( wd177x_index < wd177x_MAX_wd177x );
    
    if( wd->current_drive != drive )
    {
        /* Setup floppy disk call backs */
        floppy_drive_set_index_pulse_callback(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), NULL);
        floppy_drive_set_ready_state_change_callback(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), NULL);

        floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), 0);
        
        wd177x_write_back_current_track( wd );
        wd->current_drive = drive;
        wd177x_read_current_track( wd );

        floppy_drive_set_index_pulse_callback(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), wd177x_index_pulse_callback);
        floppy_drive_set_ready_state_change_callback(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), wd177x_ready_state_change);
    }
}

void wd177x_index_pulse_callback(mess_image *img)
{
    /* This is why we can only have a single wd177x */
    wd177x_t *wd = &(wd177x_state[ 0 ]);
    
    /* 1770 and 1772 have automatic motor control */
    if( wd->chip_type == wd177x_wd1770 || wd->chip_type == wd177x_wd1772 )
    {
        /* Decrement 'motor on' counter, turn motor off if zero */
        if( wd->motor_on != 0 && wd->motor_on-- == 1 && wd->motor_on == 0 )  /* This is the strangest C code I've ever written */
        {
            floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), 0);
            wd->status_reg &= ~0x80; /* Turn off motor on bit */
        }
    }
    
    if( wd->mode == wd177x_type_1 )
    {
        /* Under type 1 commands status bit 1 reflects the index pulse */
        wd->status_reg |= 0x02;
    }
    
    /* Trigger type 4 interrupt on index pulse - todo */
    
    /* Index pulse lasts only about 120 microseconds */
    timer_adjust(wd->timer_index_pulse_reset, TIME_IN_USEC(120), 0, 0);
}

void wd177x_timer_index_pulse_reset( int param )
{
    /* This is why we can only have a single wd177x */
    wd177x_t *wd = &(wd177x_state[ 0 ]);
    
    if( wd->mode == wd177x_type_1 )
    {
        /* Under type 1 commands status bit 1 reflects the index pulse */
        wd->status_reg &= ~0x02;
    }

    timer_reset( wd->timer_index_pulse_reset, TIME_NEVER );
}

void wd177x_ready_state_change( mess_image *img, int state )
{
    /* This is why we can only have a single wd177x */
    wd177x_t *wd = &(wd177x_state[ 0 ]);

    if( wd->chip_type == wd177x_wd1773 )
    {
        /* During a type 1 command the wd1773's status bit 7 reflects the READY line */
        if( wd->mode == wd177x_type_1 )
        {        
            /* not ready */
            if( state == 0 )
                wd->status_reg |= 0x80;
            else
                wd->status_reg &= ~0x80;
        }
        /* On a type 4 command certian conditions can cause an INTRQ */
        else if( wd->mode == wd177x_type_4)
        {
            /* Trigger interrupt if conditions are met - todo */
        }
    }
    
    wd->ready = state;
}

void wd177x_set_side( offs_t wd177x_index, UINT8 side )
{
    wd177x_t *wd = &(wd177x_state[ wd177x_index ]);
    
    assert( wd177x_index < wd177x_MAX_wd177x );
    
    if( wd->current_side != side )
    {
        wd177x_write_back_current_track( wd );
        wd->current_side = side;
        wd177x_read_current_track( wd );
    }

}

void wd177x_set_density( offs_t wd177x_index, DENSITY density )
{
    wd177x_t *wd = &(wd177x_state[ wd177x_index ]);
    
    assert( wd177x_index < wd177x_MAX_wd177x );
    wd->density = density;
}

WRITE_HANDLER ( wd177x_command_w )
{
    wd177x_t *wd = &(wd177x_state[ offset ]);
    
    assert( offset < wd177x_MAX_wd177x );
    LOG(( "wd177x: Command Write: %x, chip #%d", data, offset ));
    
    /* Check for type IV command - todo */
    
    /* Check if busy -- todo */
    
    /* Clear DRQ and INTRQ - todo */
    
    wd->command_reg = data;

    if ( (data & 0x80) == 0x80 ) /* Is Type 1 command? */
        wd177x_type_1_command( wd );

    /* Check for type II command - todo */
    
    /* Check for type III command - todo */

}

WRITE_HANDLER ( wd177x_track_w )
{
    wd177x_t *wd = &(wd177x_state[ offset ]);
    
    assert( offset < wd177x_MAX_wd177x );
    LOG(( "wd177x: Track Write: %x, chip #%d", data, offset ));

    wd->track_reg = data;
}

WRITE_HANDLER ( wd177x_sector_w )
{
    wd177x_t *wd = &(wd177x_state[ offset ]);
    
    assert( offset < wd177x_MAX_wd177x );
    LOG(( "wd177x: Sector Write: %x, chip #%d", data, offset ));
    
    wd->sector_reg = data;

}

WRITE_HANDLER ( wd177x_data_w )
{
    wd177x_t *wd = &(wd177x_state[ offset ]);
    
    assert( offset < wd177x_MAX_wd177x );
    LOG(( "wd177x: Data Write: %x, chip #%d", data, offset ));
    
    wd->data_reg = data;
    
    /* Clear DRQ - todo */

}

READ_HANDLER ( wd177x_status_r )
{
    wd177x_t *wd = &(wd177x_state[ offset ]);
    
    assert( offset < wd177x_MAX_wd177x );

    LOG(( "wd177x: Status Read: %x, chip #%d", wd->status_reg, offset ));
    return wd->status_reg;
}

READ_HANDLER ( wd177x_track_r )
{
    wd177x_t *wd = &(wd177x_state[ offset ]);
    
    assert( offset < wd177x_MAX_wd177x );

    LOG(( "wd177x: Track Read: %x, chip #%d", wd->track_reg, offset ));
    return wd->track_reg;
}

READ_HANDLER ( wd177x_sector_r )
{
    wd177x_t *wd = &(wd177x_state[ offset ]);
    
    assert( offset < wd177x_MAX_wd177x );

    LOG(( "wd177x: Sector Read: %x, chip #%d", wd->sector_reg, offset ));
    return wd->sector_reg;
}

READ_HANDLER ( wd177x_data_r )
{
    wd177x_t *wd = &(wd177x_state[ offset ]);
    
    assert( offset < wd177x_MAX_wd177x );

    /* Clear DRQ - todo */
    
    LOG(( "wd177x: Data Read: %x, chip #%d", wd->data_reg, offset ));
    return wd->data_reg;
}

void wd177x_type_1_command( wd177x_t *wd )
{
    double wait = TIME_IN_USEC(5);
    
    wd->mode = wd177x_type_1;
    
    wd->status_reg |= 0x01; /* Set busy */
    wd->status_reg &= ~0x38; /* Reset seek, crc, spin-up/head loaded */
    wd->callback( wd->wd177x_index, wd177x_IRQ_CLR ); /* Reset INTRQ */
    wd->callback( wd->wd177x_index, wd177x_DRQ_CLR ); /* Reset DRQ */
    
    /* Check if spin-up is requested (1770 and 1772 only) */
    if( wd->chip_type == wd177x_wd1770 || wd->chip_type == wd177x_wd1772 )
    {
        if( wd->motor_on == 0 ) /* only wait for spin up in MO is off */
        {
            if( (wd->command_reg & 0x08) == 0x08 ) /* test h bit in command word */
            {
                /* Wait an additional 6 disk revolutions while disk spins up */
                floppy_drive_set_motor_state(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), 1);
                wait += TIME_IN_HZ(300) * 6;
            }
        }
        
        wd->status_reg |= 0x80; /* turn on motor on bit for the 1770 and 1772 */
    }

    if( floppy_drive_get_flag_state(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), FLOPPY_DRIVE_HEAD_AT_TRACK_0) )
        wd->status_reg |= 0x04; /* Set track zero status bit */
    else
        wd->status_reg &= ~0x04; /* Reset track zero status bit */

    /* Now pause until the specified time */
    timer_adjust(wd->timer_Type1_1, wait, (int)wd, 0);
}

void wd177x_type_1_step_1( int param )
{
    int i = 255;
    double wait = TIME_IN_USEC(5);
    wd177x_t *wd = (wd177x_t *)param;
    
    timer_reset(wd->timer_Type1_1, TIME_NEVER);
    
    if( wd->chip_type == wd177x_wd1770 || wd->chip_type == wd177x_wd1772 )
        wd->status_reg |= 0x20;	/* Set spin-up flag */
    
    if( (wd->command_reg & 0x60) == 0x40 )	/* Test for step in command */
    {
        wd->step_direction = 1;
        wd177x_step_read_write_head( wd );
        wait += wd177x_get_step_rate( wd ); /* Extend wait per Step Rate */
    }
    else if( (wd->command_reg & 0x60) == 0x60 ) /* Test for step out command */
    {
        wd->step_direction = -1;
        wd177x_step_read_write_head( wd );
        wait += wd177x_get_step_rate( wd ); /* Extend wait per Step Rate */
    }
    else if( (wd->command_reg & 0x60) == 0x20 ) /* Test for step command */
    {
        wd177x_step_read_write_head( wd );
        wait += wd177x_get_step_rate( wd ); /* Extend wait per Step Rate */
    }
    else /* command is seek or restore */
    {
        if( (wd->command_reg & 0xf0) == 0 ) /* Test for restore command */
        {
            wd->track_reg = 0xff;
            wd->data_reg = 0;
        }
        
        wd->data_shift_reg = wd->data_reg;
        
        wd177x_write_back_current_track( wd );

        do
        {
            if( wd->track_reg == wd->data_shift_reg )
                exit;
            
            if( wd->data_shift_reg > wd->track_reg )
                wd->step_direction = -1;
            else
                wd->step_direction = 1;
                    
            floppy_drive_seek(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), wd->step_direction);
            wd->track_reg += wd->step_direction;
            
            wait += wd177x_get_step_rate( wd ); /* Extend wait per Step Rate */
            
            if( floppy_drive_get_flag_state(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), FLOPPY_DRIVE_HEAD_AT_TRACK_0) )
            {
                if( wd->step_direction == -1 )
                {
                    wd->track_reg = 0;
                    exit;
                }
            }
        } while( i-- > 0 );
        
        wd177x_read_current_track( wd );

        if( i == 0 ) /* Did we step 255 times? */
        {
            if( (wd->command_reg & 0x04) == 0x04 )	/* Test for V bit */
            {
                wd->status_reg |= 0x10;  /* Set seek error and INTRQ */
                timer_adjust(wd->timer_INTRQ, wait, (int)wd, 0);
                return;
            }
        }
            
    }
    
    timer_adjust(wd->timer_Type1_2, wait, (int)wd, 0);
}

void wd177x_type_1_step_2( int param )
{
    double wait = TIME_IN_USEC(5);
    wd177x_t *wd = (wd177x_t *)param;
    int	found_sector = 0;
    UINT16 crc;
    
    timer_reset(wd->timer_Type1_2, TIME_NEVER);

    if( floppy_drive_get_flag_state(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), FLOPPY_DRIVE_HEAD_AT_TRACK_0) )
        wd->status_reg |= 0x04;	/* Set Track Zero flag */
            
    if( (wd->command_reg & 0x04) == 0x04 )	/* Test for V bit */
    {
        /* Verify new track */
        int i;
        
        for( i=0; i<wd->track_length; i++ )
        {
            wd177x_read_byte_from_track( wd );
            
            if( wd->address_mark == wd177x_IDAddressMark )
            {
                /* Found ID Field */
                wd177x_read_byte_from_track( wd ); i++; /* Track number? */
                
                if( wd->track_reg == wd->data_shift_reg )
                {
                    /* Track number matches, read thru remaining ID field */
                    wd177x_read_byte_from_track( wd ); i++; /* Side number */
                    wd177x_read_byte_from_track( wd ); i++; /* Sector number */
                    wd177x_read_byte_from_track( wd ); i++; /* Sector length */
                    wd177x_read_byte_from_track( wd ); i++; /* MSB CRC byte */
                    crc = wd->data_shift_reg << 8;
                    wd177x_read_byte_from_track( wd ); i++; /* LSB CRC byte */
                    crc &= wd->data_shift_reg;
                    
                    /* Does CRCs match? */
                    if ( crc == wd->crc_reg )
                    {
                        /* Yes */
                        wd->status_reg &= ~0x08; /* Reset CRC error */
                        found_sector = 1;
                        
                        /* Add time it took to find this sector */
                        wait += TIME_IN_HZ(300) * ( (double)i / (double)wd->track_length );
                        
                        exit;
                    }
                    else
                    {
                        wd->status_reg |= 0x08; /* Set CRC error */
                    }
                }
                else
                {
                    /* Track didn't match, forward thru ID field then try again */
                    wd177x_read_byte_from_track( wd ); i++; /* Side number */
                    wd177x_read_byte_from_track( wd ); i++; /* Sector number */
                    wd177x_read_byte_from_track( wd ); i++; /* Sector length */
                    wd177x_read_byte_from_track( wd ); i++; /* MSB CRC byte */
                    wd177x_read_byte_from_track( wd ); i++; /* LSB CRC byte */
                }
            }
        }
        
        if( found_sector == 0 )
        {
            /* Sector not found, add five revloutions of search time */
            wait += TIME_IN_HZ(300) * 5;
        }
    }
    
    timer_adjust(wd->timer_INTRQ, wait, (int)wd, 0);
}

void wd177x_intrq( int param )
{
    wd177x_t *wd = (wd177x_t *)param;

    timer_reset(wd->timer_INTRQ, TIME_NEVER);
    
    if( wd->chip_type == wd177x_wd1770 || wd->chip_type == wd177x_wd1772 )
        wd->motor_on = 9; /* Spin for at least 9 more revolutions */

    wd->status_reg &= ~0x01; /* Reset busy */
    wd->callback( wd->wd177x_index, wd177x_IRQ_SET ); /* Set INTRQ */
}

void wd177x_step_read_write_head( wd177x_t *wd )
{
    int	track_reg;
    
    wd177x_write_back_current_track( wd );
    floppy_drive_seek(image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), wd->step_direction);
    wd177x_read_current_track( wd );

    /* Update Track Register if U = 1 */
    if( (wd->command_reg & 0x10) == 0x10 )
    {
        track_reg = wd->track_reg;
        track_reg += wd->step_direction;
        
        if( track_reg < 0 )
            track_reg = 0;
                
        if( track_reg > 0xff )
            track_reg = 0xff;
                
        wd->track_reg = track_reg;
    }
}

double wd177x_get_step_rate( wd177x_t *wd )
{
    const int WD1772_stepRates[] = { 6, 12, 2, 3 };
    const int wd177x_get_step_rates[] = { 6, 12, 20, 30 };
    
    if( wd->chip_type == wd177x_wd1772 )
        return TIME_IN_USEC( WD1772_stepRates[ wd->command_reg & 0x03 ] );
    else
        return TIME_IN_USEC( wd177x_get_step_rates[ wd->command_reg & 0x03 ] );
}
void wd177x_write_back_current_track( wd177x_t *wd )
{
    if( wd->track_dirty )
    {
        floppy_drive_write_track_data_info_buffer( image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), wd->current_side, wd->track_data, &(wd->track_length));
        wd->track_dirty = 0;
    }
    
}

void wd177x_read_current_track( wd177x_t *wd )
{
    wd->track_length = wd177x_MAXTRACKLENGTH;
    floppy_drive_read_track_data_info_buffer( image_from_devtype_and_index(IO_FLOPPY, wd->current_drive), wd->current_side, wd->track_data, &(wd->track_length));
    wd->track_dirty = 0;
}

/* Used for pattern matching */

void wd177x_read_byte_from_track( wd177x_t *wd )
{
    /* This implements ID/DATA address mark detection and CRC logic for FM and MFM */

    if( wd->density == DEN_FM_LO ) /* Read byte off disk (Single density) */
    {
        wd->track_position += 2;	/* In low density mode: read every other byte */
    
        /* Wrap around if necessary */
        if( wd->track_position > wd->track_length )
            wd->track_position = 0;
        
        wd->data_shift_reg = wd->track_data[ wd->track_position ];
        
        /* Process gap 2 */
        if( wd->address_mark_detected > 0 && wd->address_mark == wd177x_GAPII )
        {
            wd->address_mark_detected--;
            if( wd->address_mark_detected == 0 )
                wd->address_mark = wd177x_NoMark;
        }
        
        /* Process data inside the address mark */
        if( wd->address_mark_detected > 0 && wd->address_mark != wd177x_GAPII)
        {
            if( wd->address_mark_detected > 2 ) /* Don't CRC the CRC bytes */
                wd->crc_reg = ccitt_crc16_one( wd->crc_reg, wd->data_shift_reg );
            
            wd->address_mark_detected--;
            
            /*Save sector length in internal register */
            if( wd->address_mark == wd177x_IDAddressMark && wd->address_mark_detected == 2 )
                wd->sector_length = wd->data_shift_reg;
            
            if( wd->address_mark_detected == 0 ) /* Are we done with the data fields? */
            {
                if( wd->address_mark == wd177x_IDAddressMark)
                {
                    wd->address_mark = wd177x_GAPII; /*Yes, done with Index data fields */
                    wd->address_mark_detected = 30;  /* Switch two GAP II */
                }
                else
                    wd->address_mark = wd177x_NoMark; /* Yes, done with data field */
            }
        }
        /* Looing for ID Address mark */
        else if( wd->address_mark == wd177x_NoMark && wd->address_mark_position > 2 && wd->data_shift_reg == 0xfe )
        {
            wd->address_mark = wd177x_IDAddressMark;
            wd->address_mark_detected = 6;

            wd->crc_reg = ccitt_crc16_one( 0xffff, wd->data_shift_reg );
            wd->address_mark_position = 0;
        }
        /* Looking for Data Address mark */
        else if( wd->address_mark == wd177x_GAPII && wd->address_mark_position > 2 && ( wd->data_shift_reg > 0xf7 && wd->data_shift_reg < 0xfc ) )
        {
            /* We found a data address mark */

            wd->address_mark = wd177x_DataAddressMark;

            wd->address_mark_detected = (128 << wd->sector_length) + 2;

            wd->crc_reg = ccitt_crc16_one( 0xffff, wd->data_shift_reg );
            wd->address_mark_position = 0;
        }
        /* Synching to address mark */
        else if( wd->data_shift_reg == 0x00 )
            wd->address_mark_position++;
        /* Reset sync */
        else
            wd->address_mark_position = 0;
    }
    else   /* Read byte off disk (double density) */
    {
        wd->track_position++;

        if( wd->track_position > wd->track_length )
            wd->track_position = 0;
    
        wd->data_shift_reg = wd->track_data[ wd->track_position ];
        
        /* Process gap 2 */
        if( wd->address_mark_detected > 0 && wd->address_mark == wd177x_GAPII )
        {
            wd->address_mark_detected--;
            if( wd->address_mark_detected == 0 )
                wd->address_mark = wd177x_NoMark;
        }
        
        /* Process data inside the address mark */
        if( wd->address_mark_detected > 0 && wd->address_mark != wd177x_GAPII)
        {
            if( wd->address_mark_detected > 2 ) /* Don't CRC the CRC bytes */
                wd->crc_reg = ccitt_crc16_one( wd->crc_reg, wd->data_shift_reg );
            
            wd->address_mark_detected--;
            
            /*Save sector length in internal register */
            if( wd->address_mark == wd177x_IDAddressMark && wd->address_mark_detected == 2 )
                wd->sector_length = wd->data_shift_reg;
            
            if( wd->address_mark_detected == 0 )
            {
                if( wd->address_mark == wd177x_IDAddressMark)
                {
                    wd->address_mark = wd177x_GAPII; /* We are now done with the address mark */
                    wd->address_mark_detected = 43;
                }
                else
                    wd->address_mark = wd177x_NoMark;
            }
        }
        /* Looing for ID Address mark */
        else if( wd->address_mark == wd177x_NoMark && wd->address_mark_position > 2 && wd->data_shift_reg == 0xfe )
        {
            wd->address_mark = wd177x_IDAddressMark;
            wd->address_mark_detected = 6;

            wd->crc_reg = ccitt_crc16_one( 0xffff, 0xa1 );
            wd->crc_reg = ccitt_crc16_one( wd->crc_reg, 0xa1 );
            wd->crc_reg = ccitt_crc16_one( wd->crc_reg, 0xa1 );
            wd->crc_reg = ccitt_crc16_one( wd->crc_reg, wd->data_shift_reg );
            wd->address_mark_position = 0;
        }
        /* Looking for Data Address mark */
        else if( wd->address_mark == wd177x_GAPII && wd->address_mark_position > 2 && wd->data_shift_reg > 0xf7 )
        {
            /* We found a data address mark */
            wd->address_mark = wd177x_DataAddressMark;

            wd->address_mark_detected = (128 << wd->sector_length) + 2;

            wd->crc_reg = ccitt_crc16_one( 0xffff, 0xa1 );
            wd->crc_reg = ccitt_crc16_one( wd->crc_reg, 0xa1 );
            wd->crc_reg = ccitt_crc16_one( wd->crc_reg, 0xa1 );
            wd->crc_reg = ccitt_crc16_one( wd->crc_reg, wd->data_shift_reg );
            wd->address_mark_position = 0;
        }
        /* Synching to address mark */
        else if( wd->data_shift_reg == 0xa1 )
            wd->address_mark_position++;
        /* Reset sync */
        else
            wd->address_mark_position = 0;
    }
}
