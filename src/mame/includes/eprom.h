/*************************************************************************

    Atari Escape hardware

*************************************************************************/

/*----------- defined in video/eprom.c -----------*/

VIDEO_START( eprom );
VIDEO_UPDATE( eprom );

void eprom_scanline_update(int scanline);
