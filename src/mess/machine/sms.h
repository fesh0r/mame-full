
#ifndef _SMS_H_
#define _SMS_H_

#define SMS_ROM_MAXSIZE (0x810200)

/* Global data */

extern unsigned char sms_page_count;
extern unsigned char sms_fm_detect;
extern unsigned char sms_version;

/* Function prototypes */

void sms_cartram_w(int offset, int data);
void sms_ram_w(int offset, int data);
void sms_fm_detect_w(int offset, int data);
int sms_fm_detect_r(int offset);
void sms_version_w(int offset, int data);
int sms_version_r(int offset);
void sms_mapper_w(int offset, int data);
void gg_sio_w(int offset, int data);
int gg_sio_r(int offset);
void gg_psg_w(int offset, int data);

int sms_load_rom(int id);
void sms_init_machine (void);
int sms_id_rom (int id);
int gamegear_id_rom (int id);

#endif /* _SMS_H_ */
