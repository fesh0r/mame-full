/*************************************************************************

	Driver for Gaelco 3D games

	driver by Aaron Giles

**************************************************************************/

/*----------- defined in vidhrdw/gaelco3d.c -----------*/

offs_t gaelco3d_mask_offset;
offs_t gaelco3d_mask_size;

WRITE32_HANDLER( gaelco3d_render_w );

WRITE16_HANDLER( gaelco3d_paletteram_w );
WRITE32_HANDLER( gaelco3d_paletteram_020_w );

VIDEO_START( gaelco3d );
VIDEO_UPDATE( gaelco3d );
