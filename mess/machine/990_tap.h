/*
	990_tap.h: include file for 990_tap.c
*/

DEVICE_LOAD( ti990_cassette );
DEVICE_UNLOAD( ti990_cassette );

void ti990_tpc_init(void (*interrupt_callback)(int state));
void ti990_tpc_exit(void);

extern READ16_HANDLER(ti990_tpc_r);
extern WRITE16_HANDLER(ti990_tpc_w);

