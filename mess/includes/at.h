
void init_atcga(void);
void init_at_vga(void);

void at_machine_init(void);

int at_cga_frame_interrupt (void);
int at_vga_frame_interrupt (void);

READ_HANDLER(at_mfm_0_r);
WRITE_HANDLER(at_mfm_0_w);

READ_HANDLER(at_8042_r);
WRITE_HANDLER(at_8042_w);

void pc_ide_data_w(UINT8 data);
void pc_ide_write_precomp_w(UINT8 data);
void pc_ide_sector_count_w(UINT8 data);
void pc_ide_sector_number_w(UINT8 data);
void pc_ide_cylinder_number_l_w(UINT8 data);
void pc_ide_cylinder_number_h_w(UINT8 data);
void pc_ide_drive_head_w(UINT8 data);
void pc_ide_command_w(UINT8 data);
UINT8 pc_ide_data_r(void);
UINT8 pc_ide_error_r(void);
UINT8 pc_ide_sector_count_r(void);
UINT8 pc_ide_sector_number_r(void);
UINT8 pc_ide_cylinder_number_l_r(void);
UINT8 pc_ide_cylinder_number_h_r(void);
UINT8 pc_ide_drive_head_r(void);
UINT8 pc_ide_status_r(void);
