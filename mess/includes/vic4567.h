#ifndef __VIC4567_H_
#define __VIC4567_H_

#endif

// private area
#ifdef VIC3_MASK
/* idea: 
   with const mask compiler should be able to discard several code
   misty compiler, even with -O2 nothing, but with the help of the 
   preprocessor
   with the macro I can generate a lot of desired functions */

#if 1||VIC3_MASK&1
colors[0]=c64_memory[VIC3_ADDR(0)+offset];
#endif
#if VIC3_MASK&2
colors[1]=c64_memory[VIC3_ADDR(1)+offset]<<1;
#endif
#if 1||VIC3_MASK&4
colors[2]=c64_memory[VIC3_ADDR(2)+offset]<<2;
#endif
#if VIC3_MASK&8
colors[3]=c64_memory[VIC3_ADDR(3)+offset]<<3;
#endif
#if VIC3_MASK&0x10
colors[4]=c64_memory[VIC3_ADDR(4)+offset]<<4;
#endif
#if VIC3_MASK&0x20
colors[5]=c64_memory[VIC3_ADDR(5)+offset]<<5;
#endif
#if VIC3_MASK&0x40
colors[6]=c64_memory[VIC3_ADDR(6)+offset]<<6;
#endif
#if VIC3_MASK&0x80
colors[7]=c64_memory[VIC3_ADDR(7)+offset]<<7;
#endif
		for (i=7;i>=0;i--) {\
			vic2.bitmap->line[YPOS+y][XPOS+x+i]=
				Machine->pens[
#if 1||VIC3_MASK&1
					(colors[0]&1)
#endif
#if VIC3_MASK&2
#if (VIC3_MASK&3)!=2
					|
#endif
					(colors[1]&2)
#endif
#if 1||VIC3_MASK&4
#if 1||(VIC3_MASK&7)!=4
					|
#endif
					(colors[2]&4)
#endif
#if VIC3_MASK&8
#if (VIC3_MASK&0xf)!=8
					|
#endif
					(colors[3]&8)
#endif
#if VIC3_MASK&0x10
#if (VIC3_MASK&0x1f)!=0x10
					|
#endif
					(colors[4]&0x10)
#endif
#if VIC3_MASK&0x20
#if (VIC3_MASK&0x3f)!=0x20
					|
#endif
					(colors[5]&0x20)
#endif
#if VIC3_MASK&0x40
#if (VIC3_MASK&0x7f)!=0x40
					|
#endif
					(colors[6]&0x40)
#endif
#if VIC3_MASK&0x80
#if (VIC3_MASK&0xff)!=0x80
					|
#endif
					(colors[7]&0x80)
#endif
					];
#if 1||VIC3_MASK&1
			colors[0]>>=1;
#endif
#if VIC3_MASK&2
			colors[1]>>=1;
#endif
#if 1||VIC3_MASK&4
			colors[2]>>=1;
#endif
#if VIC3_MASK&8
			colors[3]>>=1;
#endif
#if VIC3_MASK&0x10
			colors[4]>>=1;
#endif
#if VIC3_MASK&0x20
			colors[5]>>=1;
#endif
#if VIC3_MASK&0x40
			colors[6]>>=1;
#endif
#if VIC3_MASK&0x80
			colors[7]>>=1;
#endif
		}
#endif
