#ifndef __SERIAL_TRANSMISSION_HEADER_INCLUDED__
#define __SERIAL_TRANSMISSION_HEADER_INCLUDED__

/* internal serial transmission */

/* select a file on host filesystem to transfer using serial method,
setup serial interface software in driver and let the transfer begin */

/* this is used in the Amstrad NC Notepad emulation */

#define SERIAL_STATE_CTS	0x0001
#define SERIAL_STATE_RTS	0x0002
#define SERIAL_STATE_DSR	0x0004
#define SERIAL_STATE_DTR	0x0008
#define SERIAL_STATE_IN_DATA	0x00010
#define SERIAL_STATE_OUT_DATA	0x00020

enum
{
	SERIAL_PARITY_NONE,
	SERIAL_PARITY_ODD,
	SERIAL_PARITY_EVEN
};

#define get_out_data_bit(x) ((x & SERIAL_STATE_OUT_DATA)>>5)
#define get_in_data_bit(x) ((x & SERIAL_STATE_IN_DATA)>>4)

#define set_out_data_bit(x, data) \
	x&=~SERIAL_STATE_OUT_DATA; \
	x|=(data<<5)

#define set_in_data_bit(x, data) \
	x&=~SERIAL_STATE_IN_DATA; \
	x|=(data<<4)

/* a read/write stream */
struct data_stream
{
	/* pointer to buffer */
	unsigned char *pData;
	/* length of buffer */
	unsigned long DataLength;

	/* bit offset within current byte */
	unsigned long BitCount;
	/* byte offset within data */
	unsigned long ByteCount;

	/* used in msm8251 asynchronous mode */
	/* data in stream format. (start bit, data bit(s), parity, stop bits) */
	unsigned long formatted_byte;
	/* number of bits to transfer in stream data byte */
	unsigned long formatted_byte_bit_count;
	/* number of bits actually transfered */
	unsigned long formatted_byte_bit_count_transfered;
};

/* a serial stream */
struct serial_stream
{
	/* transmit data */
	struct data_stream transmit;
	/* receive data */
	struct data_stream receive;

	unsigned long State;

	/* transmit/receive parameters */
	/* number of stop bits */
	unsigned long StopBitCount;
	/* data bit count */
	unsigned long DataBitCount;
	/* baud rate */
	unsigned long BaudRate;
	/* parity */
	int parity_code;

	void	*timer;

	/* this function is called to notify the driver end that the serial
	device has updated it's state */
	void (*serial_device_updated_callback)(int id, unsigned long State);
};

unsigned long serial_device_get_state(int id);

/* this is called by the driver to notify the serial device that it has updated
it's state */
void	serial_device_notify_driver_updated(int id, unsigned long State);

/* sets the function the serial device will call to notify driver that it
has updated it's state */
void	serial_device_set_callback(int id, void (*callback)(int,unsigned long));

int     serial_device_init(int id);
void    serial_device_exit(int id);



#endif
