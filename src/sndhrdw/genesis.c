/*
 *
 *	  Sound emulation hooks for Genesis
 *
 *   ***********************************
 *   ***    C h a n g e   L i s t    ***
 *   ***********************************
 *   Date       Name   Description
 *   ----       ----   -----------
 *   00-Jan-00  GSL    Started
 *	 03-Aug-98	GSL	   Tidied.. at last!
 *
 */                                                                       

#include "sndhrdw/psgintf.h"
#include "machine/genesis.h"


int genesis_s_interrupt(void)
{
	// if (errorlog) fprintf(errorlog, "Z80 interrupt ");
	return 0xff;  

}


int YM2612_0_r(int address)
{
	int value = YM2612_address0_0_r(address);

	// if (errorlog) fprintf(errorlog, "2612_0 read %x\n", value);

	return value;
}

int YM2612_1_r(int address)
{
	int value = YM2612_address1_0_r(address);

	// if (errorlog) fprintf(errorlog, "2612_1 read %x\n", value);

	return value;
}

int YM2612_2_r(int address)
{
	int value = YM2612_address2_0_r(address);

   //	if (errorlog) fprintf(errorlog, "2612_2 read %x\n", value);

	return value;
}

int YM2612_3_r(int address)
{
	int value = YM2612_address3_0_r(address);

   //	if (errorlog) fprintf(errorlog, "2612_3 read %x\n", value);

	return value;
}

void YM2612_0_w(int address, int data)
{
	YM2612_address0_0_w(address, data);

   //	if (errorlog) fprintf(errorlog, "2612_0 write %x\n", data);

   }

void YM2612_1_w(int address, int data)
{
	YM2612_address1_0_w(address, data);

   //	if (errorlog) fprintf(errorlog, "2612_1 write %x\n", data);

   }

void YM2612_2_w(int address, int data)
{
	YM2612_address2_0_w(address, data);

   //	if (errorlog) fprintf(errorlog, "2612_2 write %x\n", data);

   }


void YM2612_3_w(int address, int data)
{
	YM2612_address3_0_w(address, data);

   //	if (errorlog) fprintf(errorlog, "2612_3 write %x\n", data);

}

void YM2612_68000_w(int offset, int data)
{
	switch (offset)
	{
		case 0:
			if (LOWER_BYTE_ACCESS(data)) YM2612_1_w(offset, data 		& 0xff);
			if (UPPER_BYTE_ACCESS(data)) YM2612_0_w(offset, (data >> 8) & 0xff);
			break;
		case 2:
			if (LOWER_BYTE_ACCESS(data)) YM2612_3_w(offset, data 		& 0xff);
			if (UPPER_BYTE_ACCESS(data)) YM2612_2_w(offset, (data >> 8) & 0xff);
	}
}

int YM2612_68000_r(int offset)
{
	switch (offset)
	{
		case 0:
			return ((YM2612_0_r(offset) << 8) + YM2612_1_r(offset) );
			break;
		case 2:
			return ((YM2612_2_r(offset) << 8) + YM2612_3_r(offset) );
			break;
	}
	return 0;
}
