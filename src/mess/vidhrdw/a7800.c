/***************************************************************************

  vidhrdw/a7800.c

  Routines to control the Atari 7800 video hardware

  TODO:
    Kangaroo mode
    Remainder of the graphics modes

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"

#define TRIGGER_HSYNC	64717

#define READ_MEM(x) cpu_readmem16(x)

static struct osd_bitmap *maria_bitmap;

//static unsigned char *ROM;

/********** Maria ***********/

#define DPPH 0x2c
#define DPPL 0x30
#define CTRL 0x3c

int maria_palette[8][4];
int maria_write_mode;
int maria_scanline;
unsigned int maria_dll;
unsigned int maria_dl;
int maria_holey;
int maria_dmaon;
int maria_offset;
int maria_vblank;
int maria_dli;
int maria_dmaon_pending;
int maria_wsync;
int maria_backcolor;
unsigned int maria_charbase;

extern int maria_flag;
/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int a7800_vh_start(void)
{
    int i;

    if ((maria_bitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

    for(i=0; i<8; i++) {
        maria_palette[i][0]=0;
        maria_palette[i][1]=0;
        maria_palette[i][2]=0;
        maria_palette[i][3]=0;

    }
    maria_write_mode=0;
    maria_scanline=0;
    maria_dmaon=0;
    maria_vblank=0x80;
    maria_dll=0;
    maria_dmaon_pending=0;
    maria_wsync=0;
    return 0;
}

void a7800_vh_stop(void)
{
    osd_free_bitmap(maria_bitmap);
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/

void maria_draw_scanline(void) {
unsigned int graph_adr,data_addr;
int width,hpos,pal,mode,ind;
//unsigned int data_dat,dl;
unsigned int dl;
int x,d,c;
int ind_bytes;

	UINT8 *ROM = memory_region(REGION_CPU1);
    /* Process this DLL entry */
    dl = maria_dl;
    for (d=0; d<320; d++) maria_bitmap->line[maria_scanline][d] = maria_backcolor;
    /* Step through DL's */
    while (READ_MEM(dl + 1) != 0) {

	/* Extended header */
    if ((READ_MEM(dl+1) & 0x5F) == 0x40) {
        graph_adr = (READ_MEM(dl+2) << 8) | READ_MEM(dl);
        width = ((READ_MEM(dl+3) ^ 0xff) & 0x1F) + 1;
        hpos = READ_MEM(dl+4)*2;
        pal = READ_MEM(dl+3) >> 5;
        maria_write_mode = (READ_MEM(dl+1) & 0x80) >> 5;
        ind = READ_MEM(dl+1) & 0x20;
	    dl+=5;
	}
	/* Normal header */
	else {
        graph_adr = (READ_MEM(dl+2) << 8) | READ_MEM(dl);
        width = ((READ_MEM(dl+1) ^ 0xff) & 0x1F) + 1;
        hpos = READ_MEM(dl+3)*2;
        pal = READ_MEM(dl+1) >> 5;
	    ind = 0x00;
	    dl+=4;
	 }
     mode = (ROM[CTRL] & 0x03) | maria_write_mode;
//       logerror("%x DL: ADR=%x  width=%x  hpos=%x  pal=%x  mode=%x  ind=%x\n",maria_scanline,graph_adr,width,hpos,pal,mode,ind);

	 switch (mode) {
	    case 0x00:  /* 160A (160x2) */
	       for (x=0; x<width; x++) {
		  ind_bytes = 1;

		  /* Do direct mode */
		  if (!ind) {
		     data_addr = graph_adr + x + (maria_offset  << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
		  }
		  else {
             c = READ_MEM(graph_adr + x) & 0xFF;
		     data_addr= (maria_charbase | c) + (maria_offset << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
		     if (ROM[CTRL] & 0x10) ind_bytes = 2;
		  }

		  while (ind_bytes > 0) {
		  ind_bytes--;
          d = READ_MEM(data_addr++);
		  c = (d & 0xC0) >> 6;
		  if (c) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {
		      hpos+=2;
		  }

		  if (hpos > 510) hpos=0;
		  c = (d & 0x30) >> 4;
		  if (c) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {

		      hpos+=2;
		  }

		  if (hpos > 510) hpos=0;
		  c = (d & 0x0C) >> 2;
		  if (c) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {

		      hpos+=2;
		  }

		  if (hpos > 510) hpos=0;
		  c = (d & 0x03);
		  if (c) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {

		      hpos+=2;
		  }

		  if (hpos > 510) hpos=0;
	       }
	       }
	       break;
	    case 0x03:  /* MODE 320A */
	       for (x=0; x<width; x++) {
		  /* Do direct mode */
		  if (!ind) {
		     data_addr = graph_adr + x + (maria_offset  << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
             d = READ_MEM(data_addr);
		  }
		  else {
             c = READ_MEM(graph_adr + x) & 0xFF;
		     data_addr= (maria_charbase | c) + (maria_offset << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
             d = READ_MEM(data_addr);
		  }

		  if (d & 0x80) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][2];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;
		  if (d & 0x40) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][2];
		  }
		  else {

		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;
		  if (d & 0x20) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][2];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;
		  if (d & 0x10) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][2];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;
		  if (d & 0x08) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][2];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;
		  if (d & 0x04) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][2];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;
		  if (d & 0x02) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][2];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;
		  if (d & 0x01) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][2];
		  }
		  else {
		     hpos+=1;
		  }
		  if (hpos > 510) hpos=0;
	       }
	       break;
        case 0x04:  /* 160B (160x4) */
          for (x=0; x<width; x++) {
		  ind_bytes = 1;

		  /* Do direct mode */
		  if (!ind) {
		     data_addr = graph_adr + x + (maria_offset  << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
		  }
		  else {
             c = READ_MEM(graph_adr + x) & 0xFF;
		     data_addr= (maria_charbase | c) + (maria_offset << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
		     if (ROM[CTRL] & 0x10) ind_bytes = 2;
		  }

		  while (ind_bytes > 0) {
		  ind_bytes--;
          d = READ_MEM(data_addr++);

          c = (d & 0x0C) | ((d & 0xC0) >> 6);
          if (c == 4 || c == 8 || c == 12) c=0;
          if (c) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {
		      hpos+=2;
		  }
		  if (hpos > 510) hpos=0;

          c = ((d & 0x03) << 2) | ((d & 0x30) >> 4);
          if (c == 4 || c == 8 || c == 12) c=0;
		  if (c) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {

		      hpos+=2;
		  }

		  if (hpos > 510) hpos=0;
         }
       }
	       break;
       case 0x06:  /* MODE 320B */
	       for (x=0; x<width; x++) {
		  /* Do direct mode */
		  if (!ind) {
		     data_addr = graph_adr + x + (maria_offset  << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
             d = READ_MEM(data_addr);
		  }
		  else {
             c = READ_MEM(graph_adr + x) & 0xFF;
		     data_addr= (maria_charbase | c) + (maria_offset << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
             d = READ_MEM(data_addr);
		  }

          c = ((d & 0x80) >> 6) | ((d & 0x08) >> 3);
          if (c) {
              maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;

          c = ((d & 0x40) >> 5) | ((d & 0x04) >> 2);
          if (c) {
              maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;

          c = ((d & 0x20) >> 4) | ((d & 0x02) >> 1);
          if (c) {
              maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;

          c = ((d & 0x10) >> 3) | (d & 0x01);
          if (c) {
              maria_bitmap->line[maria_scanline][hpos++]=maria_palette[pal][c];
		  }
		  else {
		      hpos+=1;
		  }
		  if (hpos > 510) hpos=0;

        }
        break;

	    case 0x07: /* (320C mode) */
	      for (x=0; x<width; x++) {
		  /* Do direct mode */
		  if (!ind) {
		     data_addr = graph_adr + x + (maria_offset  << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
             d = READ_MEM(data_addr);
		  }
		  else {
             c = READ_MEM(graph_adr + x) & 0xFF;
		     data_addr= (maria_charbase | c) + (maria_offset << 8);
		     if (maria_holey == 0x02 && ((data_addr & 0x9000) == 0x9000)) continue;
		     if (maria_holey == 0x01 && ((data_addr & 0x8800) == 0x8800)) continue;
             d = READ_MEM(data_addr);
		  }
		  c = ((d & 0x0C) >> 2) | (pal & 0x04);
		  if (d & 0x80) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[c][2];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[c][2];
		  }
		  else {
		     hpos+=2;
		  }
		  if (hpos > 510) hpos=0;
		  if (d & 0x40) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[c][2];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[c][2];
		  }
		  else {
		     hpos+=2;
		  }
		  if (hpos > 510) hpos=0;
		  c = (d & 0x03) | (pal & 0x04);
		  if (d & 0x20) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[c][2];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[c][2];
		  }
		  else {
		      hpos+=2;
		  }
		  if (hpos > 510) hpos=0;
		  if (d & 0x10) {
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[c][2];
		      if (hpos > 510) hpos=0;
		      maria_bitmap->line[maria_scanline][hpos++]=maria_palette[c][2];
		  }
		  else {
		      hpos+=2;
		  }
		  if (hpos > 510) hpos=0;
	      }
	      break;
	    default:
	      logerror("Undefined mode: %x\n",mode);
	 }
    }
}


int a7800_interrupt(void)
{
    int frame_scanline;
	UINT8 *ROM = memory_region(REGION_CPU1);

    maria_scanline++;
    frame_scanline = maria_scanline % 263;

    if (maria_wsync) {
      cpu_trigger(TRIGGER_HSYNC);
      maria_wsync=0;
    }

    if (frame_scanline == 16) {
      maria_vblank=0;
      if (maria_dmaon_pending || maria_dmaon) {
	  maria_dmaon=1;
	  maria_dmaon_pending=0;
	  maria_dll = (ROM[DPPH] << 8) | ROM[DPPL];
//    logerror("DLL=%x\n",maria_dll);
      maria_dl = (READ_MEM(maria_dll+1) << 8) | READ_MEM(maria_dll+2);
      maria_offset = READ_MEM(maria_dll) & 0x0f;
      maria_holey = (READ_MEM(maria_dll) & 0x60) >> 5;
      maria_dli = READ_MEM(maria_dll) & 0x80;
//    logerror("DLL: DL = %x  dllctrl = %x\n",maria_dl,ROM[maria_dll]);
      }
    }

	if (frame_scanline > 15 && maria_dmaon) {
       if (maria_scanline < 258) maria_draw_scanline();
	   if (maria_offset == 0) {
	      maria_dll+=3;
          maria_dl = (READ_MEM(maria_dll+1) << 8) | READ_MEM(maria_dll+2);
          maria_offset = READ_MEM(maria_dll) & 0x0f;
          maria_holey = (READ_MEM(maria_dll) & 0x60) >> 5;
          maria_dli = READ_MEM(maria_dll) & 0x80;
	  }
	  else {
	     maria_offset--;
	  }
       }
    if (frame_scanline == 258) {
       maria_vblank = 0x80;
    }

    if (maria_dli) {
	maria_dli = 0;
	return M6502_INT_NMI;
    }
    else {
	return M6502_INT_NONE;
    }
}

/***************************************************************************

  Refresh the video screen

***************************************************************************/
/* This routine is called at the start of vblank to refresh the screen */
void a7800_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
    maria_scanline=0;
    copybitmap(bitmap,maria_bitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}


/****** MARIA ***************************************/

int a7800_MARIA_r(int offset) {
	UINT8 *ROM = memory_region(REGION_CPU1);
    switch (offset) {

	case 0x08:
          return maria_vblank;
	default:
	    logerror("undefined MARIA read %x\n",offset);
	    return ROM[0x20 + offset];
    }
}

void a7800_MARIA_w(int offset, int data) {
	UINT8 *ROM = memory_region(REGION_CPU1);
    switch (offset) {

	case 0x00:
	    maria_backcolor = Machine->pens[data];
	    break;
	case 0x01:
	    maria_palette[0][1] = Machine->pens[data];
	    break;
	case 0x02:
	    maria_palette[0][2] = Machine->pens[data];
	    break;
	case 0x03:
	    maria_palette[0][3] = Machine->pens[data];
	    break;
	case 0x04:
	    timer_holdcpu_trigger(0,TRIGGER_HSYNC);
	    maria_wsync=1;
	    break;

	case 0x05:
	    maria_palette[1][1] = Machine->pens[data];
	    break;
	case 0x06:
	    maria_palette[1][2] = Machine->pens[data];
	    break;
	case 0x07:
	    maria_palette[1][3] = Machine->pens[data];
	    break;

	case 0x09:
	    maria_palette[2][1] = Machine->pens[data];
	    break;
	case 0x0A:
	    maria_palette[2][2] = Machine->pens[data];
	    break;
	case 0x0B:
	    maria_palette[2][3] = Machine->pens[data];
	    break;

	case 0x0D:
	    maria_palette[3][1] = Machine->pens[data];
	    break;
	case 0x0E:
	    maria_palette[3][2] = Machine->pens[data];
	    break;
	case 0x0F:
	    maria_palette[3][3] = Machine->pens[data];
	    break;

	case 0x11:
	    maria_palette[4][1] = Machine->pens[data];
	    break;
	case 0x12:
	    maria_palette[4][2] = Machine->pens[data];
	    break;
	case 0x13:
	    maria_palette[4][3] = Machine->pens[data];
	    break;
	case 0x14:
	    maria_charbase = (data << 8);
	    break;
	case 0x15:
	    maria_palette[5][1] = Machine->pens[data];
	    break;
	case 0x16:
	    maria_palette[5][2] = Machine->pens[data];
	    break;
	case 0x17:
	    maria_palette[5][3] = Machine->pens[data];
	    break;

	case 0x19:
	    maria_palette[6][1] = Machine->pens[data];
	    break;
	case 0x1A:
	    maria_palette[6][2] = Machine->pens[data];
	    break;
	case 0x1B:
	    maria_palette[6][3] = Machine->pens[data];
	    break;

	case 0x1C:
//       logerror("MARIA CTRL=%x\n",data);
	    if ((data & 0x60) == 0x40)
		maria_dmaon_pending=1;
	    else
		maria_dmaon_pending=maria_dmaon=0;
	    break;
	case 0x1D:
	    maria_palette[7][1] = Machine->pens[data];
	    break;
	case 0x1E:
	    maria_palette[7][2] = Machine->pens[data];
	    break;
	case 0x1F:
	    maria_palette[7][3] = Machine->pens[data];
	    break;

    }
    ROM[0x20 + offset] = data;
}

