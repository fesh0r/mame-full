/******************************************************************************
	channel = get_play_channels(1); removed for now?
 *
 *	kaypro.c
 *
 *	KAYPRO terminal emulation for CP/M
 *
 *	from Juergen Buchmueller's VT52 emulation, July 1998
 *	Benjamin C. W. Sittler, July 1998
 *
 ******************************************************************************/

/*
 * The Kaypro 2/II, 4 and 10 emulate the Lear-Siegler ADM-3A terminal.
 * Here are the commands they accept:
 *
 * Control Characters
 *
 * 0x00 - Ignored
 * 0x09 - Horizontal tab (stops every 8 columns)
 * 0x07 - Ring Bell (translated to 0x04 and sent to keyboard)
 * 0x08 - Cursor Left (non-destructive backspace)
 * 0x0c - Cursor Right
 * 0x0a - Cursor Down (linefeed)
 * 0x0b - Cursor Up
 * 0x0d - Cursor to beginning of line (carriage return)
 * 0x17 - Clear to end of screen
 * 0x18 - Clear to end of line
 * 0x1a - Clear screen, home cursor
 * 0x1b - ESCape sequence prefix
 * 0x1e - Home cursor
 *
 * ESCape Sequences
 *
 * ESCape,A - Display lower case [0]
 * ESCape,G - Display greek [0]
 * ESCape,E - Insert line [1]
 * ESCape,R - Delete line [1]
 * ESCape,=,row+32,col+32 - Cursor address
 *
 * Additionally, the following codes apply to: KAYPRO 2/84, 2X, 4/84, 4X, 10,
 * ROBIE, and 1 (KAYPRO computers with graphic capability):
 *
 * ESCape,B,0 - Reverse video start
 * ESCape,C,0 - Reverse video stop
 * ESCape,B,1 - Half intensity start
 * ESCape,C,1 - Half intensity stop
 * ESCape,B,2 - Blinking start
 * ESCape,C,2 - Blinking stop
 * ESCape,B,3 - Underline start
 * ESCape,C,3 - Underline stop
 * ESCape,B,4 - Cursor on, 1/16 sec blink
 * ESCape,C,4 - Cursor off
 * ESCape,B,5 - Video mode on [2]
 * ESCape,C,5 - Video mode off [2]
 * ESCape,B,6 - Remember current cursor position
 * ESCape,C,6 - Return to last remembered cursor position
 * ESCape,B,7 - Status line preservation on
 * ESCape,C,7 - Status line preservation off
 * ESCape,*,y+32,x+32 - Set pixel
 * ESCape, ,y+32,x+32 - Clear pixel
 * ESCape,L,y+32,x+32,y2+32,x2+32 - Set line
 * ESCape,D,y+32,x+32,y2+32,x2+32 - Delete line
 *
 * Illegal escape sequences are ignored.
 *
 * [0] - These sequences were used by the Kaypro 2/II. They are ignored
 *       on later machines.
 *
 * [1] - These sequences are reversed in the KAYPRO documentation;
 *       this is how they're actually implemented by the ROM console
 *       driver.
 *
 * [2] - In video mode, block graphics (characters in the range 0x80 - 0xff)
 *       are treated specially. Every GB1 block graphic (the 1st, 3rd, etc.
 *       after starting video mode) is not printed; rather, its low bit is
 *       used to set the lower-left block of the next (GB2) block graphic (by
 *       inverting the low 7 bits and setting the reverse video bit,)
 *       without affecting the reverse video mode for normal characters.
 */

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "mess/vidhrdw/kaypro.h"

//static  int channel;
#define BELL_SIZE	1024
#define CLICK_SIZE	64
static	signed char *bell;

static	signed char *click;


enum state {
	ST_NORMAL,
	ST_ESCAPE,
	ST_CURPOS_ROW,
	ST_CURPOS_COL,
	ST_SET_ATTRIB,
	ST_CLR_ATTRIB,
	ST_SET_LINE,
	ST_CLR_LINE,
	ST_SET_PIXEL,
	ST_CLR_PIXEL

};

/* visible character attributes */
#define AT_MASK           0x0f00 /* visible attributes */
#define AT_REVERSE        0x0200 /* reverse video */
#define AT_HALF_INTENSITY 0x0400 /* half intensity */
#define AT_BLINK          0x0800 /* blinking */
#define AT_UNDERLINE      0x0100 /* underline */

/* dummy character attributes */
#define AT_VIDEO          0x1000 /* video mode */
#define AT_VIDEO_GB1      0x4000 /* video GB1 mode */

static int cur_x = 0, cur_y = 0; /* cursor position */
static int old_x = 0, old_y = 0; /* remembered cursor position */
static int scroll_lines = 0; /* number of lines to scroll */
static int state = ST_NORMAL; /* command state */
static int attrib = 0; /* current attributes */
static int gb1 = 0; /* 0, AT_REVERSE or AT_VIDEO_GB1 */
static int cursor = 1; /* cursor visibility */

/* console keyboard buffer */
static UINT8 kbd_buff[16];
static int kbd_head = 0;
static int kbd_tail = 0;

static	short * video_buffer = NULL;

void	kaypro_putstr(char * src)
{
	while (*src)
		kaypro_conout_w(0, *src++);
}

int kaypro_vh_start(void)
{
	int i;

	scroll_lines = KAYPRO_SCREEN_H;
	videoram_size = KAYPRO_SCREEN_W * KAYPRO_SCREEN_H;

	if (generic_vh_start())
		return 1;

	video_buffer = malloc(videoram_size * sizeof(short));
	if (!video_buffer)
		return 1;

	bell = malloc(BELL_SIZE);
	if (!bell)
		return 1;

	click = malloc(CLICK_SIZE);
	if (!click)
		return 1;

	for (i = 0; i < videoram_size; i++)
		video_buffer[i] = 0x20;

	for (i = 0; i < BELL_SIZE; i++)
    {
		bell[i] = (BELL_SIZE - 1 - i) >> 3;
		if (i & 4)
			bell[i] *= -1;
    }

	for (i = 0; i < CLICK_SIZE; i++)
		click[i] = (i & 4) ? -120 : +120;

//	channel = get_play_channels(1);

	kaypro_putstr(
	/* a test of GB1/GB2 video mode graphics */ \
        "\033B5" /* start video mode */ \
        "MESS KAYPRO Terminal Emulator          \200\220\200\263" \
		"\200\263\200\202\200\260\200\263\200\263\200\243\200\203" \
		"\200\260\200\263\200\263\200\263\200\221\200\263\200\263 " \
		"\200\220\200\261\200\263\200\243\200\262\200\263\200\263" \
		"\200\263\200\262\200\223\200\263\200\263\200\263\200\263" \
		"\200\262\200\220\200\260\200\261\200\263\200\263\200\263" \
		"\200\262\200\240\r\n" \
		"                                      \200\220\200\263" \
		"\200\263\200\262\200\263\200\263\200\203 \200\220\200\261" \
		"\200\263\200\223\200\263\200\263 \200\263\200\263\200\260" \
		"\200\263\200\243\200\222\200\263\200\263\200\260\200\261" \
		"\200\263\200\243\200\261\200\262\200\260\200\261\200\263" \
		"\200\223\200\261\200\263\200\202  \200\263\200\263\200\242\r\n" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\263\200\222\200\263\200\263\200\243\200\223" \
		"\200\263\200\263\200\240\200\260\200\263\200\263\200\262" \
		"\200\261\200\263\200\263\200\220\200\263\200\263\200\243" \
		"\200\202\200\261\200\263\200\263\200\203\200\203\200\223" \
		"\200\261\200\263\200\243\200\223\200\263\200\263\200\240" \
		"\200\263\200\263\200\242  \200\261\200\263\200\243\r\n" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\263\200\263\200\263\200\263\200\263\200\263" \
		"\200\263\200\222\200\263\200\263\200\243  \200\223\200\263" \
		"\200\263\200\241\200\243\200\203\200\203\200\223\200\263" \
		"\200\263\200\221\200\263\200\243 \200\261\200\263\200\263" \
		"\200\202 \200\220\200\263\200\263\200\243 \200\221\200\263" \
		"\200\263\200\242\200\203\200\263\200\263\200\263\200\263" \
		"\200\203\200\203\r\n" \
		"\033C5"); /* end video mode */
	return 0;
}

void kaypro_vh_stop(void)
{
	if (video_buffer)
		free(video_buffer);
	video_buffer = NULL;

	if (click)
		free(click);
	click = NULL;

    if (bell)
		free(bell);
	bell = NULL;

    generic_vh_stop();
}

void kaypro_vh_screenrefresh(struct osd_bitmap * bitmap, int full_refresh)
{
	static int blink_count = 0;
	static int cursor_count = 0;
	int i, j = -1;

    blink_count++;
	if (!(blink_count & 15))
    {
		if (blink_count & 16)
        {
			palette_change_color(3, 0,240,	0);
			palette_change_color(4, 0,120,	0);
        }
		else
        {
			palette_change_color(3, 0,	0,	0);
			palette_change_color(4, 0,	0,	0);
        }
    }

	palette_init_used_colors();
    memset(palette_used_colors, PALETTE_COLOR_USED, 4);

    if( palette_recalc() )
        full_refresh = 1;

    cursor_count++;
	if (cursor)
		j = cur_y * KAYPRO_SCREEN_W + cur_x;

	if (full_refresh)
    {
		copybitmap(bitmap, tmpbitmap, 0, 0, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
    }

	for (i = 0; i < videoram_size; i++)
    {
		if (dirtybuffer[i] || i == j)
		{
			int x, y, code, color;
			struct rectangle r;

			dirtybuffer[i] = 0;

			y = i / KAYPRO_SCREEN_W;
			x = i % KAYPRO_SCREEN_W;
			r.min_x = x * KAYPRO_FONT_W;
			r.max_x = r.min_x + KAYPRO_FONT_W - 1;
			r.min_y = y * KAYPRO_FONT_H;
			r.max_y = r.min_y + KAYPRO_FONT_H - 1;

			code = video_buffer[i] & 0x3ff;
			color = video_buffer[i] >> 10;

			drawgfx(tmpbitmap, Machine->gfx[0], code, color,
				0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);
			drawgfx(bitmap, Machine->gfx[0], code, color,
				0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);

			if ( i == j && (cursor_count & 16) )
            {
				/* toggle reverse */
				code ^= 0x0200;
				drawgfx(tmpbitmap, Machine->gfx[0], code, color,
					0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);
				drawgfx(bitmap, Machine->gfx[0], code, color,
					0, 0, r.min_x, r.min_y, &r, TRANSPARENCY_NONE, 0);
				dirtybuffer[i] = 1;
            }
        }
    }
}

/******************************************************
 *	Ring my bell ;)
 ******************************************************/
static void kaypro_bell(void)
{
	//osd_play_sample(channel, bell, BELL_SIZE, 22050, 255, 0);
}

/******************************************************
 *	Clicking keys (for the Kaypro 2x)
 ******************************************************/
static void kaypro_click(void)
{
	//osd_play_sample(channel, click, CLICK_SIZE, 22050, 140, 0);
}

/******************************************************
 *	kaypro_vgbout
 *	output a character code to the given offset
 *	the given attributes are combined to the code
 *	and the dirty flag is set if the code changed
 ******************************************************/
static void kaypro_vgbout(int offset, int data, short attr)
{
	data |= attr & AT_MASK;
	if (data == video_buffer[offset])
		return;
	video_buffer[offset] = data;
	dirtybuffer[offset] = 1;
}

/******************************************************
 *	kaypro_out
 *	output a character code to the given offset
 *	the current attributes are combined to the code
 *	and the dirty flag is set if the code changed
 ******************************************************/
static void kaypro_out(int offset, int data)
{
	kaypro_vgbout(offset, data, attrib);
}

/******************************************************
 * kaypro_const_r (read console status)
 * 00  no key available
 * ff  key(s) available
 ******************************************************/
int  kaypro_const_r(int offset)
{
	int data = 0x00;
	if( kbd_head != kbd_tail )
		data = 0xff;
    return data;
}

/******************************************************
 *	kaypro_const_w (write console status ;)
 *	bit
 *	0	flush keyboard buffer
 ******************************************************/
void kaypro_const_w(int offset, int data)
{
    if (data & 1)
    {
        kbd_head = kbd_tail = 0;
		if (timer_iscpususpended(0,SUSPEND_REASON_HALT))
        {
			cpu_set_reg( Z80_AF, cpu_get_reg( Z80_AF ) & 0x00ff );
			timer_suspendcpu(0, 0,SUSPEND_REASON_HALT);
            return;
        }
    }
}

/******************************************************
 *	kaypro_conin_r (read console input, ie. keyboard)
 *	returns next character from the keyboard buffer
 *	suspends CPU if no key is available
 ******************************************************/
int  kaypro_conin_r(int offset)
{
	int data = 0;

    if (kbd_tail != kbd_head)
    {
        data = kbd_buff[kbd_tail];
        kbd_tail = ++kbd_tail % sizeof(kbd_buff);
    }
    else
    {
		timer_suspendcpu(0, 1,SUSPEND_REASON_HALT);
    }
    return data;
}

/******************************************************
 *	kaypro_conin_w
 *	stuff character into the keyboard buffer
 *	releases CPU if it was waiting for a key
 *	sounds bell if buffer would overflow
 ******************************************************/
void kaypro_conin_w(int offset, int data)
{
int kbd_head_old;
    kaypro_click();
	if( timer_iscpususpended(0,SUSPEND_REASON_HALT) )
    {
		cpu_set_reg( Z80_AF, data << 8 );
		timer_suspendcpu(0, 0,SUSPEND_REASON_HALT);
        return;
    }
    kbd_head_old = kbd_head;
    kbd_buff[kbd_head] = data;
    kbd_head = ++kbd_head % sizeof(kbd_buff);
    /* will the buffer overflow ? */
    if (kbd_head == kbd_tail)
    {
        kbd_head = kbd_head_old;
        kaypro_bell();
	}
}

/******************************************************************************
 *	kaypro_scroll
 *	scroll the screen buffer from line top to line scroll_lines-1
 *	either up or down. scroll up if lines > 0, down if lines < 0
 *	repeat lines times and mark changes in dirtybuffer
 ******************************************************************************/
void kaypro_scroll(int top, int lines)
{
	int x, y;
	short attr;
	while (lines)
    {
		if (lines > 0)
		{
			for( y = top; y < scroll_lines - 1; y++ )
			{
				for( x = 0; x < KAYPRO_SCREEN_W; x++ )
				{
					if (video_buffer[y * KAYPRO_SCREEN_W + x] != video_buffer[(y + 1) * KAYPRO_SCREEN_W + x])
					{
						video_buffer[y * KAYPRO_SCREEN_W + x] = video_buffer[(y + 1) * KAYPRO_SCREEN_W + x];
						dirtybuffer[y * KAYPRO_SCREEN_W + x] = 1;
					}
				}
			}
			attr = ' ';
			for (x = 0; x < KAYPRO_SCREEN_W; x++)
			{
				if (attr != video_buffer[(scroll_lines - 1) * KAYPRO_SCREEN_W + x])
				{
					video_buffer[(scroll_lines - 1) * KAYPRO_SCREEN_W + x] = attr;
					dirtybuffer[(scroll_lines - 1) * KAYPRO_SCREEN_W + x] = 1;
				}
			}
			lines--;
		}
		else
		{
			for (y = scroll_lines - 1; y > top; y--)
			{
				for (x = 0; x < KAYPRO_SCREEN_W; x++)
				{
					if (video_buffer[y * KAYPRO_SCREEN_W + x] != video_buffer[(y - 1) * KAYPRO_SCREEN_W + x])
					{
						video_buffer[y * KAYPRO_SCREEN_W + x] = video_buffer[(y - 1) * KAYPRO_SCREEN_W + x];
						dirtybuffer[y * KAYPRO_SCREEN_W + x] = 1;
					}
				}
			}
			attr = ' ';
			for (x = 0; x < KAYPRO_SCREEN_W; x++)
			{
				if (attr != video_buffer[top * KAYPRO_SCREEN_W + x])
				{
					video_buffer[top * KAYPRO_SCREEN_W + x] = attr;
					dirtybuffer[top * KAYPRO_SCREEN_W + x] = 1;
				}
            }
			lines++;
        }
    }
}

static void kaypro_cursor_home(void)
{
	cur_x = 0;
    cur_y = 0;
}

static void kaypro_carriage_return(void)
{
    cur_x = 0;
}

static void kaypro_cursor_left(int count)
{
	while( count-- )
	{
		if( cur_x )
			cur_x--;
	}
}

static void kaypro_cursor_right(int count)
{
	while( count-- )
	{
		if( cur_x < KAYPRO_SCREEN_W )
			cur_x++;
	}
}

static void kaypro_line_feed(int count)
{
	/* don't scroll if beyond last line already (protected line 25) */
	if( cur_y >= scroll_lines )
		return;
	while( count-- )
	{
		if( ++cur_y >= scroll_lines )
		{
			cur_y = scroll_lines - 1;
			kaypro_scroll(0, +1);
		}
	}
}

static void kaypro_reverse_line_feed(int count)
{
	while( count-- )
	{
		if( --cur_y < 0 )
		{
			cur_y = 0;
			kaypro_scroll(0, -1);
		}
	}
}

static void kaypro_advance(void)
{

	kaypro_cursor_right(1);
	if (cur_x >= KAYPRO_SCREEN_W)
    {
		kaypro_carriage_return();
		kaypro_line_feed(1);
    }
}

static void kaypro_erase_end_of_line(void)
{
	int i, offs, attr;
	offs = cur_y * KAYPRO_SCREEN_W + cur_x;
	attr = ' ';
	for( i = 0; i < KAYPRO_SCREEN_W - cur_x; i++ )
    {
		if( attr != video_buffer[offs] )
        {
			video_buffer[offs] = attr;
			dirtybuffer[offs] = 1;
        }
		offs++;
    }
}

static void kaypro_erase_end_of_screen(void)
{
	int i, offs, attr;
	offs = cur_y * KAYPRO_SCREEN_W + cur_x;
	attr = ' ';
	for( i = offs; i < scroll_lines * KAYPRO_SCREEN_W; i++ )
    {
		if (attr != video_buffer[i])
        {
			video_buffer[i] = attr;
			dirtybuffer[i] = 1;
        }
    }
}

static void kaypro_clear_screen(void)
{
	attrib &= ~ AT_MASK; /* clear visible attributes */
	kaypro_cursor_home();
	kaypro_erase_end_of_screen();
}

static void kaypro_tab(void)
{
	do
    {
		kaypro_out(cur_y * KAYPRO_SCREEN_W + cur_x, ' ');
		kaypro_advance();
    } while (cur_x & 7);
}

static void kaypro_pixel(int x, int y, int set)
{
	static int attr_bits[4][2] = {
		{	  0x002, 0x001},
		{	  0x008, 0x004},
		{	  0x020, 0x010},
		{AT_REVERSE, 0x040}
	};
	int cx, cy, offs, bits;
	short attr;

	/* The Kaypro 2x font has a 2x4 pattern block graphics */
	cx = x / 2;
	cy = y / 4;
	offs = cy * KAYPRO_SCREEN_W + cx;
	attr = video_buffer[offs];

	/* if it is a space, we change it to a graphic space */
	if ((attr & 0xff) == ' ')
		attr = (attr & 0xff00) | 0x80;

	/* if it is non graphics, we do nothing */
	if (! (attr & 0x80))
		return;

	/* reverse video (lower-left pixel) inverts all the other pixels */
	if (attr & AT_REVERSE)
		attr ^= 0x7f;

	/* get the bit mask for the pixel */
	bits = attr_bits[y % 4][x % 2];

	/* set it ? */
	if (set)
		attr |= bits;
	else
		attr &= ~ bits;

	/* reverse video (lower-left pixel) inverts all the other pixels */
	if (attr & AT_REVERSE)
		attr ^= 0x7f;

	/* attributed character changed ? */
	if( attr != video_buffer[offs] )
	{
		video_buffer[offs] = attr;
		dirtybuffer[offs] = 1;
	}
}

static void kaypro_line(int x0, int y0, int x1, int y1, int set)
{
	int dx, dy, sx, sy, c;

    /* delta x and direction */
    dx = x1 - x0;
	if (dx < 0)
	{
		sx = -1;
		dx = -dx;
	}
	else
	{
		sx = +1;
	}
	/* delta y and direction */
    dy = y1 - y0;
	if (dy < 0)
	{
		sy = -1;
		dy = -dy;
	}
	else
	{
		sy = +1;
    }
	/* The standard Bresenham algorithm ;) */
    if (dx > dy)
	{
		c = dx / 2;
		for ( ; ; )
		{
			kaypro_pixel(x0, y0, set);
			if (x0 == x1)
				break;
			x0 += sx;
			if ((c -= dy) <= 0)
			{
				c += dx;
				y0 += sy;
			}
		}
	}
	else
	{
		c = dy / 2;
		for ( ; ; )
		{
			kaypro_pixel(x0, y0, set);
			if (y0 == y1)
				break;
			y0 += sy;
			if ((c -= dx) <= 0)
			{
				c += dy;
				x0 += sx;
			}
		}
    }
}


int kaypro_conout_r(int offset)
{
	return 0xFF;
}

void kaypro_conout_w(int offset, int data)
{
	static int argcnt = 0;
	static int argval[4];

	data &= 0xff;
	switch (state)
    {
	case ST_NORMAL:
		switch (data)
		{
		case 0x00: /* NUL is ignored */
			break;
		case 0x07: /* ring my bell ;) */
			if (errorlog) fprintf(errorlog, "KAYPRO <007>     bell\n");
			kaypro_bell();
			break;
		case 0x08: /* cursor left */
			kaypro_cursor_left(1);
			break;
		case 0x09: /* tabulator */
			kaypro_tab();
			break;
		case 0x0a: /* line feed */
			kaypro_line_feed(1);
			break;
		case 0x0b: /* reverse line feed */
			kaypro_reverse_line_feed(1);
			break;
		case 0x0d: /* carriage return */
			kaypro_carriage_return();
			break;
		case 0x17: /* clear to end of screen */
			if (errorlog) fprintf(errorlog, "KAYPRO <027>     clear to end of screen\n");
			kaypro_erase_end_of_screen();
			break;
		case 0x18: /* clear to end of line */
			if (errorlog) fprintf(errorlog, "KAYPRO <030>     clear to end of line\n");
			kaypro_erase_end_of_line();
			break;
		case 0x1a: /* clear screen */
			if (errorlog) fprintf(errorlog, "KAYPRO <032>     clear screen, home cursor\n");
			kaypro_clear_screen();
			break;
		case 0x1b: /* ESCape sequence prefix */
			state = ST_ESCAPE;
			break;
		case 0x1e: /* kaypro cursor home */
			if (errorlog) fprintf(errorlog, "KAYPRO <036>     cursor home\n");
			kaypro_cursor_home();
			break;
		default:
			if( (attrib & AT_VIDEO) && (data & 0x80) )
			{
				if (gb1 & AT_VIDEO_GB1)
				{
					gb1 = (data & 1) ? AT_REVERSE : 0;
				}
				else
				{
					if (gb1)
						data ^= 0x7f;
					kaypro_vgbout(cur_y * KAYPRO_SCREEN_W + cur_x, data, gb1 | attrib);
					kaypro_advance();
					gb1 = AT_VIDEO_GB1;
				}
			}
			else
			{
				kaypro_out(cur_y * KAYPRO_SCREEN_W + cur_x, data);
				kaypro_advance();
			}
		}
		break;

    case ST_ESCAPE:
		state = ST_NORMAL;
		switch (data)
		{
		case ' ': /* clear dot */
			argcnt = 2;
			state = ST_CLR_PIXEL;
			break;
		case '=': /* cursor positioning */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>=    cursor position\n");
			state = ST_CURPOS_ROW;
			break;
		case '*': /* set dot */
			argcnt = 2;
			state = ST_SET_PIXEL;
			break;
		case 'A': /* Display lower case */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>A    display lower case (ignored)\n");
			break;
		case 'B': /* enable attribute */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B    enable attribute\n");
			state = ST_SET_ATTRIB;
			break;
		case 'C': /* disable attribute */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C    disable attribute\n");
			state = ST_CLR_ATTRIB;
			break;
		case 'D': /* delete line of dots */
			argcnt = 4;
			state = ST_CLR_LINE;
			break;
		case 'E': /* insert line */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>E    insert line\n");
			kaypro_scroll(cur_y, -1);
			break;
		case 'G': /* Display greek */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>G    display greek (ignored)\n");
			break;
		case 'H': /* cursor home */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>H    cursor home\n");
			kaypro_cursor_home();
			break;
		case 'L': /* set line of dots */
			argcnt = 4;
			state = ST_SET_LINE;
			break;
		case 'R': /* delete line */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>R    delete line\n");
			kaypro_scroll(cur_y, +1);
			break;
		default:  /* some other escape sequence? */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>%c    unknown\n", data);
		}
		break;

    case ST_CURPOS_ROW:
		cur_y = data - ' ';
		if (errorlog) fprintf(errorlog, "KAYPRO cursor y  %d\n", cur_y);
		if( cur_y < 0 )
			cur_y = 0;
		if( cur_y >= KAYPRO_SCREEN_H )
			cur_y = KAYPRO_SCREEN_H - 1;
		state = ST_CURPOS_COL;
		break;

    case ST_CURPOS_COL:
		cur_x = data - ' ';
		if (errorlog) fprintf(errorlog, "KAYPRO cursor x  %d\n", cur_x);
		if( cur_x < 0 )
			cur_x = 0;
		if( cur_x >= KAYPRO_SCREEN_W )
			cur_x = KAYPRO_SCREEN_W - 1;
		state = ST_NORMAL;
		break;

    case ST_SET_ATTRIB:
		state = ST_NORMAL;
		switch (data)
		{
		case '0': /* reverse video */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B0   reverse on\n");
			attrib |= AT_REVERSE;
			break;
		case '1': /* half intensity */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B1   half intensity\n");
			attrib |= AT_HALF_INTENSITY;
			break;
		case '2': /* start blinking */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B2   start blinking\n");
			attrib |= AT_BLINK;
			break;
		case '3': /* start underlining */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B3   start underlining\n");
			attrib |= AT_UNDERLINE;
			break;
		case '4': /* cursor on */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B4   cursor on\n");
			cursor = 1;
			break;
		case '5': /* video mode on */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B5   video mode on\n");
			attrib |= AT_VIDEO;
			gb1 = AT_VIDEO_GB1;
			break;
		case '6': /* remember cursor position */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B6   save cursor (%d,%d)\n", cur_x, cur_y);
			old_x = cur_x;
			old_y = cur_y;
			break;
		case '7': /* preserve status line */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B7   preserve status line\n");
			scroll_lines = KAYPRO_SCREEN_H - 1;
			break;
		default:
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>B%c   unknown\n", data);
			break;
		}
		break;

    case ST_CLR_ATTRIB:
		state = ST_NORMAL;
		switch (data)
		{
		case '0': /* stop reverse video */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C0   reverse off\n");
			attrib &= ~ AT_REVERSE;
			break;
		case '1': /* normal intensity */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C1   normal intensity\n");
			attrib &= ~ AT_HALF_INTENSITY;
			break;
		case '2': /* stop blinking */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C2   stop blinking\n");
			attrib &= ~ AT_BLINK;
			break;
		case '3': /* stop underlining */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C3   stop underlining\n");
			attrib &= ~ AT_UNDERLINE;
			break;
		case '4': /* cursor off */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C4   cursor off\n");
			cursor = 0;
			break;
		case '5': /* video mode off */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C5   video mode off\n");
			attrib &= ~ AT_VIDEO;
			break;
		case '6': /* restore cursor position */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C6   restore cursor (%d,%d)\n", old_x, old_y);
			cur_x = old_x;
			cur_y = old_y;
			break;
		case '7': /* don't preserve status line */
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C7   don't preserve status line\n");
			scroll_lines = KAYPRO_SCREEN_H;
			break;
		default:
			if (errorlog) fprintf(errorlog, "KAYPRO <ESC>C%c   unknown\n", data);
			break;
		}
		break;

    case ST_SET_LINE:
		if( argcnt > 0 )
		{
			argval[--argcnt] = data - ' ';
			if( !argcnt )
			{
				int x0, y0, x1, y1;
				x1 = argval[0];
				y1 = argval[1];
				x0 = argval[2];
				y0 = argval[3];
				if (errorlog) fprintf(errorlog, "KAYPRO <ESC>L    set line %d,%d - %d,%d\n", x0,y0,x1,y1);
				kaypro_line(x0,y0,x1,y1, 1);
				argcnt = 0;
				state = ST_NORMAL;
			}
		}
		break;

    case ST_CLR_LINE:
		if( argcnt > 0 )
		{
			argval[--argcnt] = data - ' ';
			if( !argcnt )
			{
				int x0, y0, x1, y1;
				x1 = argval[0];
				y1 = argval[1];
				x0 = argval[2];
				y0 = argval[3];
				if (errorlog) fprintf(errorlog, "KAYPRO <ESC>D    clr line %d,%d - %d,%d\n", x0,y0,x1,y1);
				kaypro_line(x0,y0,x1,y1, 0);
				argcnt = 0;
				state = ST_NORMAL;
			}
		}
		break;

    case ST_SET_PIXEL:
		if( argcnt > 0 )
		{
			argval[--argcnt] = data - ' ';
			if( !argcnt )
			{
				int x0, y0;
				x0 = argval[0];
				y0 = argval[1];
				if (errorlog) fprintf(errorlog, "KAYPRO <ESC>*    set pixel %d,%d\n", x0, y0);
				kaypro_pixel(x0,y0, 1);
				argcnt = 0;
				state = ST_NORMAL;
			}
		}
		break;

    case ST_CLR_PIXEL:
		if( argcnt > 0 )
		{
			argval[--argcnt] = data - ' ';
			if( !argcnt )
			{
				int x0, y0;
				x0 = argval[0];
				y0 = argval[1];
				if (errorlog) fprintf(errorlog, "KAYPRO <ESC>     clr pixel %d,%d\n", x0, y0);
				kaypro_pixel(x0,y0, 0);
				argcnt = 0;
				state = ST_NORMAL;
			}
		}
		break;
    }
}

