/* paint & puzzle */
/* vidbank is a guess */
/* readme says it might be cga? */
/* what it writes to registers depends on what it reads from them ... */
/* seems weird.. */
/* i don't really understand it */


/*

Some PCB information (not from the pcb of the set we have but its probably the same)

Main PCB
Reb B
Label:
Board Num.
90

The PCB has no silkscreen or reference designators, so the numbers I am
providing are made up.

U1 32 pin IC
27C010A
Label:
Paint N Puzzle
Ver. 1.09
Odd

U2 5 pin audio amp
LM383T

U3 40 pin IC
8926S
UM6522A

U4 28 pin IC
Mosel
MS62256L-85PC
8911 5D

U5 18 pin IC
PIC16C54-HS/P
9344 CGA

U6
P8798
L3372718E
Intel
Label:
MicroTouch
5603670 REV 1.0

U7 28 pin IC
MicroTouch Systems
c 1992
19-507 Rev 2
ICS1578N 9334

U8 28 pin IC
Mosel
MS62256L-85PC
8911 5D

U9 32 pin IC
27C010A
Label:
Paint N Puzzle
Ver. 1.09
Even

U10 64 pin IC
MC68000P12
OB26M8829

X1
16.000MHz -connected to U5

X2
ECS-120
32-1
China -connected to U7
Other side is unreadable

CN1 JAMMA
CN2 ISA? Video card slot
CN3 Touchscreen input (12 pins)
CN4 2 pins, unused

1 blue potentiometer connected to audio amp
There doesnt seem to be any dedicated sound chip, and sounds are just bleeps
really.

-----------------------------------------------
Video card (has proper silk screen and designators)
JAX-8327A

X1
40.000MHz

J1 -open
J2 -closed
J3 -open

U1/2 unpopulated but have sockets

U3 20 pin IC
KM44C256BT-8
22BY
Korea

U4 20 pin IC
KM44C256BT-8
22BY
Korea

U5 160 pin QFP
Trident
TVGA9000i
34'3 Japan

U6 28 pin IC
Quadtel
TVGA9000i BIOS Software
c 1993 Ver D3.51 LH

CN1 standard DB15 VGA connector (15KHz)


*/

#include "driver.h"
#include "machine/random.h"

data16_t* pntnpuzl_3a0000ram;
data16_t* pntnpuzl_bank;
/* vid */
VIDEO_START(pntnpuzl)
{
	pntnpuzl_3a0000ram=auto_malloc(0x100000);

	return 0;
}

VIDEO_UPDATE(pntnpuzl)
{
	int x,y;
	int count;
	static int xxx=0x18f;
	static int yyy=512;
	static int sss=0xa8;
/*
	if ( code_pressed_memory(KEYCODE_Q) )
	{
		xxx--;
		printf("xxx %04x\n",xxx);
	}

	if ( code_pressed_memory(KEYCODE_W) )
	{
		xxx++;
		printf("xxx %04x\n",xxx);
	}

	if ( code_pressed_memory(KEYCODE_A) )
	{
		yyy--;
		printf("yyy %04x\n",yyy);
	}

	if ( code_pressed_memory(KEYCODE_S) )
	{
		yyy++;
		printf("yyy %04x\n",yyy);
	}

	if ( code_pressed_memory(KEYCODE_Z) )
	{
		sss--;
		printf("sss %04x\n",sss);
	}

	if ( code_pressed_memory(KEYCODE_X) )
	{
		sss++;
		printf("sss %04x\n",sss);
	}
*/


	count=sss;

	for(y=0;y<yyy;y++)
	{
		for(x=0;x<xxx;x+=2)
		{
			plot_pixel(bitmap, x,y, (pntnpuzl_3a0000ram[count]&0xff00)>>8);
			plot_pixel(bitmap, x+1,y, (pntnpuzl_3a0000ram[count]&0x00ff)>>0);
			count++;
		}
	}
}

READ16_HANDLER ( pntnpuzl_random_r )
{
	return mame_rand();
}

WRITE16_HANDLER( pntnpuzl_vid_write )
{

	logerror("write_to_videoram: pc = %06x : offset %04x data %04x reg %04x\n",activecpu_get_pc(),offset*2, data, pntnpuzl_bank[0]);
	COMBINE_DATA(&pntnpuzl_3a0000ram[offset+ (pntnpuzl_bank[0]&0x0001)*0x8000 ]);

}

READ16_HANDLER( pntnpuzl_0x3c03da_r )
{
	static int pntpuz_tog=0;
	pntpuz_tog^=0xffff;
	return pntpuz_tog;
}

static ADDRESS_MAP_START( pntnpuzl_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x080000, 0x080001) AM_READ(pntnpuzl_random_r)
	AM_RANGE(0x100000, 0x100001) AM_READ(pntnpuzl_random_r)
	AM_RANGE(0x180000, 0x180001) AM_READ(pntnpuzl_random_r)
	AM_RANGE(0x200000, 0x200001) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x280000, 0x280001) AM_READ(pntnpuzl_random_r)
	AM_RANGE(0x280008, 0x280009) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x28000a, 0x28000b) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x280010, 0x280011) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x280012, 0x280013) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x280014, 0x280015) AM_READ(pntnpuzl_random_r)
	AM_RANGE(0x280016, 0x280017) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x280018, 0x280019) AM_WRITE(MWA16_NOP)
	AM_RANGE(0x28001a, 0x28001b) AM_READ(pntnpuzl_random_r)
	AM_RANGE(0x3a0000, 0x3affff) AM_WRITE(pntnpuzl_vid_write)

	AM_RANGE(0x3C03C4, 0x3C03C5) AM_RAM AM_BASE(&pntnpuzl_bank)//??
	AM_RANGE(0x3c03da, 0x3c03db) AM_READ(pntnpuzl_0x3c03da_r)
	AM_RANGE(0x400000, 0x407fff) AM_RAM
ADDRESS_MAP_END

INPUT_PORTS_START( pntnpuzl )
INPUT_PORTS_END

static MACHINE_DRIVER_START( pntnpuzl )
	MDRV_CPU_ADD_TAG("main", M68000, 12000000)//??
	MDRV_CPU_PROGRAM_MAP(pntnpuzl_map,0)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)//others might be used too

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 50*8-1, 0*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(pntnpuzl)
	MDRV_VIDEO_UPDATE(pntnpuzl)
MACHINE_DRIVER_END

ROM_START( pntnpuzl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "pntnpuzl.u2", 0x00001, 0x40000, CRC(dfda3f73) SHA1(cca8ccdd501a26cba07365b1238d7b434559bbc6) )
	ROM_LOAD16_BYTE( "pntnpuzl.u3", 0x00000, 0x40000, CRC(4173f250) SHA1(516fe6f91b925f71c36b97532608b82e63bda436) )
ROM_END

GAMEX( 199?, pntnpuzl,    0, pntnpuzl,    pntnpuzl,    0, ROT90,  "Century?", "Paint & Puzzle",GAME_NO_SOUND|GAME_NOT_WORKING )
