/**********************************************************************
	Metal Oxid Semiconductor / Commodore Business Machines
        Complex Interface Adapter 6526

	based on 6522via emulation
**********************************************************************/

#ifndef __CIA_6526_H_
#define __CIA_6526_H_

#define MAX_CIA 8

struct cia6526_interface
{
	int (*in_a_func) (int offset);
	int (*in_b_func) (int offset);
	void (*out_a_func) (int offset, int val);
	void (*out_b_func) (int offset, int val);
	void (*out_pc_func) (int offset, int val);
	void (*in_sp_func) (int offset);
	void (*out_sp_func) (int offset);
	void (*in_cnt_func) (int offset);
	void (*out_cnt_func) (int offset);
	void (*irq_func) (int state);
	UINT8 a_pullup, b_pullup;
	int todin50hz;
};

void cia6526_init(void);
void cia6526_config(int which, const struct cia6526_interface *intf);
void cia6526_reset(void);

void cia6526_status(char *text, int size);

/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

 READ8_HANDLER  ( cia6526_0_port_r );
 READ8_HANDLER  ( cia6526_1_port_r );
 READ8_HANDLER  ( cia6526_2_port_r );
 READ8_HANDLER  ( cia6526_3_port_r );
 READ8_HANDLER  ( cia6526_4_port_r );
 READ8_HANDLER  ( cia6526_5_port_r );
 READ8_HANDLER  ( cia6526_6_port_r );
 READ8_HANDLER  ( cia6526_7_port_r );

WRITE8_HANDLER ( cia6526_0_port_w );
WRITE8_HANDLER ( cia6526_1_port_w );
WRITE8_HANDLER ( cia6526_2_port_w );
WRITE8_HANDLER ( cia6526_3_port_w );
WRITE8_HANDLER ( cia6526_4_port_w );
WRITE8_HANDLER ( cia6526_5_port_w );
WRITE8_HANDLER ( cia6526_6_port_w );
WRITE8_HANDLER ( cia6526_7_port_w );

/******************* 8-bit A/B port interfaces *******************/

WRITE8_HANDLER ( cia6526_0_porta_w );
WRITE8_HANDLER ( cia6526_1_porta_w );
WRITE8_HANDLER ( cia6526_2_porta_w );
WRITE8_HANDLER ( cia6526_3_porta_w );
WRITE8_HANDLER ( cia6526_4_porta_w );
WRITE8_HANDLER ( cia6526_5_porta_w );
WRITE8_HANDLER ( cia6526_6_porta_w );
WRITE8_HANDLER ( cia6526_7_porta_w );

WRITE8_HANDLER ( cia6526_0_portb_w );
WRITE8_HANDLER ( cia6526_1_portb_w );
WRITE8_HANDLER ( cia6526_2_portb_w );
WRITE8_HANDLER ( cia6526_3_portb_w );
WRITE8_HANDLER ( cia6526_4_portb_w );
WRITE8_HANDLER ( cia6526_5_portb_w );
WRITE8_HANDLER ( cia6526_6_portb_w );
WRITE8_HANDLER ( cia6526_7_portb_w );

 READ8_HANDLER  ( cia6526_0_porta_r );
 READ8_HANDLER  ( cia6526_1_porta_r );
 READ8_HANDLER  ( cia6526_2_porta_r );
 READ8_HANDLER  ( cia6526_3_porta_r );
 READ8_HANDLER  ( cia6526_4_porta_r );
 READ8_HANDLER  ( cia6526_5_porta_r );
 READ8_HANDLER  ( cia6526_6_porta_r );
 READ8_HANDLER  ( cia6526_7_porta_r );

void cia6526_0_set_input_flag (int data);
void cia6526_1_set_input_flag (int data);
void cia6526_2_set_input_flag (int data);
void cia6526_3_set_input_flag (int data);
void cia6526_4_set_input_flag (int data);
void cia6526_5_set_input_flag (int data);
void cia6526_6_set_input_flag (int data);
void cia6526_7_set_input_flag (int data);

void cia6526_0_set_input_sp (int data);
void cia6526_1_set_input_sp (int data);
void cia6526_2_set_input_sp (int data);
void cia6526_3_set_input_sp (int data);
void cia6526_4_set_input_sp (int data);
void cia6526_5_set_input_sp (int data);
void cia6526_6_set_input_sp (int data);
void cia6526_7_set_input_sp (int data);

void cia6526_0_set_input_cnt (int data);
void cia6526_1_set_input_cnt (int data);
void cia6526_2_set_input_cnt (int data);
void cia6526_3_set_input_cnt (int data);
void cia6526_4_set_input_cnt (int data);
void cia6526_5_set_input_cnt (int data);
void cia6526_6_set_input_cnt (int data);
void cia6526_7_set_input_cnt (int data);

#endif
