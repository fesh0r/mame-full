#ifndef PIT8253_H
#define PIT8253_H

/*****************************************************************************
 *
 *	Programmable Interval Timer 8253/8254
 *
 *****************************************************************************/

#define MAX_PIT8253 2

typedef struct {
    enum { TYPE8253, TYPE8254 } type;
	struct {
		double clockin;
		void (*irq_callback)(int state);
		void (*clk_callback)(double clockout);
	} timer[3];
} PIT8253_CONFIG;

extern void pit8253_config(int which, PIT8253_CONFIG *config);
extern void pit8253_reset(int which);

extern READ_HANDLER ( pit8253_0_r );
extern READ_HANDLER ( pit8253_1_r );

extern WRITE_HANDLER ( pit8253_0_w );
extern WRITE_HANDLER ( pit8253_1_w );

extern WRITE_HANDLER ( pit8253_0_gate_w );
extern WRITE_HANDLER ( pit8253_1_gate_w );

extern int pit8253_get_frequency(int which, int timer);
extern int pit8253_get_output(int which, int timer);
extern void pit8253_set_clockin(int which, int timer, double new_clockin);

#endif	/* PIT8253_H */

