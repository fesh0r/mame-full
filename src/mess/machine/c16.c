/***************************************************************************


***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "mess/machine/c16.h"
#include "mess/machine/c1551.h"
#include "mess/machine/vc1541.h"
#include "mess/machine/vc20tape.h"

static UINT8 keyline[10]={
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

CBM_Drive c16_drive8={0}, c16_drive9={0};

/*
  tia6523
  
  connector to floppy c1551 (delivered with c1551 as c16 expansion)
  port a for data read/write
  port b
    0 status 0
    1 status 1
  port c
    6 dav output edge data on port a available
    7 ack input edge ready for next datum
 */
typedef struct {
	CBM_Drive *drive;
	UINT8 ddra, ddrb, ddrc;
	UINT8 dataa,datab,datac;
} TIA6523;

static TIA6523 tia6523a = { &c16_drive8, 0 }, tia6523b = { &c16_drive9, 0 };

static UINT8 port6529, port7501, ddr7501;

static int lowrom=0, highrom=0;

UINT8 *c16_memory;
static UINT8 *c16_memory_10000;
static UINT8 *c16_memory_14000;
static UINT8 *c16_memory_18000;
static UINT8 *c16_memory_1c000;
static UINT8 *c16_memory_20000;
static UINT8 *c16_memory_24000;
static UINT8 *c16_memory_28000;
static UINT8 *c16_memory_2c000;


/**
  ddr bit 1 port line is output
  port bit 1 port line is high

  serial bus
  1 serial srq in (ignored)
  2 gnd
  3 atn out (pull up)
  4 clock in/out (pull up)
  5 data in/out (pull up)
  6 /reset (pull up) hardware


  p0 negated serial bus pin 5 /data out
  p1 negated serial bus pin 4 /clock out, cassette write
  p2 negated serial bus pin 3 /atn out
  p3 cassette motor out

  p4 cassette read
  p5 not connected (or not available on MOS7501?)
  p6 serial clock in
  p7 serial data in, serial bus 5
 */
void c16_m7501_port_w(int offset, int data)
{
  int dat, atn, clk;
  
  if (offset) {
    if (port7501!=data)
      port7501=data;
  } else {
    if (ddr7501!=data)
      ddr7501=data;
  }
  data=(port7501&ddr7501)|(ddr7501^0xff);
  // bit zero then output 0
  cbm_serial_atn_write(atn=!(data&4));
  cbm_serial_clock_write(clk=!(data&2));
  cbm_serial_data_write(dat=!(data&1));
  vc20_tape_write(!(data&2));
  vc20_tape_motor(data&8);
}

int c16_m7501_port_r(int offset)
{
  if (offset) {
    int data=(ddr7501&port7501)|(ddr7501^0xff);
    if (!(ddr7501&0x80)
	&& ( ((ddr7501&1)&&(port7501&1))||!cbm_serial_data_read()) )
      data&=~0x80;
    if ( !(ddr7501&0x40)
	 &&( ((ddr7501&2)&&(port7501&2)) ||!cbm_serial_clock_read()) )
      data&=~0x40;
    if (!(ddr7501&0x10)&&!vc20_tape_read()) data&=~0x10;
//		data&=~0x20; //port bit not in pinout
    return data;
  } else { return ddr7501; }
}

static void tia6523_port_w(TIA6523 *tia6523, int offset, int data)
{
  offset&=0x7;
  switch (offset) {
  case 0:
//    DBG_LOG(1,"iec8",(errorlog,"write port a %.2x\n",data));
    c1551_write_data(tia6523->drive, 
		     (data&tia6523->ddra)|(tia6523->ddra^0xff));
    tia6523->dataa=data;
    break;
  case 1:
    DBG_LOG(1,"iec8",(errorlog,"write port b %.2x\n",data));
    tia6523->datab=data;
    break;
  case 2:
//    DBG_LOG(1,"iec8",(errorlog,"write handshake out %d\n",data&0x40?1:0));
    c1551_write_handshake(tia6523->drive, (data&tia6523->ddrc&0x40)?1:0);
    tia6523->datac=data;
    break;
  case 3:
//    DBG_LOG(1,"iec8",(errorlog,"write ddr a %.2x\n",data));
    tia6523->ddra=data;
    c1551_write_data(tia6523->drive, tia6523->dataa&data);
    break;
  case 4:
    DBG_LOG(1,"iec8",(errorlog,"write ddr b %.2x\n",data));
    tia6523->ddrb=data;
    break;
  case 5:
    DBG_LOG(1,"iec8",(errorlog,"write ddr c %.2x\n",data));
    tia6523->ddrc=data;
    c1551_write_handshake(tia6523->drive, tia6523->datac&data&0x40);
    break;
  default: DBG_LOG(3,"iec8",(errorlog,"port write %.2x %.2x\n",offset,data));
  }
}

static int tia6523_port_r(TIA6523 *tia6523, int offset)
{
  int data=0;
  offset&=7;
  switch (offset) {
  case 0:
    data=(c1551_read_data(tia6523->drive)&~tia6523->ddra)
      |(tia6523->ddra&tia6523->dataa);
    DBG_LOG(1,"iec8",(errorlog,"read port a %.2x\n",data));
    break;
  case 1:
    data=(c1551_read_status(tia6523->drive)&~tia6523->ddrb)
      |(tia6523->ddrb&tia6523->datab);
//    DBG_LOG(1,"iec8",(errorlog,"read status %.2x\n",data));
    break;
  case 2:
    data=((c1551_read_handshake(tia6523->drive)?0x80:0)&~tia6523->ddrc)
      |(tia6523->ddrc&tia6523->datac);
//    DBG_LOG(1,"iec8",(errorlog,"read handshake in  %d\n",data&0x80?1:0));
    break;
  case 3: data=tia6523->ddra;break;
  case 4: data=tia6523->ddrb;break;
  case 5: data=tia6523->ddrc;break;
  }
  return data;
}


void c16_iec9_port_w(int offset, int data)
{
  offset&=7;
  tia6523_port_w(&tia6523b, offset, data);
  DBG_LOG(3,"iec9",(errorlog,"port write %.2x %.2x\n",offset,data));
}

int c16_iec9_port_r(int offset)
{
  int data=0;
  offset&=7;
  data=tia6523_port_r(&tia6523b, offset);
  DBG_LOG(3,"iec9",(errorlog,"port read %.2x %.2x\n",offset,data));
  return data;
}


void c16_iec8_port_w(int offset, int data)
{
  tia6523_port_w(&tia6523a, offset&7, data);
  DBG_LOG(3,"iec8",(errorlog,"port write %.2x %.2x\n",offset,data));
}

int c16_iec8_port_r(int offset)
{
  int data=0;
  data=tia6523_port_r(&tia6523a, offset&7);
  DBG_LOG(3,"iec8",(errorlog,"port read %.2x %.2x\n",offset,data));
  return data;
}

static void bankswitch(void)
{
  switch (lowrom) {
  case 0: cpu_setbank(2,c16_memory+0x10000);break;
  case 1: cpu_setbank(2,c16_memory+0x18000);break;
  case 2: cpu_setbank(2,c16_memory+0x20000);break;
  case 3: cpu_setbank(2,c16_memory+0x28000);break;
  }
  switch (highrom) {
  case 0:
    cpu_setbank(3,c16_memory+0x14000);
    cpu_setbank(8,c16_memory+0x17f20);
    break;
  case 1:
    cpu_setbank(3,c16_memory+0x1c000);
    cpu_setbank(8,c16_memory+0x1ff20);
    break;
  case 2:
    cpu_setbank(3,c16_memory+0x24000);
    cpu_setbank(8,c16_memory+0x27f20);
    break;
  case 3:
    cpu_setbank(3,c16_memory+0x2c000);
    cpu_setbank(8,c16_memory+0x2ff20);
    break;
  }
  cpu_setbank(4,c16_memory+0x17c00);
}

void c16_switch_to_rom(int offset, int data)
{
  ted7360_rom=1;
  bankswitch();
}

/* write access to fddX load data flipflop
	and selects roms
	a0 a1
	0  0  basic
	0  1  plus4 low
	1  0  c1 low
	1  1  c2 low

	a2 a3
	0  0  kernal
	0  1  plus4 hi
	1  0  c1 high
	1  1  c2 high */
extern void c16_select_roms(int offset, int data)
{
  lowrom=offset&3;
  highrom=(offset&0xc)>>2;
  if (ted7360_rom) bankswitch();
}

void c16_switch_to_ram(int offset, int data)
{
  ted7360_rom=0;
  switch (DIPMEMORY) {
  case MEMORY64K:
    cpu_setbank(2,c16_memory+0x8000);
    cpu_setbank(3,c16_memory+0xc000);
    cpu_setbank(4,c16_memory+0xfc00);
    cpu_setbank(8,c16_memory+0xff20);
    break;
  case MEMORY32K:
    cpu_setbank(2,c16_memory);
    cpu_setbank(3,c16_memory+0x4000);
    cpu_setbank(4,c16_memory+0x7c00);
    cpu_setbank(8,c16_memory+0x7f20);
    break;
  case MEMORY16K:
    cpu_setbank(2,c16_memory);
    cpu_setbank(3,c16_memory);
    cpu_setbank(4,c16_memory+0x3c00);
    cpu_setbank(8,c16_memory+0x3f20);
    break;
  }
}

int c16_read_keyboard(int databus)
{
  int value=0xff;

  if (!(port6529&1)) value&=keyline[0];
  if (!(port6529&2)) value&=keyline[1];
  if (!(port6529&4)) value&=keyline[2];
  if (!(port6529&8)) value&=keyline[3];
  if (!(port6529&0x10)) value&=keyline[4];
  if (!(port6529&0x20)) value&=keyline[5];
  if (!(port6529&0x40)) value&=keyline[6];
  if (!(port6529&0x80)) value&=keyline[7];
  
  /* looks like joy 0 needs dataline2 low
     and joy 1 needs dataline1 low
     write to 0xff08 (value on databus) reloads latches */
  if (!(databus&4)) value&=keyline[8];
  if (!(databus&2)) value&=keyline[9];

  return value;
}

/*
	mos 6529
	simple 1 port 8bit input output
	output with pull up resistors, 0 means low
	input, 0 means low
 */
/*
	ic used as output,
	output low means keyboard line selected
	keyboard line is then read into the ted7360 latch
 */
void c16_6529_port_w(int offset, int data)
{
  port6529=data;
}

int c16_6529_port_r(int offset)
{
  return port6529&(c16_read_keyboard(0xff/*databus!*/)|(port6529^0xff));
}

void c16_6551_port_w(int offset, int data)
{
  offset&=3;
  DBG_LOG(3,"6551",(errorlog,"port write %.2x %.2x\n",offset,data));
  port6529=data;
}

int c16_6551_port_r(int offset)
{
  int data=0;
  offset&=3;
  DBG_LOG(3,"6551",(errorlog,"port read %.2x %.2x\n",offset,data));
  return data;
}

int c16_fd1x_r(int offset)
{
  int data=0;
  if (vc20_tape_switch()) data|=4;
  return data;
}

static void c16_write_3f20(int offset, int data)
{
  c16_memory[0x3f20+offset]=data;
}

static void c16_write_3f40(int offset, int data)
{
  c16_memory[0x3f40+offset]=data;
}

static int ted7360_dma_read_16k(int offset)
{
  return c16_memory[offset&0x3fff];
}

static int ted7360_dma_read_32k(int offset)
{
  return c16_memory[offset&0x7fff];
}

static int ted7360_dma_read(int offset)
{
  return c16_memory[offset];
}

static int ted7360_dma_read_rom(int offset)
{
  /* should real real c16 system bus from 0xfd00 -ff1f */
  if (offset>=0xc000) { // rom address in rom
    if ((offset>=0xfc00)&&(offset<0xfd00)) 
      return c16_memory_10000[offset];
    switch (highrom) {
    case 0: return c16_memory_10000[offset&0x7fff];
    case 1: return c16_memory_18000[offset&0x7fff];
    case 2: return c16_memory_20000[offset&0x7fff];
    case 3: return c16_memory_28000[offset&0x7fff];
    }
  }
  if (offset>=0x8000) { // rom address in rom
    switch (lowrom) {
    case 0: return c16_memory_10000[offset&0x7fff];
    case 1: return c16_memory_18000[offset&0x7fff];
    case 2: return c16_memory_20000[offset&0x7fff];
    case 3: return c16_memory_28000[offset&0x7fff];
    }
  }
  switch (DIPMEMORY) {
  case MEMORY16K: return c16_memory[offset&0x3fff];
  case MEMORY32K: return c16_memory[offset&0x7fff];
  case MEMORY64K: return c16_memory[offset];
  }
  exit(0);
}

void c16_interrupt(int level)
{
  static int old_level;
  if (level != old_level)
    {
      DBG_LOG(3,"mos7501",(errorlog,"irq %s\n",level?"start":"end"));
      cpu_set_irq_line(0, M6510_INT_IRQ, level);
      old_level = level;
    }
}

static void c16_common_driver_init(void)
{
  c16_memory_10000=c16_memory+0x10000;
  c16_memory_14000=c16_memory+0x14000;
  c16_memory_18000=c16_memory+0x18000;
  c16_memory_1c000=c16_memory+0x1c000;
  c16_memory_20000=c16_memory+0x20000;
  c16_memory_24000=c16_memory+0x24000;
  c16_memory_28000=c16_memory+0x28000;
  c16_memory_2c000=c16_memory+0x2c000;

  //	memset(c16_memory, 0, 0xfd00);
  /* need to recognice non available tia6523's (iec8/9) */
  memset(c16_memory+0xfdc0,0xff,0x40);

  cbm_drive_init();
  c16_tape_open(options.cassette_name[0]);

  c16_rom_load();
#ifdef VC1541
  vc1541_driver_init();
#endif
}

void c16_driver_init(void)
{
  c16_common_driver_init();
  ted7360_init(1);
  ted7360_set_dma(ted7360_dma_read,ted7360_dma_read_rom);
}

void plus4_driver_init(void)
{
  c16_common_driver_init();
  ted7360_init(0);
  ted7360_set_dma(ted7360_dma_read,ted7360_dma_read_rom);
}

void c16_driver_shutdown(void)
{
  vc20_tape_close();
}

void c16_init_machine(void)
{
  int i,j;

  cpu_setbank(1,(DIPMEMORY==MEMORY16K)?c16_memory:c16_memory+0x4000);
  c16_switch_to_rom(0,0);
  c16_select_roms(0,0);
  switch (DIPMEMORY) {
  case MEMORY16K:
    install_mem_write_handler(0,0x4000,0x7fff,MWA_BANK5);
    install_mem_write_handler(0,0x8000,0xbfff,MWA_BANK6);
    install_mem_write_handler(0,0xc000,0xfcff,MWA_BANK7);
    cpu_setbank(5,c16_memory);
    cpu_setbank(6,c16_memory);
    cpu_setbank(7,c16_memory);
    install_mem_write_handler(0,0xff20,0xff3d,c16_write_3f20);
    install_mem_write_handler(0,0xff40,0xffff,c16_write_3f40);
    ted7360_set_dma(ted7360_dma_read_16k,ted7360_dma_read_rom);
    break;
  case MEMORY32K:
    install_mem_write_handler(0,0x4000,0x7fff,MWA_RAM);
    install_mem_write_handler(0,0x8000,0xfcff,MWA_BANK5);
    install_mem_write_handler(0,0xff20,0xff3d,MWA_BANK6);
    install_mem_write_handler(0,0xff40,0xffff,MWA_BANK7);
    cpu_setbank(5,c16_memory);
    cpu_setbank(6,c16_memory+0x7f20);
    cpu_setbank(7,c16_memory+0x7f40);
    ted7360_set_dma(ted7360_dma_read_32k,ted7360_dma_read_rom);
    break;
  case MEMORY64K:
    install_mem_write_handler(0,0x4000,0xfcff,MWA_RAM);
    install_mem_write_handler(0,0xff20,0xff3d,MWA_RAM);
    install_mem_write_handler(0,0xff40,0xffff,MWA_RAM);
    ted7360_set_dma(ted7360_dma_read,ted7360_dma_read_rom);
    break;
  }
  if (IEC8ON) {
    install_mem_write_handler(0,0xfee0,0xfeff,c16_iec8_port_w);
    install_mem_read_handler(0,0xfee0,0xfeff,c16_iec8_port_r);
  } else {
    install_mem_write_handler(0,0xfee0,0xfeff,MWA_NOP);
    install_mem_read_handler(0,0xfee0,0xfeff,MRA_NOP);
  }
  if (IEC9ON) {
    install_mem_write_handler(0,0xfec0,0xfedf,c16_iec9_port_w);
    install_mem_read_handler(0,0xfec0,0xfedf,c16_iec9_port_r);
  } else {
    install_mem_write_handler(0,0xfec0,0xfedf,MWA_NOP);
    install_mem_read_handler(0,0xfec0,0xfedf,MRA_NOP);
  }

  for (i = 0, j=0; i < Machine->gamedrv->num_of_floppy_drives; i++) {
    int rc;
    char *cp;
    /* no floppy name given for that drive ? */
    if (!options.floppy_name[i]) continue;
    if (!options.floppy_name[i][0]) continue;
    if ((cp=strrchr(options.floppy_name[0],'.'))!=NULL) {
      if (stricmp(cp+1,"d64")==0) {
        rc=0;
	if (j==0) {
	  if (IEC8ON) rc=c1551_d64_open(&c16_drive8, options.floppy_name[i]);
	  if (SERIAL8ON) 
	    rc=vc1541_d64_open(8,&c16_drive8, options.floppy_name[i]);
        } else
	  if (IEC9ON) rc=c1551_d64_open(&c16_drive9, options.floppy_name[i]);
	  if (SERIAL9ON)
	    rc=vc1541_d64_open(9,&c16_drive9, options.floppy_name[i]);
	if (rc==0) j++;
      }
    }
  }
  if (j==0) {
    if (IEC8ON) c1551_fs_open(&c16_drive8);
    if (SERIAL8ON) vc1541_fs_open(8,&c16_drive8);
  }
  if (j<=1) {
    if (IEC9ON) c1551_fs_open(&c16_drive9);
    if (SERIAL9ON) vc1541_fs_open(9,&c16_drive9);
  }
#ifdef VC1541
  vc1541_machine_init();
#endif
  cbm_serial_reset_write(0);
}

void c16_shutdown_machine(void)
{
  cbm_drive_close(&c16_drive8);
  cbm_drive_close(&c16_drive9);
}

int c16_quick_load(char *name)
{
  FILE *fp;
  int size, read;
  char *cp;
  unsigned int addr=0;
  
  fp = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_R, 0);
  if(!fp) {
    if (errorlog) fprintf(errorlog,"quickload %s file not found\n",name);
    return 1;
  }
    
  size = osd_fsize(fp);
    
  if ((cp=strrchr(name,'.'))!=NULL) {
    if (stricmp(cp,".prg")==0) {
      unsigned short in;
      osd_fread_lsbfirst(fp,&in, 2);
      addr=in;
      size-=2;
    }
  }
  if (addr==0) {
    osd_fclose(fp);
    return 1;
  }
  if (errorlog)
    fprintf(errorlog,"quick loading %s at %.5x size:%.4x\n",
	    name, addr, size);
  read=osd_fread(fp,c16_memory+addr,size);
  osd_fclose(fp);
  if (read!=size) return 1;
  addr+=size;
  c16_memory[0x31]=c16_memory[0x2f]=c16_memory[0x2d]=addr&0xff;
  c16_memory[0x32]=c16_memory[0x30]=c16_memory[0x2e]=addr>>8;
  return 0;
}

int c16_rom_load(void)
{
  int result	= 0;
  FILE *fp;
  int size, i, read;
  char *cp;
  unsigned int addr=0;
  
  for (i=0;(i<=2)&&(rom_name[i]!=NULL)&&(strlen(rom_name[i])!=0); i++) {
    
    if(!c16_rom_id(rom_name[i],rom_name[i])) continue;
    fp = osd_fopen(Machine->gamedrv->name, rom_name[i], OSD_FILETYPE_IMAGE_R, 0);
    if(!fp) {
      if (errorlog) fprintf(errorlog,"%s file not found\n",rom_name[i]);
      return 1;
    }
    
    size = osd_fsize(fp);
    
    if ((cp=strrchr(rom_name[i],'.'))!=NULL) {
      if (stricmp(cp,".prg")==0) {
	unsigned short in;
	osd_fread_lsbfirst(fp,&in, 2);
	if (errorlog) fprintf(errorlog,"rom prg %.4x\n",in);
	addr=in;
	size-=2;
      }
    }
    if (addr==0) {
      addr=0x20000;
    }
    if (errorlog)
      fprintf(errorlog,"loading rom %s at %.5x size:%.4x\n",
	      rom_name[i],addr, size);
    read=osd_fread(fp,c16_memory+addr,size);
    addr+=size;
    osd_fclose(fp);
    if (read!=size) return 1;
  }
  return result;
}

int c16_rom_id(const char *name, const char *gamename)
{
  // magic lowrom at offset 7: $43 $42 $4d
  // if at offset 6 stands 1 it will immediatly jumped to offset 0 (0x8000)
  int retval=0;
  char magic[]={0x43,0x42,0x4d}, buffer[sizeof(magic)];
  FILE *romfile;
  char *cp;
  
  if (errorlog) fprintf(errorlog,"c16_rom_id %s\n",gamename);
  retval = 0;
  if (!(romfile = osd_fopen (name, name, OSD_FILETYPE_IMAGE_R, 0))) {
    if (errorlog) fprintf(errorlog,"rom %s not found\n",name);
    return 0;
  }
  
  osd_fseek(romfile,7,SEEK_SET);
  osd_fread(romfile,buffer,sizeof(magic));
  osd_fclose (romfile);
  
  if (memcmp(magic,buffer,sizeof(magic))==0) {
    retval=1;
  } else if ((cp=strrchr(rom_name[0],'.'))!=NULL) {
    if ((stricmp(cp+1,"rom")==0)||(stricmp(cp+1,"prg")==0)
	||(stricmp(cp+1,"bin")==0)
	||(stricmp(cp+1,"lo")==0)||(stricmp(cp+1,"hi")==0) ) retval=1;
  }
  
  if (errorlog) {
    if (retval) fprintf(errorlog,"rom %s recognized\n",name);
    else fprintf(errorlog,"rom %s not recognized\n",name);
  }
  return retval;
}

int c16_frame_interrupt (void)
{
  static int quickload=0;
  int value;

  if (!quickload && QUICKLOAD) c16_quick_load(rom_name[0]);
  quickload=QUICKLOAD;

  value=0xff;
  if (KEY_ATSIGN) value&=~0x80;
  if (KEY_F3) value&=~0x40;
  if (KEY_F2) value&=~0x20;
  if (KEY_F1) value&=~0x10;
  if (KEY_HELP) value&=~8;
  if (KEY_POUND) value&=~4;
  if (KEY_RETURN) value&=~2;
  if (KEY_DEL) value&=~1;
  keyline[0]=value;
  
  value=0xff;
  if (KEY_SHIFT) value&=~0x80;
  if (KEY_E) value&=~0x40;
  if (KEY_S) value&=~0x20;
  if (KEY_Z) value&=~0x10;
  if (KEY_4) value&=~8;
  if (KEY_A) value&=~4;
  if (KEY_W) value&=~2;
  if (KEY_3) value&=~1;
  keyline[1]=value;
  
  value=0xff;
  if (KEY_X) value&=~0x80;
  if (KEY_T) value&=~0x40;
  if (KEY_F) value&=~0x20;
  if (KEY_C) value&=~0x10;
  if (KEY_6) value&=~8;
  if (KEY_D) value&=~4;
  if (KEY_R) value&=~2;
  if (KEY_5) value&=~1;
  keyline[2]=value;
  
  value=0xff;
  if (KEY_V) value&=~0x80;
  if (KEY_U) value&=~0x40;
  if (KEY_H) value&=~0x20;
  if (KEY_B) value&=~0x10;
  if (KEY_8) value&=~8;
  if (KEY_G) value&=~4;
  if (KEY_Y) value&=~2;
  if (KEY_7) value&=~1;
  keyline[3]=value;
  
  value=0xff;
  if (KEY_N) value&=~0x80;
  if (KEY_O) value&=~0x40;
  if (KEY_K) value&=~0x20;
  if (KEY_M) value&=~0x10;
  if (KEY_0) value&=~8;
  if (KEY_J) value&=~4;
  if (KEY_I) value&=~2;
  if (KEY_9) value&=~1;
  keyline[4]=value;
  
  value=0xff;
  if (KEY_COMMA) value&=~0x80;
  if (KEY_MINUS) value&=~0x40;
  if (KEY_SEMICOLON) value&=~0x20;
  if (KEY_POINT) value&=~0x10;
  if (KEY_UP) value&=~8;
  if (KEY_L) value&=~4;
  if (KEY_P) value&=~2;
  if (KEY_DOWN) value&=~1;
  keyline[5]=value;
  
  value=0xff;
  if (KEY_SLASH) value&=~0x80;
  if (KEY_PLUS) value&=~0x40;
  if (KEY_EQUALS) value&=~0x20;
  if (KEY_ESC) value&=~0x10;
  if (KEY_RIGHT) value&=~8;
  if (KEY_COLON) value&=~4;
  if (KEY_ASTERIX) value&=~2;
  if (KEY_LEFT) value&=~1;
  keyline[6]=value;
  
  value=0xff;
  if (KEY_STOP) value&=~0x80;
  if (KEY_Q) value&=~0x40;
  if (KEY_CBM) value&=~0x20;
  if (KEY_SPACE) value&=~0x10;
  if (KEY_2) value&=~8;
  if (KEY_CTRL) value&=~4;
  if (KEY_HOME) value&=~2;
  if (KEY_1) value&=~1;
  keyline[7]=value;
  
  value=0xff;
  if (JOYSTICK_1_BUTTON) value&=~0x40;
  if (JOYSTICK_1_RIGHT) value&=~8;
  if (JOYSTICK_1_LEFT) value&=~4;
  if (JOYSTICK_1_DOWN) value&=~2;
  if (JOYSTICK_1_UP) value&=~1;
  keyline[8]=value;
  
  value=0xff;
  if (JOYSTICK_2_BUTTON) value&=~0x80;
  if (JOYSTICK_2_RIGHT) value&=~8;
  if (JOYSTICK_2_LEFT) value&=~4;
  if (JOYSTICK_2_DOWN) value&=~2;
  if (JOYSTICK_2_UP) value&=~1;
  keyline[9]=value;
  
  ted7360_frame_interrupt();
  
  vc20_tape_config(DATASSETTE,DATASSETTE_TONE);
  vc20_tape_buttons(DATASSETTE_PLAY,DATASSETTE_RECORD,DATASSETTE_STOP);
  osd_led_w(1/*KB_CAPSLOCK_FLAG*/,KEY_SHIFTLOCK?1:0);
  
  return ignore_interrupt ();
}
