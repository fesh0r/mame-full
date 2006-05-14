###########################################################################
#
#   windows.mak
#
#   MESS Windows-specific makefile
#
###########################################################################


CFLAGS += -DWINUI -DEMULATORDLL=\"$(EMULATORDLL)\"
RCFLAGS += -DMESS

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
	$(OBJ)/mess/windows/winutils.o	\
	$(OBJ)/mess/windows/mess.res

ifdef MSVC_BUILD
MESSTEST_OBJS		+= $(OBJ)/mess/ui/dirent.o
MESSDOCS_OBJS		+= $(OBJ)/mess/ui/dirent.o
PLATFORM_TOOL_OBJS	+= $(OBJ)/mess/ui/dirent.o
endif



#-------------------------------------------------
# generic rules for the resource compiler
#-------------------------------------------------

$(OBJ)/mess/$(MAMEOS)/%.res: mess/$(MAMEOS)/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir mess/$(MAMEOS) -o $@ -i $<

$(OBJ)/ui/%.res: src/ui/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir src/ui -o $@ -i $<

$(OBJ)/mess/ui/%.res: mess/ui/%.rc
	@echo Compiling resources $<...
	$(RC) $(RCDEFS) $(RCFLAGS) --include-dir mess/ui --include-dir src/ui --include-dir src -o $@ -i $<
