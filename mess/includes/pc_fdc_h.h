#include "driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* if not 1, DACK and TC inputs to FDC are disabled, and DRQ and IRQ are held
at high impedance i.e they are not affective */ 
#define PC_FDC_FLAGS_DOR_DMA_ENABLED (1<<3)
#define PC_FDC_FLAGS_DOR_FDC_ENABLED (1<<2)
#define PC_FDC_FLAGS_DOR_MOTOR_ON	(1<<4)

/* interface has been seperated, so that it can be used in the super i/o chip */

#define PC_FDC_STATUS_REGISTER_A 0
#define PC_FDC_STATUS_REGISTER_B 1
#define PC_FDC_DIGITAL_OUTPUT_REGISTER 2
#define PC_FDC_TAPE_DRIVE_REGISTER 3
#define PC_FDC_MAIN_STATUS_REGISTER 4
#define PC_FDC_DATA_RATE_REGISTER 4
#define PC_FDC_DATA_REGISTER 5
#define PC_FDC_RESERVED_REGISTER 6
#define PC_FDC_DIGITIAL_INPUT_REGISTER 7
#define PC_FDC_CONFIGURATION_CONTROL_REGISTER 8

/* main interface */
typedef struct pc_fdc_hw_interface
{
	void	(*pc_fdc_interrupt)(int);
	void	(*pc_fdc_dma_drq)(int,int);
} pc_fdc_hw_interface;

/* registers etc */
typedef struct pc_fdc
{
	int status_register_a;
	int status_register_b;
	int digital_output_register;
	int tape_drive_register;
	int data_rate_register;
	int digital_input_register;
	int configuration_control_register;

	/* stored tc state - state present at pins */
	int tc_state;
	/* stored dma drq state */
	int dma_state;
	int dma_read;
	/* stored int state */
	int int_state;

	pc_fdc_hw_interface fdc_interface;
} pc_fdc;

READ_HANDLER(pc_fdc_r);
WRITE_HANDLER(pc_fdc_w);

void pc_fdc_init(pc_fdc_hw_interface *iface);
void pc_fdc_set_tc_state(int state);
int	pc_fdc_dack_r(void);
void pc_fdc_dack_w(int);

void pc_fdc_setup(void);

#ifdef __cplusplus
}
#endif

