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

Changes:

 V0.1 - Quickly knocked up now I have a 65816 core thanks to Karl Stenerud (well probably took a while actually ;-0 )

Notes:

 o At present I have increased the value of MH_HARDMAX in memory.h to allow me to load upto 1 megabyte of roms using the mapper as it stands.
 o Screen core is rendering in 16bit colour as this is THE best way of adding transparency effects at a later date.
 o Mosaic, 16x16 tiles, Windows, DSP, SFX, SA1 and quite a few other bits are unemulated.
 o Sprites are buggy at present, seems to be a severe problem in fzero for instance!
 o HDMA Indirect mode is not 100% yet.
 o Some Screen Modes (Namely 2,4,5 & 6) are either not supported or unfinished at this time.
 o Horizontal IRQ's are not emulated
 o To enable SPC700 emulation uncomment the EMULATE_SPC700 line in machine/snes.h
 o I need to find a way to synchronise the SPC and CPU when reading/writing to the ports otherwise spc and cpu could write data that the
  other is not ready for. This is for ROMS that don't use any form of handshaking when working with the SPC...

***************************************************************************/

#include "driver.h"
#include "cpu/g65816/g65816.h"
#include "vidhrdw/generic.h"
#include "../machine/snes.h"
#ifdef EMULATE_SPC700
#include "../sndhrdw/snes.h"
#include "cpu/spc700/spc700.h"
#endif

int paletteChanged=1;                                           // Force some form of initial palette download

int brightnessValue[16]={0x00,0x02,0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x13,0x15,0x17,0x19,0x1B,0x1D,0x1F};

int snes_line_interrupt(void)
{
    struct mame_bitmap *bitmap = Machine->scrbitmap;
    unsigned short pal,bri;
    static int CURLINE=0,a;
    int maxLines,r,g,b;

//  if (port21xx[0x33]&0x40)
        maxLines=240;
//  else
//      maxLines=223;

    if (paletteChanged)
    {
        bri=brightnessValue[0x0F - (port21xx[0x00] & 0x0F)];

        for (a=0;a<257;a++)
        {
            if (a==256)
                pal=fixedColour;
            else
                pal = ((SNES_CRAM[a*2+1]<<8)+SNES_CRAM[a*2])&0x7FFF;

            r = (pal & 0x1F) - bri;
            g = ((pal >> 5) & 0x1F) - bri;
            b = ((pal >> 10) & 0x1F) - bri;

            if (r<0)
                r=0;
            if (g<0)
                g=0;
            if (b<0)
                b=0;

            palIndx[a]=(b << 10) + (g << 5) + r;
        }
        paletteChanged=0;
    }

    if (CURLINE == maxLines)
    {
        if (port42xx[0x00]&0x80)                        // NMI only signalled by hardware if this bit set!
            cpu_cause_interrupt(0, G65816_LINE_NMI);
        port42xx[0x12]|=0x80;           // set in vblank bit
        port42xx[0x10]|=0x80;           // set nmi occurred
        port21xx[0x3E]&=0x3F;           // Clear Time Over and Range Over bits - done every nmi (presumably because no sprites drawn here)

        cpu_writemem24(0x002102,OAMADDRESS_L);          // Reset oam address at vblank
        cpu_writemem24(0x002103,OAMADDRESS_H);
    }

    if (CURLINE < maxLines)
        RenderSNESScreenLine(bitmap,CURLINE);

    if (port42xx[0x00]&0x30)
    {
        if (CURLINE == (((port42xx[0x0A]<<8)|port42xx[0x09])&0x01FF))
        {
            cpu_cause_interrupt(0, G65816_LINE_IRQ);
            port42xx[0x11]=0x80;                        // set "timeup"
        }
    }

    CURLINE=(CURLINE+1) % (262+10);

    if (CURLINE==0)
    {
        port42xx[0x12]&=0x7F;           // clear blanking bit
        port42xx[0x10]&=0x7F;           // clear nmi occurred bit
    }

    return ignore_interrupt();
}

#ifndef EMULATE_SPC700
unsigned char SPCSkipper(void)
{
    static int retType=0;
    unsigned char retVal=0x11;

//  G65816_PC=1, G65816_S, G65816_P, G65816_A, G65816_X, G65816_Y,
//  G65816_PB, G65816_DB, G65816_D, G65816_E,
//  G65816_NMI_STATE, G65816_IRQ_STATE

    switch (retType)
    {
        case 0:
            retVal=cpu_get_reg(4) & 0xFF;
            break;
        case 1:
            retVal=(cpu_get_reg(4) >> 8) & 0xFF;
            break;
        case 2:
            retVal=cpu_get_reg(5) & 0xFF;
            break;
        case 3:
            retVal=(cpu_get_reg(5) >> 8) & 0xFF;
            break;
        case 4:
            retVal=cpu_get_reg(6) & 0xFF;
            break;
        case 5:
            retVal=(cpu_get_reg(6) >> 8) & 0xFF;
            break;
        case 6:
            retVal=0xAA;
            break;
        case 7:
            retVal=0xBB;
            break;
        case 8:
            retVal=0x55;
            break;
        case 9:
            retVal=0x00;
            break;
        case 10:
            retVal=0xFF;
            break;
        case 11:
            retVal=0x01;            // A couple of roms are waiting for specific values
            break;
        case 12:
            retVal=0x07;
            break;
    }

    retType++;
    if (retType>12)
        retType=0;
    return retVal;
}
#endif

#define READ_VRAM(off)          temp1=(((port21xx[0x17]<<8) + port21xx[0x16])<<1)+off;          \
                                data=SNES_VRAM[temp1 & 0x1FFFF];                                \
                                if (((port21xx[0x15]&0x80)>>7)==off)                            \
                                {                                                               \
                                    temp1>>=1;                                                  \
                                    switch(port21xx[0x15]&0x03)                                 \
                                    {                                                           \
                                        case 0:                                                 \
                                            temp1+=1;                                           \
                                            break;                                              \
                                        case 1:                                                 \
                                            temp1+=32;                                          \
                                            break;                                              \
                                        case 2:                                                 \
                                        case 3:                                                 \
                                            temp1+=128; /* a bug in snes means +64 does +128 */ \
                                            break;                                              \
                                    }                                                           \
                                    port21xx[0x16]=temp1 & 0xFF;                                \
                                    port21xx[0x17]=(temp1>>8) & 0xFF;                           \
                                }

int oldPadRol=0;
READ_HANDLER ( snes_io_r )
{
    static int JOYPAD1H,JOYPAD1L;
    int temp1,data;
    switch (offset)             // Offset is from 0 not from 0x2000!!!!
    {
        case 0x0134:            //  MPYL    : %rrrrrrrr - r = result low
        case 0x0135:            //  MPYM    : %rrrrrrrr - r = result mid
        case 0x0136:            //  MPYH    : %rrrrrrrr - r = result hi
        case 0x013E:            //  STAT77  : %trm0vvvv - t = time over , r = range over , m=master select , v = version number
            return port21xx[offset & 0xFF];

        case 0x0140:            //  APUI00  : %dddddddd - d = data
        case 0x0141:            //  APUI01  : %dddddddd - d = data
        case 0x0142:            //  APUI02  : %dddddddd - d = data
        case 0x0143:            //  APUI03  : %dddddddd - d = data
#ifndef EMULATE_SPC700
            return SPCSkipper();
#else
            // SPC Read - Get SPC OUT value
            return SPCOUT[offset&0x0003];
#endif
        case 0x2016:            //  OLDJOY1 : %0000000b - b = bit from joypad
        case 0x2017:            //  OLDJOY2 : %0000000b - b = bit from joypad
            temp1=(JOYPAD1H << 8) | JOYPAD1L;
            temp1<<=oldPadRol;
            oldPadRol++;
            oldPadRol&=0x15;
            return temp1&0x01;
        case 0x2211:            //  TIMEUP  : %t0000000 - t = time up (h/v irq done) - reset on read
            temp1=port42xx[0x11];
            port42xx[0x11]=0;
            return temp1;
        case 0x2212:            //  HVBJOY  : %vh00000j - v = vblank , h = hblank , j = joy cont enable
            temp1=port42xx[0x12] & 0xBE;
            port42xx[0x12]=((port42xx[0x12]^0x41)&0x41)|temp1;      // JOYCONT and HBLANK are emulated wrong at present
            return port42xx[0x12];
        case 0x2214:            //  RDDIVL  : %rrrrrrrr - r = result low
        case 0x2215:            //  RDDIVH  : %rrrrrrrr - r = result hi
        case 0x2216:            //  RDMPYL  : %rrrrrrrr - r = result low
        case 0x2217:            //  RDMPYH  : %rrrrrrrr - r = result hi
        case 0x221A:            //  JOY2L   : %xylr0000 - x = xbutton pressed , y = ybutton pressed , l = tl pressed , r = tr pressed
        case 0x221B:            //  JOY2H   : %abstudlr - a = a pressed , b = b pressed , s = select pressed , t = start pressed
                                //                        u = up pressed , d = down pressed , l = left pressed , r = right pressed
        case 0x221C:            //  JOY3L   : %xylr0000 - x = xbutton pressed , y = ybutton pressed , l = tl pressed , r = tr pressed
        case 0x221D:            //  JOY3H   : %abstudlr - a = a pressed , b = b pressed , s = select pressed , t = start pressed
                                //                        u = up pressed , d = down pressed , l = left pressed , r = right pressed
        case 0x221E:            //  JOY4L   : %xylr0000 - x = xbutton pressed , y = ybutton pressed , l = tl pressed , r = tr pressed
        case 0x221F:            //  JOY4H   : %abstudlr - a = a pressed , b = b pressed , s = select pressed , t = start pressed
                                //                        u = up pressed , d = down pressed , l = left pressed , r = right pressed
            return port42xx[offset & 0xFF];

        case 0x2218:            //  JOY1L   : %xylr0000 - x = xbutton pressed , y = ybutton pressed , l = tl pressed , r = tr pressed
            JOYPAD1L=readinputport(0)&0xFF;
            return JOYPAD1L;
        case 0x2219:            //  JOY1H   : %abstudlr - a = a pressed , b = b pressed , s = select pressed , t = start pressed
                                //                        u = up pressed , d = down pressed , l = left pressed , r = right pressed
            JOYPAD1H=readinputport(1)&0xFF;
            return JOYPAD1H;
        case 0x2210:            //  RDNMI   : %n000vvvv - n = nmi enable , v = version  (reset when read)
            temp1=port42xx[0x10];
            port42xx[0x10]=0;
            return temp1;
        case 0x0139:            //  VMDATRL : %dddddddd - d = data
            READ_VRAM(0);
            return data;
        case 0x013A:            //  VMDATRH : %dddddddd - d = data
            READ_VRAM(1);
            return data;
    }
    logerror("Read From Unhandled Address : %04X\n",offset+0x2000);
    return 0x0ff;
}

#define DOUBLE_WRITE(a,b) wport21xx[(port21xx[a]++)&0x01][a]=b

#define BBUS(a) 0x00002100 + port43xx[a+1]
#define ABUS(a) 0x00000000 + (port43xx[a+4]<<16) + (port43xx[a+3]<<8) + port43xx[a+2]
#define DMAL(a) (port43xx[a+6]<<8) + port43xx[a+5]

void doGDMA(unsigned char bits)
{
    unsigned char bMask=0x01;
    int dmaBase = 0x00;
    int increment,direc,type;
    unsigned int BUSA,BUSB,LEN;
    // On the snes there are 8 dma channels - the priority of these is most likely 0 - 7, therefor the following should handle the dma

    while (bMask)
    {
        if (bMask&bits)
        {
            // Channel set so figure out operations
            if (port43xx[dmaBase + 0x00] & 0x08)
                increment=0;
            else
            {
                if (port43xx[dmaBase + 0x00] & 0x10)
                    increment=-1;
                else
                    increment=1;
            }

            BUSA=ABUS(dmaBase);
            BUSB=BBUS(dmaBase);

            if (port43xx[dmaBase + 0x00] & 0x80)
                direc=1;                            // Copy from PPU -> CPU
            else
                direc=0;                            // Copy from CPU -> PPU

            LEN=DMAL(dmaBase);

            type=port43xx[dmaBase + 0x00] & 0x07;
/*
            logerror("CPU -> %06X , DMA43%02X Type [%d] from %sPU [%08X] -> %sPU [%08X], %05X bytes , ABUS inc = %d\n",activecpu_get_pc(),dmaBase,type,direc==1 ? "P" : "C",direc==1 ? BUSB : BUSA,direc==1 ? "C" : "P",direc==1 ? BUSA : BUSB,LEN==0 ? 0x10000 : LEN,increment);
            if ((BUSB&0xFF)==0x18 || (BUSB&0xFF)==0x19)
            {
                logerror("VMAIN -> %02X\n",port21xx[0x15]);
                logerror("VRAM ADDRESS -> %04X\n",(port21xx[0x17]<<8)|port21xx[0x16]);
            }
            if ((BUSB&0xFF)==0x22)
                logerror("CRAM ADDRESS -> %04X\n",(port21xx[0x21]<<1));
*/
            switch (type)
            {
                case 0x00:                          // DMA 1 ppu address
                    while (LEN)
                    {
                        if (direc)      // PPU->CPU
                            cpu_writemem24(BUSA,cpu_readmem24(BUSB));
                        else
                            cpu_writemem24(BUSB,cpu_readmem24(BUSA));
                        BUSA+=increment;
                        LEN--;
                    }
                    break;
                case 0x01:
                    while (LEN)
                    {
                        if (direc)      // PPU->CPU
                            cpu_writemem24(BUSA,cpu_readmem24(BUSB));
                        else
                            cpu_writemem24(BUSB,cpu_readmem24(BUSA));
                        BUSA+=increment;
                        LEN--;
                        if (!LEN)
                            break;
                        if (direc)      // PPU->CPU
                            cpu_writemem24(BUSA,cpu_readmem24(BUSB+1));
                        else
                            cpu_writemem24(BUSB+1,cpu_readmem24(BUSA));
                        BUSA+=increment;
                        LEN--;
                    }
                    break;
                case 0x02:
                    while (LEN)
                    {
                        if (direc)      // PPU->CPU
                            cpu_writemem24(BUSA,cpu_readmem24(BUSB));
                        else
                            cpu_writemem24(BUSB,cpu_readmem24(BUSA));
                        BUSA+=increment;
                        LEN--;
                        if (!LEN)
                            break;
                        if (direc)      // PPU->CPU
                            cpu_writemem24(BUSA,cpu_readmem24(BUSB));
                        else
                            cpu_writemem24(BUSB,cpu_readmem24(BUSA));
                        BUSA+=increment;
                        LEN--;
                    }
                    break;
                default:
                    logerror("DMA operation [%d] unsupported at this time!\n",type);
                    break;
            }
        }
        dmaBase += 0x10;
        bMask<<=1;
    }
}

#define WRITE_VRAM(off,data)    temp1=(((port21xx[0x17]<<8) + port21xx[0x16])<<1)+off;          \
                                SNES_VRAM[temp1 & 0xFFFF]=data;                                 \
                                if (((port21xx[0x15]&0x80)>>7)==off)                            \
                                {                                                               \
                                    temp1>>=1;                                                  \
                                    switch(port21xx[0x15]&0x03)                                 \
                                    {                                                           \
                                        case 0:                                                 \
                                            temp1+=1;                                           \
                                            break;                                              \
                                        case 1:                                                 \
                                            temp1+=32;                                          \
                                            break;                                              \
                                        case 2:                                                 \
                                        case 3:                                                 \
                                            temp1+=128; /* a bug in snes means +64 does +128 */ \
                                            break;                                              \
                                    }                                                           \
                                    port21xx[0x16]=temp1 & 0xFF;                                \
                                    port21xx[0x17]=(temp1>>8) & 0xFF;                           \
                                }

#define WRITE_CRAM(data)        temp1=(port21xx[0x21]<<1) + port21xx[0x22];                     \
                                wport21xx[port21xx[0x22]][0x21]=data;                           \
                                SNES_CRAM[temp1]=data;                                          \
                                port21xx[0x22]++;                                               \
                                if (port21xx[0x22]==2)                                          \
                                {                                                               \
                                    port21xx[0x22]=0;                                           \
                                    port21xx[0x21]++;                                           \
                                    paletteChanged=1;                                           \
                                }

#define WRITE_ORAM(data)        temp1=(((port21xx[0x03]<<8) + port21xx[0x02])<<1)+port21xx[0x04];   \
                                wport21xx[port21xx[0x04]][0x04]=data;                           \
                                SNES_ORAM[temp1]=data;                                          \
                                port21xx[0x04]++;                                               \
                                if (port21xx[0x04]==2)                                          \
                                {                                                               \
                                    temp1>>=1;                                                  \
                                    port21xx[0x04]=0;                                           \
                                    temp1++;                                                    \
                                    port21xx[0x02]=temp1 & 0xFF;                                \
                                    port21xx[0x03]=(temp1>>8) & 0xFF;                           \
                                }

WRITE_HANDLER ( snes_io_w )
{
//  char tempb;
    short temps;
    int temp1,temp2,temp3;

    switch (offset)             // Offset is from 0 not from 0x2000!!!!
    {
        case 0x0115:            //  VMAIN   : %h000ffss - h = increment after write to 0x2118/0x2119 , f = full gfx , s = sc increment
            port21xx[0x15]=data;
            if (port21xx[0x15]&0x0C)
                logerror("HELP : Full graphic specified [%02X]\n",port21xx[0x15]);
            return;
        case 0x2016:            //  OLDJOY1 : %0000000b - b = bit from joypad
            oldPadRol=0;        // reset roll counter for sending pad data as bits to old nes ports
            return;
        case 0x0140:            //  APUI00  : %dddddddd - d = data          // Ignore writes to this until SPC exists
        case 0x0141:            //  APUI01  : %dddddddd - d = data
        case 0x0142:            //  APUI02  : %dddddddd - d = data
        case 0x0143:            //  APUI03  : %dddddddd - d = data
#ifndef EMULATE_SPC700
            return;
#else
            // SPC Read - Get SPC OUT value
            SPCIN[offset&0x0003]=data;
            return;
#endif
        case 0x0101:            //  OBSEL   : %sssnnbbb - s = size select , n = name select , b = base name select
        case 0x0105:            //  BGMODE  : %sssspmmm - s = 16x16 char for plane 0-3 , p = priority BG3 , m = bg mode 0 - 7
        case 0x0106:            //  MOSIAC  : %sssspppp - s = mosiac size , p = plane select 0-3
        case 0x0107:            //  BG1SC   : %bbbbbbss - b = base address * 1K , s = screen size
        case 0x0108:            //  BG2SC   : %bbbbbbss - b = base address * 1K , s = screen size
        case 0x0109:            //  BG3SC   : %bbbbbbss - b = base address * 1K , s = screen size
        case 0x010A:            //  BG4SC   : %bbbbbbss - b = base address * 1K , s = screen size
        case 0x010B:            //  BG12NBA : %aaaabbbb - a = bg2 base address , b = bg1 base address
        case 0x010C:            //  BG34NBA : %aaaabbbb - a = bg4 base address , b = bg3 base address
        case 0x011A:            //  M7SEL   : %oo0000vh - o = screen over , v = flip verticle , h = flip horizontal
        case 0x0123:            //  W12SEL  : %abcdefgh - a = bg2 w2 enable , b = bg2 w2 in/out , c = bg2 w1 enable , d = bg2 w1 in/out
                                //          :             e = bg1 w2 enable , f = bg1 w2 in/out , g = bg1 w1 enable , h = bg1 w1 in/out
        case 0x0124:            //  W34SEL  : %abcdefgh - a = bg4 w2 enable , b = bg4 w2 in/out , c = bg4 w1 enable , d = bg4 w1 in/out
                                //          :             e = bg3 w2 enable , f = bg3 w2 in/out , g = bg3 w1 enable , h = bg3 w1 in/out
        case 0x0125:            //  WOBJSEL : %abcdefgh - a = col w2 enable , b = col w2 in/out , c = col w1 enable , d = col w1 in/out
                                //          :             e = obj w2 enable , f = obj w2 in/out , g = obj w1 enable , h = obj w1 in/out
        case 0x0126:            //  WH0     : %pppppppp - p = position
        case 0x0127:            //  WH1     : %pppppppp - p = position
        case 0x0128:            //  WH2     : %pppppppp - p = position
        case 0x0129:            //  WH3     : %pppppppp - p = position
        case 0x012A:            //  WBGLOG  : %aabbccdd - a = bg4 logic , b = bg3 logic , c = bg2 logic , d = bg1 logic
        case 0x012B:            //  WOBJLOG : %0000aabb - a = col logic , b = obj logic
        case 0x012C:            //  TM      : %000obbbb - o = obj enable , b = bg1-4 enable  (main screen designation)
        case 0x012D:            //  TS      : %000obbbb - o = obj enable , b = bg1-4 enable  (sub screen designation)
        case 0x012E:            //  TMW     : %000obbbb - o = obj enable , b = bg1-4 enable  (window mask main screen designation)
        case 0x012F:            //  TSW     : %000obbbb - o = obj enable , b = bg1-4 enable  (window mask sub screen designation)
        case 0x0130:            //  CGWSEL  : %mmss00cd - m = main sw , s = sub sw , c = add enable , d = direct col select
        case 0x0131:            //  CGADDSUB: %shdobbbb - s = add/sub , h = 1/2 col , d = back , o = obj , b = bg1-bg4
        case 0x0133:            //  SETINI  : %ex00hlsi - e = external sync , x = extbg , h = 512 horizontal mode , l = 224/239 lines
                                //                        s = stretch sprites , i = interlace
            port21xx[offset&0xFF]=data;
            return;
        case 0x0132:            //  COLDATA : %bgrccccc - r = red , g = green , b = blue , c = brightness
            temps=data;
            temp1=(fixedColour & 0x001F);               // This is the fixed colour to ADD/SUB in CGADDSUB mode
            temp2=(fixedColour & 0x03E0);               //when back is specified this value is used as the furthest colour away
            temp3=(fixedColour & 0x7C00);               //ie. if no bpls or sprites obscure a pixel this value will be seen!
            if (data & 0x20)
                temp1=temps&0x1F;
            if (data & 0x40)
                temp2=(temps&0x1F)<<5;
            if (data & 0x80)
                temp3=(temps&0x1F)<<10;
            fixedColour=temp3 | temp2 | temp1;
            port21xx[0x32]=data;
            return;
        case 0x010D:            //  BG1HOFS : %oooooooo - o = offset .. NB this register is written twice to make high and low parts
        case 0x010E:            //  BG1VOFS : %oooooooo - o = offset .. NB this register is written twice to make high and low parts
        case 0x010F:            //  BG2HOFS : %oooooooo - o = offset .. NB this register is written twice to make high and low parts
        case 0x0110:            //  BG2VOFS : %oooooooo - o = offset .. NB this register is written twice to make high and low parts
        case 0x0111:            //  BG3HOFS : %oooooooo - o = offset .. NB this register is written twice to make high and low parts
        case 0x0112:            //  BG3VOFS : %oooooooo - o = offset .. NB this register is written twice to make high and low parts
        case 0x0113:            //  BG4HOFS : %oooooooo - o = offset .. NB this register is written twice to make high and low parts
        case 0x0114:            //  BG4VOFS : %oooooooo - o = offset .. NB this register is written twice to make high and low parts
        case 0x011B:            //  M7A     : %mmmmmmmm - m = matrix param .. NB this register is written twice to make high and low parts
        case 0x011D:            //  M7C     : %mmmmmmmm - m = matrix param .. NB this register is written twice to make high and low parts
        case 0x011E:            //  M7D     : %mmmmmmmm - m = matrix param .. NB this register is written twice to make high and low parts
        case 0x011F:            //  M7X     : %xxxxxxxx - x = x center .. NB this register is written twice to make high and low parts
        case 0x0120:            //  M7Y     : %yyyyyyyy - y = y center .. NB this register is written twice to make high and low parts
            DOUBLE_WRITE(offset&0xFF,data);
            return;
        case 0x011C:            //  M7B     : %mmmmmmmm - m = matrix param .. NB this register is written twice to make high and low parts
            // Performs multiply 16bit * 8bit
            temps=(wport21xx[1][0x1B]<<8) + wport21xx[0][0x1B];
            temp2=((signed char)data) * temps;
            port21xx[0x34]=temp2&0xFF;
            port21xx[0x35]=(temp2>>8)&0xFF;
            port21xx[0x36]=(temp2>>16)&0xFF;
            DOUBLE_WRITE(offset&0xFF,data);
            return;
        case 0x2200:            //  NMITIMEN: %n0vh000j - n = nmi enable , v = vert irq , h = hori irq , j = auto joy read
        case 0x2201:            //  WRIO    : %pppppppp - p = io port
        case 0x2202:            //  WRMPYA  : %vvvvvvvv - v = value of multiplicand a
        case 0x2204:            //  WRDIVL  : %vvvvvvvv - v = value of low part of dividend
        case 0x2205:            //  WRDIVH  : %vvvvvvvv - v = value of hi part of dividend
        case 0x2207:            //  HTIMEL  : %vvvvvvvv - v = horizontal timer value low
        case 0x2208:            //  HTIMEH  : %0000000v - v = horizontal timer value high
        case 0x2209:            //  VTIMEL  : %vvvvvvvv - v = vertical timer value low
        case 0x220A:            //  VTIMEH  : %0000000v - v = vertical timer value high
        case 0x220C:            //  HDMAEN  : %eeeeeeee - e = hdma channel enable 0 - 7
        case 0x220D:            //  MEMSEL  : %0000000c - c = cycle designation = 2.68 / 3.58
            port42xx[offset&0xFF]=data;
            return;
        case 0x2203:            //  WRMPYB  : %vvvvvvvv - v = value of multiplicand b
            // Perform multiply 8bit * 8bit
            temps=(unsigned char)(port42xx[0x02]);
            temp2=temps*data;
            port42xx[0x16]=temp2&0xFF;
            port42xx[0x17]=(temp2>>8)&0xFF;
            port42xx[0x03]=data;
            return;
        case 0x2206:            //  WRDIVB  : %vvvvvvvv - v = value of divisor
            // Perform division 16bit / 8bit
            temp1=(port42xx[0x05]<<8)+port42xx[0x04];
            if (data)
            {
                temp2=temp1/((unsigned char)data);
                temp3=temp1%((unsigned char)data);
            }
            else
            {
                temp2=0;
                temp3=0;
            }
            port42xx[0x14]=temp2 & 0xFF;
            port42xx[0x15]=(temp2>>8)&0xFF;
            port42xx[0x16]=temp3 & 0xFF;
            port42xx[0x17]=(temp3>>8)&0xFF;
            port42xx[0x06]=data;
            return;
        case 0x2300:            //  DMAP0   : %dt0aasss - d = direction , t = type , a = address , s = size
        case 0x2301:            //  BBAD0   : %aaaaaaaa - a = b-bus address
        case 0x2302:            //  A1T0L   : %aaaaaaaa - a = table address low
        case 0x2303:            //  A1T0H   : %aaaaaaaa - a = table address high
        case 0x2304:            //  A1B0    : %bbbbbbbb - b = table bank
        case 0x2305:            //  DAS0L   : %nnnnnnnn - n = number of bytes to transfer low
        case 0x2306:            //  DAS0H   : %nnnnnnnn - n = number of bytes to transfer high
        case 0x2307:            //  DASB0   : %bbbbbbbb - b = data bank
//      case 0x2308:            //  A2A0L   : %aaaaaaaa - a = a2 table address low
//      case 0x2309:            //  A2A0H   : %aaaaaaaa - a = a2 table address high
        case 0x2310:            //  DMAP1   : %dt0aasss - d = direction , t = type , a = address , s = size
        case 0x2311:            //  BBAD1   : %aaaaaaaa - a = b-bus address
        case 0x2312:            //  A1T1L   : %aaaaaaaa - a = table address low
        case 0x2313:            //  A1T1H   : %aaaaaaaa - a = table address high
        case 0x2314:            //  A1B1    : %bbbbbbbb - b = table bank
        case 0x2315:            //  DAS1L   : %nnnnnnnn - n = number of bytes to transfer low
        case 0x2316:            //  DAS1H   : %nnnnnnnn - n = number of bytes to transfer high
        case 0x2317:            //  DASB1   : %bbbbbbbb - b = data bank
//      case 0x2318:            //  A2A1L   : %aaaaaaaa - a = a2 table address low
//      case 0x2319:            //  A2A1H   : %aaaaaaaa - a = a2 table address high
        case 0x2320:            //  DMAP2   : %dt0aasss - d = direction , t = type , a = address , s = size
        case 0x2321:            //  BBAD2   : %aaaaaaaa - a = b-bus address
        case 0x2322:            //  A1T2L   : %aaaaaaaa - a = table address low
        case 0x2323:            //  A1T2H   : %aaaaaaaa - a = table address high
        case 0x2324:            //  A1B2    : %bbbbbbbb - b = table bank
        case 0x2325:            //  DAS2L   : %nnnnnnnn - n = number of bytes to transfer low
        case 0x2326:            //  DAS2H   : %nnnnnnnn - n = number of bytes to transfer high
        case 0x2327:            //  DASB2   : %bbbbbbbb - b = data bank
//      case 0x2328:            //  A2A2L   : %aaaaaaaa - a = a2 table address low
//      case 0x2329:            //  A2A2H   : %aaaaaaaa - a = a2 table address high
        case 0x2330:            //  DMAP3   : %dt0aasss - d = direction , t = type , a = address , s = size
        case 0x2331:            //  BBAD3   : %aaaaaaaa - a = b-bus address
        case 0x2332:            //  A1T3L   : %aaaaaaaa - a = table address low
        case 0x2333:            //  A1T3H   : %aaaaaaaa - a = table address high
        case 0x2334:            //  A1B3    : %bbbbbbbb - b = table bank
        case 0x2335:            //  DAS3L   : %nnnnnnnn - n = number of bytes to transfer low
        case 0x2336:            //  DAS3H   : %nnnnnnnn - n = number of bytes to transfer high
        case 0x2337:            //  DASB3   : %bbbbbbbb - b = data bank
//      case 0x2338:            //  A2A3L   : %aaaaaaaa - a = a2 table address low
//      case 0x2339:            //  A2A3H   : %aaaaaaaa - a = a2 table address high
        case 0x2340:            //  DMAP4   : %dt0aasss - d = direction , t = type , a = address , s = size
        case 0x2341:            //  BBAD4   : %aaaaaaaa - a = b-bus address
        case 0x2342:            //  A1T4L   : %aaaaaaaa - a = table address low
        case 0x2343:            //  A1T4H   : %aaaaaaaa - a = table address high
        case 0x2344:            //  A1B4    : %bbbbbbbb - b = table bank
        case 0x2345:            //  DAS4L   : %nnnnnnnn - n = number of bytes to transfer low
        case 0x2346:            //  DAS4H   : %nnnnnnnn - n = number of bytes to transfer high
        case 0x2347:            //  DASB4   : %bbbbbbbb - b = data bank
//      case 0x2348:            //  A2A4L   : %aaaaaaaa - a = a2 table address low
//      case 0x2349:            //  A2A4H   : %aaaaaaaa - a = a2 table address high
        case 0x2350:            //  DMAP5   : %dt0aasss - d = direction , t = type , a = address , s = size
        case 0x2351:            //  BBAD5   : %aaaaaaaa - a = b-bus address
        case 0x2352:            //  A1T5L   : %aaaaaaaa - a = table address low
        case 0x2353:            //  A1T5H   : %aaaaaaaa - a = table address high
        case 0x2354:            //  A1B5    : %bbbbbbbb - b = table bank
        case 0x2355:            //  DAS5L   : %nnnnnnnn - n = number of bytes to transfer low
        case 0x2356:            //  DAS5H   : %nnnnnnnn - n = number of bytes to transfer high
        case 0x2357:            //  DASB5   : %bbbbbbbb - b = data bank
//      case 0x2358:            //  A2A5L   : %aaaaaaaa - a = a2 table address low
//      case 0x2359:            //  A2A5H   : %aaaaaaaa - a = a2 table address high
        case 0x2360:            //  DMAP6   : %dt0aasss - d = direction , t = type , a = address , s = size
        case 0x2361:            //  BBAD6   : %aaaaaaaa - a = b-bus address
        case 0x2362:            //  A1T6L   : %aaaaaaaa - a = table address low
        case 0x2363:            //  A1T6H   : %aaaaaaaa - a = table address high
        case 0x2364:            //  A1B6    : %bbbbbbbb - b = table bank
        case 0x2365:            //  DAS6L   : %nnnnnnnn - n = number of bytes to transfer low
        case 0x2366:            //  DAS6H   : %nnnnnnnn - n = number of bytes to transfer high
        case 0x2367:            //  DASB6   : %bbbbbbbb - b = data bank
//      case 0x2368:            //  A2A6L   : %aaaaaaaa - a = a2 table address low
//      case 0x2369:            //  A2A6H   : %aaaaaaaa - a = a2 table address high
        case 0x2370:            //  DMAP7   : %dt0aasss - d = direction , t = type , a = address , s = size
        case 0x2371:            //  BBAD7   : %aaaaaaaa - a = b-bus address
        case 0x2372:            //  A1T7L   : %aaaaaaaa - a = table address low
        case 0x2373:            //  A1T7H   : %aaaaaaaa - a = table address high
        case 0x2374:            //  A1B7    : %bbbbbbbb - b = table bank
        case 0x2375:            //  DAS7L   : %nnnnnnnn - n = number of bytes to transfer low
        case 0x2376:            //  DAS7H   : %nnnnnnnn - n = number of bytes to transfer high
        case 0x2377:            //  DASB7   : %bbbbbbbb - b = data bank
//      case 0x2378:            //  A2A7L   : %aaaaaaaa - a = a2 table address low
//      case 0x2379:            //  A2A7H   : %aaaaaaaa - a = a2 table address high
            port43xx[offset&0xFF]=data;
            return;
        case 0x220B:            //  MDMAEN  : %eeeeeeee - e = general dma channel enable 0 - 7
            doGDMA(data);
            port42xx[offset&0xFF]=0;        // This is done because DMA is done in place, and on return it should clear all bits set!
            return;
        case 0x0118:            //  VMDATAL : %dddddddd - d = data
            WRITE_VRAM(0,data);
            return;
        case 0x0119:            //  VMDATAH : %dddddddd - d = data
            WRITE_VRAM(1,data);
            return;
        case 0x0122:            //  CGDATA  : %dddddddd - d = data - double write register
            WRITE_CRAM(data);
            return;
        case 0x0104:            //  OAMDATA : %dddddddd - d = data - auto increment
            WRITE_ORAM(data);
            return;
        case 0x0102:            //  OAMADDL : %aaaaaaaa - a = address LSB
            port21xx[0x02]=data;
            OAMADDRESS_L=data;
            port21xx[0x04]=0;
            return;
        case 0x0103:            //  OAMADDH : %r000000a - r = priority rotation , a = MSB of address
            port21xx[0x03]=data;
            OAMADDRESS_H=data;
            port21xx[0x04]=0;
            return;
        case 0x0116:            //  VMADDL  : %aaaaaaaa - a = LSB of vram address
        case 0x0117:            //  VMADDH  : %aaaaaaaa - a = MSB of vram address
            port21xx[offset&0xFF]=data;
            port21xx[0x18]=0;
            port21xx[0x19]=0;
            return;
        case 0x0121:            //  CGADD   : %aaaaaaaa - a = cgram address
            port21xx[0x21]=data;
            port21xx[0x22]=0;
            return;
        case 0x0100:            //  INIDISP : %b000ffff - b = blanking , f = brightness 0 to 15
            port21xx[0x00]=data;
            paletteChanged=1;
            return;
    }
    logerror("Write To Unhandled Address : %04X,%02X\n",offset+0x2000,data);
}

static struct GfxDecodeInfo snes_gfxdecodeinfo[] = {
MEMORY_END   /* end of array */

INPUT_PORTS_START( snes )

    PORT_START  // Joypad 1 - L
    PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 )
    PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 )
    PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 )
    PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 )

    PORT_START  // Joypad 1 - H
    PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 )
    PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 )
    PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_SELECT1 )
    PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
    PORT_BIT ( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
    PORT_BIT ( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
    PORT_BIT ( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
    PORT_BIT ( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )

INPUT_PORTS_END

READ_HANDLER (MRA_WRAM)
{
    return SNES_WRAM[offset];
}

WRITE_HANDLER (MWA_WRAM)
{
    SNES_WRAM[offset]=data;
}

READ_HANDLER (MRA_SRAM)
{
    return SNES_SRAM[offset];
}

WRITE_HANDLER (MWA_SRAM)
{
    SNES_SRAM[offset]=data;
}

static MEMORY_READ_START( snes_readmem )     {
     0x000000,0x001FFF,MRA_WRAM},
    { 0x002000,0x007FFF,snes_io_r},
    { 0x008000,0x00FFFF,MRA_ROM},
    { 0x010000,0x011FFF,MRA_WRAM},
    { 0x012000,0x017FFF,snes_io_r},
    { 0x018000,0x01FFFF,MRA_ROM},
    { 0x020000,0x021FFF,MRA_WRAM},
    { 0x022000,0x027FFF,snes_io_r},
    { 0x028000,0x02FFFF,MRA_ROM},
    { 0x030000,0x031FFF,MRA_WRAM},
    { 0x032000,0x037FFF,snes_io_r},
    { 0x038000,0x03FFFF,MRA_ROM},
    { 0x040000,0x041FFF,MRA_WRAM},
    { 0x042000,0x047FFF,snes_io_r},
    { 0x048000,0x04FFFF,MRA_ROM},
    { 0x050000,0x051FFF,MRA_WRAM},
    { 0x052000,0x057FFF,snes_io_r},
    { 0x058000,0x05FFFF,MRA_ROM},
    { 0x060000,0x061FFF,MRA_WRAM},
    { 0x062000,0x067FFF,snes_io_r},
    { 0x068000,0x06FFFF,MRA_ROM},
    { 0x070000,0x071FFF,MRA_WRAM},
    { 0x072000,0x077FFF,snes_io_r},
    { 0x078000,0x07FFFF,MRA_ROM},
    { 0x080000,0x081FFF,MRA_WRAM},
    { 0x082000,0x087FFF,snes_io_r},
    { 0x088000,0x08FFFF,MRA_ROM},
    { 0x090000,0x091FFF,MRA_WRAM},
    { 0x092000,0x097FFF,snes_io_r},
    { 0x098000,0x09FFFF,MRA_ROM},
    { 0x0A0000,0x0A1FFF,MRA_WRAM},
    { 0x0A2000,0x0A7FFF,snes_io_r},
    { 0x0A8000,0x0AFFFF,MRA_ROM},
    { 0x0B0000,0x0B1FFF,MRA_WRAM},
    { 0x0B2000,0x0B7FFF,snes_io_r},
    { 0x0B8000,0x0BFFFF,MRA_ROM},
    { 0x0C0000,0x0C1FFF,MRA_WRAM},
    { 0x0C2000,0x0C7FFF,snes_io_r},
    { 0x0C8000,0x0CFFFF,MRA_ROM},
    { 0x0D0000,0x0D1FFF,MRA_WRAM},
    { 0x0D2000,0x0D7FFF,snes_io_r},
    { 0x0D8000,0x0DFFFF,MRA_ROM},
    { 0x0E0000,0x0E1FFF,MRA_WRAM},
    { 0x0E2000,0x0E7FFF,snes_io_r},
    { 0x0E8000,0x0EFFFF,MRA_ROM},
    { 0x0F0000,0x0F1FFF,MRA_WRAM},
    { 0x0F2000,0x0F7FFF,snes_io_r},
    { 0x0F8000,0x0FFFFF,MRA_ROM},
/*  { 0x100000,0x101FFF,MRA_WRAM},
    { 0x102000,0x107FFF,snes_io_r},
    { 0x108000,0x10FFFF,MRA_ROM},
    { 0x110000,0x111FFF,MRA_WRAM},
    { 0x112000,0x117FFF,snes_io_r},
    { 0x118000,0x11FFFF,MRA_ROM},
    { 0x120000,0x121FFF,MRA_WRAM},
    { 0x122000,0x127FFF,snes_io_r},
    { 0x128000,0x12FFFF,MRA_ROM},
    { 0x130000,0x131FFF,MRA_WRAM},
    { 0x132000,0x137FFF,snes_io_r},
    { 0x138000,0x13FFFF,MRA_ROM},
    { 0x140000,0x141FFF,MRA_WRAM},
    { 0x142000,0x147FFF,snes_io_r},
    { 0x148000,0x14FFFF,MRA_ROM},
    { 0x150000,0x151FFF,MRA_WRAM},
    { 0x152000,0x157FFF,snes_io_r},
    { 0x158000,0x15FFFF,MRA_ROM},
    { 0x160000,0x161FFF,MRA_WRAM},
    { 0x162000,0x167FFF,snes_io_r},
    { 0x168000,0x16FFFF,MRA_ROM},
    { 0x170000,0x171FFF,MRA_WRAM},
    { 0x172000,0x177FFF,snes_io_r},
    { 0x178000,0x17FFFF,MRA_ROM},
    { 0x180000,0x181FFF,MRA_WRAM},
    { 0x182000,0x187FFF,snes_io_r},
    { 0x188000,0x18FFFF,MRA_ROM},
    { 0x190000,0x191FFF,MRA_WRAM},
    { 0x192000,0x197FFF,snes_io_r},
    { 0x198000,0x19FFFF,MRA_ROM},
    { 0x1A0000,0x1A1FFF,MRA_WRAM},
    { 0x1A2000,0x1A7FFF,snes_io_r},
    { 0x1A8000,0x1AFFFF,MRA_ROM},
    { 0x1B0000,0x1B1FFF,MRA_WRAM},
    { 0x1B2000,0x1B7FFF,snes_io_r},
    { 0x1B8000,0x1BFFFF,MRA_ROM},
    { 0x1C0000,0x1C1FFF,MRA_WRAM},
    { 0x1C2000,0x1C7FFF,snes_io_r},
    { 0x1C8000,0x1CFFFF,MRA_ROM},
    { 0x1D0000,0x1D1FFF,MRA_WRAM},
    { 0x1D2000,0x1D7FFF,snes_io_r},
    { 0x1D8000,0x1DFFFF,MRA_ROM},
    { 0x1E0000,0x1E1FFF,MRA_WRAM},
    { 0x1E2000,0x1E7FFF,snes_io_r},
    { 0x1E8000,0x1EFFFF,MRA_ROM},
    { 0x1F0000,0x1F1FFF,MRA_WRAM},
    { 0x1F2000,0x1F7FFF,snes_io_r},
    { 0x1F8000,0x1FFFFF,MRA_ROM},*/
    { 0x700000,0x74FFFF,MRA_SRAM},
    { 0x7E0000,0x7FFFFF,MRA_WRAM},
MEMORY_END

static MEMORY_WRITE_START( snes_writemem )   {
     0x000000,0x001FFF,MWA_WRAM},
    { 0x002000,0x007FFF,snes_io_w},
    { 0x008000,0x00FFFF,MWA_NOP},
    { 0x010000,0x011FFF,MWA_WRAM},
    { 0x012000,0x017FFF,snes_io_w},
    { 0x018000,0x01FFFF,MWA_NOP},
    { 0x020000,0x021FFF,MWA_WRAM},
    { 0x022000,0x027FFF,snes_io_w},
    { 0x028000,0x02FFFF,MWA_NOP},
    { 0x030000,0x031FFF,MWA_WRAM},
    { 0x032000,0x037FFF,snes_io_w},
    { 0x038000,0x03FFFF,MWA_NOP},
    { 0x040000,0x041FFF,MWA_WRAM},
    { 0x042000,0x047FFF,snes_io_w},
    { 0x048000,0x04FFFF,MWA_NOP},
    { 0x050000,0x051FFF,MWA_WRAM},
    { 0x052000,0x057FFF,snes_io_w},
    { 0x058000,0x05FFFF,MWA_NOP},
    { 0x060000,0x061FFF,MWA_WRAM},
    { 0x062000,0x067FFF,snes_io_w},
    { 0x068000,0x06FFFF,MWA_NOP},
    { 0x070000,0x071FFF,MWA_WRAM},
    { 0x072000,0x077FFF,snes_io_w},
    { 0x078000,0x07FFFF,MWA_NOP},
    { 0x080000,0x081FFF,MWA_WRAM},
    { 0x082000,0x087FFF,snes_io_w},
    { 0x088000,0x08FFFF,MWA_NOP},
    { 0x090000,0x091FFF,MWA_WRAM},
    { 0x092000,0x097FFF,snes_io_w},
    { 0x098000,0x09FFFF,MWA_NOP},
    { 0x0A0000,0x0A1FFF,MWA_WRAM},
    { 0x0A2000,0x0A7FFF,snes_io_w},
    { 0x0A8000,0x0AFFFF,MWA_NOP},
    { 0x0B0000,0x0B1FFF,MWA_WRAM},
    { 0x0B2000,0x0B7FFF,snes_io_w},
    { 0x0B8000,0x0BFFFF,MWA_NOP},
    { 0x0C0000,0x0C1FFF,MWA_WRAM},
    { 0x0C2000,0x0C7FFF,snes_io_w},
    { 0x0C8000,0x0CFFFF,MWA_NOP},
    { 0x0D0000,0x0D1FFF,MWA_WRAM},
    { 0x0D2000,0x0D7FFF,snes_io_w},
    { 0x0D8000,0x0DFFFF,MWA_NOP},
    { 0x0E0000,0x0E1FFF,MWA_WRAM},
    { 0x0E2000,0x0E7FFF,snes_io_w},
    { 0x0E8000,0x0EFFFF,MWA_NOP},
    { 0x0F0000,0x0F1FFF,MWA_WRAM},
    { 0x0F2000,0x0F7FFF,snes_io_w},
    { 0x0F8000,0x0FFFFF,MWA_NOP},
/*  { 0x100000,0x101FFF,MWA_WRAM},
    { 0x102000,0x107FFF,snes_io_w},
    { 0x108000,0x10FFFF,MWA_NOP},
    { 0x110000,0x111FFF,MWA_WRAM},
    { 0x112000,0x117FFF,snes_io_w},
    { 0x118000,0x11FFFF,MWA_NOP},
    { 0x120000,0x121FFF,MWA_WRAM},
    { 0x122000,0x127FFF,snes_io_w},
    { 0x128000,0x12FFFF,MWA_NOP},
    { 0x130000,0x131FFF,MWA_WRAM},
    { 0x132000,0x137FFF,snes_io_w},
    { 0x138000,0x13FFFF,MWA_NOP},
    { 0x140000,0x141FFF,MWA_WRAM},
    { 0x142000,0x147FFF,snes_io_w},
    { 0x148000,0x14FFFF,MWA_NOP},
    { 0x150000,0x151FFF,MWA_WRAM},
    { 0x152000,0x157FFF,snes_io_w},
    { 0x158000,0x15FFFF,MWA_NOP},
    { 0x160000,0x161FFF,MWA_WRAM},
    { 0x162000,0x167FFF,snes_io_w},
    { 0x168000,0x16FFFF,MWA_NOP},
    { 0x170000,0x171FFF,MWA_WRAM},
    { 0x172000,0x177FFF,snes_io_w},
    { 0x178000,0x17FFFF,MWA_NOP},
    { 0x180000,0x181FFF,MWA_WRAM},
    { 0x182000,0x187FFF,snes_io_w},
    { 0x188000,0x18FFFF,MWA_NOP},
    { 0x190000,0x191FFF,MWA_WRAM},
    { 0x192000,0x197FFF,snes_io_w},
    { 0x198000,0x19FFFF,MWA_NOP},
    { 0x1A0000,0x1A1FFF,MWA_WRAM},
    { 0x1A2000,0x1A7FFF,snes_io_w},
    { 0x1A8000,0x1AFFFF,MWA_NOP},
    { 0x1B0000,0x1B1FFF,MWA_WRAM},
    { 0x1B2000,0x1B7FFF,snes_io_w},
    { 0x1B8000,0x1BFFFF,MWA_NOP},
    { 0x1C0000,0x1C1FFF,MWA_WRAM},
    { 0x1C2000,0x1C7FFF,snes_io_w},
    { 0x1C8000,0x1CFFFF,MWA_NOP},
    { 0x1D0000,0x1D1FFF,MWA_WRAM},
    { 0x1D2000,0x1D7FFF,snes_io_w},
    { 0x1D8000,0x1DFFFF,MWA_NOP},
    { 0x1E0000,0x1E1FFF,MWA_WRAM},
    { 0x1E2000,0x1E7FFF,snes_io_w},
    { 0x1E8000,0x1EFFFF,MWA_NOP},
    { 0x1F0000,0x1F1FFF,MWA_WRAM},
    { 0x1F2000,0x1F7FFF,snes_io_w},
    { 0x1F8000,0x1FFFFF,MWA_NOP},*/
    { 0x700000,0x74FFFF,MWA_SRAM},
    { 0x7E0000,0x7FFFFF,MWA_WRAM},
MEMORY_END

static PALETTE_INIT( snes )
{
    int i;

    for ( i = 0; i < 65536; i++ )
    {
        int r, g, b;

        r = (i & 0x1F) << 3;
        g = ((i >> 5) & 0x1F) << 3;
        b = ((i >> 10) & 0x1F) << 3;

		palette_set_color(i, r, g, b);
        colortable[i] = i;
    }
}


static MACHINE_DRIVER_START( snes )
	/* basic machine hardware */
	MDRV_CPU_ADD(G65816, 2680000)					/* 2.68 Mhz */
	MDRV_CPU_MEMORY(snes_readmem,snes_writemem)
	MDRV_CPU_VBLANK_INT(snes_line_interrupt,262)	/* 262 scanlines */

#ifdef EMULATE_SPC700
	MDRV_CPU_ADD(SPC700, 2048000)					/* 2.048Mhz */
	MDRV_CPU_MEMORY(spc_readmem,spc_writemem)
	MDRV_CPU_VBLANK_INT(spc700Interrupt,262*10)		/* Interrupts should never be called. - Done for better sync */
#endif

	MDRV_FRAMES_PER_SECOND(50)	/* frames per second, vblank duration - Emulating a PAL machine at present*/
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( snes )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8+32, 32*8+32)
	MDRV_VISIBLE_AREA(16, 16+32*8-1, 16, 16+32*8-1)
	MDRV_GFXDECODE( snes_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(65536)
	MDRV_COLORTABLE_LENGTH(65536)
	MDRV_PALETTE_INIT( snes )

	MDRV_VIDEO_START( snes )
	MDRV_VIDEO_UPDATE( snes )

	/* sound hardware */
#ifdef EMULATE_SPC700
	/* SNES has its own sample format and since its a multiple rom machine can't use SOUND_SAMPLES */
	MDRV_SOUND_ADD(CUSTOM, snesSoundInterface)
#endif
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(snes)
    ROM_REGION(0x408000,REGION_CPU1,0)            // SNES RAM/ROM
#ifdef EMULATE_SPC700
    ROM_REGION(0x10000,REGION_CPU2,0)             // SPC RAM/ROM
    ROM_LOAD ("spc700.rom", 0xFFC0, 0x0040, 0x38000B6B)
#endif
ROM_END

static const struct IODevice io_snes[] =
{
    {
        IO_CARTSLOT,        /* type */
        1,                  /* count */
        "smc\0",            /* file extensions */
        IO_RESET_ALL,       /* reset if file changed */
        0,
        snes_load_rom,      /* init */
        snes_exit_rom,      /* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE         INPUT     INIT          COMPANY                           FULLNAME */
CONSX( 1989, snes,    0,        snes,          snes,    0,            "Nintendo",    "Super Nintendo Entertainment System" ,GAME_NOT_WORKING|GAME_NO_SOUND)
