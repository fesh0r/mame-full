#include "driver.h"
#include "vidhrdw/vector.h"
#include "mess/machine/6522via.h"
#include "cpu/m6809/m6809.h"

#define BLACK 0x00
#define RED   0x04
#define GREEN 0x02
#define BLUE  0x01
#define WHITE RED|GREEN|BLUE
#define DARKRED 0x08

#define PORTB 0
#define PORTA 1

/* from vidhrdw/vectrex.c */
extern void vector_add_point_stereo (int x, int y, int color, int intensity);
extern void (*vector_add_point_function) (int, int, int, int);

/*********************************************************************
  Global variables
 *********************************************************************/
unsigned char *vectrex_ram;        /* RAM at 0xc800 -- 0xcbff mirrored at 0xcc00 -- 0xcfff */
unsigned char vectrex_via_out[2];
int vectrex_beam_color = WHITE;    /* the color of the vectrex beam */
int vectrex_imager_status = 0;     /* 0 = off, 1 = right eye, 2 = left eye */
int vectrex_refresh_with_T2;       /* For all known games it's OK to do the screen refresh when T2 expires.
				    * This behaviour can be turned off via dipswitch settings */

/*********************************************************************
  Local variables
 *********************************************************************/

/* Colors for right and left eye */
static int imager_colors[6] = {WHITE,WHITE,WHITE,WHITE,WHITE,WHITE};

/* Startpoint (in rad) of the three colors */
/* Tanks to Chris Salomon for the values */
static const double narrow_escape_angles[3] = {0,0.15277778, 0.34444444};
static const double minestorm_3d_angles[3] = {0,0.16111111, 0.18888888};
static const double crazy_coaster_angles[3] = {0,0.15277778, 0.34444444};
static const double unknown_game_angles[3] = {0,0.16666666, 0.33333333};
static const double *vectrex_imager_angles = unknown_game_angles;
static unsigned char vectrex_imager_pinlevel=0x00;
static double imager_wheel_time = 0;

/*********************************************************************
  ROM load and id functions
 *********************************************************************/
int vectrex_load_rom (int id)
{
	const char *name = device_filename(IO_CARTSLOT,id);
    FILE *cartfile = 0;

	/* Set the whole cart ROM area to 1. This is needed to work around a bug (?)
	 * in Minestorm where the exec-rom attempts to access a vector list here.
	 * 1 signals the end of the vector list.
	 */
	memset (memory_region(REGION_CPU1), 1, 0x8000);

	cartfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0);
	if (cartfile)
	{
		osd_fread (cartfile, memory_region(REGION_CPU1), 0x8000);
		osd_fclose (cartfile);
	}

	vectrex_imager_angles = unknown_game_angles;

	if (name)
	{
		/* A bit ugly but somehow we need to know which 3D game is running */
		/* A better way would be to do this by CRC */
		if (!strcmp(name,"narrow.bin"))
			vectrex_imager_angles = narrow_escape_angles;
		if (!strcmp(name,"crazy.bin"))
			vectrex_imager_angles = crazy_coaster_angles;
		if (!strcmp(name,"mine3.bin"))
			vectrex_imager_angles = minestorm_3d_angles;
	}

	return INIT_OK;
}

int vectrex_id_rom (int id)
{
	const char *gamename = device_filename(IO_CARTSLOT,id);
    void *romfile;
	char magic[5];

	/* If no file was specified, don't bother */
	if (!gamename || strlen(gamename)==0)
		return 1;

	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_R, 0)))
		return 0;

	/* Verify the file is accepted by the Vectrex bios */
	osd_fread (romfile, magic, 5);
	osd_fclose (romfile);

	if (!memcmp(magic,"g GCE", 5))
		return 1;
	else
		return 0;
}

/*********************************************************************
  Vectrex memory handler
 *********************************************************************/
int vectrex_ram_r (int offset)
{
	return (vectrex_ram[offset]);
}

void vectrex_ram_w (int offset, int data)
{
	vectrex_ram[offset] = data;
}

/*********************************************************************
  Vectrex configuration (mainly 3D Imager)
 *********************************************************************/
void vectrex_configuration(void)
{
	unsigned char in2 = input_port_2_r (0);

	/* Vectrex 'dipswitch' configuration */
	vectrex_refresh_with_T2 = input_port_3_r (0) & 0x01;

	/* Imager control */
	if (in2 & 0x01) /* Imager enabled */
	{
		if (vectrex_imager_status == 0)
			vectrex_imager_status = in2 & 0x01;

		vector_add_point_function = in2 & 0x02 ? vector_add_point_stereo: vector_add_point;

		switch ((in2>>2) & 0x07)
		{
		case 0x00:
			imager_colors[0]=imager_colors[1]=imager_colors[2]=BLACK;
			break;
		case 0x01:
			imager_colors[0]=imager_colors[1]=imager_colors[2]=DARKRED;
			break;
		case 0x02:
			imager_colors[0]=imager_colors[1]=imager_colors[2]=GREEN;
			break;
		case 0x03:
			imager_colors[0]=imager_colors[1]=imager_colors[2]=BLUE;
			break;
		case 0x04:
			/* mine3 has a different color sequence */
			if (vectrex_imager_angles == minestorm_3d_angles)
			{
				imager_colors[0]=RED;
				imager_colors[1]=GREEN;
			}
			else
			{
				imager_colors[0]=GREEN;
				imager_colors[1]=RED;
			}
			imager_colors[2]=BLUE;
			break;
		}

		switch ((in2>>5) & 0x07)
		{
		case 0x00:
			imager_colors[3]=imager_colors[4]=imager_colors[5]=BLACK;
			break;
		case 0x01:
			imager_colors[3]=imager_colors[4]=imager_colors[5]=DARKRED;
			break;
		case 0x02:
			imager_colors[3]=imager_colors[4]=imager_colors[5]=GREEN;
			break;
		case 0x03:
			imager_colors[3]=imager_colors[4]=imager_colors[5]=BLUE;
			break;
		case 0x04:
			if (vectrex_imager_angles == minestorm_3d_angles)
			{
				imager_colors[3]=RED;
				imager_colors[4]=GREEN;
			}
			else
			{
				imager_colors[3]=GREEN;
				imager_colors[4]=RED;
			}
			imager_colors[5]=BLUE;
			break;
		}

	}
	else
	{
		vector_add_point_function = vector_add_point;
		vectrex_beam_color = WHITE;
		imager_colors[0]=imager_colors[1]=imager_colors[2]=imager_colors[3]=imager_colors[4]=imager_colors[5]=WHITE;
	}

}

/*********************************************************************
  VIA interface functions
 *********************************************************************/
void v_via_irq (int level)
{
	static int old_level;
	if (level != old_level)
	{
		cpu_set_irq_line(0, M6809_IRQ_LINE, level);
		old_level = level;
	}
}

int v_via_pb_r (int offset)
{
	/* Joystick */
	if (vectrex_via_out[PORTA] & 0x80)
	{
		if ( input_port_1_r(0) & (0x02<<(vectrex_via_out[PORTB] & 0x6)))
			vectrex_via_out[PORTB] &= ~0x20;
		else
			vectrex_via_out[PORTB] |= 0x20;
	}
	else
	{
		if ( input_port_1_r(0) & (0x01<<(vectrex_via_out[PORTB] & 0x6)))
			vectrex_via_out[PORTB] |= 0x20;
		else
			vectrex_via_out[PORTB] &= ~0x20;
	}
	return vectrex_via_out[PORTB];
}

int v_via_pa_r (int offset)
{
	if ((!(vectrex_via_out[PORTB] & 0x10)) && (vectrex_via_out[PORTB] & 0x08))
		/* BDIR inactive, we can read the PSG. BC1 has to be active. */
	{
		vectrex_via_out[PORTA] = AY8910_read_port_0_r (0)
			& ~(vectrex_imager_pinlevel & 0x80);
		vectrex_imager_pinlevel &= ~0x80;
	}
	return vectrex_via_out[PORTA];
}

int s1_via_pb_r (int offset)
{
	return (vectrex_via_out[PORTB] & ~0x40) | ((input_port_1_r(0) & 0x1)<<6);
}

/*********************************************************************
  3D Imager support
 *********************************************************************/
static void vectrex_imager_change_color (int i)
{
	vectrex_beam_color = imager_colors[i];
}

static void vectrex_imager_right_eye (int param)
{
	vectrex_imager_status = 1;
	vectrex_beam_color = imager_colors[2];
	timer_set (imager_wheel_time*vectrex_imager_angles[1], 0, vectrex_imager_change_color);
	timer_set (imager_wheel_time*vectrex_imager_angles[2], 1, vectrex_imager_change_color);
}

void vectrex_imager_left_eye (double time)
{
	imager_wheel_time = time;
	via_0_ca1_w (0, 1);
	via_0_ca1_w (0, 0);
	vectrex_imager_pinlevel |= 0x80;

	vectrex_imager_status = 2;
	vectrex_beam_color = imager_colors[5];
	timer_set (time*vectrex_imager_angles[1], 3, vectrex_imager_change_color);
	timer_set (time*vectrex_imager_angles[2], 4, vectrex_imager_change_color);
	timer_set (time/2, 0, vectrex_imager_right_eye);
}

