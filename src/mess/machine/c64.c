/***************************************************************************


***************************************************************************/
#include <ctype.h>
#include "driver.h"
#include "cpu/m6502/m6502.h"

#define VERBOSE_DBG 1
#include "cbm.h"
#include "cia6526.h"
#include "c1551.h"
#include "vc1541.h"
#include "vc20tape.h"
#include "mess/vidhrdw/vic6567.h"
#include "mess/sndhrdw/sid6581.h"

#include "c64.h"

static int ultimax=0;
static const char *romnames[2]={0};

static UINT8 keyline[10]={
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static UINT8 cartridge=0;
static enum { 
  CartridgeAuto=0, CartridgeUltimax, CartridgeC64,
  CartridgeSuperGames, CartridgeRobocop2
} cartridgetype=CartridgeAuto;
static UINT8 port6510, ddr6510;
static UINT8 game, exrom, roml_bank, romh_bank;
static UINT8 cia0porta;
static UINT8 serial_clock, serial_data, serial_atn;
static UINT8 *vicaddr;
static UINT8 vicirq=0,cia0irq=0;
static int low=0, high=0; // for addr to mess translation

UINT8 *c64_memory;
UINT8 *c64_colorram;
UINT8 *c64_basic;
UINT8 *c64_kernal;
UINT8 *c64_chargen;
UINT8 *c64_roml;
UINT8 *c64_romh;

/*
 cia 0
  port a
   7-0 keyboard line select
   7,6: paddle select( 01 port a, 10 port b)
   4: joystick a fire button
   3,2: Paddles port a fire button
   3-0: joystick a direction
  port b
   7-0: keyboard raw values
   4: joystick b fire button, lightpen select
   3,2: paddle b fire buttons (left,right)
   3-0: joystick b direction
  flag cassette read input, serial request in
  irq to irq connected
 */
static int c64_cia0_port_a_r(int offset)
{
  int value=0xff;
  value=keyline[9];
  return value;
}

static int c64_cia0_port_b_r(int offset)
{
  int value=0xff;
  if (!(cia0porta&0x80)) value&=keyline[7];
  if (!(cia0porta&0x40)) value&=keyline[6];
  if (!(cia0porta&0x20)) value&=keyline[5];
  if (!(cia0porta&0x10)) value&=keyline[4];
  if (!(cia0porta&8)) value&=keyline[3];
  if (!(cia0porta&4)) value&=keyline[2];
  if (!(cia0porta&2)) value&=keyline[1];
  if (!(cia0porta&1)) value&=keyline[0];
  value&=keyline[8];
  return value;
}

static void c64_cia0_port_a_w(int offset, int data)
{
  cia0porta=data;
}

static void c64_cia0_port_b_w(int offset, int data)
{
  vic2_lightpen_write(data&0x10);
}

static void c64_irq(int level)
{
  static int old_level=0;
  if (level != old_level)
    {
      DBG_LOG(3,"mos6510",(errorlog,"irq %s\n",level?"start":"end"));
      cpu_set_irq_line(0, M6510_INT_IRQ, level);
      old_level = level;
    }
}

static void c64_tape_read(int offset, int data)
{
  cia6526_set_input_flag(0,data);
}

static void c64_cia0_interrupt(int level)
{
  if (level != cia0irq)
    {
      c64_irq(level||vicirq);
      cia0irq = level;
    }
}

static void c64_vic_interrupt(int level)
{
  if (level != vicirq)
    {
      c64_irq(level||cia0irq);
      vicirq = level;
    }
}

/*
 cia 1
  port a
   7 serial bus data input
   6 serial bus clock input
   5 serial bus data output
   4 serial bus clock output
   3 serial bus atn output
   2 rs232 data output
   1-0 vic-chip system memory bank select

  port b
   7 user rs232 data set ready
   6 user rs232 clear to send
   5 user
   4 user rs232 carrier detect
   3 user rs232 ring indicator
   2 user rs232 data terminal ready
   1 user rs232 request to send
   0 user rs232 received data
  flag restore key or rs232 received data input
  irq to nmi connected ?
  */
static int c64_cia1_port_a_r(int offset)
{
  int value=0xff;
  if( !serial_clock || !cbm_serial_clock_read()) value&=~0x40;
  if( !serial_data || !cbm_serial_data_read()) value&=~0x80;
  return value;
}

static void c64_cia1_port_a_w(int offset, int data)
{
  static int helper[4]= { 0xc000,0x8000,0x4000,0x0000 };
  cbm_serial_clock_write(serial_clock=!(data&0x10));
  cbm_serial_data_write(serial_data=!(data&0x20));
  cbm_serial_atn_write(serial_atn=!(data&8));
  vicaddr=c64_memory+helper[data&3];
}

static void c64_cia1_interrupt(int level)
{
  static int old_level=0;
  if (level != old_level)
    {
      DBG_LOG(1,"mos6510",(errorlog,"nmi %s\n",level?"start":"end"));
      cpu_set_irq_line(0, M6510_INT_NMI, level);
      old_level = level;
    }
}

static struct cia6526_interface cia0={
  c64_cia0_port_a_r,
  c64_cia0_port_b_r,
  c64_cia0_port_a_w,
  c64_cia0_port_b_w,
  c64_cia0_interrupt
}, cia1= {
  c64_cia1_port_a_r,
  0,/*c64_cia1_port_b_r, */
  c64_cia1_port_a_w,
  0,/*c64_cia1_port_b_w, */
  c64_cia1_interrupt
};

static void c64_bankswitch(void);
static void c64_write_io(int offset, int value)
{
  if (offset<0x400) vic2_port_w(offset&0x3ff,value);
  else if (offset<0x800) sid6581_port_w(offset&0x3ff,value);
  else if (offset<0xc00) c64_colorram[offset&0x3ff]=value|0xf0;
  else if (offset<0xd00) cia6526_0_port_w(offset&0xff,value);
  else if (offset<0xe00) {
    if (!ultimax) cia6526_1_port_w(offset&0xff,value);
    else DBG_LOG(1,"io write",(errorlog,"%.3x %.2x\n",offset,value));
  } else if (offset<0xf00) {
    /* i/o 1 */
    if (cartridge && (cartridgetype==CartridgeRobocop2)) {
      /* robocop2 0xe00
	 80 94 80 94 80 
	 80 81 80 82 83 80 
	 */
      romh_bank=roml_bank=value&0xf;
      if (value&0x80) { game=value&0x10;exrom=1; }
      else { game=exrom=1; }
      c64_bankswitch();
    } else
      DBG_LOG(1,"io write",(errorlog,"%.3x %.2x\n",offset,value));
  } else {
    /* i/o 2 */
    if (cartridge && (cartridgetype==CartridgeSuperGames)) {
      /* supergam 0xf00
	 4 9 4
	 4 0 c
	 */
      romh_bank = roml_bank= value&3;
      if (value&4) { game=0;exrom=1; } 
      else { game=exrom=1; }
      if (value==0xc) { game=exrom=0; }
      c64_bankswitch();
    } else
      DBG_LOG(1,"io write",(errorlog,"%.3x %.2x\n",offset,value));
  }
}

static int c64_read_io(int offset)
{
  if (offset<0x400) return vic2_port_r(offset&0x3ff);
  else if (offset<0x800) return sid6581_port_r(offset&0x3ff);
  else if (offset<0xc00) return c64_colorram[offset&0x3ff];
  else if (offset<0xd00) return cia6526_0_port_r(offset&0xff);
  else if (!ultimax&&(offset<0xe00)) return cia6526_1_port_r(offset&0xff);
  DBG_LOG(1,"io read",(errorlog,"%.3x\n",offset));
  return 0xff;
}

/*
 two devices access bus, cpu and vic

 romh, roml chip select lines on expansion bus
 loram, hiram, charen bankswitching select by cpu
 exrom, game bankswitching select by cartridge
 va15, va14 bank select by cpu for vic

 exrom, game: normal c64 mode
 exrom, !game: ultimax mode

 romh: 8k rom at 0xa000 (hiram && !game && exrom)
       or 8k ram at 0xe000 (!game exrom)
 roml: 8k rom at 0x8000 (loram hiram !exrom)
       or 8k ram at 0x8000 (!game exrom)
 roml vic: upper 4k rom at 0x3000, 0x7000, 0xb000, 0xd000 (!game exrom)

 basic rom: 8k rom at 0xa000 (loram hiram game)
 kernal rom: 8k rom at 0xe000 (hiram !exrom, hiram game)
 char rom: 4k rom at 0xd000 (!exrom charen hiram
                             game charen !hiram loram
                             game charen hiram)
 cpu

  (write colorram)
  gr_w = !read&&!cas&&((address&0xf000)==0xd000)

  i_o = !game exrom !read ((address&0xf000)==0xd000)
        !game exrom ((address&0xf000)==0xd000)
        charen !hiram loram !read ((address&0xf000)==0xd000)
        charen !hiram loram ((address&0xf000)==0xd000)
        charen hiram !read ((address&0xf000)==0xd000)
        charen hiram ((address&0xf000)==0xd000)

 vic
  char rom: x101 (game, !exrom)
  romh: 0011 (!game, exrom)

 exrom !game (ultimax mode)
               addr    CPU     VIC-II
               ----    ---     ------
               0000    RAM     RAM
               1000    -       RAM
               2000    -       RAM
               3000    -       ROMH (upper half)
               4000    -       RAM
               5000    -       RAM
               6000    -       RAM
               7000    -       ROMH
               8000    ROML    RAM
               9000    ROML    RAM
               A000    -       RAM
               B000    -       ROMH
               C000    -       RAM
               D000    I/O     RAM
               E000    ROMH    RAM
               F000    ROMH    ROMH
 */
static void c64_bankswitch(void)
{
  static int old=-1, data, loram, hiram, charen;

  data=((port6510&ddr6510)|(ddr6510^0xff))&7;
  if (data==old) return;

  DBG_LOG(1,"bankswitch",(errorlog,"%d\n",data&7));
  loram=(data&1)?1:0;
  hiram=(data&2)?1:0;
  charen=(data&4)?1:0;

  if ( (!game&&exrom)
       ||(loram&&hiram&&!exrom) ) {
    if (cartridge&&((cartridgetype==CartridgeSuperGames)
		    ||(cartridgetype==CartridgeRobocop2)) ) {
      cpu_setbank(1,c64_roml+0x4000*roml_bank);
    } else {
      cpu_setbank(1,c64_roml);
    }
    cpu_setbankhandler_w(2,MWA_NOP);
  } else {
    cpu_setbank(1,c64_memory+0x8000);
    cpu_setbankhandler_w(2,MWA_RAM);
  }

  if ( (!game&&exrom&&hiram)
       ||(!exrom) ) {
    if (cartridge&&((cartridgetype==CartridgeSuperGames)
		    ||(cartridgetype==CartridgeRobocop2)) ) {
      cpu_setbank(3,c64_romh+0x4000*romh_bank);
    } else {
      cpu_setbank(3,c64_romh);
    }
  } else if (loram&&hiram) { cpu_setbank(3,c64_basic); }
  else { cpu_setbank(3,c64_memory+0xa000); }

  if ( (!game&&exrom)
       ||(charen&&(loram||hiram)) ) {
    cpu_setbankhandler_r(5,c64_read_io);
    cpu_setbankhandler_w(6,c64_write_io);
  } else {
    cpu_setbankhandler_r(5,MRA_BANK5);
    cpu_setbankhandler_w(6,MWA_BANK6);
    cpu_setbank(6,c64_memory+0xd000);
    if ( (1)
	 ||(!charen&&(loram||hiram)) ) { cpu_setbank(5,c64_chargen); }
    else { cpu_setbank(5,c64_memory+0xd000); }
  }

  if (!game&&exrom) {
    if (cartridge&&((cartridgetype==CartridgeSuperGames)
		    ||(cartridgetype==CartridgeRobocop2)) ) {
      cpu_setbank(7,c64_romh+0x4000*romh_bank);
    } else {
      cpu_setbank(7,c64_romh);
    }
    cpu_setbankhandler_w(8,MWA_NOP);
  } else {
    cpu_setbankhandler_w(8,MWA_RAM);
    if (hiram) { cpu_setbank(7,c64_kernal); } 
    else { cpu_setbank(7,c64_memory+0xe000); }
  }
  old=data;
}

/**
  ddr bit 1 port line is output
  port bit 1 port line is high

  p0 output loram
  p1 output hiram
  p2 output charen
  p3 output cassette data
  p4 input cassette switch
  p5 output cassette motor
  p6,7 not available on M6510
 */
void c64_m6510_port_w(int offset, int data)
{
  if (offset) {
    if (port6510!=data)
      port6510=data;
  } else {
    if (ddr6510!=data)
      ddr6510=data;
  }
  data=(port6510&ddr6510)|(ddr6510^0xff);
  vc20_tape_write(!(data&8));
  vc20_tape_motor(data&0x20);
  if (!ultimax) c64_bankswitch();
}

int c64_m6510_port_r(int offset)
{
  if (offset) {
    int data=(ddr6510&port6510)|(ddr6510^0xff);
    if (!(ddr6510&0x10)&&!vc20_tape_switch()) data&=~0x10;
    return data;
  } else { return ddr6510; }
}

int c64_paddle_read(int which)
{
  if (JOYSTICK_SWAP) {
    switch (cia0porta&0xc0) {
    case 0x80:
      if (which) return PADDLE2_VALUE;
      return PADDLE1_VALUE;
    case 0x40:
      if (which) return PADDLE4_VALUE;
      return PADDLE3_VALUE;
    default: return 0;
    }
  } else {
    switch (cia0porta&0xc0) {
    case 0x40:
      if (which) return PADDLE2_VALUE;
      return PADDLE1_VALUE;
    case 0x80:
      if (which) return PADDLE4_VALUE;
      return PADDLE3_VALUE;
    default: return 0;
    }
  }
}

void c64_colorram_write(int offset, int value)
{
  c64_colorram[offset&0x3ff]=value|0xf0;
}

/*
  only 14 address lines
  a15 and a14 portlines
  0x1000-0x1fff, 0x9000-0x9fff char rom
 */
static int c64_dma_read(int offset)
{
  if (!game&&exrom) {
    if (offset<0x3000) return c64_memory[offset];
    return c64_romh[offset&0x1fff];
  }
  if ((vicaddr==c64_memory)||(vicaddr==c64_memory+0x8000)) {
    if (offset<0x1000) return vicaddr[offset&0x3fff];
    if (offset<0x2000) return c64_chargen[offset&0xfff];
    return vicaddr[offset&0x3fff];
  }
  return vicaddr[offset&0x3fff];
}

static int c64_dma_read_ultimax(int offset)
{
  if (offset<0x3000) return c64_memory[offset];
  return c64_romh[offset&0x1fff];
}

static int c64_dma_read_color(int offset)
{
  return c64_colorram[offset&0x3ff]&0xf;
}

static void c64_common_driver_init(void)
{
  /*	memset(c64_memory, 0, 0xfd00); */

  vc20_tape_open(c64_tape_read);

  if (!ultimax) {
    cbm_drive_open();

    cbm_drive_attach_fs(0);
    cbm_drive_attach_fs(1);

#ifdef VC1541
    vc1541_driver_init();
#endif
  }

  sid6581_init(c64_paddle_read);
}

void c64_driver_init(void)
{
  c64_common_driver_init();
  cia6526_config(0,&cia0,0);
  cia6526_config(1,&cia1,0);
  vic6567_init(c64_dma_read,c64_dma_read_color,c64_vic_interrupt);
}

void ultimax_driver_init(void)
{
  ultimax=1;
  c64_common_driver_init();
  cia6526_config(0,&cia0,0);
  vic6567_init(c64_dma_read_ultimax,c64_dma_read_color,c64_vic_interrupt);
}

void c64pal_driver_init(void)
{
  c64_common_driver_init();
  cia6526_config(0,&cia0,1);
  cia6526_config(1,&cia1,1);
  vic6569_init(c64_dma_read,c64_dma_read_color, c64_vic_interrupt);
}

void c64_driver_shutdown(void)
{
  if (!ultimax) {
    cbm_drive_close();
  }
  vc20_tape_close();
}

void c64_init_machine(void)
{
  int i;

#ifdef VC1541
  vc1541_machine_init();
#endif
  if (!ultimax) {
    cbm_serial_reset_write(0);
    cbm_drive_0_config(SERIAL8ON?SERIAL:0);
    cbm_drive_1_config(SERIAL9ON?SERIAL:0);
  }
  cia6526_reset();
  vicaddr=c64_memory;
  vicirq=cia0irq=0;

  for (i=0; (romnames[i]!=0)&&(i<sizeof(romnames)/sizeof(romnames[0])); 
       i++)
    c64_rom_load(i, romnames[i]);

  exrom=1; game=1;
  if (cartridge) {
    if (AUTO_MODULE&&(cartridgetype==CartridgeAuto)) {
      if (errorlog) 
	fprintf(errorlog,"Cartridge type not recognized using Machine type\n");
    }
    if (C64_MODULE&&(cartridgetype==CartridgeUltimax)) {
      if (errorlog) 
	fprintf(errorlog,"Cartridge could be ultimax type!?\n");      
    }
    if (ULTIMAX_MODULE&&(cartridgetype==CartridgeC64)) {
      if (errorlog) 
	fprintf(errorlog,"Cartridge could be c64 type!?\n");
    }
    if (C64_MODULE) cartridgetype=CartridgeC64;
    else if (ULTIMAX_MODULE) cartridgetype=CartridgeUltimax;
    else if (SUPERGAMES_MODULE) cartridgetype=CartridgeSuperGames;
    else if (ROBOCOP2_MODULE) cartridgetype=CartridgeRobocop2;
    if (ultimax||(cartridgetype==CartridgeUltimax)) game=0;
    else exrom=0;
  }
  
  roml_bank=0;romh_bank=0;
  port6510=0xff; ddr6510=0;
  if (!ultimax) c64_bankswitch();
}

void c64_shutdown_machine(void)
{
}

static int c64_addr_to_mess(int addr, int size)
{
  if (ultimax) {
    if (addr==0) return 0x10000-size;
    return addr;
  }
  if ((addr==0x8000)&&(size==0x2000)) return (low++)*0x4000+0x16000;
  if ((addr==0x8000)&&(size==0x4000)) {
    cartridgetype=CartridgeC64;
    /*if (addr>=0x56000) addr=0x56000-in; prevent trap */
    high++;
    return (low++)*0x4000+0x16000;
  }
  if ((addr==0xa000)&&(size==0x2000)) {
    cartridgetype=CartridgeC64;
    return (high++)*0x4000+0x18000;
  }
  if (addr==0x8000) return 0x16000;
  if (addr==0x9000) return 0x17000;
  if (addr==0xe000) {
    cartridgetype=CartridgeUltimax;
    return 0x18000;
  }
  if (addr==0xf000) {
    cartridgetype=CartridgeUltimax;
    return 0x19000;
  }
  if (addr==0) {
    if (ULTIMAX_MODULE) return 0x18000+0x2000-size;
    return 0x16000+0x2000-size;
  }
  return addr;
}

int c64_rom_init(int id, const char *name)
{
  romnames[id]=name;
  return 0;
}

int c64_rom_load(int id, const char *name)
{
  UINT8 *mem=memory_region(REGION_CPU1);
  FILE *fp;
  int size, j, read;
  char *cp;
  unsigned int addr,adr=0;
  low=0; high=0;
  
  if(!c64_rom_id(name,Machine->gamedrv->name)) return 1;
  fp = osd_fopen(Machine->gamedrv->name, name, OSD_FILETYPE_IMAGE_R, 0);
  if(!fp) {
    if (errorlog) fprintf(errorlog,"%s file not found\n",name);
    return 1;
  }
  
  size = osd_fsize(fp);
  
  if ((cp=strrchr(name,'.'))!=NULL) {
    if (stricmp(cp,".prg")==0) {
      unsigned short in;
      osd_fread_lsbfirst(fp,&in, 2);
      if (errorlog) fprintf(errorlog,"rom prg %.4x\n",in);
      size-=2;
      if (errorlog)
	fprintf(errorlog,"loading rom %s at %.4x size:%.4x\n",
		name,in, size);
      addr=c64_addr_to_mess(in,size);
      read=osd_fread(fp,mem+addr,size);
      osd_fclose(fp);
      if (read!=size) return 1;
      cartridge=1;
    } else if (stricmp(cp,".crt")==0) {
      unsigned short in;
      osd_fseek(fp,64,SEEK_SET);
      j=64;
      if (errorlog) fprintf(errorlog,"loading rom %s size:%.4x\n",
			    name, size);
      while (j<size) {
	unsigned short segsize;
	unsigned char buffer[10], number;
	osd_fread(fp,buffer,6);
	osd_fread_msbfirst(fp,&segsize, 2);
	osd_fread(fp,buffer+6,3);
	osd_fread(fp,&number,1);
	osd_fread_msbfirst(fp,&adr,2); 
	osd_fread_msbfirst(fp,&in, 2);
	if (errorlog) {
	  fprintf(errorlog,"%.4s %.2x %.2x %.4x %.2x %.2x %.2x %.2x %.4x:%.4x\n",
		  buffer, buffer[4], buffer[5], segsize,
		  buffer[6], buffer[7], buffer[8], number,
		  adr,in);
	  fprintf(errorlog,"loading chip at %.4x size:%.4x\n", adr,in);
	}
	addr=c64_addr_to_mess(adr,in);
	read=osd_fread(fp,mem+addr,in);
	if (read!=in) { osd_fclose(fp); return 1; }
	j+=16+in;
      }
      osd_fclose(fp);
      cartridge=1;
    } else {
      if (stricmp(cp,".lo")==0) adr=0x8000+0x2000-size;
      if (stricmp(cp,".hi")==0) {
	if (ultimax) adr=0xe000+0x2000-size;
	else adr=0x8000+0x2000-size;
      }
      if (stricmp(cp,".80")==0) adr=0x8000;
      if (stricmp(cp,".90")==0) adr=0x9000;
      if (stricmp(cp,".a0")==0) adr=0xa000;
      if (stricmp(cp,".b0")==0) adr=0xb000;
      if (stricmp(cp,".e0")==0) adr=0xe000;
      if (stricmp(cp,".f0")==0) adr=0xf000;
      addr=c64_addr_to_mess(adr,size);
      if (errorlog)
	fprintf(errorlog,"loading %s rom at %.4x size:%.4x\n",
		name, adr, size);
      read=osd_fread(fp,mem+addr,size);
      addr+=size;
      osd_fclose(fp);
      if (read!=size) return 1;
      cartridge=1;
    }
  }
  return 0;
}

int c64_rom_id(const char *name, const char *gamename)
{
  /* magic lowrom at offset 0x8003: $c3 $c2 $cd $38 $30 */
  /* jumped to offset 0 (0x8000) */
  int retval=0;
  unsigned char magic[]={0xc3,0xc2,0xcd,0x38,0x30}, buffer[sizeof(magic)];
  FILE *romfile;
  char *cp;
  
  if (errorlog) fprintf(errorlog,"c64_rom_id %s\n",gamename);
  retval = 0;
  if (!(romfile = osd_fopen (name, name, OSD_FILETYPE_IMAGE_R, 0))) {
    if (errorlog) fprintf(errorlog,"rom %s not found\n",name);
    return 0;
  }
  
  osd_fseek(romfile,3,SEEK_SET);
  osd_fread(romfile,buffer,sizeof(magic));
  osd_fclose (romfile);
  
  if (memcmp(magic,buffer,sizeof(magic))==0) {
    /* cartridgetype=CartridgeC64; */
    retval=1;
  } else if ((cp=strrchr(name,'.'))!=NULL) {
    if ( (stricmp(cp+1,"prg")==0)
	 ||(stricmp(cp+1,"crt")==0)
	 ||(stricmp(cp+1,"80")==0)
	 ||(stricmp(cp+1,"90")==0)
	 ||(stricmp(cp+1,"e0")==0)
	 ||(stricmp(cp+1,"f0")==0)
	 ||(stricmp(cp+1,"a0")==0)
	 ||(stricmp(cp+1,"b0")==0)
	 ||(stricmp(cp+1,"lo")==0)||(stricmp(cp+1,"hi")==0) ) retval=1;
  }
  
  if (errorlog) {
    if (retval) fprintf(errorlog,"rom %s recognized\n",name);
    else fprintf(errorlog,"rom %s not recognized\n",name);
  }
  return retval;
}

int c64_frame_interrupt (void)
{
  static int quickload=0;
  int value;

  cia6526_1_set_input_flag(!KEY_RESTORE);

  if (!quickload && QUICKLOAD) cbm_quick_open(0,c64_memory);
  quickload=QUICKLOAD;

  value=0xff;
  if (KEY_CURSOR_DOWN) value&=~0x80;
  if (KEY_F5) value&=~0x40;
  if (KEY_F3) value&=~0x20;
  if (KEY_F1) value&=~0x10;
  if (KEY_F7) value&=~8;
  if (KEY_CURSOR_RIGHT) value&=~4;
  if (KEY_RETURN) value&=~2;
  if (KEY_DEL) value&=~1;
  keyline[0]=value;
  
  value=0xff;
  if (KEY_LEFT_SHIFT) value&=~0x80;
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
  if (KEY_ATSIGN) value&=~0x40;
  if (KEY_SEMICOLON) value&=~0x20;
  if (KEY_POINT) value&=~0x10;
  if (KEY_MINUS) value&=~8;
  if (KEY_L) value&=~4;
  if (KEY_P) value&=~2;
  if (KEY_PLUS) value&=~1;  
  keyline[5]=value;
  
  value=0xff;
  if (KEY_SLASH) value&=~0x80;
  if (KEY_ARROW_UP) value&=~0x40;
  if (KEY_EQUALS) value&=~0x20;
  if (KEY_RIGHT_SHIFT) value&=~0x10;
  if (KEY_HOME) value&=~8;
  if (KEY_COLON) value&=~4;
  if (KEY_ASTERIX) value&=~2;
  if (KEY_POUND) value&=~1;
  keyline[6]=value;
  
  value=0xff;
  if (KEY_STOP) value&=~0x80;
  if (KEY_Q) value&=~0x40;
  if (KEY_CBM) value&=~0x20;
  if (KEY_SPACE) value&=~0x10;
  if (KEY_2) value&=~8;
  if (KEY_CTRL) value&=~4;
  if (KEY_ARROW_LEFT) value&=~2;
  if (KEY_1) value&=~1;
  keyline[7]=value;
  
  value=0xff;
  if (JOYSTICK_1_BUTTON) value&=~0x10;
  if (JOYSTICK_1_RIGHT||PADDLE2_BUTTON) value&=~8;
  if (JOYSTICK_1_LEFT||PADDLE1_BUTTON) value&=~4;
  if (JOYSTICK_1_DOWN) value&=~2;
  if (JOYSTICK_1_UP) value&=~1;
  if (JOYSTICK_SWAP) keyline[9]=value;
  else keyline[8]=value;
  
  value=0xff;
  if (JOYSTICK_2_BUTTON) value&=~0x10;
  if (JOYSTICK_2_RIGHT||PADDLE4_BUTTON) value&=~8;
  if (JOYSTICK_2_LEFT||PADDLE3_BUTTON) value&=~4;
  if (JOYSTICK_2_DOWN) value&=~2;
  if (JOYSTICK_2_UP) value&=~1;
  if (JOYSTICK_SWAP) keyline[8]=value;
  else keyline[9]=value;

  vic2_frame_interrupt();
  
  vc20_tape_config(DATASSETTE,DATASSETTE_TONE);
  vc20_tape_buttons(DATASSETTE_PLAY,DATASSETTE_RECORD,DATASSETTE_STOP);
  osd_led_w(1/*KB_CAPSLOCK_FLAG*/,KEY_SHIFTLOCK?1:0);
  osd_led_w(0/*KB_NUMLOCK_FLAG*/,JOYSTICK_SWAP?1:0);
  
  return ignore_interrupt ();
}

/*only for debugging */
void c64_status(char *text, int size)
{
#if VERBOSE_DBG
  snprintf(text,size,"c64 vic:%.4x m6510:%d exrom:%d game:%d roml:%d romh:%d",
	   vicaddr-c64_memory, port6510&7, exrom, game, roml_bank, romh_bank);  
#endif
}
