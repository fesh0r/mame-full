/*************************************************************************

	Driver for Gaelco 3D games

	driver by Aaron Giles

**************************************************************************/

#include "driver.h"
#include "gaelco3d.h"
#include "vidhrdw/poly.h"

offs_t gaelco3d_mask_offset;
offs_t gaelco3d_mask_size;


#define MAX_POLYGONS	4096
#define LOG_POLYGONS	1
#define ENABLE_ZBUFFER	0

#define IS_POLYEND(x)	(((x) ^ ((x) >> 1)) & 0x4000)


static struct mame_bitmap *screenbits;
static struct mame_bitmap *zbufbits;
static int polygons;
static UINT32 polydata_buffer[MAX_POLYGONS * 21];
static UINT32 polydata_count;

/*

d[0] = ($9C66) * ($9C70)
d[1] = (matrix[1][2] * vector[0]) - (matrix[1][0] * vector[2])
d[2] = (matrix[1][1] * vector[2]) - (matrix[1][2] * vector[1])
d[3] = matrix[0][1]
d[4] = matrix[0][0]
d[5] = (matrix[2][0] * vector[2]) - (matrix[2][2] * vector[0])
d[6] = (matrix[2][2] * vector[1]) - (matrix[2][1] * vector[2])
d[7] = ($9C70) * ((matrix[1][0] * vector[1]) - (matrix[1][1] * vector[0]))
d[8] = ($9C70) * matrix[0][2]
d[9] = ($9C70) * ((matrix[2][1] * vector[0]) - (matrix[2][0] * vector[1]))


  ( m00 m01 m02 ) ( v0 )    ( m00 * v0 + m01 * v1 + m02 * v2 )
  ( m10 m11 m12 ) ( v1 ) =  ( m10 * v0 + m11 * v1 + m12 * v2 )
  ( m20 m21 m22 ) ( v2 )    ( m20 * v0 + m21 * v1 + m22 * v2 )

*/

typedef union int_double
{
	double d;
	float f[2];
	UINT32 i[2];
} int_double;


static float dsp_to_float(UINT32 val)
{
	INT32 _mantissa = val << 8;
	INT8 _exponent = (INT32)val >> 24;
	int_double id;

	if (_mantissa == 0 && _exponent == -128)
		return 0;
	else if (_mantissa >= 0)
	{
		int exponent = (_exponent + 127) << 23;
		id.i[0] = exponent + (_mantissa >> 8);
	}
	else
	{
		int exponent = (_exponent + 127) << 23;
		INT32 man = -_mantissa;
		id.i[0] = 0x80000000 + exponent + ((man >> 8) & 0x00ffffff);
	}
	return id.f[0];
}


static void render_poly(UINT32 *polydata)
{
	float ooz = dsp_to_float(polydata[8]);
	float dzdx = dsp_to_float(polydata[4]);
	float dzdy = -dsp_to_float(polydata[3]);
	float dudx = dsp_to_float(polydata[6]);
	float dudy = -dsp_to_float(polydata[5]);
	float dvdx = dsp_to_float(polydata[2]);
	float dvdy = -dsp_to_float(polydata[1]);
	int xorigin = (INT32)polydata[12] >> 16;
	int yorigin = (INT32)(polydata[12] << 18) >> 18;
	int color = (polydata[10] & 0x7f) << 8;
	int midx = Machine->drv->screen_width/2;
	int midy = Machine->drv->screen_height/2;
	int i, x0, y0, x1, y1, x2, y2;
	
	if (LOG_POLYGONS)
	{
		logerror("poly: %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %12.2f %08X %08X (%4d,%4d)",
				(double)dsp_to_float(polydata[0]),
				(double)dsp_to_float(polydata[1]),
				(double)dsp_to_float(polydata[2]),
				(double)dsp_to_float(polydata[3]),
				(double)dsp_to_float(polydata[4]),
				(double)dsp_to_float(polydata[5]),
				(double)dsp_to_float(polydata[6]),
				(double)dsp_to_float(polydata[7]),
				(double)dsp_to_float(polydata[8]),
				(double)dsp_to_float(polydata[9]),
				polydata[10],
				polydata[11],
				(INT16)(polydata[12] >> 16), (INT16)(polydata[12] << 2) >> 2);
		
		logerror(" (%4d,%4d) %08X", (INT16)(polydata[13] >> 16), (INT16)(polydata[13] << 2) >> 2, polydata[14]);
		for (i = 15; !IS_POLYEND(polydata[i - 2]); i += 2)
			logerror(" (%4d,%4d) %08X", (INT16)(polydata[i] >> 16), (INT16)(polydata[i] << 2) >> 2, polydata[i+1]);
		logerror("\n");
	}

	x0 = (INT32)polydata[13] >> 16;
	y0 = (INT32)(polydata[13] << 18) >> 18;
	x1 = (INT32)polydata[15] >> 16;
	y1 = (INT32)(polydata[15] << 18) >> 18;
	
	xorigin = midx + xorigin;
	yorigin = midy - yorigin;

	for (i = 17; !IS_POLYEND(polydata[i - 2]); i += 2)
	{
		x2 = (INT32)polydata[i] >> 16;
		y2 = (INT32)(polydata[i] << 18) >> 18;
		{
			UINT8 *src = memory_region(REGION_USER3);
			offs_t endmask = gaelco3d_mask_offset - 1;
			const struct poly_scanline_data *scans;
			struct poly_vertex vert[3];
			UINT32 tex = polydata[11];
			int x, y, minx;
			
			vert[0].x = midx + x0;
			vert[0].y = midy - y0;
			vert[1].x = midx + x1;
			vert[1].y = midy - y1;
			vert[2].x = midx + x2;
			vert[2].y = midy - y2;
			scans = setup_triangle_0(&vert[0], &vert[1], &vert[2], &Machine->visible_area);
			if (scans)
			{
				minx = x0;
				if (x1 < minx) minx = x1;
				if (x2 < minx) minx = x2;
			
				for (y = scans->sy; y <= scans->ey; y++)
				{
					const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
					UINT16 *dest = screenbits->line[y];
					float *zbuf = zbufbits->line[y];
					
					for (x = scan->sx; x <= scan->ex; x++)
					{
						float z = 1.0f / (ooz + dzdx * (x - xorigin) + dzdy * (y - yorigin));
						int u = (int)((dudx * (x - xorigin) + dudy * (y - yorigin)) * z);
						int v = (int)((dvdx * (x - xorigin) + dvdy * (y - yorigin)) * z);
						int pixeloffs = (tex + v * 4096 + u) & endmask;
						if (!ENABLE_ZBUFFER || ooz > zbuf[x])
							if (pixeloffs >= gaelco3d_mask_size || !src[pixeloffs + gaelco3d_mask_offset])
							{
								dest[x] = color | src[pixeloffs];
								if (ENABLE_ZBUFFER)
									zbuf[x] = ooz;
							}
					}
				}
			}
		}
		x1 = x2;
		y1 = y2;
	}
}


WRITE32_HANDLER( gaelco3d_render_w )
{
//logerror("%06X:gaelco3d_render_w(%d,%08X)\n", activecpu_get_pc(), offset, data);

	polydata_buffer[polydata_count++] = data;
	if (polydata_count >= 15 && (polydata_count & 1) && IS_POLYEND(polydata_buffer[polydata_count - 2]))
	{
		if (ENABLE_ZBUFFER && polygons == 0)
		{
			float val = -1000000;
			fillbitmap(zbufbits, *(UINT32 *)&val, NULL);
		}
		polygons++;
		render_poly(polydata_buffer);
		polydata_count = 0;
	}
}


INLINE void update_palette_entry(offs_t offset, data16_t data)
{
	int r = (data >> 10) & 0x1f;
	int g = (data >> 5) & 0x1f;
	int b = (data >> 0) & 0x1f;
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	palette_set_color(offset, r, g, b);
}


WRITE16_HANDLER( gaelco3d_paletteram_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	update_palette_entry(offset, paletteram16[offset]);
}


WRITE32_HANDLER( gaelco3d_paletteram_020_w )
{
	COMBINE_DATA(&paletteram32[offset]);
	update_palette_entry(offset*2+0, paletteram32[offset] >> 16);
	update_palette_entry(offset*2+1, paletteram32[offset]);
}


VIDEO_START( gaelco3d )
{
	screenbits = auto_bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 16);
	if (!screenbits)
		return 1;
	
	if (ENABLE_ZBUFFER)
	{
		zbufbits = auto_bitmap_alloc_depth(Machine->drv->screen_width, Machine->drv->screen_height, 32);
		if (!zbufbits)
			return 1;
	}
	
	return 0;
}


VIDEO_UPDATE( gaelco3d )
{
	int x, y;

	if (keyboard_pressed(KEYCODE_Z))
	{
		static int xv = 0, yv = 0x1000;
		UINT8 *base = memory_region(REGION_USER3);
		
		if (keyboard_pressed(KEYCODE_LEFT) && xv >= 4)
			xv -= 4;
		if (keyboard_pressed(KEYCODE_RIGHT) && xv < 4096 - 4)
			xv += 4;
		
		if (keyboard_pressed(KEYCODE_UP) && yv >= 4)
			yv -= 4;
		if (keyboard_pressed(KEYCODE_DOWN) && yv < 0x40000)
			yv += 4;
		
		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			UINT16 *dest = bitmap->line[y];
			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				int offs = (yv + y - cliprect->min_y) * 4096 + xv + x - cliprect->min_x;
				if (offs < memory_region_length(REGION_USER3))
					dest[x] = base[offs];
				else
					dest[x] = 0;
			}
		}
		usrintf_showmessage("(%04X,%04X)", xv, yv);
	}
	else
		copybitmap(bitmap, screenbits, 0,0, 0,0, cliprect, TRANSPARENCY_NONE, 0);

	polygons = 0;
	logerror("---update---\n");
}
