# only Windows specific output files and rules
# the first two targets generate the prefix.h header
# note this requires that OSOBJS be the first target
OSOBJS = $(OBJ)/windows/winmain.o  $(OBJ)/windows/config.o \
	 $(OBJ)/windows/ticker.o $(OBJ)/windows/fronthlp.o $(OBJ)/windows/video.o \
	 $(OBJ)/windows/input.o $(OBJ)/windows/sound.o $(OBJ)/windows/blit.o \
	 $(OBJ)/windows/snprintf.o $(OBJ)/windows/rc.o $(OBJ)/windows/misc.o \
	 $(OBJ)/windows/window.o $(OBJ)/windows/winddraw.o $(OBJ)/windows/asmblit.o

ifndef MESS
OSOBJS += $(OBJ)/windows/fileio.o 
else
OSOBJS += $(OBJ)/mess/windows/fileio.o	$(OBJ)/mess/windows/dirio.o \
	  $(OBJ)/mess/windows/messwin.o $(OBJ)/mess/windows/messopts.o
#CMDOSOBJS = $(OBJ)/mess/windows/fileio2.o
endif 
      
# video blitting functions
$(OBJ)/windows/asmblit.o: src/windows/asmblit.asm
	@echo Assembling $<...
#	$(ASM) -e $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<
	$(ASM) -o $@ $(ASMFLAGS) $(subst -D,-d,$(ASMDEFS)) $<

ifndef MSVC
# add our prefix files to the mix
CFLAGS += -mwindows -include src/windows/winprefix.h

# add the windows libaries
LIBS += -luser32 -lgdi32 -lddraw -ldsound -ldinput -ldxguid -lwinmm
endif

ifdef MSVC
FFLAGS += /FI"src/windows/winprefix.h"
CFLAGS += $(FFLAGS)
endif

# obj files for gui
GUIOBJS = \
        $(OBJ)/windowsui/MAME32.o \
        $(OBJ)/windowsui/M32Util.o \
        $(OBJ)/windowsui/DirectInput.o \
        $(OBJ)/windowsui/DIJoystick.o \
        $(OBJ)/windowsui/DirectDraw.o \
        $(OBJ)/windowsui/Joystick.o \
        $(OBJ)/windowsui/file.o \
        $(OBJ)/windowsui/Directories.o \
        $(OBJ)/windowsui/mzip.o \
        $(OBJ)/windowsui/audit32.o \
        $(OBJ)/windowsui/ColumnEdit.o \
        $(OBJ)/windowsui/Screenshot.o \
        $(OBJ)/windowsui/TreeView.o \
        $(OBJ)/windowsui/Splitters.o \
        $(OBJ)/windowsui/Bitmask.o \
        $(OBJ)/windowsui/DataMap.o \
        $(OBJ)/windowsui/dxdecode.o \
        $(OBJ)/windowsui/ChildOutputStream.o \
        $(OBJ)/windowsui/help.o

ifdef MESS
GUIOBJS += \

GUIRES = $(OBJ)/mess/windowsui/mess32.res
else
GUIOBJS += \
		$(OBJ)/windowsui/Win32ui.o \
		$(OBJ)/windowsui/options.o \
        $(OBJ)/windowsui/Properties.o

GUIRES = $(OBJ)/windowsui/mame32.res
endif

# due to quirks of using /bin/sh, we need to explicitly specify the current path
CURPATH = ./

include src/windowsui/windowsui.mak
