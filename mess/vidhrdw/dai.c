/***************************************************************************

  dai.c

  Functions to emulate the video hardware of PK-01 dai.

  Krzysztof Strzecha

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/dai.h"

unsigned char dai_palette[16*3] =
{
	0x00, 0x00, 0x00,	/*  0 Black		*/
	0x00, 0x00, 0x8b,	/*  1 Dark Blue		*/
	0xff, 0x00, 0x80,	/*  2 Purple Red	*/
	0xff, 0x00, 0x00,	/*  3 Red		*/
	0xee, 0xff, 0xbb,	/*  4 Purple Brown	*/
	0x00, 0xc9, 0x57,	/*  5 Emerald Green	*/
	0x8b, 0x86, 0x4e,	/*  6 Kakhi Brown	*/
	0xff, 0x88, 0x55,	/*  7 Mustard Brown	*/
	0x99, 0x99, 0x99,	/*  8 Grey		*/
	0x00, 0x00, 0xcd,	/*  9 Middle Blue	*/
	0xff, 0xa5, 0x00,	/* 10 Orange		*/
	0xff, 0xc0, 0xcb,	/* 11 Pink		*/
	0x00, 0x00, 0xff,	/* 12 Light Blue	*/
	0x00, 0xff, 0x00,	/* 13 Light Green	*/
	0xff, 0xff, 0x00,	/* 14 Light Yellow	*/
	0xff, 0xff, 0xff,	/* 15 White		*/
};

unsigned short dai_colortable[1][16] =
{
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }
};

PALETTE_INIT( dai )
{
	palette_set_colors(0, dai_palette, sizeof(dai_palette) / 3);
	memcpy(colortable, dai_colortable, sizeof (dai_colortable));
}


VIDEO_START( dai )
{
	return 0;
}

VIDEO_UPDATE( dai )
{
	int i, j, k;

	UINT8 * char_rom = memory_region(REGION_GFX1);

	UINT16 dai_video_memory_start = 0xbfff;
	UINT16 dai_scan_lines = 520;	/* visible scan lines of PAL tv */

	UINT16 current_scan_line = 0;
	UINT16 current_video_memory_address = dai_video_memory_start;

	UINT8 mode;			/* mode byte of line
					   bits 0-3 - line repeat count
					   bits 4-5 - resolution control
					   bits 6-7 - display mode control */
	UINT8 colour;                   /* colour byte of line
					   bits 0-3 - one of 16 colours
					   bits 4-5 - colour register for update
					   bit  6   - if unset force 'unit colour mode'
					   bit  7   - enable coulor change
					              if unset bits 0-5 are ignored */

	UINT8 line_repeat_count;	/* number of horizontalraster scans
					   for which same data will be displayed
					   0000 - 2 lines
				  	   each additional repeat adds 2 scans */
	UINT8 horizontal_resolution;	/* number of blobs per line
					   00 - 88 (low resolution graphics)
					   01 - 176 (medium resolution graphics)
					   10 - 352 (high resolution graphics)
					   11 - 528 (text with 66 chars per line) */
	UINT8 display_mode;		/* determine how data will be used
					   to generate the picture
					   00 - four colour graphics
					   01 - four colour characters
					   10 - sixteen colour graphics
				 	   11 - sixteen colour characters */

	UINT8 unit_mode;

	UINT8 current_data, current_color;

	while (current_scan_line < dai_scan_lines)
	{
		mode = cpu_readmem16(current_video_memory_address--);
		colour = cpu_readmem16(current_video_memory_address--);
		line_repeat_count = mode & 0x0f;
		horizontal_resolution = (mode & 0x30) >> 4;
		display_mode = (mode & 0xc0) >> 6;
		unit_mode = (colour & 0x40) >> 6;

		switch (display_mode)
		{
			case 0x00:	/* 4 colour grahics */
				switch (horizontal_resolution)
				{
					case 0x00:	/* 88 pixels */
						current_scan_line = 520;
						break;
					case 0x01:	/* 176 pixels */
						current_scan_line = 520;
						break;
					case 0x02:	/* 352 pixels */
						current_scan_line = 520;
						break;
					case 0x03:	/* 528 pixels */
						switch (unit_mode)
						{
							case 0:
								current_data = cpu_readmem16(current_video_memory_address--);
								current_color = cpu_readmem16(current_video_memory_address--);
								for (i=0; i<66; i++)
								{
									for (j=0; j<=line_repeat_count; j++)
									{
										for (k=0; k<8; k++)
										{
											plot_pixel(bitmap, i*8+k, current_scan_line/2 + j, Machine->pens[ (current_data>>k) & 0x01 ? 15 : 5 ]);
										}
									}
								}
								break;
						}
//						logerror ("Mode 0, Resolution 3, Lines %d\n", line_repeat_count);
						current_scan_line += line_repeat_count*2+2;
						break;
				}
				break;

			case 0x01:	/* 4 colour characters */
				switch (horizontal_resolution)
				{
					case 0x00:	/* 11 chars */
						current_scan_line = 520;
						break;
					case 0x01:	/* 22 chars */
						for (i=0; i<22; i++)
						{
							current_data = cpu_readmem16(current_video_memory_address--);
							current_color = cpu_readmem16(current_video_memory_address--);
							for (j=0; j<=line_repeat_count; j++)
							{
								for (k=0; k<8; k++)
								{
									plot_pixel(bitmap, i*8*3+k*3+0, current_scan_line/2 + j, Machine->pens[ (char_rom[current_data*16+j]>>k) & 0x01 ? 15 : 5 ]);
									plot_pixel(bitmap, i*8*3+k*3+1, current_scan_line/2 + j, Machine->pens[ (char_rom[current_data*16+j]>>k) & 0x01 ? 15 : 5 ]);
									plot_pixel(bitmap, i*8*3+k*3+2, current_scan_line/2 + j, Machine->pens[ (char_rom[current_data*16+j]>>k) & 0x01 ? 15 : 5 ]);
								}
							}
						}
//						logerror ("Mode 1, Resolution 1, Lines %d\n", line_repeat_count);
						current_scan_line += line_repeat_count*2+2;
						current_video_memory_address-=2;
						break;
					case 0x02:	/* 44 chars */
						current_scan_line = 520;
						break;
					case 0x03:	/* 66 chars */
						switch (unit_mode)
						{
							case 0:
								current_data = cpu_readmem16(current_video_memory_address--);
								current_color = cpu_readmem16(current_video_memory_address--);
								for (i=0; i<66; i++)
								{
									for (j=0; j<=line_repeat_count; j++)
									{
										for (k=0; k<8; k++)
										{
											plot_pixel(bitmap, i*8+k, current_scan_line/2 + j, Machine->pens[ (char_rom[current_data*16+j]>>k) & 0x01 ? 15 : 5 ]);
										}
									}
								}
								break;
							case 1:
								for (i=0; i<66; i++)
								{
									current_data = cpu_readmem16(current_video_memory_address--);
									current_color = cpu_readmem16(current_video_memory_address--);
									for (j=0; j<=line_repeat_count; j++)
									{
										for (k=0; k<8; k++)
										{
											plot_pixel(bitmap, i*8+k, current_scan_line/2 + j, Machine->pens[ (char_rom[current_data*16+j]>>k) & 0x01 ? 15 : 5 ]);
										}
									}
								}
								break;
						}
						current_scan_line += line_repeat_count*2+2;
//						logerror ("Mode 1, Resolution 3, Lines %d\n", line_repeat_count);
						break;
				}
				break;

			case 0x02:	/* 16 colour graphics */
				current_scan_line = 520;
				break;

			case 0x03:	/* 16 colour characters */
				switch (horizontal_resolution)
				{
					case 0x00:	/* 11 chars */
						break;
					case 0x01:	/* 22 chars */
						break;
					case 0x02:	/* 44 chars */
						break;
					case 0x03:	/* 66 chars */
						break;
				}
				current_scan_line = 520;
				break;
		}
	}
}
