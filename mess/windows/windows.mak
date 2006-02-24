###########################################################################
#
#   windows.mak
#
#   Windows-specific makefile
#
###########################################################################


#-------------------------------------------------
# nasm for Windows (but not cygwin) has a "w"
# at the end
#-------------------------------------------------

ifndef COMPILESYSTEM_CYGWIN
ASM = @nasmw
endif

# only Windows specific output files and rules
# the first two targets generate the prefix.h header
# note this requires that OSOBJS be the first target
#
OSOBJS = \
	$(OBJ)/$(MAMEOS)/asmblit.o \
	$(OBJ)/$(MAMEOS)/asmtile.o \
	$(OBJ)/$(MAMEOS)/blit.o \
	$(OBJ)/$(MAMEOS)/config.o \
	$(OBJ)/$(MAMEOS)/fileio.o \
	$(OBJ)/$(MAMEOS)/fronthlp.o \
	$(OBJ)/$(MAMEOS)/input.o \
	$(OBJ)/$(MAMEOS)/misc.o \
	$(OBJ)/$(MAMEOS)/rc.o \
	$(OBJ)/$(MAMEOS)/sound.o \
	$(OBJ)/$(MAMEOS)/ticker.o \
	$(OBJ)/$(MAMEOS)/video.o \
	$(OBJ)/$(MAMEOS)/window.o \
	$(OBJ)/$(MAMEOS)/wind3d.o \
	$(OBJ)/$(MAMEOS)/wind3dfx.o \
	$(OBJ)/$(MAMEOS)/winddraw.o \
	$(OBJ)/$(MAMEOS)/winmain.o \

OSTOOLOBJS = \
	$(OBJ)/$(MAMEOS)/osd_tool.o

ifdef MESS
CFLAGS += -DWINUI -DEMULATORDLL=\"$(EMULATORDLL)\"
OSOBJS += \
	$(OBJ)/mess/windows/dirio.o		\
	$(OBJ)/mess/windows/dirutils.o	\
	$(OBJ)/mess/windows/configms.o	\
	$(OBJ)/mess/windows/menu.o		\
	$(OBJ)/mess/windows/opcntrl.o	\
	$(OBJ)/mess/windows/dialog.o	\
	$(OBJ)/mess/windows/tapedlg.o	\
	$(OBJ)/mess/windows/parallel.o	\
	$(OBJ)/mess/windows/strconv.o	\
	$(OBJ)/mess/windows/winutils.o
endif 

# add resource file if no UI
ifeq ($(WINUI),)
OSOBJS += $(OBJ)/windows/mame.res
endif

# add debugging info
ifdef DEBUG
ifdef NEW_DEBUGGER
OSOBJS += $(OBJ)/windows/debugwin.o
endif

# enable guard pages on all memory allocations in the debug build
DEFS += -DMALLOC_DEBUG

OSDBGOBJS = $(OBJ)/windows/winalloc.o
OSDBGLDFLAGS = -Wl,--allow-multiple-definition
else
OSDBGOBJS =
OSDBGLDFLAGS =
endif

RESFILE=$(OBJ)/mess/windows/mess.res

# video blitting functions
$(OBJ)/windows/asmblit.o: src/windows/asmblit.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# tilemap blitting functions
$(OBJ)/windows/asmtile.o: src/windows/asmtile.asm
	@echo Assembling $<...
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

# add our prefix files to the mix (we need -Wno-strict-aliasing for DirectX)
ifndef MSVC
CFLAGS += -mwindows -include src/$(MAMEOS)/winprefix.h
# CFLAGSOSDEPEND += -Wno-strict-aliasing
else
CFLAGS += /FI"windows/winprefix.h"
endif

# add the windows libaries
ifndef MSVC
LIBS += -luser32 -lgdi32 -lddraw -ldsound -ldinput -ldxguid -lwinmm
else
LIBS += dinput.lib
endif

# due to quirks of using /bin/sh, we need to explicitly specify the current path
CURPATH = ./

# if building with a UI, set the C flags and include the ui.mak
ifneq ($(WINUI),)
CFLAGS+= -DWINUI=1
include src/ui/ui.mak
endif

# if we are not using x86drc.o, we should be
ifndef X86_MIPS3_DRC
COREOBJS += $(OBJ)/x86drc.o
endif

#####################################################################
# Resources

ifndef MESS
RC = @windres --use-temp-file

RCDEFS = -DNDEBUG -D_WIN32_IE=0x0400

RCFLAGS = -O coff --include-dir src/windows

$(OBJ)/windows/%.res: src/windows/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<
endif
