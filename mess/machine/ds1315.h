/* This is an emulation of Dallas Semiconductor's Phantom Time Chip.
   DS1315.
   
   by tim lindner, November 2001.
*/

void ds1315_init( void );
READ_HANDLER ( ds1315_r_0 );
READ_HANDLER ( ds1315_r_1 );
READ_HANDLER ( ds1315_r_data );
WRITE_HANDLER ( ds1315_w_data );
