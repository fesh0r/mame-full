/* MSM 8251/INTEL 8251 UART */

#include "includes/msm8251.h"


/* uncomment to enable verbose comments */
#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror("MSM8251: "x)
#else
#define LOG(x)
#endif

static struct msm8251 uart;

/* init */
void	msm8251_init(struct msm8251_interface *iface)
{
	/* copy interface */
	if (iface!=NULL)
	{
		memcpy(&uart.interface, iface, sizeof(struct msm8251_interface));
	}
	
	uart.baud_rate = -1;
	/* reset chip */
	msm8251_reset();
}

/* stop */
void	msm8251_stop(void)
{
	if (uart.timer!=NULL)
	{
		timer_remove(uart.timer);
		uart.timer = NULL;
	}
}

static void msm8251_update_rx_ready(void)
{
	UINT8 state;

	state = uart.status & MSM8251_STATUS_RX_READY;

	/* masked? */
	if ((uart.command & (1<<2))==0)
	{
		state = 0;
	}

	if (uart.interface.rx_ready_callback)
		uart.interface.rx_ready_callback((state!=0));
	
}

static void uart_timer_callback(int dummy)
{


}


static void msm8251_update_tx_ready(void)
{
	UINT8 state;

	state = uart.flags & MSM8251_TX_READY;

	/* masked? */
	if ((uart.command & (1<<0))==0)
	{
		state = 0;

	}

	if (uart.interface.tx_ready_callback)
		uart.interface.tx_ready_callback((state!=0));
}

static void msm8251_update_tx_empty(void)
{
	if (uart.interface.tx_empty_callback)
		uart.interface.tx_empty_callback(((uart.status & MSM8251_STATUS_TX_EMPTY)!=0));
}

/* set baud rate */
void	msm8251_set_baud_rate(unsigned long rate)
{
	if (rate!=uart.baud_rate)
	{
		if (uart.timer!=NULL)
		{
			timer_remove(uart.timer);
		}
		uart.timer = NULL;
	}

	uart.timer = timer_pulse(TIME_IN_HZ(rate), 0, uart_timer_callback);
}

void	msm8251_reset(void)
{
	LOG("MSM8251 Reset\r\n");

	uart.flags |= MSM8251_EXPECTING_MODE;
	uart.flags &= ~MSM8251_EXPECTING_SYNC_BYTE;
	uart.status |= MSM8251_STATUS_TX_EMPTY | MSM8251_STATUS_RX_READY;
	uart.flags |= MSM8251_TX_READY;
	msm8251_update_tx_empty();
	msm8251_update_rx_ready();
	msm8251_update_tx_ready();
}

/* write command */
WRITE_HANDLER(msm8251_control_w)
{
	if (uart.flags & MSM8251_EXPECTING_MODE)
	{
		if (uart.flags & MSM8251_EXPECTING_SYNC_BYTE)
		{
			LOG("Sync byte\r\n");

#ifdef VERBOSE
			logerror("Sync byte: %02x\r\n", data);
#endif
			/* store sync byte written */
			uart.sync_bytes[uart.sync_byte_offset] = data;
			uart.sync_byte_offset++;
			uart.sync_byte_count--;

			if (uart.sync_byte_count==0)
			{
				/* finished transfering sync bytes, now expecting command */
				uart.flags &= ~(MSM8251_EXPECTING_MODE | MSM8251_EXPECTING_SYNC_BYTE);
			}
		}
		else
		{
			LOG("Mode byte\r\n");

			/* Synchronous or Asynchronous? */
			if ((data & 0x03)==0)
			{
				/*	Asynchronous
				
					bit 7,6: stop bit length
						0 = inhibit
						1 = 1 bit
						2 = 1.5 bits
						3 = 2 bits
					bit 5: parity type
						0 = parity odd
						1 = parity even
					bit 4: parity test enable
						0 = disable
						1 = enable
					bit 3,2: character length
						0 = 5 bits
						1 = 6 bits
						2 = 7 bits
						3 = 8 bits
					bit 1,0: baud rate factor 
						0 = defines command byte for synchronous or asynchronous
						1 = x1
						2 = x16
						3 = x64
				*/

				LOG("Asynchronous operation\r\n");

				/* expecting mode byte now */
				uart.flags &= ~MSM8251_EXPECTING_MODE;
			}
			else
			{
				/*	bit 7: Number of sync characters 
						0 = 1 character
						1 = 2 character
					bit 6: Synchronous mode
						0 = Internal synchronisation
						1 = External synchronisation
					bit 5: parity type
						0 = parity odd
						1 = parity even
					bit 4: parity test enable
						0 = disable
						1 = enable
					bit 3,2: character length
						0 = 5 bits
						1 = 6 bits
						2 = 7 bits
						3 = 8 bits
					bit 1,0 = 0
				*/
				LOG("Synchronous operation\r\n");

				/* setup for sync byte(s) */
				uart.flags |= MSM8251_EXPECTING_SYNC_BYTE;
				uart.sync_byte_offset = 0;
				if (data & 0x07)
				{
					uart.sync_byte_count = 1;
				}
				else
				{
					uart.sync_byte_count = 2;
				}

			}
		}
	}
	else
	{
		/* command */
		LOG("Command byte\r\n");

		uart.command = data;
#ifdef VERBOSE
			logerror("Command byte: %02x\r\n", data);
#endif


		/*	bit 7: 
				0 = normal operation
				1 = hunt mode
			bit 6: 
				0 = normal operation
				1 = internal reset
			bit 5:
				0 = /RTS set to 1
				1 = /RTS set to 0
			bit 4:
				0 = normal operation
				1 = reset error flag
			bit 3:
				0 = normal operation
				1 = send break character
			bit 2:
				0 = receive disable
				1 = receive enable
			bit 1:
				0 = /DTR set to 1
				1 = /DTR set to 0
			bit 0:
				0 = transmit disable
				1 = transmit enable 
		*/

		if (data & (1<<4))
		{
			uart.status &= ~(MSM8251_STATUS_PARITY_ERROR | MSM8251_STATUS_OVERRUN_ERROR | MSM8251_STATUS_FRAMING_ERROR);
		}

		if (data & (1<<6))
		{
			msm8251_reset();
		}

	}
}

/* read status */
READ_HANDLER(msm8251_status_r)
{
	uart.status &= ~((1<<2) | (1<<1));

	return uart.status;
}

/* write data */
WRITE_HANDLER(msm8251_data_w)
{
	uart.data = data;

	/* writing clears */
	uart.flags &= ~MSM8251_TX_READY;
	/* writing sets - it's not empty */
	uart.status &= ~MSM8251_STATUS_TX_EMPTY;

	msm8251_update_tx_ready();
	msm8251_update_tx_empty();

}

/* called when last bit of data has been received */
static void msm8251_receive_character(UINT8 ch)
{
	uart.data = ch;
	
	/* char has not been read and another has arrived! */
	if (uart.status & MSM8251_STATUS_RX_READY)
	{
		uart.status |= MSM8251_STATUS_OVERRUN_ERROR;
	}
	uart.status |= MSM8251_STATUS_RX_READY;

	msm8251_update_rx_ready();
}

/* read data */
READ_HANDLER(msm8251_data_r)
{
	/* reading clears */
	uart.status &= ~MSM8251_STATUS_RX_READY;
	
	msm8251_update_tx_ready();

	return uart.data;
}
