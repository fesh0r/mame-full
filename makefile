# before compiling for the first time, use "make makedir" to create all the
# necessary subdirectories

CC = gcc
LD = gcc
ASM = nasm
#ASM = nasmw
ASMFLAGS = -f coff

# uncomment next line to use Assembler 6808 engine
#X86_ASM_6808 = 1
# uncomment next line to use Assembler 68k engine
#X86_ASM_68K = 1

# ASMDEFS = -dMAME_DEBUG
ASMDEFS =

ifdef X86_ASM_6808
M6808OBJS = obj/M6808/m6808.oa obj/M6808/6808dasm.o
else
M6808OBJS = obj/m6808/m6808.o obj/M6808/6808dasm.o
endif

ifdef X86_ASM_68K
M68KOBJS = obj/m68000/asmintf.o obj/m68000/68kem.oa
M68KDEF  = -DA68KEM
else
M68KOBJS = obj/M68000/opcode0.o obj/M68000/opcode1.o obj/M68000/opcode2.o obj/M68000/opcode3.o obj/M68000/opcode4.o obj/M68000/opcode5.o \
          obj/M68000/opcode6.o obj/M68000/opcode7.o obj/M68000/opcode8.o obj/M68000/opcode9.o obj/M68000/opcodeb.o \
          obj/M68000/opcodec.o obj/M68000/opcoded.o obj/M68000/opcodee.o obj/M68000/mc68kmem.o \
          obj/M68000/cpufunc.o
M68KDEF  =
endif

# add -DMAME_DEBUG to include the debugger
#DEFS	= -DX86_ASM -DLSB_FIRST -DSIGNED_SAMPLES -Dinline=__inline__ -Dasm=__asm__
#DEFS   = -DX86_ASM -DLSB_FIRST -DSIGNED_SAMPLES -Dinline=__inline__ -Dasm=__asm__ \
	-DBETA_VERSION
DEFS   = -DX86_ASM -DLSB_FIRST -DSIGNED_SAMPLES -Dinline=__inline__ -Dasm=__asm__ \
	-DMAME_DEBUG

CFLAGS = -Isrc -Isrc/msdos -fomit-frame-pointer -O3 -mpentium -Werror -Wall \
	-W -Wno-sign-compare -Wno-unused \
	-Wpointer-arith -Wbad-function-cast -Wcast-align -Waggregate-return \
	-pedantic \
	-Wshadow \
	-Wstrict-prototypes
#	-Wredundant-decls \
#	-Wlarger-than-27648 \
#	-Wcast-qual \
#	-Wwrite-strings \
#	-Wconversion \
#	-Wmissing-prototypes \
#	-Wmissing-declarations
#CFLAGS = -Isrc -Isrc/msdos -O -mpentium -Wall -Werror -g
LDFLAGS = -s
#LDFLAGS =
LIBS   = -lalleg $(DJDIR)/lib/audiodjf.a
OBJS   = obj/mess.o obj/audit.o obj/common.o obj/usrintrf.o obj/driver.o \
         obj/cpuintrf.o obj/memory.o obj/timer.o obj/palette.o obj/gfxlayer.o \
         obj/inptport.o obj/cheat.o obj/unzip.o obj/inflate.o \
         obj/sndhrdw/adpcm.o \
         obj/sndhrdw/ym2203.opm obj/sndhrdw/psg.o obj/sndhrdw/psgintf.o \
         obj/sndhrdw/2151intf.o obj/sndhrdw/fm.o \
         obj/sndhrdw/ym2151.o obj/sndhrdw/ym3812.o \
	 obj/sndhrdw/tms5220.o obj/sndhrdw/5220intf.o obj/sndhrdw/vlm5030.o \
	 obj/sndhrdw/pokey.o obj/sndhrdw/sn76496.o \
	 obj/sndhrdw/nes.o obj/sndhrdw/nesintf.o \
	 obj/sndhrdw/votrax.o obj/sndhrdw/dac.o obj/sndhrdw/samples.o \
         obj/vidhrdw/generic.o obj/sndhrdw/generic.o \
         obj/sndhrdw/namco.o \
	 obj/machine/wd179x.o \
	 obj/vidhrdw/tms9928a.o \
	 obj/vidhrdw/m6845.o \
         obj/vidhrdw/vector.o \
	 obj/drivers/nes.o obj/machine/nes.o obj/machine/nes_mmc.o obj/vidhrdw/nes.o \
	 obj/drivers/sms.o obj/machine/sms.o obj/vidhrdw/sms.o obj/vidhrdw/newsms.o \
	 obj/drivers/genesis.o obj/machine/genesis.o obj/sndhrdw/genesis.o obj/vidhrdw/genesis.o \
	 obj/drivers/coleco.o obj/machine/coleco.o obj/vidhrdw/coleco.o \
	 obj/drivers/astrocde.o obj/machine/astrocde.o obj/vidhrdw/astrocde.o \
         obj/drivers/apple2.o obj/machine/apple2.o obj/machine/ap_disk2.o obj/machine/ay3600.o obj/vidhrdw/apple2.o \
	 obj/drivers/atari.o obj/machine/atari.o obj/vidhrdw/atari.o obj/vidhrdw/antic.o obj/vidhrdw/gtia.o \
	 obj/drivers/cgenie.o obj/machine/cgenie.o obj/sndhrdw/cgenie.o \
	 obj/machine/cpm_bios.o obj/drivers/kaypro.o obj/machine/kaypro.o obj/vidhrdw/kaypro.o \
	 obj/drivers/trs80.o obj/machine/trs80.o obj/sndhrdw/trs80.o obj/vidhrdw/trs80.o \
         obj/drivers/vectrex.o obj/machine/vectrex.o obj/machine/vect_via.o obj/vidhrdw/vectrex.o \
	 obj/drivers/pdp1.o obj/machine/pdp1.o obj/vidhrdw/pdp1.o \
	 obj/Z80/Z80.o obj/M6502/M6502.o obj/I86/I86.o obj/I8039/I8039.o obj/I8085/I8085.o \
	 obj/M6809/m6809.o obj/M6805/m6805.o \
	 obj/S2650/s2650.o obj/T11/t11.o obj/PDP1/pdp1.o \
         $(M6808OBJS) \
         $(M68KOBJS) \
         obj/mamedbg.o obj/asg.o obj/M6502/6502dasm.o obj/I8085/8085dasm.o \
         obj/M6809/6809dasm.o obj/M6805/6805dasm.o  obj/I8039/8039dasm.o \
	 obj/S2650/2650dasm.o obj/T11/t11dasm.o obj/PDP1/pdp1dasm.o obj/M68000/m68kdasm.o \
         obj/msdos/msdos.o obj/msdos/video.o obj/msdos/vector.o obj/msdos/sound.o \
	 obj/msdos/input.o obj/msdos/fileio.o obj/msdos/config.o obj/msdos/fronthlp.o \
	 obj/msdos/nec765.o

VPATH=src src/z80 src/m6502 src/i86 src/m6809 src/m6808

all: mess.exe

mess.exe:  $(OBJS)
	$(LD) $(LDFLAGS) -o mess.exe $(OBJS) $(LIBS)

obj/%.o: src/%.c mame.h driver.h
	 $(CC) $(DEFS) $(CFLAGS) -o $@ -c $<

# dependencies
obj/z80/z80.o:		z80.c z80.h z80codes.h z80io.h z80daa.h
obj/m6502/m6502.o:	m6502.c m6502.h m6502ops.h tbl6502.c tbl65c02.c tbl6510.c
obj/i86/i86.o:		i86.c i86.h i86intrf.h ea.h host.h instr.h modrm.h
obj/m6809/m6809.o:	m6809.c m6809.h 6809ops.c
obj/m6808/m6808.o:	m6808.c m6808.h

makedir:
	deltree obj
	mkdir obj
	mkdir obj\Z80
	mkdir obj\M6502
	mkdir obj\I86
	mkdir obj\I8039
	mkdir obj\I8085
	mkdir obj\M6809
	mkdir obj\M6808
	mkdir obj\M6805
	mkdir obj\M68000
	mkdir obj\T11
	mkdir obj\S2650
	mkdir obj\PDP1
	mkdir obj\drivers
	mkdir obj\machine
	mkdir obj\vidhrdw
	mkdir obj\sndhrdw
	mkdir obj\msdos
	copy ym2203.opm obj\sndhrdw\ym2203.opm

clean:
	del obj\*.o
	del obj\Z80\*.o
	del obj\M6502\*.o
	del obj\I86\*.o
	del obj\I8039\*.o
	del obj\I8085\*.o
	del obj\M6809\*.o
	del obj\M6808\*.o
	del obj\M6805\*.o
	del obj\M68000\*.o
	del obj\T11\*.o
	del obj\S2650\*.o
	del obj\PDP1\*.o
	del obj\drivers\*.o
	del obj\machine\*.o
	del obj\vidhrdw\*.o
	del obj\sndhrdw\*.o
	del obj\msdos\*.o
	del mess.exe

cleandebug:
	del obj\*.o
	del obj\Z80\*.o
	del obj\I86\*.o
	del obj\I8039\*.o
	del obj\I8085\*.o
	del obj\M6502\*.o
	del obj\M6809\*.o
	del obj\M6808\*.o
	del obj\M6805\*.o
	del obj\M68000\*.o
	del obj\T11\*.o
	del obj\S2650\*.o
	del obj\PDP1\*.o
	del mess.exe
