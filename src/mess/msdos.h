#ifndef MESSDOS_H
#define MESSDOS_H


//#include "mess/mess.h"

char *get_alias(const char *driver_name, char *argv);
int check_crc(int crc, int length, char *driver);
int load_image(int argc, char **argv, int j, int game_index);
void list_mess_info(char *gamename, char *arg);


#endif
