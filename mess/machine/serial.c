/* internal serial transmission */

/* select a file on host filesystem to transfer using serial method,
setup serial interface software in driver and let the transfer begin */

/* this is used in the Amstrad NC Notepad emulation */

#include "driver.h"
#include "includes/serial.h"


/* number of serial streams supported. This is also the number
of serial ports supported */
#define MAX_SERIAL_STREAMS	4

/* the serial streams */
static struct serial_stream	serial_streams[MAX_SERIAL_STREAMS];



/***** SERIAL DEVICE ******/
void serial_device_setup(int id, int baud_rate, int num_data_bits, int start_bit_count, int stop_bit_count)
{
	/* check id is valid */
	if ((id<0) || (id>=MAX_SERIAL_STREAMS))
		return;

	serial_streams[id].BaudRate = baud_rate;
	serial_streams[id].DataBitCount = num_data_bits;
	serial_streams[id].StartBitCount = start_bit_count;
	serial_streams[id].StopBitCount = stop_bit_count;
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


static int	data_stream_get_data_bit_from_sync_byte(struct data_stream *stream)
{
	int data = (stream->sync_byte & 0x080)>>7;
	stream->sync_byte = stream->sync_byte<<1;
	stream->BitCount++;

	if (stream->BitCount==8)
	{
		stream->BitCount = 0;
		stream->sync_byte = stream->sync_byte_reload;
	}

	return data;
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

static int data_stream_get_data_bit(struct data_stream *stream)
{
//	if (stream->State == 0)
//	{
//		return data_stream_get_data_bit_from_sync_byte(stream);
//	}

	return data_stream_get_data_bit_from_data_byte(stream);
}


void	serial_device_baud_rate_callback(int id)
{
	int data;
	
	data = data_stream_get_data_bit(&serial_streams[id].transmit);

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
	stream->sync_byte = 0x037;
	stream->sync_byte_reload = stream->sync_byte;
	stream->State = 0;
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

		serial_device_setup(id, 600, 8, 0,1);
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
