/*
	smartmed.h: header file for smartmed.c
*/

DEVICE_INIT( smartmedia );
DEVICE_LOAD( smartmedia );
DEVICE_UNLOAD( smartmedia );

int smartmedia_machine_init(int id);

int smartmedia_present(int id);
int smartmedia_protected(int id);

data8_t smartmedia_data_r(int id);
void smartmedia_command_w(int id, data8_t data);
void smartmedia_address_w(int id, data8_t data);
void smartmedia_data_w(int id, data8_t data);
