#####################################################################
# make SUFFIX=32

# don't create gamelist.txt
TEXTS=

# remove pedantic
$(OBJ)/windowsui/%.o: src/windowsui/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

# remove pedantic
$(OBJ)/mess/windowsui/%.o: mess/windowsui/%.c
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@

OBJDIRS += $(OBJ)/windowsui

# only OS specific output files and rules
OSOBJS += \
	$(OBJ)/windowsui/MAME32.o \
	$(OBJ)/windowsui/M32Util.o \
	$(OBJ)/windowsui/DirectInput.o \
	$(OBJ)/windowsui/DIJoystick.o \
	$(OBJ)/windowsui/DirectDraw.o \
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
	$(OBJ)/windowsui/help.o \
	$(OBJ)/mess/windowsui/mess32ui.o \
	$(OBJ)/mess/windowsui/MessOptions.o \
	$(OBJ)/mess/windowsui/MessProperties.o \
	$(OBJ)/mess/windowsui/SmartListView.o \
	$(OBJ)/mess/windowsui/SoftwareList.o

        #$(OBJ)/windowsui/Win32ui.o \ 		<<--- included from MESS' version
        #$(OBJ)/windowsui/Properties.o \	<<--- included from MESS' version
        #$(OBJ)/windowsui/options.o \		<<--- included from MESS' version

# add resource file
OSOBJS += $(OBJ)/mess/windowsui/mess32.res

#####################################################################
# compiler

#
# Preprocessor Definitions
#

DEFS += -DDIRECTSOUND_VERSION=0x0300 \
        -DDIRECTINPUT_VERSION=0x0500 \
        -DDIRECTDRAW_VERSION=0x0300 \
        -DWINVER=0x0400 \
        -D_WIN32_IE=0x0400 \
        -D_WIN32_WINNT=0x0400 \
        -DWIN32 \
        -UWINNT \
        -DCLIB_DECL=__cdecl \
        -DDECL_SPEC= \
        -DZEXTERN=extern \

#####################################################################
# Resources

RC = windres

RCDEFS = -DMESS -DNDEBUG -D_WIN32_IE=0x0400

RCFLAGS = -O coff --include-dir mess/windowsui --include-dir src/windowsui

$(OBJ)/mess/windowsui/%.res: mess/windowsui/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) -o $@ -i $<



#####################################################################
# Linker

LIBS += -lkernel32 \
        -lshell32 \
        -lcomctl32 \
        -lcomdlg32 \
        -ladvapi32 \
        -lhtmlhelp

# use -mconsole for romcmp
LDFLAGS += -mwindows -mconsole

#####################################################################

