#####################################################################
# make SUFFIX=32

# don't create gamelist.txt
# TEXTS = gamelist.txt

ifndef MSVC
# remove pedantic
$(OBJ)/ui/%.o: src/windowsui/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

# remove pedantic
$(OBJ)/mess/windowsui/%.o: mess/windowsui/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@
endif

OBJDIRS += $(OBJ)/ui

$(OBJ)/mess/ui/mess32ui.o: src/ui/win32ui.c mess/ui/mess32ui.c

$(OBJ)/mess/ui/optionsms.o: src/ui/options.c mess/ui/optionsms.c

$(OBJ)/mess/ui/mess32.res:	src/ui/mame32.rc mess/ui/mess32.rc src/ui/resource.h mess/ui/resourcems.h

# only OS specific output files and rules
OSOBJS += \
	$(OBJ)/ui/m32util.o \
	$(OBJ)/ui/directinput.o \
	$(OBJ)/ui/dijoystick.o \
	$(OBJ)/ui/directdraw.o \
	$(OBJ)/ui/directories.o \
	$(OBJ)/ui/audit32.o \
	$(OBJ)/ui/columnedit.o \
	$(OBJ)/ui/screenshot.o \
	$(OBJ)/ui/treeview.o \
	$(OBJ)/ui/splitters.o \
	$(OBJ)/ui/bitmask.o \
	$(OBJ)/ui/datamap.o \
	$(OBJ)/ui/dxdecode.o \
	$(OBJ)/ui/picker.o \
	$(OBJ)/ui/properties.o \
	$(OBJ)/ui/tabview.o \
	$(OBJ)/ui/help.o \
	$(OBJ)/ui/history.o \
	$(OBJ)/ui/dialogs.o \
	$(OBJ)/ui/optcore.o	\
	$(OBJ)/ui/inifile.o	\
	$(OBJ)/ui/dirwatch.o	\
	$(OBJ)/ui/datafile.o	\
	
ifdef MESS
OSOBJS += \
	$(OBJ)/mess/ui/mess32ui.o \
	$(OBJ)/mess/ui/optionsms.o \
 	$(OBJ)/mess/ui/layoutms.o \
	$(OBJ)/mess/ui/ms32util.o \
	$(OBJ)/mess/ui/propertiesms.o \
	$(OBJ)/mess/ui/softwarepicker.o \
	$(OBJ)/mess/ui/devview.o
else
OSOBJS += \
	$(OBJ)/ui/win32ui.o \
	$(OBJ)/ui/options.o \
 	$(OBJ)/ui/layout.o \
	$(OBJ)/ui/m32main.o
endif

# add resource file
ifdef MESS
GUIRESFILE = $(OBJ)/mess/ui/mess32.res
else
OSOBJS += $(OBJ)/ui/mame32.res
endif

#####################################################################
# compiler

#
# Preprocessor Definitions
#

ifdef MSVC_BUILD
DEFS += -DWINVER=0x0500
else
DEFS += -DWINVER=0x0400
endif

DEFS += \
	-D_WIN32_IE=0x0500 \
	-DDECL_SPEC= \
	-DZEXTERN=extern \

#	-DSHOW_UNAVAILABLE_FOLDER


#####################################################################
# Resources

ifndef MESS
UI_RC = @windres --use-temp-file

UI_RCDEFS = -DNDEBUG -D_WIN32_IE=0x0400

UI_RCFLAGS = -O coff --include-dir src/ui

$(OBJ)/ui/%.res: src/ui/%.rc
	@echo Compiling mame32 resources $<...
	$(UI_RC) $(UI_RCDEFS) $(UI_RCFLAGS) -o $@ -i $<
endif

#####################################################################
# Linker

ifndef MSVC
LIBS += -lkernel32 \
        -lshell32 \
        -lcomctl32 \
        -lcomdlg32 \
        -ladvapi32 \

endif

ifndef MESS
LDFLAGS += -mwindows
endif

#####################################################################

