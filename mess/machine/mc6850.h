/**********************************************************************

	Motorola 6850 ACIA interface and emulation

	This function is a simple emulation of up to 4 MC6850 
	Asynchronous Communications Interface Adapter.

**********************************************************************/

#ifndef ACIA_6850
#define ACIA_6850

#define ACIA_6850_MAX 4

#define ACIA_6850_CTRL 0
#define ACIA_6850_DATA 1

#define ACIA_6850_RDRF	0x01	/* Receive data register full */
#define ACIA_6850_TDRE	0x02	/* Transmit data register empty */
#define ACIA_6850_dcd	0x04	/* Data carrier detect, active low */
#define ACIA_6850_cts	0x08	/* Clear to send, active low */
#define ACIA_6850_FE	0x10	/* Framing error */
#define ACIA_6850_OVRN	0x20	/* Receiver overrun */
#define ACIA_6850_PE	0x40	/* Parity error */
#define ACIA_6850_irq	0x80	/* Interrupt request, active low */

struct acia6850_interface
{
	read8_handler in_status_func;
	read8_handler in_recv_func;
	write8_handler out_status_func;
	write8_handler out_tran_func;
};

void acia6850_unconfig(void);
void acia6850_config( int which, const struct acia6850_interface *intf);
void acia6850_reset(void);
int acia6850_read( int which, int offset);
void acia6850_write( int which, int offset, int data);

READ_HANDLER( acia6850_0_r );
READ_HANDLER( acia6850_1_r );
READ_HANDLER( acia6850_2_r );
READ_HANDLER( acia6850_3_r );

WRITE_HANDLER( acia6850_0_w );
WRITE_HANDLER( acia6850_1_w );
WRITE_HANDLER( acia6850_2_w );
WRITE_HANDLER( acia6850_3_w );

#endif /* ACIA_6850 */
