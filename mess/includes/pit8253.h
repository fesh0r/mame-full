#ifndef PIT8253_H
#define PIT8253_H

/*****************************************************************************
 *
 *	Programmable Interval Timer 8253/8254
 *
 *****************************************************************************/

#define MAX_PIT8253 2

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { TYPE8253, TYPE8254 } PIT8253_TYPE;

typedef struct {
    PIT8253_TYPE type;
	struct {
		double clockin;
		void (*irq_callback)(int state);
		void (*clk_callback)(double clockout);
	} timer[3];
} PIT8253_CONFIG;

void pit8253_config(int which, PIT8253_CONFIG *config);
void pit8253_reset(int which);

READ_HANDLER ( pit8253_0_r );
READ_HANDLER ( pit8253_1_r );

WRITE_HANDLER ( pit8253_0_w );
WRITE_HANDLER ( pit8253_1_w );

WRITE_HANDLER ( pit8253_0_gate_w );
WRITE_HANDLER ( pit8253_1_gate_w );

int pit8253_get_frequency(int which, int timer);
int pit8253_get_output(int which, int timer);
void pit8253_set_clockin(int which, int timer, double new_clockin);

#ifdef __cplusplus
}
#endif

#endif	/* PIT8253_H */

