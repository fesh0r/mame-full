/* MSM 8251/INTEL 8251 UART */

#include "includes/msm8251.h"
#include "includes/serial.h"

/* uncomment to enable verbose comments */
#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror("MSM8251: "x)
#else
#define LOG(x)
#endif

static struct msm8251 uart;
static void msm8251_receive_character(UINT8 ch);

static UINT8 msm8251_parity_table[256];


/* init */
void	msm8251_init(struct msm8251_interface *iface)
{
	int i;

	/* if sum of all bits in the byte is even, then the data
	has even parity, otherwise it has odd parity */
	for (i=0; i<256; i++)
	{
		int data;
		int sum;
		int b;

		sum = 0;
		data = i;

		for (b=0; b<8; b++)
		{
			sum+=data & 0x01;

			data = data>>1;
		}

		msm8251_parity_table[i] = sum & 0x01;
	}
	
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

static void	msm8251_receive_bit(int bit)
{
	logerror("msm8251 receive bit: %1x\n",bit);

	/* shift previous bit 7 out */
	uart.receive_char = uart.data<<1;
	/* shift new bit in */
	uart.receive_char = (uart.data & 0x0fe) | bit;
	/* update bit count received */
	uart.bit_count_received++;

	if (uart.bit_count_received==uart.receive_char_length)
	{
		UINT8 data;

		uart.bit_count_received = 0;

		logerror("msm8251 receive char\n");

		/* mask off other bits so data byte has 0's in unused bits */
		data = uart.receive_char & (0x0ff
			<<
			(8-(((uart.mode_byte>>2) & 0x03)+5)));

		/* parity enable? */
		if (uart.mode_byte & (1<<4))
		{
			logerror("checking parity\n");

			if (msm8251_parity_table[data]!=(uart.receive_char & 0x01))
			{
				uart.status |= MSM8251_STATUS_PARITY_ERROR;
			}
		}
		
		/* signal character received */
		msm8251_receive_character(uart.receive_char);		

	}
}

static int	msm8251_transmit_bit(void)
{

	/* top bit is data */
	int bit;

	
	bit = (uart.data & 0x080)>>7;
	/* shift for next time */
	uart.data = uart.data<<1;
	/* update transmitted bit count */
	uart.bit_count_transmitted++;

	logerror("msm8251 transmit bit: %1x\n",bit);

	return bit;
}

static void uart_timer_callback(int dummy)
{
	unsigned long State = serial_device_get_state(0);

	/* shift bit in */
	msm8251_receive_bit(get_out_data_bit(State));
	
	/* hunt mode? */
	/* after each bit has been shifted in, it is compared against the current sync byte */
	if (uart.command & (1<<7))
	{
		/* data matches sync byte? */
		if (uart.data == uart.sync_bytes[uart.sync_byte_offset])
		{
			/* sync byte matches */

			logerror("msm8251 sync byte matches!\n");
			/* update for next sync byte? */
			uart.sync_byte_offset++;

			/* do all sync bytes match? */
			if (uart.sync_byte_offset == uart.sync_byte_count)
			{
				/* ent hunt mode */
				uart.command &=~(1<<7);
			}
		}
		else
		{
			/* if there is no match, reset */
			uart.sync_byte_offset = 0;
		}
	}
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
	logerror("msm8251 set baud rate %d\n", rate);
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
	LOG("MSM8251 Reset\n");

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
	logerror("msm8251 w: %02x\n",data);

	if (uart.flags & MSM8251_EXPECTING_MODE)
	{
		if (uart.flags & MSM8251_EXPECTING_SYNC_BYTE)
		{
			LOG("Sync byte\n");

#ifdef VERBOSE
			logerror("Sync byte: %02x\n", data);
#endif
			/* store sync byte written */
			uart.sync_bytes[uart.sync_byte_offset] = data;
			uart.sync_byte_offset++;

			if (uart.sync_byte_offset==uart.sync_byte_count)
			{
				/* finished transfering sync bytes, now expecting command */
				uart.flags &= ~(MSM8251_EXPECTING_MODE | MSM8251_EXPECTING_SYNC_BYTE);
				uart.sync_byte_offset = 0;
			}
		}
		else
		{
			LOG("Mode byte\n");

			uart.mode_byte = data;

			/* Synchronous or Asynchronous? */
			if ((data & 0x03)!=0)
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

				LOG("Asynchronous operation\n");

#ifdef VERBOSE
                logerror("Character length: %d\n", (((data>>2) & 0x03)+5));
				
				if (data & (1<<4))
				{
					logerror("enable parity checking\n");
				}

				if (data & (1<<5))
				{
					logerror("even parity\n");
				}
				else
				{
					logerror("odd parity\n");
				}

				{
					UINT8 stop_bit_length;

					stop_bit_length = (data>>6) & 0x03;

					switch (stop_bit_length)
					{
						case 0:
						{
							/* inhibit */
							logerror("stop bit: inhibit\n");
						}
						break;

						case 1:
						{
							/* 1 */
							logerror("stop bit: 1 bit\n");
						}
						break;

						case 2:
						{
							/* 1.5 */
							logerror("stop bit: 1.5 bits\n");
						}
						break;

						case 3:
						{
							/* 2 */
							logerror("stop bit: 2 bits\n");

						}
						break;
					}
				}
#endif
	
				/* data bits */
				uart.receive_char_length = (((data>>2) & 0x03)+5);
				uart.bit_count_received = 0;

				/* not expecting mode byte now */
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
				LOG("Synchronous operation\n");

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
		LOG("Command byte\n");

		uart.command = data;
#ifdef VERBOSE
		logerror("Command byte: %02x\n", data);

		if (data & (1<<7))
		{
			logerror("hunt mode\n");
		}

		if (data & (1<<5))
		{
			logerror("/rts set to 1\n");
		}
		else
		{
			logerror("/rts set to 0\n");
		}

		if (data & (1<<2))
		{
			logerror("receive enable\n");
		}
		else
		{
			logerror("receive disable\n");
		}

		if (data & (1<<1))
		{
			logerror("/dtr set to 0\n");
		}
		else
		{
			logerror("/dtr set to 1\n");
		}

		if (data & (1<<0))
		{
			logerror("transmit enable\n");
		}
		else
		{
			logerror("transmit disable\n");
		}


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

/* SERIAL DEVICE! */

static unsigned long msm_state;

void	msm8251_serial_device_updated_callback(int id, unsigned long State)
{
	/* syncronous */


	/* serial device has updated it's state */

	if (uart.msm8251_updated_callback)
		uart.msm8251_updated_callback(id, msm_state);
}



/* initialise transfer using serial device - set the callback which will
be called when serial device has updated it's state */
void	msm8251_init_serial_transfer(int id)
{
	serial_device_set_callback(id,msm8251_serial_device_updated_callback);
}
