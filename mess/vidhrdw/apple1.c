/***************************************************************************

  apple1.c

  Functions to emulate the video hardware of the Apple 1.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/apple1.h"
#include "mscommon.h"

static struct terminal *apple1_terminal;

/**************************************************************************/

static int apple1_getcursorcode(int original_code)
{
	return 1;
}

/**************************************************************************/

VIDEO_START( apple1 )
{
	apple1_terminal = terminal_create(
		0,
		0,
		8,
		apple1_getcursorcode,
		40, 24);
	if (!apple1_terminal)
		return 1;

	terminal_setcursor(apple1_terminal, 0, 0);
	return 0;
}

void apple1_vh_dsp_w (int data)
{
	int	x, y, thischar, abovechar;
	int cur_x, cur_y;

	data &= 0x7f;

	terminal_getcursor(apple1_terminal, &cur_x, &cur_y);

	switch (data) {
	case 0x0a:
	case 0x0d:
		cur_x = 0;
		cur_y++;
		break;
	case 0x5f:
		if (cur_x)
		{
			cur_x--;
		}
		else if (cur_y)
		{
			cur_x = 39;
			cur_y--;
		}
		terminal_putchar(apple1_terminal, cur_x, cur_y, 0);
		break;
	default:
		terminal_putchar(apple1_terminal, cur_x, cur_y, data);
		if (cur_x < 39)
		{
			cur_x++;
		}
		else
		{
			cur_x = 0;
			cur_y++;
		}
		break;
	}

	/* scroll */
	while (cur_y >= 24)
	{
		for (y = 1; y < 24; y++)
		{
			for (x = 0; x < 40; x++)
			{
				thischar = terminal_getchar(apple1_terminal, x, y);
				abovechar = terminal_getchar(apple1_terminal, x, y-1);

				if ((abovechar <= 32) || (abovechar > 96))
				{
					if ((thischar > 32) && (thischar <= 96))
						terminal_putchar(apple1_terminal, x, y-1, thischar);
				}
				else if (thischar != abovechar)
				{
					terminal_putchar(apple1_terminal, x, y-1, thischar);
				}
			}
		}

		for (x = 0; x < 40; x++)
		{
			thischar = terminal_getchar(apple1_terminal, x, 23);
			if ((thischar > 32) && (thischar <= 96))
				terminal_putchar(apple1_terminal, x, 23, 0);
		}
		cur_y--;
	}
	terminal_setcursor(apple1_terminal, cur_x, cur_y);
}

void apple1_vh_dsp_clr (void)
{
	terminal_setcursor(apple1_terminal, 0, 0);
	terminal_clear(apple1_terminal, 0);
}

VIDEO_UPDATE( apple1 )
{
	terminal_draw(bitmap, NULL, apple1_terminal);
}
