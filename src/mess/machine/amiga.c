/***************************************************************************
Commodore Amiga - (c) 1985, Commodore Bussines Machines Co.

Preliminary driver by:

Ernesto Corvi
ernesto@imagina.com

***************************************************************************/

#include "driver.h"
#include "mess/systems/amiga.h"

#define EMULATE_INDEX_PULSE 1

#define LOG_CUSTOM 0

/***************************************************************************

	General routines and registers

***************************************************************************/

/* required prototype */
void amiga_custom_w( int offs, int data );

custom_regs_def custom_regs;

/* from vidhrdw */
void amiga_update_line( int line, int start_h, int end_h, custom_regs_def *regs );

static int get_int_level( void ) {

	int ints = custom_regs.INTENA & custom_regs.INTREQ;

	if ( custom_regs.INTENA & 0x4000 ) { /* Master interrupt switch */

		/* Level 7 = NMI Can only be triggered from outside the hardware */

		if ( ints & 0x2000 )
			return 6;

		if ( ints & 0x1800 )
			return 5;

		if ( ints & 0x0780 )
			return 4;

		if ( ints & 0x0070 )
			return 3;

		if ( ints & 0x0008 )
			return 2;

		if ( ints & 0x0007 )
			return 1;
	}

	return 0;
}

static void trigger_int( void ) {

	int level = get_int_level();

	if ( level )
		cpu_cause_interrupt( 0, level );
}

/***************************************************************************

	Blitter emulation

***************************************************************************/

static unsigned short blitter_fill( unsigned short src, int mode, int *fc ) {
	int i, data;
	unsigned short dst = 0;

	for ( i = 0; i < 16; i++ ) {
		data = ( src >> i ) & 1;
		if ( data ) {
			*fc ^= 1;
			if ( mode & 0x0010 ) /* Exclusive mode */
				dst |= ( data ^ *fc ) << i;
			if ( mode & 0x0008 ) /* Inclusive mode */
				dst |= ( data | *fc ) << i;
		} else
			dst |= ( *fc << i );
	}

	return dst;
}

/* Ascending */
#define BLIT_FUNCA( num, op ) static int blit_func_a##num ( unsigned short *old_data,		\
															unsigned short *new_data,		\
														 	int first, int last ) {			\
	unsigned short dataA, dataB, dataC; 													\
	int shiftA, shiftB;																		\
																							\
	dataA =	new_data[0];																	\
	dataB =	new_data[1];																	\
	dataC =	new_data[2];																	\
																							\
	if ( first )																			\
		dataA &= custom_regs.BLTAFWM;														\
																							\
	if ( last )																				\
		dataA &= custom_regs.BLTALWM;														\
																							\
	shiftA = ( custom_regs.BLTCON0 >> 12 ) & 0x0f;											\
	shiftB = ( custom_regs.BLTCON1 >> 12 ) & 0x0f;											\
																							\
	if ( shiftA ) {																			\
		dataA >>= shiftA;																	\
		if ( !first )																		\
			dataA |= ( old_data[0] & ( ( 1 << shiftA ) - 1 ) ) << ( 16 - shiftA );			\
	}																						\
																							\
	if ( shiftB ) {																			\
		dataB >>= shiftB;																	\
		if ( !first )																		\
			dataB |= ( old_data[1] & ( ( 1 << shiftB ) - 1 ) ) << ( 16 - shiftB );			\
	}																						\
																							\
	return ( op );																			\
}

BLIT_FUNCA( 0, ( ~dataA & ~dataB & ~dataC ) ) /* /A/B/C */
BLIT_FUNCA( 1, ( ~dataA & ~dataB &  dataC ) ) /* /A/B C */
BLIT_FUNCA( 2, ( ~dataA &  dataB & ~dataC ) ) /* /A B/C */
BLIT_FUNCA( 3, ( ~dataA &  dataB &  dataC ) ) /* /A B C */
BLIT_FUNCA( 4, (  dataA & ~dataB & ~dataC ) ) /*  A/B/C */
BLIT_FUNCA( 5, (  dataA & ~dataB &  dataC ) ) /*  A/B C */
BLIT_FUNCA( 6, (  dataA &  dataB & ~dataC ) ) /*  A B/C */
BLIT_FUNCA( 7, (  dataA &  dataB &  dataC ) ) /*  A B C */

static int (*blit_func_a[8])( unsigned short *old_data, unsigned short *new_data, int first, int last ) = {
	blit_func_a0,
	blit_func_a1,
	blit_func_a2,
	blit_func_a3,
	blit_func_a4,
	blit_func_a5,
	blit_func_a6,
	blit_func_a7
};

/* Descending */
#define BLIT_FUNCD( num, op ) static int blit_func_d##num ( unsigned short *old_data,		\
															unsigned short *new_data,		\
															int first, int last ) {			\
	unsigned short dataA, dataB, dataC; 													\
	int shiftA, shiftB;																		\
																							\
	dataA =	new_data[0];																	\
	dataB =	new_data[1];																	\
	dataC =	new_data[2];																	\
																							\
	if ( first )																			\
		dataA &= custom_regs.BLTAFWM;														\
																							\
	if ( last )																				\
		dataA &= custom_regs.BLTALWM;														\
																							\
	shiftA = ( custom_regs.BLTCON0 >> 12 ) & 0x0f;											\
	shiftB = ( custom_regs.BLTCON1 >> 12 ) & 0x0f;											\
																							\
	if ( shiftA ) {																			\
		dataA <<= shiftA;																	\
		if ( !first )																		\
			dataA |= ( old_data[0] >> ( 16 - shiftA ) ) & ( ( 1 << shiftA ) - 1 );			\
	}																						\
																							\
	if ( shiftB ) {																			\
		dataB <<= shiftB;																	\
		if ( !first )																		\
			dataB |= ( old_data[1] >> ( 16 - shiftB ) ) & ( ( 1 << shiftB ) - 1 );			\
	}																						\
																							\
	return ( op );																			\
}

BLIT_FUNCD( 0, ( ~dataA & ~dataB & ~dataC ) ) /* /A/B/C */
BLIT_FUNCD( 1, ( ~dataA & ~dataB &  dataC ) ) /* /A/B C */
BLIT_FUNCD( 2, ( ~dataA &  dataB & ~dataC ) ) /* /A B/C */
BLIT_FUNCD( 3, ( ~dataA &  dataB &  dataC ) ) /* /A B C */
BLIT_FUNCD( 4, (  dataA & ~dataB & ~dataC ) ) /*  A/B/C */
BLIT_FUNCD( 5, (  dataA & ~dataB &  dataC ) ) /*  A/B C */
BLIT_FUNCD( 6, (  dataA &  dataB & ~dataC ) ) /*  A B/C */
BLIT_FUNCD( 7, (  dataA &  dataB &  dataC ) ) /*  A B C */

static int (*blit_func_d[8])( unsigned short *old_data, unsigned short *new_data, int first, int last ) = {
	blit_func_d0,
	blit_func_d1,
	blit_func_d2,
	blit_func_d3,
	blit_func_d4,
	blit_func_d5,
	blit_func_d6,
	blit_func_d7
};

/* General for Ascending/Descending */
static int (**blit_func)( unsigned short *old_data, unsigned short *new_data, int first, int last );

/* Line mode */
#define BLIT_FUNCL( num, op ) static int blit_func_line##num ( unsigned short dataA,		\
															   unsigned short dataB,		\
															   unsigned short dataC ) {		\
	return ( op );																			\
}

BLIT_FUNCL( 0, ( ~dataA & ~dataB & ~dataC ) ) /* /A/B/C */
BLIT_FUNCL( 1, ( ~dataA & ~dataB &  dataC ) ) /* /A/B C */
BLIT_FUNCL( 2, ( ~dataA &  dataB & ~dataC ) ) /* /A B/C */
BLIT_FUNCL( 3, ( ~dataA &  dataB &  dataC ) ) /* /A B C */
BLIT_FUNCL( 4, (  dataA & ~dataB & ~dataC ) ) /*  A/B/C */
BLIT_FUNCL( 5, (  dataA & ~dataB &  dataC ) ) /*  A/B C */
BLIT_FUNCL( 6, (  dataA &  dataB & ~dataC ) ) /*  A B/C */
BLIT_FUNCL( 7, (  dataA &  dataB &  dataC ) ) /*  A B C */

static int (*blit_func_line[8])( unsigned short dataA, unsigned short dataB, unsigned short dataC ) = {
	blit_func_line0,
	blit_func_line1,
	blit_func_line2,
	blit_func_line3,
	blit_func_line4,
	blit_func_line5,
	blit_func_line6,
	blit_func_line7
};

static void blitter_proc( int param ) {
	/* Now we do the real blitting */
	unsigned char *RAM = memory_region(REGION_CPU1);
	int	blt_total = 0;

	custom_regs.DMACON |= 0x2000; /* Blit Zero, we modify it later */

	if ( custom_regs.BLTCON1 & 1 ) { /* Line mode */
		int linesize, single_bit, single_flag, texture, sign, start, ptr[4], temp_ptr3;
		unsigned short dataA, dataB;

		if ( ( custom_regs.BLTSIZE & 0x003f ) != 0x0002 ) {
			if ( errorlog )
				fprintf( errorlog, "Blitter: BLTSIZE.w != 2 in line mode!\n" );
		}

		if ( ( custom_regs.BLTCON0 & 0x0b00 ) != 0x0b00 ) {
			if ( errorlog )
				fprintf( errorlog, "Blitter: Channel selection incorrect in line mode!\n" );
		}

		linesize = ( custom_regs.BLTSIZE >> 6 ) & 0x3ff;
		if ( linesize == 0 )
			linesize = 0x400;

		single_bit = custom_regs.BLTCON1 & 0x0002;
		single_flag = 0;
		sign = custom_regs.BLTCON1 & 0x0040;
		texture = ( custom_regs.BLTCON1 >> 12 ) & 0x0f;
		start = ( custom_regs.BLTCON0 >> 12 ) & 0x0f;

		dataA = ( custom_regs.BLTxDAT[0] >> start );

		ptr[0] = ( custom_regs.BLTxPTH[0] << 16 ) | custom_regs.BLTxPTL[0];
		ptr[2] = ( custom_regs.BLTxPTH[2] << 16 ) | custom_regs.BLTxPTL[2];
		ptr[3] = ( custom_regs.BLTxPTH[3] << 16 ) | custom_regs.BLTxPTL[3];

		dataB = ( custom_regs.BLTxDAT[1] >> texture ) | ( custom_regs.BLTxDAT[1] << ( 16 - texture ) );

		while ( linesize-- ) {
			int dst_data = 0;
			int i;

			temp_ptr3 = ptr[3];

			if ( custom_regs.BLTCON0 & 0x0200 )
				custom_regs.BLTxDAT[2] = READ_WORD( &RAM[ptr[2]] );

			dataA = ( custom_regs.BLTxDAT[0] >> start );

			if ( single_bit && single_flag ) dataA = 0;
			single_flag = 1;

			for ( i = 0; i < 8; i++ ) {
				if ( custom_regs.BLTCON0 & ( 1 << i ) ) {
					dst_data |= (*blit_func_line[i])( dataA, ( dataB & 1 ) * 0xffff, custom_regs.BLTxDAT[2] );
				}
			}

			if ( !sign ) {
				ptr[0] += custom_regs.BLTxMOD[0];
				if ( custom_regs.BLTCON1 & 0x0010 ) {
					if ( custom_regs.BLTCON1 & 0x0008 ) { /* Decrement Y */
						ptr[2] -= custom_regs.BLTxMOD[2];
						ptr[3] -= custom_regs.BLTxMOD[2]; /* ? */
						single_flag = 0;
					} else { /* Increment Y */
						ptr[2] += custom_regs.BLTxMOD[2];
						ptr[3] += custom_regs.BLTxMOD[2]; /* ? */
						single_flag = 0;
					}
				} else {
					if ( custom_regs.BLTCON1 & 0x0008 ) { /* Decrement X */
						if ( start-- == 0 ) {
							start = 15;
							ptr[2] -= 2;
							ptr[3] -= 2;
						}
					} else { /* Increment X */
						if ( ++start == 16 ) {
							start = 0;
							ptr[2] += 2;
							ptr[3] += 2;
						}
					}
				}
			} else
				ptr[0] += custom_regs.BLTxMOD[1];

			if ( custom_regs.BLTCON1 & 0x0010 ) {
				if ( custom_regs.BLTCON1 & 0x0004 ) { /* Decrement X */
					if ( start-- == 0 ) {
						start = 15;
						ptr[2] -= 2;
						ptr[3] -= 2;
					}
				} else {
					if ( ++start == 16 ) { /* Increment X */
						start = 0;
						ptr[2] += 2;
						ptr[3] += 2;
					}
				}
			} else {
				if ( custom_regs.BLTCON1 & 0x0004 ) { /* Decrement Y */
					ptr[2] -= custom_regs.BLTxMOD[2];
					ptr[3] -= custom_regs.BLTxMOD[2]; /* ? */
					single_flag = 0;
				} else { /* Increment Y */
					ptr[2] += custom_regs.BLTxMOD[2];
					ptr[3] += custom_regs.BLTxMOD[2]; /* ? */
					single_flag = 0;
				}
			}

			sign = 0 > ( signed short )ptr[0];

			blt_total |= dst_data;

			if ( custom_regs.BLTCON0 & 0x0100 )
				WRITE_WORD( &RAM[temp_ptr3], dst_data );

			dataB = ( dataB << 1 ) | ( dataB >> 15 );
		}

		custom_regs.BLTxPTH[0] = ( ptr[0] >> 16 ) & 0x1f;
		custom_regs.BLTxPTL[0] = ptr[0] & 0xffff;
		custom_regs.BLTxPTH[2] = ( ptr[2] >> 16 ) & 0x1f;
		custom_regs.BLTxPTL[2] = ptr[2] & 0xffff;
		custom_regs.BLTxPTH[3] = ( ptr[3] >> 16 ) & 0x1f;
		custom_regs.BLTxPTL[3] = ptr[3] & 0xffff;

		custom_regs.BLTxDAT[0] = dataA;
		custom_regs.BLTxDAT[1] = dataB;

	} else { /* Blit mode */
		if ( custom_regs.BLTCON0 & 0x0f00 ) {
			int i, x, y;
			int ptr[4] = { -1, -1, -1, -1 };
			int width = ( custom_regs.BLTSIZE & 0x3f );
			int height = ( ( custom_regs.BLTSIZE >> 6 ) & 0x3ff );
			unsigned short old_data[3];
			unsigned short new_data[3];
			int fc;

			if ( custom_regs.BLTCON0 & 0x800 )
				ptr[0] = ( custom_regs.BLTxPTH[0] << 16 ) | custom_regs.BLTxPTL[0];

			if ( custom_regs.BLTCON0 & 0x400 )
				ptr[1] = ( custom_regs.BLTxPTH[1] << 16 ) | custom_regs.BLTxPTL[1];

			if ( custom_regs.BLTCON0 & 0x200 )
				ptr[2] = ( custom_regs.BLTxPTH[2] << 16 ) | custom_regs.BLTxPTL[2];

			if ( custom_regs.BLTCON0 & 0x100 )
				ptr[3] = ( custom_regs.BLTxPTH[3] << 16 ) | custom_regs.BLTxPTL[3];

			if ( custom_regs.BLTCON1 & 0x0002 ) { /* Descending mode */
				blit_func = blit_func_d;
				fc = ( custom_regs.BLTCON1 >> 2 ) & 1; /* fill carry setup */
			} else {
				blit_func = blit_func_a;
			}

			if ( width == 0 )
				width = 0x40;

			if ( height == 0 )
				height = 0x400;

			for ( y = 0; y < height; y++ ) {
				for ( x = 0; x < width; x++ ) {
					int dst_data = 0;

					/* get old data */
					new_data[0] = old_data[0] = custom_regs.BLTxDAT[0];
					new_data[1] = old_data[1] = custom_regs.BLTxDAT[1];
					new_data[2] = old_data[2] = custom_regs.BLTxDAT[2];

					/* get new data */
					if ( ptr[0] != -1 )
						new_data[0] = READ_WORD( &RAM[ptr[0]] );

					if ( ptr[1] != -1 )
						new_data[1] = READ_WORD( &RAM[ptr[1]] );

					if ( ptr[2] != -1 )
						new_data[2] = READ_WORD( &RAM[ptr[2]] );

					for ( i = 0; i < 8; i++ ) {
						if ( custom_regs.BLTCON0 & ( 1 << i ) ) {
							dst_data |= (*blit_func[i])( old_data, new_data, x == 0, x == (width - 1) );
						}
					}

					/* store new data */
					custom_regs.BLTxDAT[0] = new_data[0];
					custom_regs.BLTxDAT[1] = new_data[1];
					custom_regs.BLTxDAT[2] = new_data[2];

					if ( custom_regs.BLTCON1 & 0x0002 ) { /* Descending mode */
						if ( custom_regs.BLTCON1 & 0x0018 ) /* Fill mode */
							dst_data = blitter_fill( dst_data, custom_regs.BLTCON1 & 0x18, &fc );

						if ( ptr[3] != -1 ) {
							WRITE_WORD( &RAM[ptr[3]], dst_data );
							ptr[3] -= 2;
						}

						if ( ptr[0] != -1 )
							ptr[0] -= 2;

						if ( ptr[1] != -1 )
							ptr[1] -= 2;

						if ( ptr[2] != -1 )
							ptr[2] -= 2;
					} else {
						if ( ptr[3] != -1 ) {
							WRITE_WORD( &RAM[ptr[3]], dst_data );
							ptr[3] += 2;
						}

						if ( ptr[0] != -1 )
							ptr[0] += 2;

						if ( ptr[1] != -1 )
							ptr[1] += 2;

						if ( ptr[2] != -1 )
							ptr[2] += 2;
					}
					blt_total |= dst_data;
				}

				if ( custom_regs.BLTCON1 & 0x0002 ) { /* Descending mode */
					if ( ptr[0] != -1 )
						ptr[0] -= custom_regs.BLTxMOD[0];

					if ( ptr[1] != -1 )
						ptr[1] -= custom_regs.BLTxMOD[1];

					if ( ptr[2] != -1 )
						ptr[2] -= custom_regs.BLTxMOD[2];

					if ( ptr[3] != -1 )
						ptr[3] -= custom_regs.BLTxMOD[3];
				} else {
					if ( ptr[0] != -1 )
						ptr[0] += custom_regs.BLTxMOD[0];

					if ( ptr[1] != -1 )
						ptr[1] += custom_regs.BLTxMOD[1];

					if ( ptr[2] != -1 )
						ptr[2] += custom_regs.BLTxMOD[2];

					if ( ptr[3] != -1 )
						ptr[3] += custom_regs.BLTxMOD[3];
				}
			}

			/* We're done, update the ptr's now */
			if ( ptr[0] != -1 ) {
				custom_regs.BLTxPTH[0] = ptr[0] >> 16;
				custom_regs.BLTxPTL[0] = ptr[0];
			}

			if ( ptr[1] != -1 ) {
				custom_regs.BLTxPTH[1] = ptr[1] >> 16;
				custom_regs.BLTxPTL[1] = ptr[1];
			}

			if ( ptr[2] != -1 ) {
				custom_regs.BLTxPTH[2] = ptr[2] >> 16;
				custom_regs.BLTxPTL[2] = ptr[2];
			}

			if ( ptr[3] != -1 ) {
				custom_regs.BLTxPTH[3] = ptr[3] >> 16;
				custom_regs.BLTxPTL[3] = ptr[3];
			}
		}
	}

	custom_regs.DMACON ^= 0x4000; /* signal we're done */

	if ( blt_total )
		custom_regs.DMACON ^= 0x2000;


	amiga_custom_w( 0x009c, 0x8040 );
}

static void blitter_setup( void ) {
	int ticks, width, height;
	double blit_time;

	if ( ( custom_regs.DMACON & 0x0240 ) != 0x0240 ) /* Enabled ? */
		return;

	if ( custom_regs.DMACON & 0x4000 ) { /* Is there another blitting in progress? */
		if ( errorlog )
			fprintf( errorlog, "This program is playing tricks with the blitter\n" );
		return;
	}

	if ( custom_regs.BLTCON1 & 1 ) { /* Line mode */
		ticks = 8;
	} else {
		ticks = 4;
		if ( custom_regs.BLTCON0 & 0x0400 ) /* channel B, +2 ticks */
			ticks += 2;
		if ( ( custom_regs.BLTCON0 & 0x0300 ) == 0x0300 ) /* Both channel C and D, +2 ticks */
			ticks += 2;
	}

	custom_regs.DMACON |= 0x4000; /* signal blitter busy */

	width = ( custom_regs.BLTSIZE & 0x3f );
	if ( width == 0 )
		width = 0x40;

	height = ( ( custom_regs.BLTSIZE >> 6 ) & 0x3ff );
	if ( height == 0 )
		height = 0x400;

	blit_time = ( (double)ticks * (double)width * (double)height );

	blit_time /= ( (double)Machine->drv->cpu[0].cpu_clock / 1000000.0 );

	timer_set( TIME_IN_USEC( blit_time ), 0, blitter_proc );
}

/***************************************************************************

	Floppy disk controller emulation

***************************************************************************/

/* required prototype */
static void cia_issue_index( void );

typedef struct {
	int motor_on;
	int sel;
	int side;
	int dir;
	int step;
	int ready;
	int track0;
	int wprot;
	int disk_changed;
	void *f;
	int cyl;
	unsigned char mfm[544*2*11];
	void *rev_timer;
	void *spin_timer;
	int pos;
} fdc_def;

static fdc_def fdc_status[4];

static void fdc_init( void ) {
	int i;

	for ( i = 0; i < 4; i++ ) {
		fdc_status[i].motor_on = 0;
		fdc_status[i].sel = 0;
		fdc_status[i].side = 0;
		fdc_status[i].dir = 0;
		fdc_status[i].step = 0;
		fdc_status[i].ready = 0;
		fdc_status[i].track0 = 1;
		fdc_status[i].wprot = 1;
		fdc_status[i].cyl = 0;
		fdc_status[i].rev_timer = 0;
		fdc_status[i].spin_timer = 0;
		fdc_status[i].pos = 0;

		memset( fdc_status[i].mfm, 0xaa, 544*2*11 );

		fdc_status[i].disk_changed = 1;
		if ( floppy_name[i] && floppy_name[i][0] ) {
			fdc_status[i].f = osd_fopen(Machine->gamedrv->name,floppy_name[i],OSD_FILETYPE_IMAGE_RW,0);
			if ( fdc_status[i].f == NULL ) {
				if ( errorlog )
					fprintf( errorlog, "Could not open image %s\n", floppy_name[i] );
				continue;
			}
			fdc_status[i].disk_changed = 0;
		}
	}
}

static int fdc_get_curpos( int drive ) {
	double elapsed;
	int speed;
	int bytes;
	int pos;

	if ( fdc_status[drive].rev_timer == 0 ) {
		if ( errorlog )
			fprintf( errorlog, "Rev timer not started on drive %d, cant get position!\n", drive );
		return 0;
	}

	elapsed = timer_timeelapsed( fdc_status[drive].rev_timer );
	speed = ( custom_regs.ADKCON & 0x100 ) ? 2 : 4;

	bytes = elapsed / ( TIME_IN_USEC( speed * 8 ) );
	pos = bytes % ( 544*2*11 );

	return pos;
}

static unsigned short fdc_get_byte( void ) {
	int pos;
	int i, drive = -1;
	unsigned short ret;

	ret = ( ( custom_regs.DSKLEN >> 1 ) & 0x4000 ) & ( ( custom_regs.DMACON << 10 ) & 0x4000 );
	ret |= ( custom_regs.DSKLEN >> 1 ) & 0x2000;

	for ( i = 0; i < 4; i++ ) {
		if ( fdc_status[i].sel )
			drive = i;
	}

	if ( drive == -1 )
		return ret;

	if ( fdc_status[drive].disk_changed || fdc_status[drive].ready == 0 )
		return ret;

	pos = fdc_get_curpos( drive );

	if ( fdc_status[drive].mfm[pos] == ( custom_regs.DSKSYNC >> 8 ) &&
		 fdc_status[drive].mfm[pos+1] == ( custom_regs.DSKSYNC & 0xff ) )
			ret |= 0x1000;

	if ( pos != fdc_status[drive].pos ) {
		ret |= 0x8000;
		fdc_status[drive].pos = pos;
	}

	ret |= fdc_status[drive].mfm[pos];

	return ret;
}

static void fdc_dma_proc( int drive ) {

	if ( ( custom_regs.DSKLEN & 0x8000 ) == 0 )
		return;

	if ( ( custom_regs.DMACON & 0x0210 ) == 0 )
		return;

	if ( fdc_status[drive].disk_changed || fdc_status[drive].ready == 0 )
		return;

	if ( fdc_status[drive].sel == 0 )
		return;

	if ( custom_regs.DSKLEN & 0x4000 ) {
		if ( errorlog )
			fprintf( errorlog, "Write to disk unsupported yet\n" );
	} else {
		unsigned char *RAM = &memory_region(REGION_CPU1)[( custom_regs.DSKPTH << 16 ) | custom_regs.DSKPTL];
		int cur_pos = fdc_status[drive].pos;
		int len = custom_regs.DSKLEN & 0x3fff;

		while ( len-- ) {
			*RAM++ = fdc_status[drive].mfm[cur_pos++];
			if ( cur_pos == ( 544 * 2 * 11 ) )
				cur_pos = 0;
			*RAM++ = fdc_status[drive].mfm[cur_pos++];
			if ( cur_pos == ( 544 * 2 * 11 ) )
				cur_pos = 0;
		}
	}

	amiga_custom_w( 0x009c, 0x8002 );
}

static void fdc_setup_dma( void ) {
	int i, cur_pos, drive = -1;
	int time = 0;

	for ( i = 0; i < 4; i++ ) {
		if ( fdc_status[i].sel ) {
			drive = i;
			break;
		}
	}

	if ( drive == -1 ) {
		if ( errorlog )
			fprintf( errorlog, "Disk DMA started with no drive selected!\n" );
		return;
	}

	if ( fdc_status[drive].disk_changed || fdc_status[drive].ready == 0 ) {
		return;
	}

	fdc_status[drive].pos = cur_pos = fdc_get_curpos( drive );

	if ( custom_regs.ADKCON & 0x0400 ) { /* Wait for sync */
		if ( custom_regs.DSKSYNC != 0x4489 ) {
			if ( errorlog )
				fprintf( errorlog, "Attempting to read a non-standard SYNC\n" );
		}

		i = cur_pos;
		do {
			if ( fdc_status[drive].mfm[i] == ( custom_regs.DSKSYNC >> 8 ) &&
				 fdc_status[drive].mfm[i+1] == ( custom_regs.DSKSYNC & 0xff ) )
				 	break;

			i++;
			i %= ( 544 * 2 * 11 );
			time++;
		} while( i != cur_pos );

		if ( i == cur_pos && time != 0 ) {
			if ( errorlog )
				fprintf( errorlog, "SYNC not found on track!\n" );
			return;
		} else {
			fdc_status[drive].pos = i + 2;
		}

		time += ( custom_regs.DSKLEN & 0x3fff ) * 2;
		time *= ( custom_regs.ADKCON & 0x0100 ) ? 2 : 4;
		time *= 8;
		timer_set( TIME_IN_USEC( time ), drive, fdc_dma_proc );
	} else {
		time = ( custom_regs.DSKLEN & 0x3fff ) * 2;
		time *= ( custom_regs.ADKCON & 0x0100 ) ? 2 : 4;
		time *= 8;
		timer_set( TIME_IN_USEC( time ), drive, fdc_dma_proc );
	}
}

static void setup_fdc_buffer( int drive ) {
	int sector, offset, len;
	unsigned char temp_cyl[512*11];

	/* no disk in drive */
	if ( fdc_status[drive].disk_changed )
		return;

	/* drive is not ready */
	if ( fdc_status[drive].ready == 0 )
		return;

	if ( fdc_status[drive].f == NULL ) {
		fdc_status[drive].disk_changed = 1;
		return;
	}

	len = 512*11;

	offset = ( fdc_status[drive].cyl << 1 ) | fdc_status[drive].side;

	if ( osd_fseek( fdc_status[drive].f, offset * len, SEEK_SET ) ) {
		if ( errorlog )
			fprintf( errorlog, "FDC: osd_fseek failed!\n" );
		osd_fclose( fdc_status[drive].f );
		fdc_status[drive].f = NULL;
		fdc_status[drive].disk_changed = 1;
	}

	osd_fread( fdc_status[drive].f, temp_cyl, len );

	for ( sector = 0; sector < 11; sector++ ) {
		unsigned char secbuf[544];
	    int i;
	    unsigned short *mfmbuf = ( unsigned short * )( &fdc_status[drive].mfm[544*2*sector] );
		unsigned long deven,dodd;
		unsigned long hck = 0,dck = 0;

	    secbuf[0] = secbuf[1] = 0x00;
	    secbuf[2] = secbuf[3] = 0xa1;
	    secbuf[4] = 0xff;
	    secbuf[5] = offset;
	    secbuf[6] = sector;
	    secbuf[7] = 11 - sector;

	    for ( i = 8; i < 24; i++ )
			secbuf[i] = 0;

	    memcpy( &secbuf[32], &temp_cyl[sector*512], 512 );

	    mfmbuf[0] = mfmbuf[1] = 0xaaaa;
	    mfmbuf[2] = mfmbuf[3] = 0x4489;

	    deven = ( ( secbuf[4] << 24) | ( secbuf[5] << 16 ) | ( secbuf[6] << 8 ) | ( secbuf[7] ) );
	    dodd = deven >> 1;
	    deven &= 0x55555555; dodd &= 0x55555555;

	    mfmbuf[4] = dodd >> 16;
	    mfmbuf[5] = dodd;
	    mfmbuf[6] = deven>> 16;
	    mfmbuf[7] = deven;

	    for ( i = 8; i < 48; i++ )
			mfmbuf[i] = 0;
	    for (i = 0; i < 512; i += 4) {
			deven = ((secbuf[i+32] << 24) | (secbuf[i+33] << 16)
				 | (secbuf[i+34] << 8) | (secbuf[i+35]));
			dodd = deven >> 1;
			deven &= 0x55555555; dodd &= 0x55555555;
			mfmbuf[(i>>1) + 32] = dodd >> 16;
			mfmbuf[(i>>1) + 33] = dodd;
			mfmbuf[(i>>1) + 256 + 32] = deven >> 16;
			mfmbuf[(i>>1) + 256 + 33] = deven;
	    }

	    for(i = 4; i < 24; i += 2)
			hck ^= (mfmbuf[i] << 16) | mfmbuf[i+1];

	    deven = dodd = hck; dodd >>= 1;
	    mfmbuf[24] = dodd >> 16; mfmbuf[25] = dodd;
	    mfmbuf[26] = deven >> 16; mfmbuf[27] = deven;

	    for(i = 32; i < 544; i += 2)
			dck ^= (mfmbuf[i] << 16) | mfmbuf[i+1];

	    deven = dodd = dck; dodd >>= 1;
	    mfmbuf[28] = dodd >> 16; mfmbuf[29] = dodd;
	    mfmbuf[30] = deven >> 16; mfmbuf[31] = deven;
	}
}

static void fdc_rev_proc( int drive ) {
	cia_issue_index();
}

static void start_rev_timer( int drive ) {
	int time;

	if ( fdc_status[drive].rev_timer ) {
		if ( errorlog )
			fprintf( errorlog, "Revolution timer started twice?!\n" );
		return;
	}

	time = ( custom_regs.ADKCON & 0x100 ) ? 2 : 4;
	time *= ( 544 * 2 * 11 );
	time *= 8;

	fdc_status[drive].rev_timer = timer_pulse( TIME_IN_USEC( time ) , drive, fdc_rev_proc );
}

static void stop_rev_timer( int drive ) {
	if ( fdc_status[drive].rev_timer == 0 ) {
		if ( errorlog )
			fprintf( errorlog, "Revolution timer never started?!\n" );
		return;
	}

	timer_remove( fdc_status[drive].rev_timer );
	fdc_status[drive].rev_timer = 0;
}

static void fdc_setup_leds( int drive ) {

	if ( drive == 0 )
		osd_led_w( 1, fdc_status[drive].motor_on ); /* update internal drive led */

	if ( drive == 1 )
		osd_led_w( 2, fdc_status[drive].motor_on ); /* update internal drive led */
}

static void spin_proc( int drive ) {
	fdc_status[drive].spin_timer = 0;

	/* if theres no disk in the drive, the drive cannot go to ready state */
	if ( fdc_status[drive].disk_changed ) {
		fdc_setup_leds( drive );
		return;
	}

	fdc_status[drive].ready = 1;

	setup_fdc_buffer( drive );
	start_rev_timer( drive );
}

static void fdc_control_w( int data ) {
	int i;
	int change;
	static int fdc_last_control;

	change = fdc_last_control ^ data;

	fdc_last_control = data;

	if ( change & 0x78 ) { /* selection change */
		if ( change & 0x40 )
			fdc_status[3].sel = ( data & 0x40 ) == 0;

		if ( change & 0x20 )
			fdc_status[2].sel = ( data & 0x20 ) == 0;

		if ( change & 0x10 )
			fdc_status[1].sel = ( data & 0x10 ) == 0;

		if ( change & 0x08 )
			fdc_status[0].sel = ( data & 0x08 ) == 0;

		/* if selection changed, make sure we recheck current state of motor, side and dir */
		change |= 0x86;
	}

	for( i = 0; i < 4; i++ ) {
		/* process drives that are selected */
		if ( fdc_status[i].sel ) {

			if ( change & 0x80 ) { /* drive motor change */
				fdc_status[i].motor_on = ( data & 0x80 ) == 0;

				fdc_setup_leds( i );

				if ( fdc_status[i].motor_on ) { /* on? */
					if ( fdc_status[i].ready == 0 ) { /* Was it off? */
						if ( fdc_status[i].spin_timer )
							timer_remove( fdc_status[i].spin_timer );
						fdc_status[i].spin_timer = timer_set( TIME_IN_MSEC( 250 ), i, spin_proc ); /* 250ms to spin up */
					}
				} else { /* or off? */
					if ( fdc_status[i].spin_timer ) {
						timer_remove( fdc_status[i].spin_timer );
						fdc_status[i].spin_timer = 0;
					}

					if ( fdc_status[i].ready == 1 ) { /* Was it on? */
						fdc_status[i].ready = 0; /* Shut it down immediately */
						stop_rev_timer( i );
					}
				}
			}

			if ( change & 0x04 ) { /* side change */
				fdc_status[i].side = ( data & 0x04 ) == 0;
					setup_fdc_buffer( i );
			}

			if ( change & 0x02 ) { /* dir change */
				fdc_status[i].dir = ( data & 0x02 ) != 0;
			}

			if ( change & 0x01 ) { /* step pulse */
				fdc_status[i].step = ( data & 0x01 ) == 0;

				if ( fdc_status[i].step ) { /* Step pulse */
					if ( fdc_status[i].dir ) { /* move inward */
						fdc_status[i].cyl--;
						if ( fdc_status[i].cyl < 0 ) {
							if ( errorlog )
								fprintf( errorlog, "FDC: going for track < 0!\n" );
							fdc_status[i].cyl = 0;
						}
						setup_fdc_buffer( i );
					} else { /* move outward */
						fdc_status[i].cyl++;
						if ( fdc_status[i].cyl > 79 ) {
							if ( errorlog )
								fprintf( errorlog, "FDC: going for track > 79!\n" );
							fdc_status[i].cyl = 79;
						}
						setup_fdc_buffer( i );
					}

					fdc_status[i].track0 = ( fdc_status[i].cyl == 0 );
				}
			}
		}
	}
}

static int fdc_status_r( void ) {
	int i, drive, ret;

	drive = -1;

	for ( i = 0; i < 4; i++ ) {
		if ( fdc_status[i].sel ) {
			drive = i;
			break;
		}
	}

	if ( drive == -1 ) /* No drive selected! */
		return 0x3c;

	ret = fdc_status[drive].ready << 5;
	ret |= fdc_status[drive].track0 << 4;
	ret |= fdc_status[drive].wprot << 3;
	ret |= fdc_status[drive].disk_changed << 2;
	ret ^= 0xff;
	ret &= 0x3c;

	return ret;
}

/***************************************************************************

	8520 CIA emulation

***************************************************************************/

/* required prototype */
static void cia_fire_timer( int cia, int timer );

typedef struct {
	unsigned char 	ddra;
	unsigned char 	ddrb;
	int	(*portA_read)( void );
	int	(*portB_read)( void );
	void (*portA_write)( int data );
	void (*portB_write)( int data );
	unsigned char	data_latchA;
	unsigned char	data_latchB;
	/* Timer A */
	unsigned short	timerA_latch;
	unsigned short	timerA_count;
	unsigned char	timerA_mode;
	/* Timer B */
	unsigned short	timerB_latch;
	unsigned short	timerB_count;
	unsigned char	timerB_mode;
	/* Time Of the Day clock (TOD) */
	unsigned long	tod;
	int				tod_running;
	unsigned long	alarm;
	/* Interrupts */
	int				icr;
	int				ics;
	/* MESS timers */
	void			*timerA;
	void			*timerB;
} cia_8520_def;

static cia_8520_def cia_8520[2];

static void cia_timer_proc( int param ) {
	int cia = param >> 8;
	int timer = param & 1;

	if ( timer == 0 )
		cia_8520[cia].timerA = 0;
	else
		cia_8520[cia].timerB = 0;

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA_mode & 0x08 ) { /* One shot */
			cia_8520[cia].ics |= 0x81; /* set status */
			if ( cia_8520[cia].icr & 1 ) {
				if ( cia == 0 ) {
					amiga_custom_w( 0x009c, 0x8008 );
				} else {
					amiga_custom_w( 0x009c, 0xa000 );
				}
			}
			cia_8520[cia].timerA_count = cia_8520[cia].timerA_latch; /* Reload the timer */
			cia_8520[cia].timerA_mode &= 0xfe; /* Shut it down */
		} else { /* Continuous */
			cia_8520[cia].ics |= 0x81; /* set status */
			if ( cia_8520[cia].icr & 1 ) {
				if ( cia == 0 ) {
					amiga_custom_w( 0x009c, 0x8008 );
				} else {
					amiga_custom_w( 0x009c, 0xa000 );
				}
			}
			cia_8520[cia].timerA_count = cia_8520[cia].timerA_latch; /* Reload the timer */
			cia_fire_timer( cia, 0 ); /* keep going */
		}
	} else {
		if ( cia_8520[cia].timerB_mode & 0x08 ) { /* One shot */
			cia_8520[cia].ics |= 0x82; /* set status */
			if ( cia_8520[cia].icr & 2 ) {
				if ( cia == 0 ) {
					amiga_custom_w( 0x009c, 0x8008 );
				} else {
					amiga_custom_w( 0x009c, 0xa000 );
				}
			}
			cia_8520[cia].timerB_count = cia_8520[cia].timerB_latch; /* Reload the timer */
			cia_8520[cia].timerB_mode &= 0xfe; /* Shut it down */
		} else { /* Continuous */
			cia_8520[cia].ics |= 0x82; /* set status */
			if ( cia_8520[cia].icr & 2 ) {
				if ( cia == 0 ) {
					amiga_custom_w( 0x009c, 0x8008 );
				} else {
					amiga_custom_w( 0x009c, 0xa000 );
				}
			}
			cia_8520[cia].timerB_count = cia_8520[cia].timerB_latch; /* Reload the timer */
			cia_fire_timer( cia, 1 ); /* keep going */
		}
	}
}

static int cia_get_count( int cia, int timer ) {
	int time;

	/* 715909 Hz for NTSC, 709379 for PAL */

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA )
			time = cia_8520[cia].timerA_count - ( int )( timer_timeelapsed( cia_8520[cia].timerA ) / TIME_IN_HZ( 715909 ) );
		else
			time = cia_8520[cia].timerA_count;
	} else {
		if ( cia_8520[cia].timerB )
			time = cia_8520[cia].timerB_count - ( int )( timer_timeelapsed( cia_8520[cia].timerB ) / TIME_IN_HZ( 715909 ) );
		else
			time = cia_8520[cia].timerB_count;
	}

	return time;
}

static void cia_stop_timer( int cia, int timer ) {

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA )
			timer_remove( cia_8520[cia].timerA );
		cia_8520[cia].timerA = 0;
	} else {
		if ( cia_8520[cia].timerB )
			timer_remove( cia_8520[cia].timerB );
		cia_8520[cia].timerB = 0;
	}
}

static void cia_fire_timer( int cia, int timer ) {

	/* 715909 Hz for NTSC, 709379 for PAL */

	if ( timer == 0 ) {
		if ( cia_8520[cia].timerA == 0 )
			cia_8520[cia].timerA = timer_set( cia_8520[cia].timerA_count * TIME_IN_HZ( 715909 ), ( cia << 8 ) | timer, cia_timer_proc );
	} else {
		if ( cia_8520[cia].timerB == 0 )
			cia_8520[cia].timerB = timer_set( cia_8520[cia].timerB_count * TIME_IN_HZ( 715909 ), ( cia << 8 ) | timer, cia_timer_proc );
	}
}

/* Update TOD on CIA A */
static void cia_vblank_update( void ) {
	if ( cia_8520[0].tod_running ) {
		cia_8520[0].tod++;
		if ( cia_8520[0].tod == cia_8520[0].alarm ) {
			cia_8520[0].ics |= 0x84;
			if ( cia_8520[0].icr & 0x04 ) {
				amiga_custom_w( 0x009c, 0x8008 );
			}
		}
	}
}

/* Update TOD on CIA B */
static void cia_hblank_update( void ) {
	if ( cia_8520[1].tod_running ) {
		cia_8520[1].tod++;
		if ( cia_8520[1].tod == cia_8520[1].alarm ) {
			cia_8520[1].ics |= 0x84;
			if ( cia_8520[1].icr & 0x04 ) {
				amiga_custom_w( 0x009c, 0xa000 );
			}
		}
	}
}

/* Issue a index pulse when a disk revolution completes */
static void cia_issue_index( void ) {
	cia_8520[1].ics |= 0x90;
	if ( cia_8520[1].icr & 0x10 ) {
		amiga_custom_w( 0x009c, 0xa000 );
	}
}

static int cia_0_portA_r( void ) {
	int ret = fdc_status_r() | 0xc0;

	if ( osd_is_key_pressed( KEYCODE_Z ) )
		ret &= ~0x40;

	if ( osd_is_key_pressed( KEYCODE_X ) )
		ret &= ~0x80;

	return ret; /* I should add the mouse button 1 and 0 later on */
}

static int cia_0_portB_r( void ) {
	if ( errorlog )
		fprintf( errorlog, "Program read from the parallel port\n" );
	return 0;
}

static void cia_0_portA_w( int data ) {
	osd_led_w( 0, ( data & 2 ) ? 0 : 1 ); /* bit 2 = Power Led */
}

static void cia_0_portB_w( int data ) {
	if ( errorlog )
		fprintf( errorlog, "Program wrote to the parallel port\n" );
}

static int cia_1_portA_r( void ) {
	return 0;
}

static int cia_1_portB_r( void ) {
	return 0;
}

static void cia_1_portA_w( int data ) {
}

static void cia_1_portB_w( int data ) {
	fdc_control_w( data );
}

static void cia_init( void ) {
	int i;

	/* Initialize port handlers */
	cia_8520[0].portA_read = cia_0_portA_r;
	cia_8520[0].portB_read = cia_0_portB_r;
	cia_8520[0].portA_write = cia_0_portA_w;
	cia_8520[0].portB_write = cia_0_portB_w;
	cia_8520[1].portA_read = cia_1_portA_r;
	cia_8520[1].portB_read = cia_1_portB_r;
	cia_8520[1].portA_write = cia_1_portA_w;
	cia_8520[1].portB_write = cia_1_portB_w;

	/* Initialize data direction registers */
	cia_8520[0].ddra = 0x03;
	cia_8520[0].ddrb = 0x00; /* undefined */
	cia_8520[1].ddra = 0xff;
	cia_8520[1].ddrb = 0xff;

	/* Initialize timer's, TOD's and interrupts */
	for ( i = 0; i < 2; i++ ) {
		cia_8520[i].data_latchA = 0;
		cia_8520[i].data_latchB = 0;
		cia_8520[i].timerA_latch = 0xffff;
		cia_8520[i].timerA_count = 0;
		cia_8520[i].timerA_mode = 0;
		cia_8520[i].timerB_latch = 0xffff;
		cia_8520[i].timerB_count = 0;
		cia_8520[i].timerB_mode = 0;
		cia_8520[i].tod = 0;
		cia_8520[i].tod_running = 0;
		cia_8520[i].alarm = 0;
		cia_8520[i].icr = 0;
		cia_8520[i].ics = 0;
		if ( cia_8520[i].timerA )
			timer_remove( cia_8520[i].timerA );
		cia_8520[i].timerA = 0;
		if ( cia_8520[i].timerB )
			timer_remove( cia_8520[i].timerA );
		cia_8520[i].timerB = 0;
	}
}

int amiga_cia_r( int offs ) {
	int cia_sel = 1, mask, data;

	if ( offs >= 0x1000 )
		cia_sel = 0;

	switch ( offs & 0xffe ) {
		case 0x000:
			data = (*cia_8520[cia_sel].portA_read)();
			mask = ~( cia_8520[cia_sel].ddra );
			data &= mask;
			mask = cia_8520[cia_sel].ddra & cia_8520[cia_sel].data_latchA;
			data |= mask;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x100:
			data = (*cia_8520[cia_sel].portB_read)();
			mask = ~( cia_8520[cia_sel].ddrb );
			data &= mask;
			mask = cia_8520[cia_sel].ddrb & cia_8520[cia_sel].data_latchB;
			data |= mask;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x200:
			data = cia_8520[cia_sel].ddra;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x300:
			data = cia_8520[cia_sel].ddrb;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x400:
			data = cia_get_count( cia_sel, 0 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x500:
			data = ( cia_get_count( cia_sel, 0 ) >> 8 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x600:
			data = cia_get_count( cia_sel, 1 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x700:
			data = ( cia_get_count( cia_sel, 1 ) >> 8 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x800:
			data = cia_8520[cia_sel].tod & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0x900:
			data = ( cia_8520[cia_sel].tod >> 8 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xa00:
			data = ( cia_8520[cia_sel].tod >> 16 ) & 0xff;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xd00:
			data = cia_8520[cia_sel].ics;
			cia_8520[cia_sel].ics = 0; /* clear on read */
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xe00:
			data = cia_8520[cia_sel].timerA_mode;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;

		case 0xf00:
			data = cia_8520[cia_sel].timerB_mode;
			return ( cia_sel == 0 ) ? data : ( data << 8 );
		break;
	}

#if 0
	if ( errorlog )
		fprintf( errorlog, "PC = %06x - Read from CIA %01x\n", cpu_getpc(), cia_sel );
#endif

	return 0;
}

void amiga_cia_w( int offs, int data ) {
	int cia_sel = 1, mask;

	if ( offs >= 0x1000 )
		cia_sel = 0;
	else
		data >>= 8;

	data &= 0xff;

	switch ( offs & 0xffe ) {
		case 0x000:
			mask = cia_8520[cia_sel].ddra;
			cia_8520[cia_sel].data_latchA = data;
			(*cia_8520[cia_sel].portA_write)( data & mask );
		break;

		case 0x100:
			mask = cia_8520[cia_sel].ddrb;
			cia_8520[cia_sel].data_latchB = data;
			(*cia_8520[cia_sel].portB_write)( data & mask );
		break;

		case 0x200:
			cia_8520[cia_sel].ddra = data;
		break;

		case 0x300:
			cia_8520[cia_sel].ddrb = data;
		break;

		case 0x400:
			cia_8520[cia_sel].timerA_latch &= 0xff00;
			cia_8520[cia_sel].timerA_latch |= data;
		break;

		case 0x500:
			cia_8520[cia_sel].timerA_latch &= 0x00ff;
			cia_8520[cia_sel].timerA_latch |= data << 8;

			/* If it's one shot, start the timer */
			if ( cia_8520[cia_sel].timerA_mode & 0x08 ) {
				cia_8520[cia_sel].timerA_count = cia_8520[cia_sel].timerA_latch;
				cia_8520[cia_sel].timerA_mode |= 1;
				cia_fire_timer( cia_sel, 0 );
			}
		break;

		case 0x600:
			cia_8520[cia_sel].timerB_latch &= 0xff00;
			cia_8520[cia_sel].timerB_latch |= data;
		break;

		case 0x700:
			cia_8520[cia_sel].timerB_latch &= 0x00ff;
			cia_8520[cia_sel].timerB_latch |= data << 8;

			/* If it's one shot, start the timer */
			if ( cia_8520[cia_sel].timerB_mode & 0x08 ) {
				cia_8520[cia_sel].timerB_count = cia_8520[cia_sel].timerB_latch;
				cia_8520[cia_sel].timerB_mode |= 1;
				cia_fire_timer( cia_sel, 1 );
			}
		break;

		case 0x800:
			if ( cia_8520[cia_sel].timerB_mode & 0x80 ) { /* set alarm? */
				cia_8520[cia_sel].alarm &= 0xffff00;
				cia_8520[cia_sel].alarm |= data;
			} else {
				cia_8520[cia_sel].tod &= 0xffff00;
				cia_8520[cia_sel].tod |= data;
				cia_8520[cia_sel].tod_running = 1;
			}
		break;

		case 0x900:
			if ( cia_8520[cia_sel].timerB_mode & 0x80 ) { /* set alarm? */
				cia_8520[cia_sel].alarm &= 0xff00ff;
				cia_8520[cia_sel].alarm |= data << 8;
			} else {
				cia_8520[cia_sel].tod &= 0xff00ff;
				cia_8520[cia_sel].tod |= data << 8;
				cia_8520[cia_sel].tod_running = 0;
			}
		break;

		case 0xa00:
			if ( cia_8520[cia_sel].timerB_mode & 0x80 ) { /* set alarm? */
				cia_8520[cia_sel].alarm &= 0x00ffff;
				cia_8520[cia_sel].alarm |= data << 16;
			} else {
				cia_8520[cia_sel].tod &= 0x00ffff;
				cia_8520[cia_sel].tod |= data << 16;
				cia_8520[cia_sel].tod_running = 0;
			}
		break;

		case 0xd00:
			if ( data & 0x80 ) /* set */
				cia_8520[cia_sel].icr |= ( data & 0x7f );
			else /* clear */
				cia_8520[cia_sel].icr &= ~( data & 0x7f );
		break;

		case 0xe00:
			if ( data & 0x10 ) { /* force load */
				cia_8520[cia_sel].timerA_count = cia_8520[cia_sel].timerA_latch;
				cia_stop_timer( cia_sel, 0 );
			}

			if ( data & 0x01 )
				cia_fire_timer( cia_sel, 0 );
			else
				cia_stop_timer( cia_sel, 0 );

			cia_8520[cia_sel].timerA_mode = data & 0xef;
		break;

		case 0xf00:
			if ( data & 0x10 ) { /* force load */
				cia_8520[cia_sel].timerB_count = cia_8520[cia_sel].timerB_latch;
				cia_stop_timer( cia_sel, 1 );
			}

			if ( data & 0x01 )
				cia_fire_timer( cia_sel, 1 );
			else
				cia_stop_timer( cia_sel, 1 );

			cia_8520[cia_sel].timerB_mode = data & 0xef;
		break;
	}

#if 0
	if ( errorlog )
		fprintf( errorlog, "PC = %06x - Wrote to CIA %01x (%02x)\n", cpu_getpc(), cia_sel, data );
#endif
}

/***************************************************************************

	Custom chips emulation

***************************************************************************/

#if EMULATE_INDEX_PULSE

static void *custom_hblank_counter = 0;

static void custom_hsync_proc( int param ) {

	/* Update CIA B */
	cia_hblank_update();
}
#endif

static void amiga_custom_init( void ) {

#if EMULATE_INDEX_PULSE
	custom_hblank_counter = 0;

	if ( custom_hblank_counter )
		timer_remove( custom_hblank_counter );


	/* cpu_getscanlineperiod() just wont cut it here, it triggers up to 301 lines */
	custom_hblank_counter = timer_pulse( ( TIME_IN_HZ( 60 ) / Machine->drv->screen_height ), 0, custom_hsync_proc );
#endif
}

int amiga_custom_r( int offs ) {

	switch ( offs ) {
		case 0x0002: /* DMACON */
			return custom_regs.DMACON;
		break;

		case 0x0004: /* VPOSR */
			return ( ( cpu_getscanline() >> 8 ) & 1 );
		break;

		case 0x0006: /* VHPOSR */
			{
				int h, v;

				h = ( cpu_gethorzbeampos() >> 1 ); /* in color clocks */
				v = ( cpu_getscanline() & 0xff );

				return ( v << 8 ) | h;
			}
		break;

		case 0x0010: /* ADKCONR */
			return custom_regs.ADKCON;
		break;

		case 0x0016: /* POTGOR */
			custom_regs.POTGOR = custom_regs.POTGO & 0xaa00;
			custom_regs.POTGOR |= 0x0400; /* Right mouse button up */
			custom_regs.POTGOR |= 0x0100; /* Middle mouse button up */
			return custom_regs.POTGOR;
		break;

		case 0x001a: /* DSKBYTR */
			return fdc_get_byte();
		break;


		case 0x001c: /* INTENA */
			return custom_regs.INTENA;
		break;

		case 0x001e: /* INTREQ */
			return custom_regs.INTREQ;
		break;
	}

#if LOG_CUSTOM
	if ( errorlog )
		fprintf( errorlog, "PC = %06x - Read from Custom %04x\n", cpu_getpc(), offs );
#endif
	return 0;
}

#define SETCLR( reg, data ) { \
	if ( data & 0x8000 ) \
		reg |= ( data & 0x7fff ); \
	else \
		reg &= ~( data & 0x7fff ); }

void amiga_custom_w( int offs, int data ) {

	switch ( offs ) {
		case 0x0020: /* DSKPTH */
			custom_regs.DSKPTH = data;
			return;
		break;

		case 0x0022: /* DSKPTL */
			custom_regs.DSKPTL = data;
			return;
		break;

		case 0x0024: /* DSKLEN */
			if ( data & 0x8000 ) {
				if ( custom_regs.DSKLEN & 0x8000 )
					fdc_setup_dma();
			}
			custom_regs.DSKLEN = data;
			return;
		break;

		case 0x002e: /* COPCON */
			custom_regs.COPCON = data & 2;
			return;
		break;

		case 0x0034: /* POTGO */
			custom_regs.POTGO = data;
			return;
		break;

		case 0x0040: /* BLTCON0 */
			custom_regs.BLTCON0 = data;
			return;
		break;

		case 0x0042: /* BLTCON1 */
			custom_regs.BLTCON1 = data;
			return;
		break;

		case 0x0044: /* BLTAFWM */
			custom_regs.BLTAFWM = data;
			return;
		break;

		case 0x0046: /* BLTALWM */
			custom_regs.BLTALWM = data;
			return;
		break;

		case 0x0058: /* BLTSIZE */
			custom_regs.BLTSIZE = data;
			blitter_setup();
			return;
		break;

		case 0x007e: /* DSKSYNC */
			custom_regs.DSKSYNC = data;
			return;
		break;

		case 0x0088: /* COPJMPA */
			copper_setpc( ( custom_regs.COPLCH[0] << 16 ) | custom_regs.COPLCL[0] );
			return;
		break;

		case 0x008a: /* COPJMPB */
			copper_setpc( ( custom_regs.COPLCH[1] << 16 ) | custom_regs.COPLCL[1] );
			return;
		break;

		case 0x008e: /* DIWSTRT */
			custom_regs.DIWSTRT = data;
			return;
		break;

		case 0x0090: /* DIWSTOP */
			custom_regs.DIWSTOP = data;
			return;
		break;

		case 0x0092: /* DDFSTRT */
			custom_regs.DDFSTRT = data & 0xfc;
			/* impose hardware limits ( HRM, page 75 ) */
			if ( custom_regs.DDFSTRT < 0x18 )
				custom_regs.DDFSTRT = 0x18;
			return;
		break;

		case 0x0094: /* DDFSTOP */
			custom_regs.DDFSTOP = data & 0xfc;
			/* impose hardware limits ( HRM, page 75 ) */
			if ( custom_regs.DDFSTOP > 0xd8 )
				custom_regs.DDFSTOP = 0xd8;
			return;
		break;

		case 0x0096: /* DMACON */
			SETCLR( custom_regs.DMACON, data )
			return;
		break;

		case 0x009a: /* INTENA */
			SETCLR( custom_regs.INTENA, data )
			if ( data & 0x8000 )
				trigger_int();
			return;
		break;

		case 0x009c: /* INTREQ */
			SETCLR( custom_regs.INTREQ, data )
			if ( data & 0x8000 )
				trigger_int();
			return;
		break;

		case 0x009e: /* ADKCON */
			SETCLR( custom_regs.ADKCON, data )
			return;
		break;

		case 0x0100: /* BPLCON0 */
			custom_regs.BPLCON0 = data;

			if ( ( custom_regs.BPLCON0 & ( BPLCON0_BPU0 | BPLCON0_BPU1 | BPLCON0_BPU2 ) ) == ( BPLCON0_BPU0 | BPLCON0_BPU1 | BPLCON0_BPU2 ) ) {
				/* planes go from 0 to 6, inclusive */
				if ( errorlog )
					printf( "This game is doing funky planes stuff. (planes > 6)\n" );
				custom_regs.BPLCON0 &= ~BPLCON0_BPU0;
			}

			return;
		break;

		case 0x0108: /* BPL1MOD */
			custom_regs.BPL1MOD = data;
			return;
		break;

		case 0x010a: /* BPL2MOD */
			custom_regs.BPL2MOD = data;
			return;
		break;

		default:
			if ( offs >= 0x48 && offs <= 0x56 ) { /* set Blitter pointer */
				int lo = ( offs & 2 );
				int loc = ( offs - 0x48 ) >> 2;
				int order[4] = { 2, 1, 0, 3 };

				if ( lo )
					custom_regs.BLTxPTL[order[loc]] = ( data & 0xfffe ); /* should be word aligned, we make sure is is */
				else
					custom_regs.BLTxPTH[order[loc]] = ( data & 0x1f );
				return;
			}

			if ( offs >= 0x60 && offs <= 0x66 ) { /* set Blitter modulo */
				int loc = ( offs >> 1 ) & 3;
				int order[4] = { 2, 1, 0, 3 };

				custom_regs.BLTxMOD[order[loc]] = ( signed short )data;
				custom_regs.BLTxMOD[order[loc]] &= ~1; /* strip off lsb */
				return;
			}

			if ( offs >= 0x70 && offs <= 0x74 ) { /* set Blitter data */
				int loc = ( offs >> 1 ) & 3;
				int order[3] = { 2, 1, 0 };

				custom_regs.BLTxDAT[order[loc]] = data;
				return;
			}

			if ( offs >= 0x80 && offs <= 0x86 ) { /* set Copper pointer */
				int lo = ( offs & 2 );
				int loc = ( offs >> 2 ) & 1;

				if ( lo )
					custom_regs.COPLCL[loc] = ( data & 0xfffe ); /* should be word aligned, we make sure it is */
				else
					custom_regs.COPLCH[loc] = ( data & 0x1f );
				return;
			}

			if ( offs >= 0xe0 && offs <= 0xf6 ) { /* set Bitplane pointer */
				int lo = ( offs & 2 );
				int plane = ( offs >> 2 ) & 0x07;

				if ( lo ) {
					custom_regs.BPLPTR[plane] &= 0x001f0000;
					custom_regs.BPLPTR[plane] |= ( data & 0xfffe );
				} else {
					custom_regs.BPLPTR[plane] &= 0x0000ffff;
					custom_regs.BPLPTR[plane] |= ( data & 0x1f ) << 16;
				}
				return;
			}

			if ( offs >= 0x120 && offs <= 0x13e ) { /* set sprite plane pointer */
				int lo = ( offs & 2 );
				int num = ( offs >> 2 ) & 0x07;

				if ( lo )
					custom_regs.SPRxPTL[num] = ( data & 0xfffe ); /* should be word aligned, we make sure is is */
				else
					custom_regs.SPRxPTH[num] = ( data & 0x1f );
				return;
			}

			if ( offs >= 0x180 && offs <= 0x1be ) { /* set color */
				int color = ( offs - 0x180 ) >> 1;

				custom_regs.COLOR[color] = data;
				return;
			}
		break;
	}

#if LOG_CUSTOM
	if ( errorlog )
		fprintf( errorlog, "PC = %06x - Wrote to Custom %04x (%04x)\n", cpu_getpc(), offs, data );
#endif
}

/***************************************************************************

	Interrupt handling

***************************************************************************/

int amiga_vblank_irq( void ) {

	/* Update TOD on CIA A */
	cia_vblank_update();

	amiga_custom_w( 0x009c, 0x8020 );

	return ignore_interrupt();
}

/***************************************************************************

	Init Machine

***************************************************************************/

void amiga_init_machine( void ) {
	/* Fake our reset pointer */
	/* This is done with a hardware overlay from the 8520 CIA A in the real hardware */

	unsigned char *RAM = memory_region(REGION_CPU1);

	memcpy( &RAM[0x000004], &RAM[0xf80004], 4 );

	/* Initialize the CIA's */
	cia_init();

	/* Initialize the Floppy */
	fdc_init();

	/* Initialize the Custom chips */
	amiga_custom_init();
}

