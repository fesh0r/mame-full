/* PGM Protection related */

#include "driver.h"

extern data16_t *pgm_mainram;

/*** ASIC 3 - Protection? ****************************************************/

/* this isn't right, we patch the branch instructions for now */

READ16_HANDLER ( orlegend_kludge_r )
{
	/* the game reads from within this address range, starting at 0x820000,
	and increasing to 0x??????, and seems to corrupt ram based on the result? */

	return 0x2;
}

READ16_HANDLER( orlegnde_prot_r )
{
	int res;
	res = -1;

	/* Start of Boot Up */
	if (activecpu_get_pc() == 0x145bda) res = 0x80;
	if (activecpu_get_pc() == 0x145c00) res = 0x00; // NOT 0x80 set
	if (activecpu_get_pc() == 0x145c28) res = 0x84;
	if (activecpu_get_pc() == 0x145c54) res = 0x80;
	if (activecpu_get_pc() == 0x145c80) res = 0x04;
	if (activecpu_get_pc() == 0x145caa) res = 0x00; // NOT 0x84 set
	if (activecpu_get_pc() == 0x145cd2) res = 0x20;
	if (activecpu_get_pc() == 0x145cfc) res = 0x00; // NOT 0x20 set

	/* region is supplied by the protection device */
	if (activecpu_get_pc() == 0x145D8E) res = (readinputport(4) & 1) << 3;
	if (activecpu_get_pc() == 0x145D72) res = (readinputport(4) & 2) << 6;


	if (res != -1) logerror("%06x: prot_r %04x\n",activecpu_get_pc(),res);
	else logerror("%06x: prot_r unknown\n",activecpu_get_pc());

	if (res == -1) res = 0x00;

	return res;
}

void remove_orlegend_commonprot(void)
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	mem16[0x145D48/2]=0x4e71; mem16[0x145D4a/2]=0x4e71; /* dif to others. */

	mem16[0x145D02/2]=0x4e71; mem16[0x145D04/2]=0x4e71;
	mem16[0x145CDC/2]=0x4e71; mem16[0x145CDE/2]=0x4e71;
	mem16[0x145CB0/2]=0x4e71; mem16[0x145CB2/2]=0x4e71;
	mem16[0x145C8A/2]=0x4e71; mem16[0x145C8C/2]=0x4e71;
	mem16[0x145C5E/2]=0x4e71; mem16[0x145C60/2]=0x4e71;
	mem16[0x145C32/2]=0x4e71; mem16[0x145C34/2]=0x4e71;
	mem16[0x145C06/2]=0x4e71; mem16[0x145C08/2]=0x4e71;
	mem16[0x145BE4/2]=0x4e71; mem16[0x145BE6/2]=0x4e71;
}

void remove_orlegend_prot(void)
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	mem16[0x146AE4/2]=0x4e71; mem16[0x146AE6/2]=0x4e71;
	remove_orlegend_commonprot();
}

void remove_orlegnde_prot(void)
{
	data16_t *mem16 = (data16_t *)memory_region(REGION_CPU1);
	mem16[0x146AF4/2]=0x4e71; mem16[0x146AF6/2]=0x4e71;
	remove_orlegend_commonprot();
}

/*

ASIC3 is the orlegend protection, its not perfect as it still needs 1 of the
patches.

ASIC3 emulation:

unsigned short BACKREGS1[]={'I','G', 'S', 'A', 'A', 0x7f,'A', 'A', 0x3e,'A',
'I', 0xf9,0xa, '&', 'I','I', 'I',  '2'};
unsigned short REGS[]=
{0x20,0x21,0x22,0x24,0x25,0x26,0x27,0x28,0x2a,0x2b,0x2c,0x2d,0x2e,0x30,0x31,
0x32,0x33,0x34};
unsigned short Backup[0x100];
unsigned short BLastReg=0;


ASIC3Init()
{
  Backup[0]=0xff;
  for(int i=0;i<0x12;++i)
      Backup[REGS[i]]=BACKREGS1[i];
}


unsigned shortASIC3Read16(unsigned int addr)
{
  addr&=0xff;
  if(addr==0xe)
  {
//   if(BLastReg==3)
//    return 0xfc;
//   else

   if(BLastReg<0x20 || BLastReg>0x34
     )
     return Backup[BLastReg]*2;
   else
    return Backup[BLastReg];
}

void ASIC3Write16(unsigned int addr,unsigned short val)
{
  addr&=0xff;
  if(addr==0)
   BLastReg=data;
  else if(addr==0xe)
  {
   if(BLastReg!=3 &&
      (BLastReg<0x20 || BLastReg>0x34)
     )
    Backup[BLastReg]=data;
  }
}

*/


/*** Knights of Valour / Sango Protection (from ElSemi) (ASIC28) ***/

static unsigned short ASIC28KEY;
static unsigned short ASIC28REGS[10];
static unsigned short ASICPARAMS[256];
static unsigned short ASIC28RCNT=0;
static unsigned int B0TABLE[16]={2,0,1,4,3}; //maps char portraits to tables

//Not sure if BATABLE is complete
static unsigned int BATABLE[0x40]=
	{0x00,0x29,0x2c,0x35,0x3a,0x41,0x4a,0x4e,  //0x00
     0x57,0x5e,0x77,0x79,0x7a,0x7b,0x7c,0x7d, //0x08
     0x7e,0x7f,0x80,0x81,0x82,0x85,0x86,0x87, //0x10
     0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x90,  //0x18
     0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,
     0x9e,0xa3,0xd4,0xa9,0xaf,0xb5,0xbb,0xc1};

static unsigned int E0REGS[16];


READ16_HANDLER (sango_protram_r)
{


	// at offset == 4 is region (supplied by device)
	// 0 = china
	// 1 = taiwan
	// 2 = japan
	// 3 = korea
	// 4 = hong kong
	// 5 = world

	if (offset == 4)	return readinputport(4);

	// otherwise it doesn't seem to use the ram for anything important, we return 0 to avoid test mode corruption
	// kovplus reads from offset 000e a lot ... why?
#ifdef MAME_DEBUG
	usrintf_showmessage ("protection ram r %04x",offset);
#endif
	return 0x0000;
}

READ16_HANDLER (ASIC28_r16)
//unsigned short ASIC28_r16(unsigned int addr)
{

	unsigned int val=(ASIC28REGS[1]<<16)|(ASIC28REGS[0]);

//logerror("Asic28 Read PC = %06x Command = %02x ??\n",activecpu_get_pc(), ASIC28REGS[1]);

	switch(ASIC28REGS[1]&0xff)
	{
		case 0x99:
			val=0x880000;
			break;
		case 0xd0:	//txt palette
			val=0xa01000+(ASIC28REGS[0]<<5);
			break;
		case 0xdc:	//bg palette
			val=0xa00800+(ASIC28REGS[0]<<6);
			break;
		case 0x9d:
		case 0xe0:	//spr palette
			val=0xa00000+((ASIC28REGS[0]&0x1f)<<6);
			break;
		case 0xc0:
			val=0x880000;
			break;
		case 0xc3:	//TXT tile position Uses C0 to select column
			{
				val=0x904000+(ASICPARAMS[0xc0]+ASICPARAMS[0xc3]*64)*4;
			}
			break;
		case 0xcb:
			val=0x880000;
			break;
		case 0xe7:
			val=0x880000;
			break;
		case 0xe5:
			val=0x880000;
			break;
		case 0xB0:
			val=B0TABLE[ASIC28REGS[0]&0xf];
			break;
		case 0xBA:
			val=BATABLE[ASIC28REGS[0]&0x3f];
			if(ASIC28REGS[0]>0x2f)
			{
//				PutMessage("Unmapped BA com, report ElSemi",60);
				usrintf_showmessage	("Unmapped BA com %02x, contact ElSemi / MameDev", ASIC28REGS[0]);
			}
			break;
		case 0xfe:	//todo
			val=0x880000;
			break;
		case 0xfc:	//Adjust damage level to char experience level
			{
			val=(ASICPARAMS[0xfc]*ASICPARAMS[0xfe])>>6;
			break;
			}
		case 0xf8:
			val=E0REGS[ASIC28REGS[0]&0xf]&0xffffff;
			break;
		case 0xcc: //BG
   			{
   	 		int y=ASICPARAMS[0xcc];
    		if(y&0x400)    //y is signed (probably x too and it also applies to TXT, but I've never seen it used)
     			y=-(0x400-(y&0x3ff));
    		val=0x900000+(((ASICPARAMS[0xcb]+(y)*64)*4)/*&0x1fff*/);
   			}
   			break;
		case 0xf0:
			{
				val=0x00C000;
			}
			break;
		case 0xb4:
			{
				int v2=ASIC28REGS[0]&0x0f;
				int v1=(ASIC28REGS[0]&0x0f00)>>8;
//				unsigned short tmp=E0REGS[v2];
				//E0REGS[v2]=E0REGS[v1];
				//E0REGS[v1]=tmp;
				if(ASIC28REGS[0]==0x102)
					E0REGS[1]=E0REGS[0];
				else
					E0REGS[v1]=E0REGS[v2];

				val=0x880000;
			}
			break;
		case 0xd6:	//???? check it
			{
				int v2=ASIC28REGS[0]&0xf;
//				int v1=(ASIC28REGS[0]&0xf0)>>4;
				E0REGS[0]=E0REGS[v2];
				//E0REGS[v2]=0;
				val=0x880000;
			}
			break;
		default:
			{
				val=0x880000;
			}
	}

//	if(addr==0x500000)
	if(offset==0)
	{
		unsigned short d=val&0xffff;
		unsigned short realkey;
		realkey=ASIC28KEY>>8;
		realkey|=ASIC28KEY;
		d^=realkey;
		return d;
	}
//	else if(addr==0x500002)
	else if(offset==1)
	{
		unsigned short d=val>>16;
		unsigned short realkey;
		realkey=ASIC28KEY>>8;
		realkey|=ASIC28KEY;
		d^=realkey;
		ASIC28RCNT++;
		ASIC28RCNT&=0xf;	//16 busy states
		if(ASIC28RCNT==0)
		{
			ASIC28KEY+=0x100;
			ASIC28KEY&=0xFF00;
		}
		return d;
	}
	return 0xff;
}

WRITE16_HANDLER (ASIC28_w16)
//void ASIC28_w16(unsigned int addr,unsigned short data)
{
//	if(addr==0x500000)
	if(offset==0)
	{
		unsigned short realkey;
		realkey=ASIC28KEY>>8;
		realkey|=ASIC28KEY;
		data^=realkey;
		ASIC28REGS[0]=data;
		return;
	}
//	if(addr==0x500002)
	if(offset==1)
	{
		unsigned short realkey;

		ASIC28KEY=data&0xff00;

		realkey=ASIC28KEY>>8;
		realkey|=ASIC28KEY;
		data^=realkey;
		ASIC28REGS[1]=data;
//		ErrorLogMessage("ASIC28 CMD %X  PARAM %X",ASIC28REGS[1],ASIC28REGS[0]);
		logerror("ASIC28 CMD %04x  PARAM %04x\n",ASIC28REGS[1],ASIC28REGS[0]);

		ASICPARAMS[ASIC28REGS[1]&0xff]=ASIC28REGS[0];
		if(ASIC28REGS[1]==0xE7)
		{
			unsigned int E0R=(ASICPARAMS[0xE7]>>12)&0xf;
			E0REGS[E0R]&=0xffff;
			E0REGS[E0R]|=ASIC28REGS[0]<<16;
		}
		if(ASIC28REGS[1]==0xE5)
		{
			unsigned int E0R=(ASICPARAMS[0xE7]>>12)&0xf;
			E0REGS[E0R]&=0xff0000;
			E0REGS[E0R]|=ASIC28REGS[0];
		}
		ASIC28RCNT=0;
	}
}

/* Dragon World 2 */

#define DW2BITSWAP(s,d,bs,bd)  d=((d&(~(1<<bd)))|(((s>>bs)&1)<<bd))
//Use this handler for reading from 0xd80000-0xd80002
READ16_HANDLER (dw2_d80000_r )
{
//addr&=0xff;
// if(dw2reg<0x20) //NOT SURE!!
	{
		//The value at 0x80EECE is computed in the routine at 0x107c18
		data16_t d=pgm_mainram[0xEECE/2];
		data16_t d2=0;
		d=(d>>8)|(d<<8);
		DW2BITSWAP(d,d2,7 ,0);
		DW2BITSWAP(d,d2,4 ,1);
		DW2BITSWAP(d,d2,5 ,2);
		DW2BITSWAP(d,d2,2 ,3);
		DW2BITSWAP(d,d2,15,4);
		DW2BITSWAP(d,d2,1 ,5);
		DW2BITSWAP(d,d2,10,6);
		DW2BITSWAP(d,d2,13,7);
		// ... missing bitswaps here (8-15) there is not enough data to know them
		// the code only checks the lowest 8 bytes
		return d2;
	}
}
