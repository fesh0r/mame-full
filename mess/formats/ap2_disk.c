#include "ap2_disk.h"

BLOCKDEVICE_FORMATDRIVER_START( apple2_dsk )
	BDFD_NAME( "dsk" )
	BDFD_HUMANNAME( "DOS3.3 Order Disk Image" )
	BDFD_EXTENSION( "dsk" )
	BDFD_TRACKS_OPTION( 35 )
	BDFD_SECTORS_OPTION( 16 )
	BDFD_BYTES_PER_SECTOR( 256 )
BLOCKDEVICE_FORMATDRIVER_END

/* ----------------------------------------------------------------------- */

BLOCKDEVICE_FORMATCHOICES_START( apple2 )
	BDFC_CHOICE( apple2_dsk )
BLOCKDEVICE_FORMATCHOICES_END

