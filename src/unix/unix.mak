##############################################################################
# None user configurable settings
##############################################################################

# *** Comment this line to get verbose make output, for debugging build
# problems
QUIET = 0


##############################################################################
# **** CPU dependent settings.
##############################################################################
#note : -D__CPU_$(MY_CPU) is added automaticly later on.
CFLAGS.i386       = -DLSB_FIRST -DX86_ASM
CFLAGS.i386_noasm = -DLSB_FIRST
CFLAGS.alpha      = -DLSB_FIRST -DALIGN_INTS -DALIGN_SHORTS -D__LP64__
CFLAGS.m68k       = 
CFLAGS.risc       = -DALIGN_INTS -DALIGN_SHORTS 
CFLAGS.risc_lsb   = -DALIGN_INTS -DALIGN_SHORTS -DLSB_FIRST
CFLAGS.mips       = -DALIGN_INTS -DALIGN_SHORTS -DSGI_FIX_MWA_NOP

##############################################################################
# **** Architecture dependent settings.
##############################################################################
LIBS.solaris       = -lnsl -lsocket
LIBS.irix          = -laudio
LIBS.irix_al       = -laudio
LIBS.aix           = -lUMSobj

##############################################################################
# **** Display dependent settings.
##############################################################################
#first calculate the X11 Joystick driver settings, this is done here since
#they are only valid for X11 based display methods
ifdef JOY_X11
JOY_X11_CFLAGS = -DX11_JOYSTICK "-DX11_JOYNAME='$(X11_JOYNAME)'" -DUSE_X11_JOYEVENTS
JOY_X11_LIBS   = -lXi
endif

# svga and ggi also use $(X11LIB) since that's where zlib often is
LIBS.x11        = $(X11LIB) $(JOY_X11_LIBS) -lX11 -lXext 
LIBS.svgalib    = $(X11LIB) -lvga -lvgagl
LIBS.ggi        = $(X11LIB) -lggi
LIBS.xgl        = $(X11LIB) $(JOY_X11_LIBS) -lX11 -lXext $(GLLIBS) -ljpeg
LIBS.xfx        = $(X11LIB) $(JOY_X11_LIBS) -lX11 -lXext -lglide2x
LIBS.svgafx     = $(X11LIB) -lvga -lvgagl -lglide2x
LIBS.openstep	= -framework AppKit -framework SoundKit
LIBS.SDL	= -ldl -lSDL -lpthread -D_REENTRANT

CFLAGS.x11      = $(X11INC) $(JOY_X11_CFLAGS)
CFLAGS.xgl      = $(X11INC) $(JOY_X11_CFLAGS) $(GLDRIVER)
CFLAGS.xfx      = $(X11INC) $(JOY_X11_CFLAGS) -I/usr/include/glide
CFLAGS.svgafx   = -I/usr/include/glide
CFLAGS.SDL      = -D_REENTRANT

INST.x11        = doinstall
INST.ggi        = doinstall
INST.svgalib    = doinstallsuid
INST.xgl        = doinstallsuid copycab
INST.xfx        = doinstallsuid
INST.svgafx     = doinstallsuid
INST.SDL	= doinstallsuid

# handle X11 display method additonal settings
ifdef X11_MITSHM
CFLAGS.x11 += -DUSE_MITSHM
endif
ifdef X11_DGA
CFLAGS.x11 += -DUSE_DGA
LIBS.x11   += -lXxf86dga -lXxf86vm
endif
ifdef X11_XIL
CFLAGS.x11 += -DUSE_XIL
LIBS.x11   += -lxil -lpthread
endif


##############################################################################
# Quiet the compiler output if requested
##############################################################################

ifdef QUIET
CC_COMMENT = 
CC_COMPILE = @
AR_OPTS = rc
else
CC_COMMENT = \#
CC_COMPILE = 
AR_OPTS = rcv
endif


##############################################################################
# these are the object subdirectories that need to be created.
##############################################################################
OBJ     = $(TARGET).obj

OBJDIRS = $(OBJ) \
	$(OBJ)/drivers $(OBJ)/machine $(OBJ)/vidhrdw $(OBJ)/sndhrdw \
	$(OBJ)/cpu $(OBJ)/sound $(OBJ)/cpu/i8039 $(OBJ)/cpu/i8085 \
	$(OBJ)/cpu/i86 $(OBJ)/cpu/m6502 $(OBJ)/cpu/m68000 $(OBJ)/cpu/m6805 \
	$(OBJ)/cpu/m6800 $(OBJ)/cpu/m6809 $(OBJ)/cpu/s2650 $(OBJ)/cpu/t11 \
	$(OBJ)/cpu/z80 $(OBJ)/cpu/tms34010 $(OBJ)/cpu/tms9900 \
	$(OBJ)/cpu/z8000 $(OBJ)/cpu/tms32010 $(OBJ)/cpu/h6280 \
	$(OBJ)/cpu/ccpu $(OBJ)/cpu/pdp1 $(OBJ)/cpu/konami $(OBJ)/cpu/nec \
	$(OBJ)/cpu/gensync $(OBJ)/cpu/adsp2100 $(OBJ)/cpu/z80gb $(OBJ)/cpu/arm \
	$(OBJ)/cpu/mips $(OBJ)/cpu/sc61860 \
	$(OBJ)/mess $(OBJ)/mess/formats $(OBJ)/mess/systems $(OBJ)/mess/machine \
	$(OBJ)/mess/vidhrdw $(OBJ)/mess/sndhrdw $(OBJ)/mess/tools

IMGTOOL_OBJS = $(OBJ)/unix.$(DISPLAY_METHOD)/dirio.o

##############################################################################
# "Calculate" the final CFLAGS, unix CONFIG, LIBS and OBJS
##############################################################################
ifdef ZLIB
ZLIB    = contrib/cutzlib-1.1.3/libz.a
endif

all: $(ZLIB) $(OBJDIRS) osdepend x$(TARGET).$(DISPLAY_METHOD)

# CPU core include paths
VPATH=src $(wildcard src/cpu/*)

include src/core.mak
include src/$(TARGET).mak
include src/rules.mak

# Perhaps one day original mame/mess sources will use POSIX strcasecmp and
# M_PI instead MS-DOS counterparts... ( a long and sad history ...)
MY_CFLAGS = $(CFLAGS) $(IL) $(CFLAGS.$(MY_CPU)) \
	-D__ARCH_$(ARCH) -D__CPU_$(MY_CPU) -D$(DISPLAY_METHOD) \
	-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp \
	-DPI=M_PI -DUNIX -DSIGNED_SAMPLES \
	$(COREDEFS) $(SOUNDDEFS) $(CPUDEFS) $(ASMDEFS) \
	$(INCLUDES) -Isrc -Imess -Isrc/unix -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000

MY_LIBS = $(LIBS) $(LIBS.$(ARCH)) $(LIBS.$(DISPLAY_METHOD)) -lz -lm

ifdef ZLIB
MY_CFLAGS += -Icontrib/cutzlib-1.1.3 -I../../contrib/cutzlib-1.1.3
MY_LIBS   += -Lcontrib/cutzlib-1.1.3
endif

ifdef MAME_DEBUG
MY_CFLAGS += -DMAME_DEBUG
MY_LIBS   += -lcurses
endif
   
# CONFIG are the cflags used to build the unix tree, this is were most defines
# go
CONFIG = $(MY_CFLAGS) $(CFLAGS.$(DISPLAY_METHOD)) -DNAME='\"x$(TARGET)\"' \
	-DDISPLAY_METHOD='\"$(DISPLAY_METHOD)\"' -DXMAMEROOT='\"$(XMAMEROOT)\"'

ifdef HAVE_GETTIMEOFDAY
CONFIG += -DHAVE_GETTIMEOFDAY
endif

# Sound drivers config
ifdef SOUND_ESOUND
CONFIG  += -DSYSDEP_DSP_ESOUND `esd-config --cflags`
MY_LIBS += `esd-config --libs`
endif

# Joystick drivers config
ifdef JOY_I386
CONFIG += -DI386_JOYSTICK
endif
ifdef JOY_PAD
CONFIG += -DLIN_FM_TOWNS
endif

ifdef EFENCE
MY_LIBS += -lefence
endif

#we remove $(OBJ)/vidhrdw/vector.o from $(COREOBJS) since we have our own
#build rules for this object because it is display dependent.
OBJS  += $(subst $(OBJ)/vidhrdw/vector.o, ,$(COREOBJS)) $(DRVLIBS) \
 $(OBJ)/unix.$(DISPLAY_METHOD)/osdepend.a $(OBJ)/unix.$(DISPLAY_METHOD)/vector.o

##############################################################################
# Begin of the real makefile.
##############################################################################
x$(TARGET).$(DISPLAY_METHOD): $(OBJS)
	$(CC_COMMENT) @echo 'Linking $@ ...'
	$(CC_COMPILE) $(LD) -o $@ $(OBJS) $(MY_LIBS)

tools: $(ZLIB) $(OBJDIRS) $(TOOLS)

gamelist: all
	./x$(TARGET).$(DISPLAY_METHOD) -listgamelistheader > doc/gamelist.$(TARGET)
	./x$(TARGET).$(DISPLAY_METHOD) -listgamelist >> doc/gamelist.$(TARGET)

$(OBJDIRS):
	-mkdir $@

xlistdev: contrib/tools/xlistdev.c
	$(CC_COMMENT) @echo 'Compiling $< ...'
	$(CC_COMPILE) $(CC) $(X11INC) contrib/tools/xlistdev.c -o xlistdev $(JSLIB) $(LIBS.$(ARCH)) $(LIBS.$(DISPLAY_METHOD)) -lm


osdepend:
	$(CC_COMMENT) @echo 'Compiling in the unix directory...'
	$(CC_COMPILE) \
	 ( \
	 cd src/unix; \
	  $(MAKE) CC="$(CC)" RANLIB="$(RANLIB)" ARCH="$(ARCH)" \
	  DISPLAY_METHOD="$(DISPLAY_METHOD)" CFLAGS="$(CONFIG) -I../../mess" \
	  CC_COMMENT="$(CC_COMMENT)" CC_COMPILE="$(CC_COMPILE)" \
	  AR_OPTS="$(AR_OPTS)" OBJ="$(OBJ)" \
	 )

contrib/cutzlib-1.1.3/libz.a:
	( \
	cd contrib/cutzlib-1.1.3; \
	./configure; \
	$(MAKE) libz.a \
	)

ifdef MESS
$(OBJ)/mess/%.o: mess/%.c
	$(CC_COMMENT) @echo '[MESS] Compiling $< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<
endif

$(OBJ)/%.o: src/%.c
	$(CC_COMMENT) @echo 'Compiling $< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<

$(OBJ)/%.a:
	$(CC_COMMENT) @echo 'Archiving $@ ...'
	$(CC_COMPILE) ar $(AR_OPTS) $@ $^
	$(CC_COMPILE) $(RANLIB) $@

# special cases for the 68000 core
#
# this generates the C source files for the 68000 emulator
$(OBJ)/cpu/m68000/m68kmake $(OBJ)/cpu/m68000/m68kops.h: src/cpu/m68000/m68kmake.c src/cpu/m68000/m68k_in.c
	$(CC_COMMENT) @echo 'Compiling src/cpu/m68000/m68kmake.c ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $(OBJ)/cpu/m68000/m68kmake src/cpu/m68000/m68kmake.c
	$(CC_COMMENT) @echo 'Compiling src/cpu/m68000/m68k_in.c ...'
	$(CC_COMPILE) $(OBJ)/cpu/m68000/m68kmake $(OBJ)/cpu/m68000 src/cpu/m68000/m68k_in.c

# to compile generated C-files
$(M68000_GENERATED_OBJS): $(OBJ)/cpu/m68000/m68kmake
	$(CC_COMMENT) @echo 'Compiling $*.c ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $*.c

# src/cpu/m68000/m68kcpu.c needs a generated .h file
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kops.h

# The m68000 asm core
$(OBJ)/cpu/m68000/68kem.asm: src/cpu/m68000/make68k.c
	$(CC_COMMENT) @echo 'Compiling $< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $(OBJ)/cpu/m68000/make68k $<
	$(CC_COMPILE) $(OBJ)/cpu/m68000/make68k $@ $(OBJ)/cpu/m68000/comptab.asm

$(OBJ)/cpu/m68000/68kem.o: $(OBJ)/cpu/m68000/68kem.asm
	$(CC_COMMENT) @echo 'Assembling $< ...'
	$(CC_COMPILE) $(ASM_STRIP) $<
	$(CC_COMPILE) nasm $(NASM_FMT) -o $@ $<

romcmp$(EXE): $(OBJ)/romcmp.o $(OBJ)/unzip.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ -lz -o $@

#some tricks, since vector.o these days is display-method dependent:
$(OBJ)/unix.$(DISPLAY_METHOD)/vector.o: src/vidhrdw/vector.c
	$(CC_COMMENT) @echo 'Compiling $< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<

#make sure this isn't accidently in makefile.$(OBJ):
$(OBJ)/vidhrdw/vector.o: bla

install: $(INST.$(DISPLAY_METHOD)) install-man
	@echo x$(TARGET) for $(ARCH)-$(MY_CPU) installation completed

install-man:
	@echo installing manual pages under $(MANDIR) ...
	-mkdir $(MANDIR)
	$(INSTALL) doc/xmame.man $(MANDIR)/xmame.6

doinstall:
	@echo installing binaries under $(DESTDIR)...
	$(INSTALL) x$(TARGET).$(DISPLAY_METHOD) $(DESTDIR)

doinstallsuid:
	@echo installing binaries under $(DESTDIR)...
	$(INSTALL) x$(TARGET).$(DISPLAY_METHOD) $(DESTDIR)
	chmod 4755 $(DESTDIR)/$(NAME).$(DISPLAY_METHOD)

copycab:
	@echo installing cabinet files under $(XMAMEROOT)...
	$(INSTALL) -R cab $(XMAMEROOT)

clean: 
	rm -fr $(OBJ) x$(TARGET).* xlistdev contrib/cutzlib-1.1.3/libz.a contrib/cutzlib-1.1.3/*.o
