
#ifndef _SMS_H_
#define _SMS_H_

#define SMS_ROM_MAXSIZE (0x810200)

/* Global data */

extern unsigned char sms_page_count;
extern unsigned char sms_fm_detect;
extern unsigned char sms_version;

/* Function prototypes */

WRITE_HANDLER ( sms_cartram_w );
WRITE_HANDLER ( sms_ram_w );
WRITE_HANDLER ( sms_fm_detect_w );
READ_HANDLER  ( sms_fm_detect_r );
WRITE_HANDLER ( sms_version_w );
READ_HANDLER  ( sms_version_r );
WRITE_HANDLER ( sms_mapper_w );
WRITE_HANDLER ( gg_sio_w );
READ_HANDLER  ( gg_sio_r );
WRITE_HANDLER ( gg_psg_w );

int sms_load_rom(int id);
void sms_init_machine (void);
int sms_id_rom (int id);
int gamegear_id_rom (int id);

#endif /* _SMS_H_ */
