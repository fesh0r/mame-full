/* internal serial transmission */

/* select a file on host filesystem to transfer using serial method,
setup serial interface software in driver and let the transfer begin */

/* this is used in the Amstrad NC Notepad emulation */



/* 

	the output starts at 1 level. It changes to 0 when the start bit has been transmitted.
	This therefore signals that data is following.
	When all data bits have been received, stop bits are transmitted with a value of 1.

	msm8251 expects this in asynchronous mode:
	packet format:

	bit count			function		value
	1					start bit		0
	note 1				data bits		x
	note 2				parity bit		x
	note 3				stop bits		1

	Note:
	1. Data size can be defined (usual value is 8)
	2. Parity bit (if parity is set to odd or even). Value of bit
		is defined by data parity.
	3. There should be at least 1 stop bit.
*/

#include "driver.h"
#include "includes/serial.h"


/* number of serial streams supported. This is also the number
of serial ports supported */
#define MAX_SERIAL_STREAMS	4

/* the serial streams */
static struct serial_stream	serial_streams[MAX_SERIAL_STREAMS];

static unsigned char serial_device_parity_table[256];

/***** SERIAL DEVICE ******/
void serial_device_setup(int id, int baud_rate, int num_data_bits, int stop_bit_count, int parity_code)
{
	int i;

	/* check id is valid */
	if ((id<0) || (id>=MAX_SERIAL_STREAMS))
		return;

	serial_streams[id].BaudRate = baud_rate;
	serial_streams[id].DataBitCount = num_data_bits;
	serial_streams[id].StopBitCount = stop_bit_count;
	serial_streams[id].parity_code = parity_code;

	/* data is initially high state */
	set_out_data_bit(serial_streams[id].State, 1);

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

		serial_device_parity_table[i] = sum & 0x01;
	}

}

unsigned long serial_device_get_state(int id)
{
	if ((id<0) || (id>=MAX_SERIAL_STREAMS))
		return 0;
	return serial_streams[id].State;
}


void	serial_device_notify_driver_updated(int id, unsigned long State)
{
	if ((id<0) || (id>=MAX_SERIAL_STREAMS))
		return;


	if (serial_streams[id].serial_device_updated_callback)
		serial_streams[id].serial_device_updated_callback(id, State);

}

static int	data_stream_get_data_bit_from_data_byte(struct data_stream *stream)
{
	int data_bit;
	int data_byte;
	if (stream->ByteCount<stream->DataLength)
	{
		/* get data from buffer */
		data_byte= stream->pData[stream->ByteCount];
	}
	else
	{
		/* over end of buffer, so return 0 */
		data_byte= 0;
	}
	
	/* get bit from data */
	data_bit = (data_byte>>(7-stream->BitCount)) & 0x01;

	/* update bit count */
	stream->BitCount++;
	/* ripple overflow onto byte count */
	stream->ByteCount+=stream->BitCount>>3;
	/* lock bit count into range */
	stream->BitCount &=0x07;

	/* do not let it read over end of data */
	if (stream->ByteCount>=stream->DataLength)
	{
		stream->ByteCount = stream->DataLength-1;
	}

	return data_bit;
}

/* used to construct data in stream format */
static void data_stream_add_bit_to_formatted_byte(struct data_stream *stream, int bit)
{
	/* combine bit */
	stream->formatted_byte = stream->formatted_byte<<1;
	stream->formatted_byte |= bit;
	/* update bit count */
	stream->formatted_byte_bit_count++;
}


/* generate data in stream format ready for transfer */
static void data_stream_generate_formatted_stream_byte(struct serial_stream *stream)
{
	int i;
	unsigned char data;
	
	/* used for generating parity */
	data = 0;

	/* reset bit count */
	stream->transmit.formatted_byte_bit_count = 0;
	/* reset formatted byte format */
	stream->transmit.formatted_byte = 0;

	/* start bit */
	data_stream_add_bit_to_formatted_byte(&stream->transmit,0);
	
	/* data bits */
	for (i=0; i<stream->DataBitCount; i++)
	{
		int databit;

		/* get bit from data */
		databit = data_stream_get_data_bit_from_data_byte(&stream->transmit);
		/* add bit to formatted byte */
		data_stream_add_bit_to_formatted_byte(&stream->transmit, databit);
		/* update data byte read for parity generation */
		data = data<<1;
		data = data|databit;
	}

	/* parity */
	if (stream->parity_code!=SERIAL_PARITY_NONE)
	{
		/* odd or even parity */
		unsigned char parity;

		/* get parity */
		/* if parity = 0, data has even parity - i.e. there is an even number of one bits in the data */
		/* if parity = 1, data has odd parity - i.e. there is an odd number of one bits in the data */
		parity = serial_device_parity_table[data];

		data_stream_add_bit_to_formatted_byte(&stream->transmit, parity);
	}

	/* stop bit(s) */
	for (i=0; i<stream->StopBitCount; i++)
	{
		data_stream_add_bit_to_formatted_byte(&stream->transmit,1);
	}
}

static int data_stream_get_data_bit(struct serial_stream *stream)
{
	int bit;

	/* have all bits of this stream formatted byte been sent? */
	if (stream->transmit.formatted_byte_bit_count_transfered==stream->transmit.formatted_byte_bit_count)
	{
		/* yes - generate a new byte to send */
		data_stream_generate_formatted_stream_byte(stream);
		stream->transmit.formatted_byte_bit_count_transfered = 0;
	}

	bit = (stream->transmit.formatted_byte>>
		(stream->transmit.formatted_byte_bit_count - 1 - 
		stream->transmit.formatted_byte_bit_count_transfered)) & 0x01;

	stream->transmit.formatted_byte_bit_count_transfered++;

	return bit;
}


void	serial_device_baud_rate_callback(int id)
{
	int data;
	
	data = data_stream_get_data_bit(&serial_streams[id]);

	logerror("serial device transmit bit: %01x\n", data);

	set_out_data_bit(serial_streams[id].State, data);
}


void	serial_device_begin_transfer(int id)
{
	serial_streams[id].timer = timer_pulse(TIME_IN_HZ(serial_streams[id].BaudRate), id, serial_device_baud_rate_callback);
}

void	serial_device_set_callback(int id, void (*callback)(int, unsigned long))
{
	/* check id is valid */
	if ((id<0) || (id>=MAX_SERIAL_STREAMS))
		return;

	serial_streams[id].serial_device_updated_callback = callback;
}


/* load image */
int serial_device_load(int type, int id, unsigned char **ptr, int *pDataSize)
{
	void *file;

	file = image_fopen(type, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);

	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			/* malloc memory for this data */
			data = malloc(datasize);

			if (data!=NULL)
			{
				/* read whole file */
				osd_fread(file, data, datasize);

				*ptr = data;
				*pDataSize = datasize;

				/* close file */
				osd_fclose(file);

				logerror("File loaded!\r\n");

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	}

	return 0;
}

/* reset position in stream */
static void data_stream_reset(struct data_stream *stream)
{
	/* reset byte offset */
	stream->ByteCount= 0;
	/* reset bit count */
	stream->BitCount = 0;

	/* async mode parameters */
	/* reset number of bits in formatted byte */
	stream->formatted_byte_bit_count = 0;
	stream->formatted_byte_bit_count_transfered = 0;

	/* sync mode parameters */
}

/* free stream */
static void data_stream_free(struct data_stream *stream)
{
	if (stream->pData!=NULL)
	{
		free(stream->pData);
		stream->pData = NULL;
	}
	stream->DataLength = 0;
}

/* initialise stream */
static void data_stream_init(struct data_stream *stream, unsigned char *pData, unsigned long DataLength)
{
	stream->pData = pData;
	stream->DataLength = DataLength;
	data_stream_reset(stream);
}


int		serial_device_init(int id)
{
	int data_length;
	unsigned char *data;

	/* check id is valid */
	if ((id<0) || (id>=MAX_SERIAL_STREAMS))
		return INIT_FAILED;

	/* load file and setup transmit data */
	if (serial_device_load(IO_SERIAL, id, &data,&data_length))
	{
		data_stream_init(&serial_streams[id].transmit, data, data_length);

		serial_device_setup(id, 600, 8, 1,SERIAL_PARITY_NONE);
		serial_device_begin_transfer(id);
		return INIT_OK;
	}

	return INIT_FAILED;
}


void	serial_device_exit(int id)
{
	if ((id<0) || (id>=MAX_SERIAL_STREAMS))
		return;

	data_stream_free(&serial_streams[id].transmit);
	data_stream_free(&serial_streams[id].receive);
}
