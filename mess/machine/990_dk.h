/*
	990_dk.h: include file for 990_dk.c
*/

DEVICE_INIT( fd800 );
DEVICE_LOAD( fd800 );
/*DEVICE_UNLOAD( fd800 );*/

void fd800_machine_init(void (*interrupt_callback)(int state));

extern READ_HANDLER(fd800_cru_r);
extern WRITE_HANDLER(fd800_cru_w);

