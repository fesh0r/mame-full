# before compiling for the first time, use "make makedir" to create all the
# necessary subdirectories

CC = gcc
LD = gcc

# add -DMAME_DEBUG to include the debugger
DEFS   = -DX86_ASM -DLSB_FIRST -DSIGNED_SAMPLES
#DEFS   = -DX86_ASM -DLSB_FIRST -DSIGNED_SAMPLES -DMAME_DEBUG
CFLAGS = -Isrc -Isrc/msdos -fomit-frame-pointer -fstrength-reduce -funroll-loops -O3 -mpentium -Werror -Wall \
	-W -Wno-sign-compare -Wno-unused \
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
	-Wstrict-prototypes
#	-pedantic \
#	-Wredundant-decls \
#	-Wlarger-than-27648 \
#	-Wshadow \
#	-Wcast-qual \
#	-Wwrite-strings \
#	-Wconversion \
#	-Wmissing-prototypes \
#	-Wmissing-declarations
#CFLAGS = -Isrc -Isrc/msdos -O -mpentium -Wall -Werror -g
LDFLAGS = -s
#LDFLAGS =
LIBS   = -lalleg $(DJDIR)/lib/audiodjf.a
OBJS   = obj/mame.o obj/common.o obj/usrintrf.o obj/driver.o \
         obj/cpuintrf.o obj/memory.o obj/timer.o obj/palette.o obj/gfxlayer.o \
         obj/inptport.o obj/cheat.o obj/unzip.o obj/inflate.o \
         obj/sndhrdw/adpcm.o \
         obj/sndhrdw/ym2203.opm obj/sndhrdw/psg.o obj/sndhrdw/psgintf.o \
         obj/sndhrdw/2151intf.o obj/sndhrdw/fm.o \
         obj/sndhrdw/ym2151.o obj/sndhrdw/ym3812.o \
		 obj/sndhrdw/tms5220.o obj/sndhrdw/5220intf.o obj/sndhrdw/vlm5030.o \
		 obj/sndhrdw/pokey.o obj/sndhrdw/pokyintf.o obj/sndhrdw/sn76496.o \
		 obj/sndhrdw/nes.o obj/sndhrdw/nesintf.o \
		 obj/sndhrdw/votrax.o obj/sndhrdw/dac.o obj/sndhrdw/samples.o \
         obj/vidhrdw/generic.o obj/sndhrdw/generic.o \
         obj/sndhrdw/namco.o \
		 obj/machine/wd179x.o \
		 obj/vidhrdw/TMS9928A.o \
		 obj/vidhrdw/M6845.o \
		 obj/drivers/nes.o obj/machine/nes.o obj/machine/nes_mmc.o obj/vidhrdw/nes.o \
		 obj/drivers/genesis.o obj/machine/genesis.o obj/sndhrdw/genesis.o obj/vidhrdw/genesis.o \
		 obj/drivers/coleco.o obj/machine/coleco.o obj/vidhrdw/coleco.o \
		 obj/drivers/cgenie.o obj/machine/cgenie.o obj/sndhrdw/cgenie.o \
		 obj/drivers/trs80.o obj/machine/trs80.o obj/sndhrdw/trs80.o obj/vidhrdw/trs80.o \
         obj/Z80/Z80.o obj/M6502/M6502.o obj/I86/I86.o obj/I8039/I8039.o \
		 obj/M6809/m6809.o obj/M6808/m6808.o obj/M6805/m6805.o \
         obj/M68000/opcode0.o obj/M68000/opcode1.o obj/M68000/opcode2.o obj/M68000/opcode3.o obj/M68000/opcode4.o obj/M68000/opcode5.o \
         obj/M68000/opcode6.o obj/M68000/opcode7.o obj/M68000/opcode8.o obj/M68000/opcode9.o obj/M68000/opcodeb.o \
         obj/M68000/opcodec.o obj/M68000/opcoded.o obj/M68000/opcodee.o obj/M68000/mc68kmem.o \
         obj/M68000/cpufunc.o \
         obj/mamedbg.o obj/asg.o obj/M6502/6502dasm.o \
         obj/M6809/6809dasm.o obj/M6808/6808dasm.o obj/M6805/6805dasm.o \
         obj/M68000/m68kdasm.o \
         obj/msdos/msdos.o obj/msdos/video.o obj/msdos/sound.o \
         obj/msdos/input.o obj/msdos/fileio.o obj/msdos/config.o obj/msdos/fronthlp.o

VPATH=src src/Z80 src/M6502 src/I86 src/M6809

all: mess.exe

mess.exe:  $(OBJS)
	$(LD) $(LDFLAGS) -o mess.exe $(OBJS) $(LIBS)

obj/%.o: src/%.c mame.h driver.h
	 $(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

# dependencies
obj/Z80/Z80.o:  Z80.c Z80.h Z80Codes.h Z80IO.h Z80DAA.h
obj/M6502/M6502.o:	M6502.c M6502.h Tables.h Codes.h
obj/I86/I86.o:  I86.c I86.h I86intrf.h ea.h host.h instr.h modrm.h
obj/M6809/m6809.o:  m6809.c m6809.h 6809ops.c
obj/M6808/M6808.o:  m6808.c m6808.h


makedir:
#        mkdir obj
	mkdir obj\Z80
	mkdir obj\M6502
	mkdir obj\I86
	mkdir obj\I8039
	mkdir obj\M6809
	mkdir obj\M6808
	mkdir obj\M6805
	mkdir obj\M68000
	mkdir obj\drivers
	mkdir obj\machine
	mkdir obj\vidhrdw
#        mkdir obj\sndhrdw
	mkdir obj\msdos

clean:
	del obj\*.o
	del obj\Z80\*.o
	del obj\M6502\*.o
	del obj\I86\*.o
	del obj\I8039\*.o
	del obj\M6809\*.o
	del obj\M6808\*.o
	del obj\M6805\*.o
	del obj\M68000\*.o
	del obj\drivers\*.o
	del obj\machine\*.o
	del obj\vidhrdw\*.o
	del obj\sndhrdw\*.o
	del obj\msdos\*.o
	del mess.exe

cleandebug:
	del obj\*.o
	del obj\Z80\*.o
	del obj\M6502\*.o
	del obj\M6809\*.o
	del obj\M6808\*.o
	del obj\M6805\*.o
	del obj\M68000\*.o
	del mess.exe
