BLOCKDEVICE_FORMATDRIVER_START( pc_disk )
	BDFD_NAME( "pc_disk" )
	BDFD_HUMANNAME( "PC floppy disk image" )
	BDFD_EXTENSIONS( "ima\0dsk\0" )
	BDFD_GEOMETRY(        8/9, 1/2, 40, 512 )	/* 5 1/4 inch double density */
	BDFD_GEOMETRY(         10,   2, 40, 512 )
	BDFD_GEOMETRY( 9/15/18/36,   2, 80, 512 )	/* 3 1/2 inch double/high/enhanced density */
BLOCKDEVICE_FORMATDRIVER_END


