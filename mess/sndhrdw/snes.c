/***************************************************************************

 Super Nintendo Entertainment System Driver - Written By Lee Hammerton aKa Savoury SnaX

 Acknowledgements

	I gratefully acknowledge the work of Karl Stenerud for his work on the processor
  cores used in this emulation and of course I hope you'll continue to work with me
  to improve this project.

	I would like to acknowledge the support of all those who helped me during SNEeSe and
  in doing so have helped get this project off the ground. There are many, many people
  to thank and so little space that I am keeping this as brief as I can :

		All snes technical document authors!
		All snes emulator authors!
			ZSnes
			Snes9x
			Snemul
			Nlksnes
			Esnes
			and the others....
		The original SNEeSe team members (other than myself ;-)) - 
			Charles Bilyue - Your continued work on SNEeSe is fantastic!
			Santeri Saarimaa - Who'd have thought I'd come back to emulation ;-)

	***************************************************************************

 - !!DOES ABSOLUTELY NOTHING AT THIS TIME!! -

***************************************************************************/

#include "driver.h"
#include "../machine/snes.h"

#ifdef EMULATE_SPC700											// If this is not defined there really is no point in compiling this module

#include "../sndhrdw/snes.h"
#include "cpu/spc700/spc700.h"

int snesFirstChannel;
unsigned char *SPCRamPtr;
#define SOUND_INTERLEAVE_RATE		TIME_IN_USEC(50)
#define SOUND_INTERLEAVE_REPEAT		20
static void spc_sound_comm_timer(int reps_left)
{
	if (--reps_left)
		timer_set(SOUND_INTERLEAVE_RATE, reps_left, spc_sound_comm_timer);
}

static void spc_delayed_sound_w_0(int param)
{
	SPCIN[0]=param;
	/* allocate a high frequency timer until a response is generated */
	/* the main CPU is *very* sensistive to the timing of the response */
	timer_set(SOUND_INTERLEAVE_RATE, SOUND_INTERLEAVE_REPEAT, spc_sound_comm_timer);
}

static void spc_delayed_sound_w_1(int param)
{
	SPCIN[1]=param;
	/* allocate a high frequency timer until a response is generated */
	/* the main CPU is *very* sensistive to the timing of the response */
	timer_set(SOUND_INTERLEAVE_RATE, SOUND_INTERLEAVE_REPEAT, spc_sound_comm_timer);
}

static void spc_delayed_sound_w_2(int param)
{
	SPCIN[2]=param;
	/* allocate a high frequency timer until a response is generated */
	/* the main CPU is *very* sensistive to the timing of the response */
	timer_set(SOUND_INTERLEAVE_RATE, SOUND_INTERLEAVE_REPEAT, spc_sound_comm_timer);
}

static void spc_delayed_sound_w_3(int param)
{
	SPCIN[3]=param;
	/* allocate a high frequency timer until a response is generated */
	/* the main CPU is *very* sensistive to the timing of the response */
	timer_set(SOUND_INTERLEAVE_RATE, SOUND_INTERLEAVE_REPEAT, spc_sound_comm_timer);
}

static int snesStartSamples(const struct MachineSound *msound)
{
	int a,vol[MIXER_MAX_CHANNELS];
	struct GameSamples *samples;

	if ((Machine->samples = malloc(sizeof(struct GameSamples)*8)) == NULL)
		return 1;														// Unable to allocate sample structures

	samples=Machine->samples;

	// We will need 8 * 64k for sample space (actually could be nearly 32k * 4 - but there would be no space for program code in spc)
	for (a=0;a<8;a++)
	{
		if ((samples->sample[a] = malloc(sizeof(struct GameSample) + (0x10000)*sizeof(short))) == NULL)
			return 1;

		samples->sample[a]->length = 0x10000*2;
		samples->sample[a]->smpfreq = 8000;
		samples->sample[a]->resolution = 16;
	}
	samples->total = 8;

	for (a=0;a<8;a++)
		vol[a]=100;

	snesFirstChannel=mixer_allocate_channels(8,vol);

	for (a=0;a<8;a++)
	{
		char buf[40];

		sprintf(buf,"SNES Sample #%d",a);
		mixer_set_name(snesFirstChannel+a,buf);
	}

	SPCRamPtr=memory_region(REGION_CPU2);

	return 0;
}

struct CustomSound_interface snesSoundInterface =
{
	snesStartSamples,						// Used to allocate memory for samples and channels
	0,										// No need to de-allocate memory for samples and channels ?????
	0,										// No need for an update samples (at this point in time!!)
};

int spc700Interrupt(void)
{
	return ignore_interrupt();
}

// SPC700 memory mapper		- Quite a simple affair here

void spcTimerTick(int param)			// Param is not used
{
	// This will be called every 15.6 MSECS

	spcPort[18]++;					// Increment timer
	if (spcPort[0x0C]==spcPort[18])
	{
		// Generate pulse on C2 Line
		spcPort[18]=0;				// Clear timer
		if (spcPort[0x01]&0x04)
		{
			spcPort[0x0F]++;		// Increment counter
			spcPort[0x0F]&=0x0F;		// Clear upper bits
		}
	}
	spc700TimerExtra++;
	if (spc700TimerExtra&0x08)		// If set time to increment T0 & T1
	{
		spcPort[16]++;
		if (spcPort[0x0A]==spcPort[16])
		{
			// Generate pulse on C0 Line
			spcPort[16]=0;				// Clear timer
			if (spcPort[0x01]&0x01)
			{
				spcPort[0x0D]++;		// Increment counter
				spcPort[0x0D]&=0x0F;		// Clear upper bits
			}
		}
		spcPort[17]++;
		if (spcPort[0x0B]==spcPort[17])
		{
			// Generate pulse on C0 Line
			spcPort[17]=0;				// Clear timer
			if (spcPort[0x01]&0x02)
			{
				spcPort[0x0E]++;		// Increment counter
				spcPort[0x0E]&=0x0F;		// Clear upper bits
			}				  
		}
		spc700TimerExtra=0;
	}
}

int spcReadDSP(int addr)			// DSP connected to spc chip.
{
	switch (addr)
	{

	}
	logerror("Unsupported Read From SPC DSP at address %02X\n",addr);
	return 0xFF;
}

READ_HANDLER ( spc_io_r )		// Not very many ports to deal with on the spc (thank god) - probably should be in sndhrdw
{
	switch (offset)				// Offset is from 0 not from 0x00F0
	{
		case 0x0000:			//  (test)	:	??? not sure at this time
		case 0x0002:			// Reg Add	:	Register address
		case 0x0001:			//	Control	:
		case 0x000A:			//	Timer 0 :
		case 0x000B:			//	Timer 1 :
		case 0x000C:			//	Timer 2 :
		case 0x000D:			//	Count 0 :
		case 0x000E:			//	Count 1 :
		case 0x000F:			//	Count 2 :
			return spcPort[offset&0x0F];
		case 0x0004:			// Port 0	:
		case 0x0005:			// Port 1	:
		case 0x0006:			// Port 2	:
		case 0x0007:			// Port 3	:
			return SPCIN[offset&0x03];			// Read SPC Input port - comms between spc and cpu
		case 0x0003:			// Reg Data :	Register address (this and above go to dsp chip.ie they do the sound stuff)
			return spcReadDSP(spcPort[0x02]);
	}
	logerror("SPC Read From Unhandled Address : %04X\n",offset+0x00F0);
	return 0x0ff;
}

struct
{
	unsigned short DIR;
	signed char MVOLR,MVOLL;
	signed char VOLL[8],VOLR[8];
	unsigned short PITCH[8];
	unsigned short SRCN[8];
} SNES_DSP;

void snesUnpackSample(int sample,int *length,int *looping)
{
	signed short *data=(signed short *)Machine->samples->sample[sample]->data;
	int tableAddress=SNES_DSP.DIR + SNES_DSP.SRCN[sample];
	unsigned char *startAddress=SPCRamPtr + ((SPCRamPtr[tableAddress+1]<<8)|SPCRamPtr[tableAddress]);
	unsigned char *loopAddress=SPCRamPtr + ((SPCRamPtr[tableAddress+3]<<8)|SPCRamPtr[tableAddress+2]);
	int lastBlock=0,RF,granularity,filter,a;
	unsigned short temp;

	*length=0;
	*looping=0;

	logerror("CPU -> %04X\n",cpu_get_pc());
	logerror("Table Address = %04X\n",(int)(tableAddress));
	logerror("Start Address = %04X\n",(int)(startAddress-SPCRamPtr));
	logerror("Loop Address = %04X\n",(int)(loopAddress-SPCRamPtr));

	return;

	do
	{
		if (startAddress==loopAddress)
			*looping=*length;
		RF = *startAddress++;
		lastBlock=RF&0x01;						// If bit 0x01 then this is the last block of sample data
		if (lastBlock && !(RF&0x02))			// If its the last block then check the loop bit (if loop is not set then zero loop)
			*looping=0;
		filter=(RF>>2)&0x03;
		granularity=RF>>4;

		for (a=0;a<8;a++)						// For each byte (each byte is two samples worth of data)
		{
			temp=((*startAddress)>>4)&0x0F;		// Get upper 4 bits
			if (temp>7)							// If negative sign extend
				temp|=0xFFF0;
			temp<<=granularity;
			switch(filter)						// Handle filters
			{
				case 1:
					temp+=(*(data + (*length)-1)*15)/16;		// Thanks to Archeide + Brad Martin for information on filters
					break;
				case 2:
					temp+=(*(data + (*length)-1)*61)/32 - (*(data+(*length)-2)*15)/16;
					break;
				case 3:
					temp+=(*(data + (*length)-1)*115)/64 - (*(data+(*length)-2)*13)/15;
					break;
			}
			*(data + (*length))=temp;
			(*length)++;

			temp=(*startAddress++)&0x0F;		// Get lower 4 bits
			if (temp>7)							// If negative sign extend
				temp|=0xFFF0;
			temp<<=granularity;
			switch(filter)						// Handle filters
			{
				case 1:
					temp+=(*(data + (*length)-1)*15)/16;		// Thanks to Archeide + Brad Martin for information on filters
					break;
				case 2:
					temp+=(*(data + (*length)-1)*61)/32 - (*(data+(*length)-2)*15)/16;
					break;
				case 3:
					temp+=(*(data + (*length)-1)*115)/64 - (*(data+(*length)-2)*13)/15;
					break;
			}
			*(data + (*length))=temp;
			(*length)++;
		}
	} while (!lastBlock);
}

void DSPPlaySamp(unsigned char data)
{
	unsigned char bMask=0x01;
	unsigned char VoiceOffs=0;
	int length,loop;

	while (bMask)
	{
		if (data & bMask)
		{
			// Play this key

			snesUnpackSample(VoiceOffs,&length,&loop);
			mixer_play_sample_16(VoiceOffs,(signed short *)Machine->samples->sample[VoiceOffs]->data,length,8000,loop);
		}

		bMask<<=1;
		VoiceOffs+=1;
	}
}

void DSPStopSamp(unsigned char data)
{
	unsigned char bMask=0x01;
	unsigned char VoiceOffs=0;

	while (bMask)
	{
		if (data & bMask)
		{
			mixer_stop_sample(VoiceOffs);
		}

		bMask<<=1;
		VoiceOffs+=1;
	}
}

void spcWriteDSP(int addr,unsigned char data)			// DSP connected to spc chip.
{
	if (addr<0x80)										// Any higher and it can't possibly be an DSP command!!
	{
		switch (addr)
		{
			case 0x4C:									//	4c         KON          Key On. D0-D7 correspond to Voice0-Voice7
				// This will play the sample selected...
				DSPPlaySamp(data);
				return;
			case 0x5C:									//	5c         KOF          key Off.
				// This will stop the sample selected...
				DSPStopSamp(data);
				return;
			case 0x5D:									//	5d         DIR          Off-set address of source directory
				SNES_DSP.DIR=data * 0x100;
				return;
			case 0x1C:									//	1c         MVOL (R)     Main Volume (R)
				SNES_DSP.MVOLR=data;
				return;
			case 0x0C:									//	0c         MVOL (L)     Main Volume (L)
				SNES_DSP.MVOLL=data;
				return;
			case 0x60:									//	x0          VOL (L)   \ left and right volume
				SNES_DSP.VOLL[6]=data;
				return;
			case 0x61:									//	x1          VOL (R)   /
				SNES_DSP.VOLR[6]=data;
				return;
			case 0x62:									//	x2           P (L)    \ The total 14 bits of P(H) & P(L) express
				SNES_DSP.PITCH[6]=(SNES_DSP.PITCH[6]&0xFF00) | data;
				return;
			case 0x63:									//	x3           P (H)    / pitch height
				SNES_DSP.PITCH[6]=(SNES_DSP.PITCH[6]&0x00FF) | ((data<<8)&0x3F00);
				return;
			case 0x64:									//	x4      Voice x SRCN    Designates source number from 0-256
				SNES_DSP.SRCN[6]=data*4;
				return;


// - FOLLOWING ARE UNSUPPORTED AT THIS TIME -
//			case 0x7D:									//	7d         EDL          Echo Delay. Only lower 4 bits operative.
//			case 0x6D:									//	6d         ESA          Off-set address of echo region. Echo Start Address
//			case 0x4D:									//	4d         EOV          Echo On/Off
//			case 0x3D:									//	3d         NOV          Noise on/off. D0-D7 correspond to Voice0-Voice7
//			case 0x2D:									//	2d         PMON         Pitch modulation of Voice i with OUTX of Voice
//																					(i=1) as modulated wave.
//			case 0x0D:									//	0d         EFB          Echo Feed-Back
//			case 0x6C:									//	6c         FLG          Designated on/off of reset, mute, echo and noise
//																					clock.
//			case 0x3C:									//	3c         EVOL (R)     Echo Volume (R)
//			case 0x2C:									//	2c         EVOL (L)     Echo Volume (L)
//			case 0x65:									//	x5         ADSR (1)   \ Address is designated by D7 = 1 of ADSR(1):
//			case 0x66:									//	x6         ADSR (2)   / when D7= 0 GAIN is operative.
//			case 0x67:									//	x7         GAIN         Envelope can be freely designated by the program.
//			case 0x68:									//	x8         *ENVX        Present value of evelope which DSP rewrittes at
//																					each Ts.
//			case 0x69:									//	x9         *OUTX        Value after envelope multiplication & before
//																					VOL multiplication (present wave height value)

		}
	}
	logerror("Unsupported Write to SPC DSP at address %02X with byte %02X\n",addr,data);
}

WRITE_HANDLER ( spc_io_w )		// Not very many ports to deal with on the spc (thank god) - probably should be in sndhrdw
{
	switch (offset)				// Offset is from 0 not from 0x00F0
	{
		case 0x0001:			//	Control	:   %00ab0xyz - a clear port 2&3, b clear port 0&1, zyx stop / start timer
			if (data&0x20)
			{
				SPCIN[2]=SPCOUT[2]=0;			// Clear requested ports
				SPCIN[3]=SPCOUT[3]=0;
			}
			if (data&0x10)
			{
				SPCIN[0]=SPCOUT[0]=0;
				SPCIN[1]=SPCOUT[1]=0;
			}
			spcPort[0x01]=data;
			return;
		case 0x0000:			//  (test)	:	??? not sure at this time
		case 0x0002:			// Reg Add	:	Register address
		case 0x000A:			//	Timer 0 :
		case 0x000B:			//	Timer 1 :
		case 0x000C:			//	Timer 2 :
		case 0x000D:			//	Count 0 :
		case 0x000E:			//	Count 1 :
		case 0x000F:			//	Count 2 :
			spcPort[offset&0x0F]=data;
			return;
		case 0x0004:			// Port 0	:
#ifdef EMULATE_SPC700
			timer_set(TIME_NOW, data, spc_delayed_sound_w_0);
			return;
#else
			return;
#endif
		case 0x0005:			// Port 1	:
#ifdef EMULATE_SPC700
			timer_set(TIME_NOW, data, spc_delayed_sound_w_1);
			return;
#else
			return;
#endif
		case 0x0006:			// Port 2	:
#ifdef EMULATE_SPC700
			timer_set(TIME_NOW, data, spc_delayed_sound_w_2);
			return;
#else
			return;
#endif
		case 0x0007:			// Port 3	:
#ifdef EMULATE_SPC700
			timer_set(TIME_NOW, data, spc_delayed_sound_w_3);
			return;
#else
			return;
#endif
//			SPCOUT[offset&0x03]=data;			// Write SPC Input port - comms between spc and cpu
//			return;
		case 0x0003:			// Reg Data :	Register address (this and above go to dsp chip.ie they do the sound stuff)
			spcWriteDSP(spcPort[0x02],data);
			return;
	}
	logerror("SPC Write To Unhandled Address : %04X\n",offset+0x00F0);
}

MEMORY_READ_START( spc_readmem )
	{ 0x0000,0x00EF,MRA_RAM},							// lower 32k ram
	{ 0x00F0,0x00FF,spc_io_r},							// i/o
	{ 0x0100,0x7FFF,MRA_RAM},							// lower 32k ram
	{ 0x8000,0xFFBF,MRA_NOP},							// upper 32k unmapped on snes (except for switchable rom)
	{ 0xFFC0,0xFFFF,MRA_ROM},							// will need switching to BANK
MEMORY_END

MEMORY_WRITE_START( spc_writemem )
	{ 0x0000,0x00EF,MWA_RAM},							// lower 32k ram
	{ 0x00F0,0x00FF,spc_io_w},							// i/o
	{ 0x0100,0x7FFF,MWA_RAM},							// lower 32k ram
	{ 0x8000,0xFFBF,MWA_NOP},							// upper 32k unmapped on snes (except for switchable rom)
	{ 0xFFC0,0xFFFF,MWA_ROM},
MEMORY_END

#endif


