#include "devices/coco_vhd.h"

/***************************************************************************
  Technical specs on the Virtual Hard Disk interface
 ***************************************************************************

  Address       Description
  -------       -----------
  FF80          Logical record number (high byte)
  FF81          Logical record number (middle byte)
  FF82          Logical record number (low byte)
  FF83          Command/status register
  FF84          Buffer address (high byte)
  FF85          Buffer address (low byte)

  Set the other registers, and then issue a command to FF83 as follows:

  0 = read 256-byte sector at LRN
  1 = write 256-byte sector at LRN
  2 = flush write cache (Closes and then opens the image file)

  Error values:

   0 = no error
  -1 = power-on state (before the first command is recieved)
  -2 = invalid command
   2 = VHD image does not exist
   4 = Unable to open VHD image file
   5 = access denied (may not be able to write to VHD image)

  IMPORTANT: The I/O buffer must NOT cross an 8K MMU bank boundary.

***************************************************************************/

static long	logicalRecordNumber;
static long	bufferAddress;
static UINT8 vhdStatus;

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

static mess_image *vhd_image(void)
{
	return image_instance(IO_VHD, 0);
}

int coco_vhd_init(mess_image *img)
{
	vhdStatus = 2;	/* No VHD attached */
	return INIT_PASS;
}

int coco_vhd_load(mess_image *img, mame_file *fp, int open_mode)
{
	vhdStatus = 0xff; /* -1, Power on state */
	logicalRecordNumber = 0;
	bufferAddress = 0;
	return INIT_PASS;

}

static void coco_vhd_readwrite(UINT8 data)
{
	mame_file *vhdfile;
	int result;
	int phyOffset;
	long nBA = bufferAddress;

	vhdfile = image_fp(vhd_image());
	if (!vhdfile)
	{
		vhdStatus = 2; /* No VHD attached */
		return;
	}

	result = mame_fseek(vhdfile, ((logicalRecordNumber)) * 256, SEEK_SET);

	if (result < 0)
	{
		vhdStatus = 5; /* access denied */
		return;
	}

	phyOffset = coco3_mmu_translate( (nBA >> 12 ) / 2, nBA % 8192 );

	switch(data) {
	case 0: /* Read sector */
		result = mame_fread(vhdfile, &(mess_ram[phyOffset]), 256);

		if( result != 256 )
		{
			vhdStatus = 5; /* access denied */
			return;
		}

		vhdStatus = 0; /* Aok */
		break;

	case 1: /* Write Sector */
		result = mame_fwrite(vhdfile, &(mess_ram[phyOffset]), 256);

		if (result != 256)
		{
			vhdStatus = 5; /* access denied */
			return;
		}

		vhdStatus = 0; /* Aok */
		break;

	case 2: /* Flush file cache */
		vhdStatus = 0; /* Aok */
		break;

	default:
		vhdStatus = 0xfe; /* -2, Unknown command */
		break;
	}
}

READ_HANDLER(coco_vhd_io_r)
{
	data8_t result = 0;

	switch(offset) {
	case 0xff83 - 0xff40:
		LOG(( "vhd: Status read: %d\n", vhdStatus ));
		result = vhdStatus;
		break;
	}
	return result;
}

WRITE_HANDLER(coco_vhd_io_w)
{
	int pos;
	
	switch(offset) {
	case 0xff80 - 0xff40:
	case 0xff81 - 0xff40:
	case 0xff82 - 0xff40:
		pos = ((0xff82 - 0xff40) - offset) * 8;
		logicalRecordNumber &= ~(0xFF << pos);
		logicalRecordNumber += data << pos;
		LOG(( "vhd: LRN write: %6.6X\n", logicalRecordNumber ));
		break;

	case 0xff83 - 0xff40:
		coco_vhd_readwrite( data );
		LOG(( "vhd: Command: %d\n", data ));
		break;

	case 0xff84 - 0xff40:
		bufferAddress &= 0xFFFF00FF;
		bufferAddress += data << 8;
		LOG(( "vhd: BA write: %X (%2.2X..)\n", bufferAddress, data ));
		break;

	case 0xff85 - 0xff40:
		bufferAddress &= 0xFFFFFF00;
		bufferAddress += data;
		LOG(( "vhd: BA write: %X (..%2.2X)\n", bufferAddress, data ));
		break;
	}
}


