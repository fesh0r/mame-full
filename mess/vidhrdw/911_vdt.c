/*
	TI 911 VDT core.  To be operated with the TI 990 line of computers (can be connected to
	any model, as communication uses the CRU bus).

	Raphael Nabet 2002
*/


#include "drawgfx.h"

#include "911_vdt.h"


#include "911_chr.h"


#define MAX_VDT 1

static struct GfxLayout fontlayout_7bit =
{
	7, 10,			/* 7*10 characters */
	128,			/* 128 characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ /*0,*/ 1, 2, 3, 4, 5, 6, 7 }, 			/* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8 			/* every char takes 10 consecutive bytes */
};

static struct GfxLayout fontlayout_8bit =
{
	7, 10,			/* 7*10 characters */
	128,			/* 128 characters */
	1,				/* 1 bit per pixel */
	{ 0 },
	{ 1, 2, 3, 4, 5, 6, 7 }, 				/* straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8 			/* every char takes 10 consecutive bytes */
};

struct GfxDecodeInfo vdt911_gfxdecodeinfo[] =
{	/* array must use same order as vdt911_model_t!!! */
	/* US */
	{ vdt911_chr_region, vdt911_US_chr_offset, &fontlayout_7bit, 0, 2 },
	/* UK */
	{ vdt911_chr_region, vdt911_UK_chr_offset, &fontlayout_7bit, 0, 2 },
	/* French */
	{ vdt911_chr_region, vdt911_US_chr_offset, &fontlayout_7bit, 0, 2 },
	/* German */
	{ vdt911_chr_region, vdt911_german_chr_offset, &fontlayout_7bit, 0, 2 },
	/* Swedish */
	{ vdt911_chr_region, vdt911_swedish_chr_offset, &fontlayout_7bit, 0, 2 },
	/* Norwegian */
	{ vdt911_chr_region, vdt911_norwegian_chr_offset, &fontlayout_7bit, 0, 2 },
	/* Japanese */
	{ vdt911_chr_region, vdt911_japanese_chr_offset, &fontlayout_8bit, 0, 2 },
	/* Arabic */
	/*{ vdt911_chr_region, vdt911_arabic_chr_offset, &fontlayout_8bit, 0, 2 },*/
	/* FrenchWP */
	{ vdt911_chr_region, vdt911_frenchWP_chr_offset, &fontlayout_7bit, 0, 2 },
	{ -1 }	/* end of array */
};

unsigned char vdt911_palette[] =
{
	0x00,0x00,0x00,	/* black */
	0xC0,0xC0,0xC0,	/* low intensity */
	0xFF,0xFF,0xFF	/* high intensity */
};

unsigned short vdt911_colortable[] =
{
	0, 2,
	0, 1
};

typedef struct vdt_t
{
	vdt911_screen_size_t screen_size;
	vdt911_model_t model;
	void (*int_callback)(int state);

	UINT8 data_reg;
	UINT8 display_RAM[2048];

	unsigned int cursor_address;
	unsigned int cursor_address_mask; /* 1023 for 960-char model, 2047 for 1920-char model */

	UINT8 keyboard_data;
	unsigned int keyboard_data_ready : 1;
	unsigned int keyboard_interrupt_enable : 1;

	unsigned int dual_intensity_enable : 1;

	unsigned int word_select : 1;
	unsigned int previous_word_select : 1;
} vdt_t;

static vdt_t vdt[MAX_VDT];


void vdt911_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *dummy)
{
	if ((sizeof(vdt911_palette) != vdt911_palette_size)
		|| (sizeof(vdt911_colortable) != (vdt911_colortable_size*2)))
	{
		logerror("Major internal error\n");
		exit(1);
	}

	memcpy(palette, & vdt911_palette, sizeof(vdt911_palette));
	memcpy(colortable, & vdt911_colortable, sizeof(vdt911_colortable));
}

static void copy_character_matrix_array(const UINT8 char_array[128][10], UINT8 *dest)
{
	int i, j;

	for (i=0; i<128; i++)
		for (j=0; j<10; j++)		
			*(dest++) = char_array[i][j];
}

/*
	Initialize the 911 vdt core
*/
void vdt911_init(void)
{
	UINT8 *base;

	memset(vdt, 0, sizeof(vdt));

	/* set up US character definitions*/
	base = memory_region(vdt911_chr_region)+vdt911_US_chr_offset;
	copy_character_matrix_array(US_character_set, base);

	/* todo: initialize other characters sets... */
}

/*
	Initialize one 911 vdt controller/terminal
*/
int vdt911_init_term(int unit, const vdt911_init_params_t *params)
{
	vdt[unit].screen_size = params->screen_size;
	vdt[unit].model = params->model;
	vdt[unit].int_callback = params->int_callback;

	if (vdt[unit].screen_size == char_960)
		vdt[unit].cursor_address_mask = 0x3ff;	/* 1kb of RAM */
	else
		vdt[unit].cursor_address_mask = 0x7ff;	/* 2 kb of RAM */

	return 0;
}

/*
	CRU interface read
*/
int vdt911_cru_r(int offset, int unit)
{
	int reply;

	offset &= 0x1;

	if (! vdt[unit].word_select)
	{	/* select word 0 */
		switch (offset)
		{
		case 0:
			reply = vdt[unit].data_reg;
			break;

		case 1:
			reply = vdt[unit].keyboard_data & 0x7f;
			if (vdt[unit].keyboard_data_ready)
				reply |= 0x80;
			break;
		}
	}
	else
	{	/* select word 1 */
		switch (offset)
		{
		case 0:
			reply = vdt[unit].cursor_address & 0xff;
			break;

		case 1:
			reply = (vdt[unit].cursor_address >> 8) & 0x07;
			if (vdt[unit].keyboard_data & 0x80)
				reply |= 0x08;
			/*if (! vdt[unit].terminal_ready)
				reply |= 0x10;*/
			if (vdt[unit].previous_word_select)
				reply |= 0x20;
			/*if (vdt[unit].keyboard_parity_error)
				reply |= 0x40;*/
			if (vdt[unit].keyboard_data_ready)
				reply |= 0x80;
			break;
		}
	}

	return reply;
}

/*
	CRU interface write
*/
void vdt911_cru_w(int offset, int data, int unit)
{
	offset &= 0xf;

	if (! vdt[unit].word_select)
	{	/* select word 0 */
		switch (offset)
		{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			/* display memory write data */
			if (data)
				vdt[unit].data_reg |= (1 << offset);
			else
				vdt[unit].data_reg &= ~ (1 << offset);
			break;

		case 0x8:
			/* write data strobe */
			 vdt[unit].display_RAM[vdt[unit].cursor_address] = vdt[unit].data_reg;
			break;

		case 0x9:
			/* test mode */
			/* ... */
			break;

		case 0xa:
			/* cursor move */
			if (data)
				vdt[unit].cursor_address--;
			else
				vdt[unit].cursor_address++;
			vdt[unit].cursor_address &= vdt[unit].cursor_address_mask;

			vdt[unit].data_reg = vdt[unit].display_RAM[vdt[unit].cursor_address];
			break;

		case 0xb:
			/* blinking cursor enable */
			/* ... */
			break;

		case 0xc:
			/* keyboard interrupt enable */
			vdt[unit].keyboard_interrupt_enable = data;
			(*vdt[unit].int_callback)(vdt[unit].keyboard_interrupt_enable && vdt[unit].keyboard_data_ready);
			break;

		case 0xd:
			/* dual intensity enable */
			vdt[unit].dual_intensity_enable = data;
			break;

		case 0xe:
			/* display enable */
			/* ... */
			break;

		case 0xf:
			/* select word */
			vdt[unit].previous_word_select = vdt[unit].word_select;
			vdt[unit].word_select = data;
			break;
		}
	}
	else
	{	/* select word 1 */
		switch (offset)
		{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
		case 0x8:
		case 0x9:
		case 0xa:
			/* cursor address */
			if (data)
				vdt[unit].cursor_address |= (1 << offset);
			else
				vdt[unit].cursor_address &= ~ (1 << offset);
			vdt[unit].cursor_address &= vdt[unit].cursor_address_mask;

			vdt[unit].data_reg = vdt[unit].display_RAM[vdt[unit].cursor_address];
			break;

		case 0xb:
			/* not used */
			break;

		case 0xc:
			/* display cursor */
			/* ... */
			break;

		case 0xd:
			/* keyboard acknowledge */
			if (vdt[unit].keyboard_data_ready)
			{
				vdt[unit].keyboard_data_ready = 0;
				if (vdt[unit].keyboard_interrupt_enable)
					(*vdt[unit].int_callback)(0);
			}
			/*vdt[unit].keyboard_parity_error = 0;*/
			break;

		case 0xe:
			/* beep enable strobe */
			/* ... */
			break;

		case 0xf:
			/* select word */
			vdt[unit].previous_word_select = vdt[unit].word_select;
			vdt[unit].word_select = data;
			break;
		}
	}
}

READ16_HANDLER(vdt911_0_cru_r)
{
	return vdt911_cru_r(offset, 0);
}

WRITE16_HANDLER(vdt911_0_cru_w)
{
	vdt911_cru_w(offset, data, 0);
}


void vdt911_refresh(struct mame_bitmap *bitmap, int full_refresh, int unit, int x, int y)
{
	const struct GfxElement *gfx = Machine->gfx[unit];
	int height = (vdt[unit].screen_size == char_960) ? 12 : /*25*/24;
	int use_8bit_charcode = (vdt[unit].model == vdt911_model_Japanese) /*|| (vdt[unit].model == vdt911_model_Arabic)*/;
	int address = 0;
	int i, j;
	int cur_char;
	int color;

	/*if (use_8bit_charcode)
		color = vdt[unit].dual_intensity_enable ? 1 : 0;*/

	for (i=0; i<height; i++)
	{
		for (j=0; j<80; j++)
		{
			cur_char = vdt[unit].display_RAM[address++];
			/* does dual intensity work with 8-bit character set? */
			color = (vdt[unit].dual_intensity_enable && (cur_char & 0x80)) ? 1 : 0;
			if (! use_8bit_charcode)
				cur_char &= 0x7f;

			drawgfx(bitmap, gfx, cur_char, color, 0, 0,
					x+j*7, y+i*10, &Machine->visible_area, TRANSPARENCY_NONE, 0);
		}
	}
}


static US_key_translate[4][90] =
{
	{	/* lower case */
		0x92,
		0x93,
		0x94,
		0x95,
		0x96,
		0x97,
		0x98,
		0x99,
		0x9B,
		0x9C,


		0x80,
		0x81,
		0x9F,

		0,
		0x31,
		0x32,
		0x33,
		0x34,
		0x35,
		0x36,
		0x37,
		0x38,
		0x39,
		0x30,
		0x2B,
		0x2D,
		0x5F,
		0x1B,

		0x37,
		0x38,
		0x39,


		0x9A,
		0x89,
		0,

		0xA0,
		0x71,
		0x77,
		0x65,
		0x72,
		0x74,
		0x79,
		0x75,
		0x69,
		0x6F,
		0x70,
		0x88,
		0x87,
		0x0D,

		0x34,
		0x35,
		0x36,


		0x88,
		0x82,
		0x8A,

		0,
		0x61,
		0x73,
		0x64,
		0x66,
		0x67,
		0x68,
		0x6A,
		0x6B,
		0x6C,
		0x3B,
		0x27,
		0,
		0x85,

		0x31,
		0x32,
		0x33,


		0x86,
		0x8B,
		0x84,

		0,
		0x7A,
		0x78,
		0x63,
		0x76,
		0x62,
		0x6E,
		0x6D,
		0x2C,
		0x2E,
		0x2F,
		0,

		0x30,
		0x2E,

		0,
		0x20
	},
	{	/* upper case */
		0x92,
		0x93,
		0x94,
		0x95,
		0x96,
		0x97,
		0x98,
		0x99,
		0x9B,
		0x9C,


		0x80,
		0x81,
		0x9F,

		0,
		0x31,
		0x32,
		0x33,
		0x34,
		0x35,
		0x36,
		0x37,
		0x38,
		0x39,
		0x30,
		0x2B,
		0x2D,
		0x5F,
		0x1B,

		0x37,
		0x38,
		0x39,


		0x9A,
		0x89,
		0,

		0xA0,
		0x51,
		0x57,
		0x45,
		0x52,
		0x54,
		0x59,
		0x55,
		0x49,
		0x4F,
		0x50,
		0x88,
		0x87,
		0x0D,

		0x34,
		0x35,
		0x36,


		0x88,
		0x82,
		0x8A,

		0,
		0x41,
		0x53,
		0x44,
		0x46,
		0x47,
		0x48,
		0x4A,
		0x4B,
		0x4C,
		0x3B,
		0x27,
		0,
		0x85,

		0x31,
		0x32,
		0x33,


		0x86,
		0x8B,
		0x84,

		0,
		0x5A,
		0x58,
		0x43,
		0x56,
		0x42,
		0x4E,
		0x4D,
		0x2C,
		0x2E,
		0x2F,
		0,

		0x30,
		0x2E,

		0,
		0x20
	},
	{	/* shifted */
		0x92,
		0x93,
		0x94,
		0x95,
		0x96,
		0x97,
		0x98,
		0x99,
		0x9B,
		0x9C,


		0x80,
		0x81,
		0x9F,

		0,
		0x21,
		0x40,
		0x23,
		0x24,
		0x25,
		0x5E,
		0x26,
		0x2A,
		0x28,
		0x29,
		0x5B,
		0x5D,
		0x3D,
		0x1B,

		0x37,
		0x38,
		0x39,


		0x9A,
		0x89,
		0,

		0xA0,
		0x51,
		0x57,
		0x45,
		0x52,
		0x54,
		0x59,
		0x55,
		0x49,
		0x4F,
		0x50,
		0x8A,
		0x8C,
		0x0D,

		0x34,
		0x35,
		0x36,


		0x88,
		0x82,
		0x8A,

		0,
		0x41,
		0x53,
		0x44,
		0x46,
		0x47,
		0x48,
		0x4A,
		0x4B,
		0x4C,
		0x3A,
		0x22,
		0,
		0x83,

		0x31,
		0x32,
		0x33,


		0x86,
		0x8B,
		0x84,

		0,
		0x5A,
		0x58,
		0x43,
		0x56,
		0x42,
		0x4E,
		0x4D,
		0x3C,
		0x3E,
		0x3F,
		0,

		0x30,
		0x2E,

		0,
		0x20
	},
	{	/* control */
		0x92,
		0x93,
		0x94,
		0x95,
		0x96,
		0x97,
		0x98,
		0x99,
		0x9B,
		0x9C,


		0x80,
		0x81,
		0x9F,

		0,
		0x90,
		0x91,
		0x00,
		0xA1,
		0x8D,
		0x8E,
		0x8F,
		0x7C,
		0x60,
		0x7E,
		0x1D,
		0x7F,
		0x5C,
		0x1B,

		0x37,
		0x38,
		0x39,


		0x9A,
		0x89,
		0,

		0xA0,
		0x11,
		0x17,
		0x05,
		0x12,
		0x14,
		0x19,
		0x15,
		0x09,
		0x0F,
		0x10,
		0x88,
		0x87,
		0x0D,

		0x34,
		0x35,
		0x36,


		0x88,
		0x82,
		0x8A,

		0,
		0x01,
		0x13,
		0x04,
		0x06,
		0x07,
		0x08,
		0x0A,
		0x0B,
		0x0C,
		0x7B,
		0x7D,
		0,
		0x85,

		0x31,
		0x32,
		0x33,


		0x86,
		0x8B,
		0x84,

		0,
		0x1A,
		0x18,
		0x03,
		0x16,
		0x02,
		0x0E,
		0x0D,
		0x1C,
		0x1E,
		0x1F,
		0,

		0x30,
		0x2E,

		0,
		0x20
	}
};

void vdt911_keyboard(int unit)
{
	typedef enum { lower_case = 0, upper_case, shift, control, special_debounce = -1 } modifier_state_t;

	static unsigned char repeat_timer;
	enum { repeat_delay = 5 /* approx. 1/10s */ };
	static char last_key_pressed = -1;
	static modifier_state_t last_modifier_state;

	UINT16 key_buf[6];
	int i, j;
	modifier_state_t modifier_state;
	int repeat_mode;


	/* read current key state */
	for (i=0; i<6; i++)
		key_buf[i] = readinputport(i);


	/* parse modifier keys */
	if (key_buf[3] & 0x0040)
		modifier_state = control;
	else if ((key_buf[4] & 0x0400) || (key_buf[5] & 0x0020))
		modifier_state = shift;
	else if ((key_buf[0] & 0x2000))
		modifier_state = upper_case;
	else
		modifier_state = lower_case;

	/* test repeat key */
	repeat_mode = key_buf[2] & 0x0002;


	/* remove modifier keys */
	key_buf[0] &= ~0x2000;
	key_buf[2] &= ~0x0002;
	key_buf[3] &= ~0x0040;
	key_buf[4] &= ~0x0400;
	key_buf[5] &= ~0x0520;

	/* remove unused keys */	
	if ((vdt[unit].model == vdt911_model_US) || (vdt[unit].model == vdt911_model_UK) || (vdt[unit].model == vdt911_model_French))
		key_buf[4] &= ~0x0004;


	if (! repeat_mode)
		/* reset REPEAT timer if the REPEAT key is not pressed */
		repeat_timer = 0;

	if ((last_key_pressed >= 0) && (key_buf[last_key_pressed >> 4] & (1 << (last_key_pressed & 0xf))))
	{
		/* last key has not been released */
		if (modifier_state == last_modifier_state)
		{
			/* handle REPEAT mode if applicable */
			if ((repeat_mode) && (++repeat_timer == repeat_delay))
			{
				if (vdt[unit].keyboard_data_ready)
				{	/* keyboard buffer full */
					repeat_timer--;
				}
				else
				{	/* repeat current key */
					vdt[unit].keyboard_data_ready = 1;
					repeat_timer = 0;
				}
			}
		}
		else
		{
			repeat_timer = 0;
			last_modifier_state = special_debounce;
		}
	}
	else
	{
		last_key_pressed = -1;

		if (vdt[unit].keyboard_data_ready)
		{	/* keyboard buffer full */
			/* do nothing */
		}
		else
		{
			for (i=0; i<6; i++)
			{
				for (j=0; j<16; j++)
				{
					if (key_buf[i] & (1 << j))
					{
						last_key_pressed = (i << 4) | j;
						last_modifier_state = modifier_state;

						vdt[unit].keyboard_data = US_key_translate[modifier_state][last_key_pressed];
						vdt[unit].keyboard_data_ready = 1;
						if (vdt[unit].keyboard_interrupt_enable)
							(*vdt[unit].int_callback)(1);
						return;
					}
				}
			}
		}
	}
}
