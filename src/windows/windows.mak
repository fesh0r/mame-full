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

# due to quirks of using /bin/sh, we need to explicitly specify the current path
CURPATH = ./
