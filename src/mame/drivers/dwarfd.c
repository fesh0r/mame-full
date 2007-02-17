/*
  Dwarfs Den by Electro-Sport
  http://www.arcadeflyers.net/?page=flyerdb&subpage=thumbs&id=3993

  driver by
   David Haywood
   Tomasz Slanina

 TODO:

 - finish 8275 CRT emulation
 - fix gfx glitches
 - correct colors
 - DIPs
 - layout(lamps)
 - NVRAM


DD hardware is weird and complicated. There's no dedicated DMA controller
for CRT.  When serial output pin of cpu is in high state, all reads from
work RAM are redirected to CRT:

005B: 3E C0      mvi  a,$c0
005D: 30         sim
005E: E1         pop  h ; 40 times "pop h" = transfer 80 bytes (line) from ram to crt fifo
...
0087: 0F         rrc
0088: 30         sim

$4c00-$4fff part of work RAM is adressed in different way than other blocks. No idea why
(this part of the pcb is a bit damaged).
Gfx decode is strange - may be wrong.
Gfx rom bank (6d/6b or 6a/6c pair) is switched by timer circuit - few 74161 counters connected to xtal.
Current implementations is just a guess, and doesn't work on test screen.


                                               ______________
  _____________________________________________||||||||||||||_______________________________________
  |         1         2         3         4         5         6         7         8         9      |
  |                                                                                      ________  |
  ||| L                                                   SN7445N74  74LS224N  7404-PC   |9L    |  |
D |||                                                                                    |______|  |
  |||                                                                   _____________    ________  |
  ||| K                         BATTERY         16-1-471  7400-PC       | M5L8085AP |    |9K    |  |
  |                                                                     |___________|    |______|  |
  |                                                                                      ________  |
  ||| J           74LS273NA                     --------  --------             74LS373N  |9J    |  |
C |||                                                                                    |______|  |
  |||                                                                                    ________  |
  ||| H           74LS273NA  SN7442AN  74107N   74161N    SN7432N    M3-7602-5 MDP1603   |9H    |  |
  |                                                                                      |______|  |
  |                                                                                                |
  ||| F           74LS273NA  7408N     7486N    OSC       7404                 MM2114N   MM2114N   |
P |||                                                                                              |
  |||                                  _____________                                               |
  ||| E                      SN7442AN  |iP8275     |      7400       74LS244N  MM2114N   MM2114N   |
  |                                    |___________|                                               |
  |                                    _____________      ________                                 |
  ||| D 16-1-471  MDP1603    7414      |AY-3-8910  |      |6D    |   74LS245   MM2114N   MM2114N   |
B |||                                  |___________|      |______|                                 |
  |||                                                     ________   _______                       |
  ||| C 16-1-471  MDP1603    7414      7414     7400      |6C    |   |7C   |   M3-6514-9 M3-6514-9 |
  |                                                       |______|                                 |
  |                                                       ________   __________________            |
  ||| B SN74175N             74174N    74LS374N 74LS374N  |6B    |   |                |  6-1-471   |
A |||                                                     |______|   |                |            |
  |||                        _______                      ________   |    ______      |            |
  ||| A DN74505N             |3A   |   74153N   74153N    |6A    |   |    LM324N      |  DIP-SW    |
  |                                                       |______|   |                |            |
  |                                                                  |________________|            |
  |________________________________________________________________________________________________|

  OSC = 10.595 MHz
  D,C,A = 20-pin connectors
  B = 26-pin connector
  P = 12-pin connector (seems power)
  Edge connector (top) is not JAMMA
  3A (63S080N) dumped as 82S123
  7C = non populated

*/

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "sound/ay8910.h"

//should be taken from crt params
static int maxy=25;
static int maxx=80;
static int bank=0;

static UINT8 *videobuf;
static UINT8 *dwarfd_ram;

static int line=0;
static int idx=0;
static int crt_access=0;


#define TOPLINE 7
#define BOTTOMLINE 18


#define CHARACTERS_UNDEFINED -1

enum
{
	I8275_COMMAND_RESET=0,
	I8275_COMMAND_START,
	I8275_COMMAND_STOP,
	I8275_COMMAND_READ_LIGHT_PEN,
	I8275_COMMAND_LOAD_CURSOR,
	I8275_COMMAND_EI,
	I8275_COMMAND_DI,
	I8275_COMMAND_PRESET,

	NUM_I8275_COMMANDS
};

enum
{
	I8275_COMMAND_RESET_LENGTH=4,
	I8275_COMMAND_START_LENGTH=0,
	I8275_COMMAND_STOP_LENGTH=0,
	I8275_COMMAND_EI_LENGTH=0,
	I8275_COMMAND_DI_LENGTH=0,
	I8275_COMMAND_READ_LIGHT_PEN_LENGTH=2,
	I8275_COMMAND_LOAD_CURSOR_LENGTH=2,
	I8275_COMMAND_PRESET_LENGTH=0
};
int i8275Command;
int i8275HorizontalCharactersRow;
int i8275CommandSeqCnt;
int i8275SpacedRows;
int i8275VerticalRows;
int i8275VerticalRetraceRows;
int i8275Underline;
int i8275Lines;
int i8275LineCounterMode;
int i8275FieldAttributeMode;
int i8275CursorFormat;
int i8275HorizontalRetrace;

WRITE8_HANDLER (i8275_preg_w) //param reg
{
	switch(i8275Command)
	{
		case I8275_COMMAND_RESET:
		{
			switch(i8275CommandSeqCnt)
			{
				case 4:
				{
					//screen byte comp byte 1
					i8275SpacedRows=data>>7;
					i8275HorizontalCharactersRow=(data&0x7f)+1;
					if(i8275HorizontalCharactersRow>80)
					{
						logerror("i8275 Undefined num of characters/Row! = %d\n", i8275HorizontalCharactersRow);
						i8275HorizontalCharactersRow=CHARACTERS_UNDEFINED;
					}
					else
					{
						logerror("i8275 %d characters/row\n", i8275HorizontalCharactersRow);
					}
					if(i8275SpacedRows&1)
					{
						logerror("i8275 spaced rows\n");
					}
					else
					{
						logerror("i8275 normal rows\n");
					}
					i8275CommandSeqCnt--;
				}
				break;

				case 3:
				{
					//screen byte comp byte 2
					i8275VerticalRows=(data&0x3f)+1;
					i8275VerticalRetraceRows=(data>>6)+1;

					logerror("i8275 %d rows\n", i8275VerticalRows);
					logerror("i8275 %d vertical retrace rows\n", i8275VerticalRetraceRows);

					i8275CommandSeqCnt--;
				}
				break;

				case 2:
				{
					//screen byte comp byte 3
					i8275Underline=(data>>4)+1;
					i8275Lines=(data&0xf)+1;
					logerror("i8275 underline placement: %d\n", i8275Underline);
					logerror("i8275 %d lines/row\n", i8275Lines);

					i8275CommandSeqCnt--;
				}
				break;

				case 1:
				{
					//screen byte comp byte 4
					i8275LineCounterMode=data>>7;
					i8275FieldAttributeMode=(data>>6)&1;
					i8275CursorFormat=(data>>4)&3;
					i8275HorizontalRetrace=((data&0xf)+1)<<1;
					logerror("i8275 line counter mode: %d\n", i8275LineCounterMode);
					if(i8275FieldAttributeMode)
					{
						logerror("i8275 field attribute mode non-transparent\n");
					}
					else
					{
						logerror("i8275 field attribute mode transparent\n");
					}

					switch(i8275CursorFormat)
					{
						case 0:	{logerror("i8275 cursor format - blinking reverse video block\n");}	break;
						case 1:	{logerror("i8275 cursor format - blinking underline\n");}break;
						case 2:	{logerror("i8275 cursor format - nonblinking reverse video block\n");}break;
						case 3:	{logerror("i8275 cursor format - nonblinking underline\n");}break;
					}

					logerror("i8275 %d chars for horizontal retrace\n",i8275HorizontalRetrace );
					i8275CommandSeqCnt--;
				}
				break;

				default:
				{
					logerror("i8275 illegal\n");
				}

			}
		}
		break;

		case I8275_COMMAND_START:
		{

		}
		break;

		case I8275_COMMAND_STOP:
		{

		}
		break;

	}

}

READ8_HANDLER (i8275_preg_r) //param reg
{


	return 0;
}

WRITE8_HANDLER (i8275_creg_w) //comand reg
{
	switch(data>>5)
	{
		case 0:
		{
			/* reset */
			i8275Command=I8275_COMMAND_RESET;
			i8275CommandSeqCnt=I8275_COMMAND_RESET_LENGTH;
		}
		break;

		case 5:
		{
			/* enable interrupt */
			i8275Command=I8275_COMMAND_EI;
			i8275CommandSeqCnt=I8275_COMMAND_EI_LENGTH;
		}
		break;

		case 6:
		{
			/* disable interrupt */
			i8275Command=I8275_COMMAND_DI;
			i8275CommandSeqCnt=I8275_COMMAND_DI_LENGTH;
		}
		break;

		case 7:
		{
			/* preset counters */
			i8275CommandSeqCnt=I8275_COMMAND_PRESET_LENGTH;

		}
		break;
	}
}

READ8_HANDLER (i8275_sreg_r) //status
{
	return 0;
}

static READ8_HANDLER(dwarfd_ram_r)
{
	if(crt_access==0)
	{
		return dwarfd_ram[offset];
	}
	else
	{
		videobuf[line*256+idx]=dwarfd_ram[offset];
		idx++;
		return dwarfd_ram[offset];
	}
}

static WRITE8_HANDLER(dwarfd_ram_w)
{
	dwarfd_ram[offset]=data;
}

static WRITE8_HANDLER(output1_w)
{
/*
 bits:
     0 - pp lamp
     1 - start lamp
     2 - ?
     3 - ?
     4 - unzap lamp
     5 - z1 lamp
     6 - z2 lamp
     7 - z3 lamp
*/
}

static WRITE8_HANDLER(output2_w)
{
/*
 bits:
     0 - z4
     1 - z5
     2 - coin counter?
     3 - ?
     4 - ?
     5 - ?
     6 - ?
     7 - ?
*/
}




static ADDRESS_MAP_START( mem_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x4fff) AM_READWRITE(dwarfd_ram_r, dwarfd_ram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x01) AM_READ(AY8910_read_port_0_r)
	AM_RANGE(0x02, 0x02) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(AY8910_control_port_0_w)

	AM_RANGE(0x20, 0x20) AM_READWRITE(i8275_preg_r, i8275_preg_w)
	AM_RANGE(0x21, 0x21) AM_READWRITE(i8275_sreg_r, i8275_creg_w)
	AM_RANGE(0x40, 0x40) AM_WRITENOP // unknown
	AM_RANGE(0x60, 0x60) AM_WRITE(output1_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(output2_w)
	AM_RANGE(0xc0, 0xc0) AM_READ(input_port_0_r)
	AM_RANGE(0xc1, 0xc1) AM_READ(input_port_1_r)
ADDRESS_MAP_END


INPUT_PORTS_START( dwarfd )
	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Zap 1") PORT_CODE(KEYCODE_Z) //z1 zap 1
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Zap 2") PORT_CODE(KEYCODE_X) //z2 zap 2
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Zap 3") PORT_CODE(KEYCODE_C) //z3 zap 3
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Zap 4") PORT_CODE(KEYCODE_V) //z4 zap 4
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Zap 5") PORT_CODE(KEYCODE_B) //z5 zap 5

	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Bet x1") PORT_CODE(KEYCODE_S) //x1 bet x1
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Bet x2") PORT_CODE(KEYCODE_D) //x2 bet x2

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("PP ?") PORT_CODE(KEYCODE_A) //pp
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Replace") PORT_CODE(KEYCODE_F) //rp replace
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Take") PORT_CODE(KEYCODE_G) //tk take
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE ) PORT_NAME("Unzap") PORT_CODE(KEYCODE_N) //uz unzap

INPUT_PORTS_END




VIDEO_START(dwarfd)
{
	return 0;
}

void drawCrt(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int x,y;
	for (y=0;y<maxy;y++)
	{
		int count=y*256;
		int bank2=4;

		if(y<TOPLINE || y>BOTTOMLINE)
		{
				bank2=0;
		}
		for (x=0;x<maxx;x++)
		{
			int tile=0;

			int b=0; //end marker
			while(b==0)
			{
				if(count<0x8000)
				{
					tile = videobuf[count++];
				}
				else
				{
						return;
				}
				if(tile&0x80)
				{
					if((tile&0xfc)==0xf0)
					{
						switch(tile&3)
						{
							case 0:
							case 1: break;

							case 2:
							case 3: return;
						}
					}
					if((tile&0xc0)==0x80)
					{
						bank=(tile>>2)&3;
					}
					if((tile&0xc0) ==0xc0)
					{
						b=1;
						tile=rand()&0x7f;//(tile>>2)&0xf;
					}
				}
				else
				{
					b=1;
				}
			}
			drawgfx(bitmap,Machine->gfx[0],
				tile+(bank+bank2)*128,
				0,
				0, 0,
				x*8,y*8,
				cliprect,TRANSPARENCY_PEN,0);
		}
	}
}


VIDEO_UPDATE( dwarfd )
{
	fillbitmap(bitmap, get_black_pen(machine), cliprect);
	drawCrt(bitmap,cliprect);
	return 0;
}

static void dwarfd_sod_callback(int nSO)
{
	crt_access=nSO;
}

#define NUM_LINES 25
INTERRUPT_GEN( dwarfd_interrupt )
{
	if(cpu_getiloops() < NUM_LINES)
	{
		cpunum_set_info_fct(0, CPUINFO_PTR_I8085_SOD_CALLBACK, (void*)dwarfd_sod_callback);
		cpunum_set_input_line(0,I8085_RST65_LINE,HOLD_LINE); // 34 - every 8th line
		line=cpu_getiloops();
		idx=0;
	}
	else
	{
		if(cpu_getiloops() == NUM_LINES)
		{
			cpunum_set_input_line(0,I8085_RST55_LINE,HOLD_LINE);//2c - generated by  crt - end of frame
		}
	}
}

#if 0
static const gfx_layout tiles8x8_layout =
{
	4,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3},
	{8,0,24,16 },
	//{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*32, 6*32, 5*32, 4*32, 3*32, 2*32, 1*32, 0*32 },
	8*32
};
#endif

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3},
	{8,12,0,4,24,28,16,20 },
//  {12,8,4,0,28,24,20,16 },
	//{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*32, 6*32, 5*32, 4*32, 3*32, 2*32, 1*32, 0*32 },
	8*32
};


static const gfx_layout tiles8x8_layout0 =
{
	4,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{8,0,24,16 },
	//{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*32, 6*32, 5*32, 4*32, 3*32, 2*32, 1*32, 0*32 },
	8*32
};

static const gfx_layout tiles8x8_layout1 =
{
	4,8,
	RGN_FRAC(1,1),
	1,
	{ 1 },
	{8,0,24,16 },
	//{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*32, 6*32, 5*32, 4*32, 3*32, 2*32, 1*32, 0*32 },
	8*32
};

static const gfx_layout tiles8x8_layout2 =
{
	4,8,
	RGN_FRAC(1,1),
	1,
	{ 2 },
	{8,0,24,16 },
	//{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*32, 6*32, 5*32, 4*32, 3*32, 2*32, 1*32, 0*32 },
	8*32
};

static const gfx_layout tiles8x8_layout3 =
{
	4,8,
	RGN_FRAC(1,1),
	1,
	{ 3 },
	{8,0,24,16 },
	//{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 7*32, 6*32, 5*32, 4*32, 3*32, 2*32, 1*32, 0*32 },
	8*32
};
/*
static const gfx_layout tiles8x8_layout =
{
    8,8,
    RGN_FRAC(1,1),
    2,
    { 1,1},
    {6,6,2,2,14,14,10,10 },
    //{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
    { 7*16, 6*16, 5*16, 4*16, 3*16, 2*16, 1*16, 0*16 },
    8*16
};
*/

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout0, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout1, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout2, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout3, 0, 16 },
	{ -1 }
};

PALETTE_INIT(dwarfd)
{
	int i;

	for (i = 0;i < 256;i++)
	{
		int r = rand()|0x80;
		int g = rand()|0x80;
		int b = rand()|0x80;
		if (i == 0) r = g = b = 0;

		palette_set_color(machine,i,r,g,b);
	}
	palette_set_color(machine, 8, 255, 255, 0);
	palette_set_color(machine, 12, 127, 127, 255);
	palette_set_color(machine, 4, 0, 255, 0);
	palette_set_color(machine, 6, 255, 0, 0);
}

static struct AY8910interface ay8910_interface =
{
	input_port_3_r,
	input_port_2_r,
};


static MACHINE_DRIVER_START( dwarfd )
	/* basic machine hardware */
	MDRV_CPU_ADD(8085A, 10595000/3)        /* ? MHz */

	MDRV_CPU_PROGRAM_MAP(mem_map, 0)
	MDRV_CPU_IO_MAP(io_map, 0)

	MDRV_CPU_VBLANK_INT(dwarfd_interrupt,NUM_LINES+4) //16 +vblank + 1 unused
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(272*2, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 272*2-1, 0, 200-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_PALETTE_INIT(dwarfd)

	MDRV_VIDEO_START(dwarfd)
	MDRV_VIDEO_UPDATE(dwarfd)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 1500000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( dwarfd )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "9l_pd_50-3196_m5l2732k.bin", 0x0000, 0x1000, CRC(34e942ae) SHA1(d4f0ee7f29e1c1a93b4b30b950023dbf60596100) )
	ROM_LOAD( "9k_pd_50-3193_hn462732g.bin",0x1000, 0x1000, CRC(78f0c260) SHA1(d6c3b8b3ef4ce99a811e291f1396a47106683df9) )
	ROM_LOAD( "9j_pd_50-3192_mbm2732.bin",  0x2000, 0x1000, CRC(9c66ee6e) SHA1(49c20fa276508b3c7b0134909295ae04ee46890f) )
	ROM_LOAD( "9h_pd_50-3375_2732.bin",     0x3000, 0x1000, CRC(daf5551d) SHA1(933e3453c9e74ca6695137c9f6b1abc1569ad019) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "6a_pd_50_1991_2732.bin"      ,0x0000, 0x1000, CRC(6da494bc) SHA1(0323eaa5f81e3b8561225ccdd4654c9a11f2167c) )
	ROM_LOAD16_BYTE( "6b_pd_50-1992_tms2732ajl.bin",0x2000, 0x1000, CRC(69208e1a) SHA1(8706f8f0d2dfeba5cebc71985ea46a67de13bc7d) )
	ROM_LOAD16_BYTE( "6c_pd_50-1993_tms2732ajl.bin",0x0001, 0x1000, CRC(cd8e5e54) SHA1(0961739d72d80e0ac00e6cbf9643bcebfe74830d) )
	ROM_LOAD16_BYTE( "6d_pd_50-1994_tms2732ajl.bin",0x2001, 0x1000, CRC(ef52b88c) SHA1(3405152da3194a71f6dac6492f275c746e781ee7) )

	ROM_REGION( 0x4000*2, REGION_GFX2, 0 )
	ROM_FILL(0,  0x4000*2, 0)

	ROM_REGION( 0x40, REGION_PROMS, 0 )
	/* ??? colors */
	ROM_LOAD( "3a_50-1381_63s080n.bin",0x00, 0x20, CRC(451d0a72) SHA1(9ff6e2c5bd2b57bd607cb33e60e7ed25bea164b3) )
	/* memory map */
	ROM_LOAD( "7h_7602.bin",0x20, 0x20, CRC(d5457333) SHA1(5872c868638c08faef7365d9c6e41dc3f070bd97) )
ROM_END

static DRIVER_INIT(dwarfd)
{
	int i;

	/* expand gfx roms */
	for (i=0;i<0x4000;i++)
	{
		UINT8 *src    = memory_region       ( REGION_GFX1 );
		UINT8 *dst    = memory_region       ( REGION_GFX2 );

		UINT8 dat;
		dat = (src[i]&0xf0)>>0;
		dst[i*2] = dat;

		dat = (src[i]&0x0f)<<4;
		dst[i*2+1] = dat;
	}

	/* use low bit as 'interpolation' bit */
	for (i=0;i<0x8000;i++)
	{
		UINT8 *src    = memory_region       ( REGION_GFX2 );
		if (src[i]&0x10)
		{
			src[i] = src[i]&0xe0;
	//      src[i] |= ((src[(i+1)&0x7fff]&0xe0) >> 4);

		}
		else
		{
			src[i] = src[i]&0xe0;
			src[i] |= (src[i] >> 4);

		}
	//      src[i] = src[i]&0xe0;
	}

	cpunum_set_info_fct(0, CPUINFO_PTR_I8085_SOD_CALLBACK, (void*)dwarfd_sod_callback);

	videobuf=auto_malloc(0x8000);
	dwarfd_ram=auto_malloc(0x1000);
	memset(videobuf,0,0x8000);
	memset(dwarfd_ram,0,0x1000);

}

GAME( 1981, dwarfd, 0, dwarfd, dwarfd, dwarfd, ORIENTATION_FLIP_Y, "Electro-Sport", "Dwarfs Den", GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS )
