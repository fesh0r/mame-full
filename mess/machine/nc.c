/***************************************************************************

  nc.c

***************************************************************************/

#include "driver.h"
#include "includes/nc.h"
#include "includes/serial.h"
#include "includes/msm8251.h"


/*************************************************************************************************/
/* PCMCIA Ram Card management */

/* the data is stored as a simple memory dump, there is no header or other information */

/* stores size of actual file on filesystem */
static int nc_card_size;
/* pointer to loaded data */
extern unsigned char *nc_card_ram;
/* mask used to stop access over end of card ram area */
extern int nc_membank_card_ram_mask;

/* save card data back */
static void	nc_card_save(int id)
{
	void *file;

	/* if there is no data to write, quit */
	if ((nc_card_ram==NULL) || (nc_card_size==0))
	{
		return;
	}

	logerror("attempting card save\n");

	/* open file for writing */
	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_WRITE);

	if (file)
	{
		/* write data */
		osd_fwrite(file, nc_card_ram, nc_card_size);

		/* close file */
		osd_fclose(file);

		logerror("write succeeded!\r\n");
	}
}

/* this mask will prevent overwrites from end of data */
static int nc_card_calculate_mask(int size)
{
	int i;

	/* memory block is visible as 16k blocks */
	/* mask can only operate on power of two sizes */
	/* memory cards normally in power of two sizes */
	/* maximum of 64 16k blocks can be accessed using memory paging of nc computer */
	/* max card size is therefore 1mb */
	for (i=14; i<20; i++)
	{
		if (size<(1<<i))
			return 0x03f>>(19-i);
	}

	return 0x03f;
}


/* load card image */
static int nc_card_load(int id, unsigned char **ptr)
{
	void *file;

	file = image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE_RW, OSD_FOPEN_READ);

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
				nc_card_size = datasize;

				/* read whole file */
				osd_fread(file, data, datasize);

				*ptr = data;

				/* close file */
				osd_fclose(file);

				logerror("File loaded!\r\n");

				nc_membank_card_ram_mask = nc_card_calculate_mask(datasize);

				logerror("Mask: %02x\n",nc_membank_card_ram_mask);

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	}

	return 0;
}

/* load pcmcia card */
int nc_pcmcia_card_load(int id)
{
	if (nc_card_load(id,&nc_card_ram))
	{
		if (nc_card_ram!=NULL)
		{
			/* card present! */
			if (nc_membank_card_ram_mask!=0)
			{
				nc_set_card_present_state(1);
			}
			return INIT_PASS;
		}
	}

	/* when MESS is first initialised, and a NULL filename
	is passed to this function, the load will fail, but this
	is a good opportunity to setup some variables */
	/* init failed! */
	logerror("failed!\n");

	/* card not present */
	nc_set_card_present_state(0);
	/* card ram NULL */
	nc_card_ram = NULL;
	nc_card_size = 0;
	return INIT_FAIL;
}

void nc_pcmcia_card_exit(int id)
{
	/* save card data if there is any */
	nc_card_save(id);

	/* free ram allocated to card */
	if (nc_card_ram!=NULL)
	{
		free(nc_card_ram);
		nc_card_ram = NULL;
	}
	nc_card_size = 0;

	/* set card not present state */
	nc_set_card_present_state(0);
}


/*************************************************************************************************/
/* Serial */

int	nc_serial_init(int id)
{
	if (serial_device_init(id)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(id, 9600, 8, 1,SERIAL_PARITY_NONE);

		/* connect serial chip to serial device */
		msm8251_connect_to_serial_device(id);

      serial_device_set_protocol(id, SERIAL_PROTOCOL_NONE);
    //    serial_device_set_protocol(id, SERIAL_PROTOCOL_XMODEM);

		/* and start transmit */
		serial_device_set_transmit_state(id,1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}
