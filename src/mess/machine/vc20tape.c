/***************************************************************************

  cbm vc20/c64
  cbm c16 series (other physical representation)
  tape/cassette/datassette emulation

***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "unzip.h"
#include "cpu/m6502/m6502.h"
#include "mess/machine/vc20tape.h"

struct DACinterface vc20tape_sound_interface={
	1,
	{ 25 }
};

#define TONE_ON_VALUE 0xff

// write line high active,
// read line low active!?

static int tape_on, tape_noise;
static int tape_play, tape_record;

#define TAPE_WAV 1
#define TAPE_PRG 2
#define TAPE_ZIP 3
static int tape_type; // 0 nothing
static int tape_data;
static void (*tape_read_callback)(int,int);
static int tape_motor;

static int state=0;
static void *tape_timer;
static char *tape_imagename;
static int tape_pos;

// these are the values for wav files
static struct GameSample *tape_sample;

// these are the values for prg files
#define VC20_SHORT		(176e-6)
#define VC20_MIDDLE		(256e-6)
#define VC20_LONG		(336e-6)
#define C16_SHORT	(246e-6) // messured
#define C16_MIDDLE	(483e-6)
#define C16_LONG	(965e-6)
#define PCM_SHORT	(prg_c16?C16_SHORT:VC20_SHORT)
#define PCM_MIDDLE	(prg_c16?C16_MIDDLE:VC20_MIDDLE)
#define PCM_LONG	(prg_c16?C16_LONG:VC20_LONG)
static int prg_c16;
static UINT8 *tape_prg;
static int tape_length;
static int tape_stateblock, tape_stateheader, tape_statebyte, tape_statebit;
static int tape_prgdata;
static char tape_name[16]; //name for cbm
static UINT8 tape_chksum;
static double tape_lasttime;

// these are values for zip files
static ZIP *tape_zip;
static struct zipent* tape_zipentry;

// from sound/samples.c no changes (static declared)
// readsamples not useable (loads files only from sample or game directory)
// and doesn't search the rompath
#ifdef LSB_FIRST
#define intelLong(x) (x)
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif
static struct GameSample *vc20_read_wav_sample(void *f)
{
	unsigned long offset = 0;
	UINT32 length, rate, filesize, temp32;
	UINT16 bits, temp16;
	char buf[32];
	struct GameSample *result;

	/* read the core header and make sure it's a WAVE file */
	offset += osd_fread(f, buf, 4);
	if (offset < 4)
		return NULL;
	if (memcmp(&buf[0], "RIFF", 4) != 0)
		return NULL;

	/* get the total size */
	offset += osd_fread(f, &filesize, 4);
	if (offset < 8)
		return NULL;
	filesize = intelLong(filesize);

	/* read the RIFF file type and make sure it's a WAVE file */
	offset += osd_fread(f, buf, 4);
	if (offset < 12)
		return NULL;
	if (memcmp(&buf[0], "WAVE", 4) != 0)
		return NULL;

	/* seek until we find a format tag */
	while (1)
	{
		offset += osd_fread(f, buf, 4);
		offset += osd_fread(f, &length, 4);
		length = intelLong(length);
		if (memcmp(&buf[0], "fmt ", 4) == 0)
			break;

		/* seek to the next block */
		osd_fseek(f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return NULL;
	}

	/* read the format -- make sure it is PCM */
	offset += osd_fread_lsbfirst(f, &temp16, 2);
	if (temp16 != 1)
		return NULL;

	/* number of channels -- only mono is supported */
	offset += osd_fread_lsbfirst(f, &temp16, 2);
	if (temp16 != 1)
		return NULL;

	/* sample rate */
	offset += osd_fread(f, &rate, 4);
	rate = intelLong(rate);

	/* bytes/second and block alignment are ignored */
	offset += osd_fread(f, buf, 6);

	/* bits/sample */
	offset += osd_fread_lsbfirst(f, &bits, 2);
	if (bits != 8 && bits != 16)
		return NULL;

	/* seek past any extra data */
	osd_fseek(f, length - 16, SEEK_CUR);
	offset += length - 16;

	/* seek until we find a data tag */
	while (1)
	{
		offset += osd_fread(f, buf, 4);
		offset += osd_fread(f, &length, 4);
		length = intelLong(length);
		if (memcmp(&buf[0], "data", 4) == 0)
			break;

		/* seek to the next block */
		osd_fseek(f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return NULL;
	}

	/* allocate the game sample */
	result = malloc(sizeof(struct GameSample) + length);
	if (result == NULL)
		return NULL;

	/* fill in the sample data */
	result->length = length;
	result->smpfreq = rate;
	result->resolution = bits;

	/* read the data in */
	if (bits == 8)
	{
		osd_fread(f, result->data, length);

		/* convert 8-bit data to signed samples */
		for (temp32 = 0; temp32 < length; temp32++)
			result->data[temp32] ^= 0x80;
	}
	else
	{
		/* 16-bit data is fine as-is */
		osd_fread_lsbfirst(f, result->data, length);
	}

	return result;
}

static void vc20_wav_timer(int data);
static void vc20_wav_state(void)
{
  switch (state) {
  case 0: break; // not inited
  case 1: // off
    if (tape_on) { state=2;break; }
    break;
  case 2: // on
    if (!tape_on) {
      state=1;
      tape_play=0;tape_record=0;DAC_data_w(0,0);
      break;
    }
    if (tape_motor&&tape_play) {
      state=3;
      tape_timer=timer_pulse(1.0/tape_sample->smpfreq, 0, vc20_wav_timer);
      break;
    }
    if (tape_motor&&tape_record) {
      state=4;
      break;
    }
    break;
  case 3: // reading
    if (!tape_on) {
      state=1;
      tape_play=0;tape_record=0;DAC_data_w(0,0);
      if (tape_timer) timer_remove(tape_timer);
      break;
    }
    if (!tape_motor||!tape_play) {
      state=2;
      if (tape_timer) timer_remove(tape_timer);
      DAC_data_w(0,0);
      break;
    }
    break;
  case 4: // saving
    if (!tape_on) {
      state=1;
      tape_play=0;tape_record=0;DAC_data_w(0,0);
      break;
    }
    if (!tape_motor||!tape_record) {
      state=2;
      DAC_data_w(0,0);
      break;
    }
    break;
  }
}

static void vc20_wav_open(char *name)
{
  FILE *fp;

  fp = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_R, 0);
  if (!fp) {
    if (errorlog) fprintf(errorlog,"tape %s file not found\n",name);
    return;
  }
  if ((tape_sample=vc20_read_wav_sample(fp))==NULL) {
    if (errorlog) fprintf(errorlog,"tape %s could not be loaded\n",name);
    osd_fclose(fp);
    return;
  }
  if (errorlog) fprintf(errorlog,"tape %s loaded\n", name);
  osd_fclose(fp);

  strcpy(tape_imagename,name);
  tape_type=TAPE_WAV;
  tape_pos=0;
  tape_on=1;
  state=2;
}

static void vc20_wav_write(int data)
{
  if (tape_noise) DAC_data_w(0,data);
}

static void vc20_wav_timer(int data)
{
  if (tape_sample->resolution==8) {
    tape_data=tape_sample->data[tape_pos]>0x0;
    tape_pos++;
    if (tape_pos>=tape_sample->length) { tape_pos=0;tape_play=0; }
  } else {
    tape_data=((short*)(tape_sample->data))[tape_pos]>0x0;
    tape_pos++;
    if (tape_pos*2>=tape_sample->length) { tape_pos=0;tape_play=0; }
  }
  if (tape_noise) DAC_data_w(0,tape_data?TONE_ON_VALUE:0);
  if (tape_read_callback) tape_read_callback(0,tape_data);
  //	vc20_wav_state(); // removing timer in timer puls itself hangs
}

static void vc20_prg_timer(int data);
static void vc20_prg_state(void)
{
  switch (state) {
  case 0: break; // not inited
  case 1: // off
    if (tape_on) { state=2;break; }
    break;
  case 2: // on
    if (!tape_on) {
      state=1;
      tape_play=0;tape_record=0;DAC_data_w(0,0);
      break;
    }
    if (tape_motor&&tape_play) {
      state=3;
      tape_timer=timer_set(0.0, 0, vc20_prg_timer);
      break;
    }
    if (tape_motor&&tape_record) {
      state=4;
      break;
    }
    break;
  case 3: // reading
    if (!tape_on) {
      state=1;
      tape_play=0;tape_record=0;DAC_data_w(0,0);
      if (tape_timer) timer_remove(tape_timer);
      break;
    }
    if (!tape_motor||!tape_play) {
      state=2;
      if (tape_timer) timer_remove(tape_timer);
      DAC_data_w(0,0);
      break;
    }
    break;
  case 4: // saving
    if (!tape_on) {
      state=1;
      tape_play=0;tape_record=0;DAC_data_w(0,0);
      break;
    }
    if (!tape_motor||!tape_record) {
      state=2;
      DAC_data_w(0,0);
      break;
    }
    break;
  }
}

static void vc20_prg_open(char *name)
{
  FILE *fp;
  int i;

  fp = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_R, 0);
  if (!fp) {
    if (errorlog) fprintf(errorlog,"tape %s file not found\n",name);
    return;
  }
  tape_length=osd_fsize(fp);
  if ( (tape_prg=(UINT8*)malloc(tape_length))==NULL ) {
    if (errorlog) fprintf(errorlog,"tape %s could not be loaded\n",name);
    osd_fclose(fp);
    return;
  }
  osd_fread(fp,tape_prg,tape_length);
  if (errorlog) fprintf(errorlog,"tape %s loaded\n", name);
  osd_fclose(fp);

  for (i=0; name[i]!=0; i++) tape_name[i]=toupper(name[i]);
  for (;i<16;i++) tape_name[i]=' ';

  tape_stateblock=0;
  tape_stateheader=0;
  tape_statebyte=0;
  tape_statebit=0;
  tape_type=TAPE_PRG;
  prg_c16=0;
  tape_on=1;
  state=2;
  tape_pos=0;
}

static void vc20_prg_write(int data)
{
#if 0
  // this was used to decode cbms tape format, but could
  // be converted to a real program writer
  // c16: be sure the cpu clock is about 1.8 MHz (when screen is off)
  static int count=0;
  static int old=0;
  static double time=0;
  static int bytecount=0, byte;

  if (old!=data) {
    double neu=timer_get_time();
    int diff=(neu-time)*1000000;
    count++;
    if (errorlog) fprintf(errorlog,"%s %d\n",old?"high":"low", diff);
    if (old) {
      if (count>0/*27000*/) {
	switch (bytecount) {
	case 0:
	  if (diff>(PCM_LONG+PCM_MIDDLE)/2) { bytecount++;byte=0; }
	  break;
	case 1:case 3:case 5:case 7:case 9:case 11:
	case 13:case 15:case 17:
	  bytecount++;break;
	case 2:
	  if (diff>(PCM_MIDDLE+PCM_SHORT)/2) byte|=1;
	  bytecount++;
	  break;
	case 4:
	  if (diff>(PCM_MIDDLE+PCM_SHORT)/2) byte|=2;
	  bytecount++;
	  break;
	case 6:
	  if (diff>(PCM_MIDDLE+PCM_SHORT)/2) byte|=4;
	  bytecount++;
	  break;
	case 8:
	  if (diff>(PCM_MIDDLE+PCM_SHORT)/2) byte|=8;
	  bytecount++;
	  break;
	case 10:
	  if (diff>(PCM_MIDDLE+PCM_SHORT)/2) byte|=0x10;
	  bytecount++;
	  break;
	case 12:
	  if (diff>(PCM_MIDDLE+PCM_SHORT)/2) byte|=0x20;
	  bytecount++;
	  break;
	case 14:
	  if (diff>(PCM_MIDDLE+PCM_SHORT)/2) byte|=0x40;
	  bytecount++;
	  break;
	case 16:
	  if (diff>(PCM_MIDDLE+PCM_SHORT)/2) byte|=0x80;
	  if (errorlog) fprintf(errorlog,"byte %.2x\n",byte);
	  bytecount=0;
	  break;
	}
      }
    }
    old=data;
    time=timer_get_time();
  }
#endif
  if (tape_noise) DAC_data_w(0,data?TONE_ON_VALUE:0);
}

static void vc20_tape_bit(int bit)
{
  switch (tape_statebit) {
  case 0:
    if (bit) {
      timer_reset(tape_timer,tape_lasttime=PCM_MIDDLE);
      tape_statebit=2;
    } else {
      tape_statebit++;
      timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
    }
    break;
  case 1:
    timer_reset(tape_timer,tape_lasttime=PCM_MIDDLE);
    tape_statebit=0;
    break;
  case 2:
    timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
    tape_statebit=0;
    break;
  }
}

static void vc20_tape_byte(void)
{
  static int bit=0, parity=0;

  /* convert one byte to vc20 tape data
     puls wide modulation
     3 type of pulses (quadratic on/off pulse)
     K (short) 176 microseconds
     M 256
     L 336
     LM bit0 bit1 bit2 bit3 bit4 bit5 bit6 bit7 oddparity
     0 coded as KM, 1 as MK
     gives 8.96 milliseconds for 1 byte
     */
  switch (tape_statebyte) {
  case 0:
    timer_reset(tape_timer,tape_lasttime=PCM_LONG);
    tape_statebyte++;
    break;
  case 1:
    timer_reset(tape_timer,tape_lasttime=PCM_MIDDLE);
    tape_statebyte++;
    bit=1;
    parity=0;
    break;
  case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 9:
    vc20_tape_bit(tape_prgdata&bit);
    if (tape_prgdata&bit) parity=!parity;
    bit<<=1;
    tape_statebyte++;
    break;
  case 10:
    vc20_tape_bit(!parity);
    tape_chksum^=tape_prgdata;
    tape_statebyte=0;
    tape_pos--;
    break;
  }
}

/* 01 prg id
   lo hi load address
   lo hi end address
   192-5-1 bytes: filename (filled with 0x20)
   xor chksum */
static void vc20_tape_prgheader(void)
{
  static int i=0;
  switch (tape_stateheader) {
  case 0:
    tape_chksum=0;
    tape_prgdata=1;
    tape_stateheader++;
    vc20_tape_byte();
    break;
  case 1:
    tape_prgdata=tape_prg[0];
    tape_stateheader++;
    vc20_tape_byte();
    break;
  case 2:
    tape_prgdata=tape_prg[1];
    tape_stateheader++;
    vc20_tape_byte();
    break;
  case 3:
    tape_prgdata=(tape_prg[0]+tape_length-2)&0xff;
    tape_stateheader++;
    vc20_tape_byte();
    break;
  case 4:
    tape_prgdata=((tape_prg[0]+(tape_prg[1]<<8))+tape_length-2)>>8;
    tape_stateheader++;i=0;
    vc20_tape_byte();
    break;
  case 5:
    if ((i!=16)&&(tape_name[i]!=0)) {
      tape_prgdata=tape_name[i];
      i++;
      vc20_tape_byte();
      break;
    }
    tape_prgdata=0x20;
    tape_stateheader++;
    vc20_tape_byte();
    break;
  case 6:
    if (i!=192-5-1) { vc20_tape_byte();i++;break; }
    tape_prgdata=tape_chksum;
    vc20_tape_byte();
    tape_stateheader=0;
    break;
  }
}

static void vc20_tape_program(void)
{
  static int i=0;
  switch (tape_stateblock) {
  case 0:
    tape_pos=(9+192+1)*2+(9+tape_length-2+1)*2;
    i=0;
    tape_stateblock++;
    timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
    break;
  case 1:
    i++;if (i<4000/*27136*/) { // this time is not so important
      timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
      break;
    }
    // writing countdown $89 ... $80
    tape_stateblock++;
    tape_prgdata=0x89;
    vc20_tape_byte();
    break;
  case 2:
    if (tape_prgdata!=0x81) {
      tape_prgdata--;
      vc20_tape_byte();
      break;
    }
    tape_stateblock++;
    vc20_tape_prgheader();
    break;
  case 3:
    timer_reset(tape_timer,tape_lasttime=PCM_LONG);
    tape_stateblock++;
    i=0;
    break;
  case 4:
    if (i<80) {
      i++;
      timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
      break;
    }
    // writing countdown $09 ... $00
    tape_prgdata=9;
    tape_stateblock++;
    vc20_tape_byte();
    break;
  case 5:
    if (tape_prgdata!=1) {
      tape_prgdata--;
      vc20_tape_byte();
      break;
    }
    tape_stateblock++;
    vc20_tape_prgheader();
    break;
  case 6:
    timer_reset(tape_timer,tape_lasttime=PCM_LONG);
    tape_stateblock++;
    i=0;
    break;
  case 7:
    if (i<80) {
      i++;
      timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
      break;
    }
    i=0;
    tape_stateblock++;
    timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
    break;
  case 8:
    if (i<3000/*5376*/) {
      i++;
      timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
      break;
    }
    tape_prgdata=0x89;
    tape_stateblock++;
    vc20_tape_byte();
    break;
  case 9:
    if (tape_prgdata!=0x81) {
      tape_prgdata--;
      vc20_tape_byte();
      break;
    }
    i=2;
    tape_chksum=0;
    tape_prgdata=tape_prg[i];i++;
    vc20_tape_byte();
    tape_stateblock++;
    break;
  case 10:
    if (i<tape_length) {
      tape_prgdata=tape_prg[i];
      i++;
      vc20_tape_byte();
      break;
    }
    tape_prgdata=tape_chksum;
    vc20_tape_byte();
    tape_stateblock++;
    break;
  case 11:
    timer_reset(tape_timer,tape_lasttime=PCM_LONG);
    tape_stateblock++;
    i=0;
    break;
  case 12:
    if (i<80) {
      i++;
      timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
      break;
    }
    // writing countdown $09 ... $00
    tape_prgdata=9;
    tape_stateblock++;
    vc20_tape_byte();
    break;
  case 13:
    if (tape_prgdata!=1) {
      tape_prgdata--;
      vc20_tape_byte();
      break;
    }
    tape_chksum=0;
    i=2;
    tape_prgdata=tape_prg[i];i++;
    vc20_tape_byte();
    tape_stateblock++;
    break;
  case 14:
    if (i<tape_length) {
      tape_prgdata=tape_prg[i];
      i++;
      vc20_tape_byte();
      break;
    }
    tape_prgdata=tape_chksum;
    vc20_tape_byte();
    tape_stateblock++;
    break;
  case 15:
    timer_reset(tape_timer,tape_lasttime=PCM_LONG);
    tape_stateblock++;
    i=0;
    break;
  case 16:
    if (i<80) {
      i++;
      timer_reset(tape_timer,tape_lasttime=PCM_SHORT);
      break;
    }
    tape_stateblock=0;
    break;
  }

}

static void vc20_prg_timer(int data)
{
  if (!tape_data) { // send the same low phase
    if (tape_noise) DAC_data_w(0,0);
    tape_data=1;
    timer_reset(tape_timer, tape_lasttime);
  } else {
    if (tape_noise) DAC_data_w(0,TONE_ON_VALUE);
    tape_data=0;
    if (tape_statebit) {
      vc20_tape_bit(0);
    } else if (tape_statebyte) { // send the rest of the byte
      vc20_tape_byte();
    } else if (tape_stateheader) {
      vc20_tape_prgheader();
    } else {
      vc20_tape_program();
      if (!tape_stateblock) {
	tape_timer=0;
	tape_play=0;
      }
    }
  }
  if (tape_read_callback) tape_read_callback(0,tape_data);
  vc20_prg_state();
}

static void vc20_zip_timer(int data);
static void vc20_zip_state(void)
{
  switch (state) {
  case 0: break; // not inited
  case 1: // off
    if (tape_on) { state=2;break; }
    break;
  case 2: // on
    if (!tape_on) {
      state=1;
      tape_play=0;tape_record=0;DAC_data_w(0,0);
      break;
    }
    if (tape_motor&&tape_play) {
      state=3;
      tape_timer=timer_set(0.0, 0, vc20_zip_timer);
      break;
    }
    if (tape_motor&&tape_record) {
      state=4;
      break;
    }
    break;
  case 3: // reading
    if (!tape_on) {
      state=1;
      tape_play=0;tape_record=0;DAC_data_w(0,0);
      if (tape_timer) timer_remove(tape_timer);
      break;
    }
    if (!tape_motor||!tape_play) {
      state=2;
      if (tape_timer) timer_remove(tape_timer);
      DAC_data_w(0,0);
      break;
    }
    break;
  case 4: // saving
    if (!tape_on) {
      state=1;
      tape_play=0;tape_record=0;DAC_data_w(0,0);
      timer_remove(tape_timer);
      break;
    }
    if (!tape_motor||!tape_record) {
      state=2;
      timer_remove(tape_timer);
      DAC_data_w(0,0);
      break;
    }
    break;
  }
}

static void vc20_zip_readfile(void)
{
  int i;
  char *cp;

  for (i=0; i<2; i++) {
    tape_zipentry=readzip(tape_zip);
    if (tape_zipentry==NULL) {
      i++;rewindzip(tape_zip);continue;
    }
    if ((cp=strrchr(tape_zipentry->name,'.'))==NULL) continue;
    if (stricmp(cp,".prg")==0) break;
  }

  if (i==2) { state=0;return; }
  for (i=0; tape_zipentry->name[i]!=0; i++)
    tape_name[i]=toupper(tape_zipentry->name[i]);
  for (;i<16;i++) tape_name[i]=' ';

  tape_length=tape_zipentry->uncompressed_size;
  if ( (tape_prg=(UINT8*)malloc(tape_length))==NULL ) {
    if (errorlog) fprintf(errorlog,"out of memory\n");
    state=0;
  }
  readuncompresszip(tape_zip,tape_zipentry,(char*)tape_prg);
}

static void vc20_zip_open(char *name)
{
  if (!(tape_zip=openzip(name))) {
    if (errorlog) fprintf(errorlog,"tape %s not found\n",name);
    return;
  }

  if (errorlog) fprintf(errorlog,"tape %s linked\n", name);

  tape_stateblock=0;
  tape_stateheader=0;
  tape_statebyte=0;
  tape_statebit=0;
  tape_type=TAPE_ZIP;
  tape_on=1;
  state=2;
  tape_pos=0;
  prg_c16=0;

  vc20_zip_readfile();
}

static void vc20_zip_timer(int data)
{
  if (!tape_data) { // send the same low phase
    if (tape_noise) DAC_data_w(0,0);
    tape_data=1;
    timer_reset(tape_timer, tape_lasttime);
  } else {
    if (tape_noise) DAC_data_w(0,TONE_ON_VALUE);
    tape_data=0;
    if (tape_statebit) {
      vc20_tape_bit(0);
    } else if (tape_statebyte) { // send the rest of the byte
      vc20_tape_byte();
    } else if (tape_stateheader) {
      vc20_tape_prgheader();
    } else {
      vc20_tape_program();
      if (!tape_stateblock) {
	// loading next file of zip
	timer_reset(tape_timer,0.0);
	free(tape_prg);
	vc20_zip_readfile();
      }
    }
  }
  if (tape_read_callback) tape_read_callback(0,tape_data);
  vc20_prg_state();
}

void vc20_tape_open(char *name, void (*read_callback)(int,int) )
{
  char *cp;

  tape_read_callback=read_callback;
  tape_type=0;state=0;
  tape_on=0; tape_noise=0;
  tape_play=0;tape_record=0;
  tape_motor=0;tape_data=0;
  tape_imagename=name;

  if ((cp=strrchr(name,'.'))==NULL) return;
  if (stricmp(cp,".wav")==0) {
    vc20_wav_open(name);
  } else if (stricmp(cp,".prg")==0) {
    vc20_prg_open(name);
  } else if (stricmp(cp,".zip")==0) {
    vc20_zip_open(name);
  } else return;
}

void c16_tape_open(char *name)
{
  char *cp;

  tape_read_callback=NULL;
  tape_type=0;state=0;
  tape_on=0;tape_noise=0;
  tape_play=0;tape_record=0;
  tape_motor=0;tape_data=0;
  tape_imagename=name;

  if ((cp=strrchr(name,'.'))==NULL) return;
  if (stricmp(cp,".wav")==0) {
    vc20_wav_open(name);
  } else if (stricmp(cp,".prg")==0) {
    vc20_prg_open(name);
    prg_c16=1;
  } else if (stricmp(cp,".zip")==0) {
    vc20_zip_open(name);
    prg_c16=1;
  } else return;
}

void vc20_tape_close()
{
  switch (tape_type) {
  case TAPE_WAV:
    free(tape_sample);
    break;
  case TAPE_PRG:
    free(tape_prg);
    break;
  case TAPE_ZIP:
    free(tape_prg);
    closezip(tape_zip);
    break;
  }
}

static void vc20_state(void)
{
  switch (tape_type) {
  case TAPE_WAV: vc20_wav_state();break;
  case TAPE_PRG: vc20_prg_state();break;
  case TAPE_ZIP: vc20_zip_state();break;
  }
}

int vc20_tape_switch(void)
{
  int data=!((state>1)&&(tape_play||tape_record));
  return data;
}

int vc20_tape_read(void)
{
  if (state==3) return tape_data;
  return 0;
}

// here for decoding tape formats
void vc20_tape_write(int data)
{
  if (state==4) {
    if (tape_type==TAPE_WAV) vc20_wav_write(data);
    if (tape_type==TAPE_PRG) vc20_prg_write(data);
    if (tape_type==TAPE_ZIP) vc20_prg_write(data);
  }
}

void vc20_tape_config(int on, int noise)
{
  tape_on=(state!=0)&&on;
  tape_noise=tape_on&&noise;
  vc20_state();
}

void vc20_tape_buttons(int play, int record, int stop)
{
  if (stop) {
    tape_play=0, tape_record=0;
  } else if (play&&!tape_record) {
    tape_play=tape_on;
  } else if (record&&!tape_play) {
    tape_record=tape_on;
  }
  vc20_state();
}

void vc20_tape_motor(int data)
{
  tape_motor=!data;
  vc20_state();
}

void vc20_tape_status(char *text, int size)
{
  text[0]=0;
  switch (state) {
  case 4: strncpy(text,"Tape saving",size);break;
  case 3:
    switch (tape_type) {
    case TAPE_WAV:
      /*snprintf(text,size,"Tape (%s) loading %d/%dsec",
	       tape_imagename,tape_pos/tape_sample->smpfreq,
	       tape_sample->length/tape_sample->smpfreq);*/
      break;
    case TAPE_PRG:
      /*snprintf(text,size,"Tape (%s) loading %d", tape_imagename,tape_pos);*/
      break;
    case TAPE_ZIP:
      /*snprintf(text,size,"Tape (%s) loading %d", tape_imagename,tape_pos);*/
      break;
    }
    break;
  }
}
