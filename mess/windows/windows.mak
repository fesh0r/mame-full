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
	$(OBJ)/mess/windows/configms.o	\
	$(OBJ)/mess/windows/dialog.o	\
	$(OBJ)/mess/windows/menu.o		\
	$(OBJ)/mess/windows/mess.res	\
	$(OBJ)/mess/windows/messlib.o	\
	$(OBJ)/mess/windows/opcntrl.o	\
	$(OBJ)/mess/windows/tapedlg.o	\
	$(OBJ)/mess/windows/winutils.o

OSDCOREOBJS += \
	$(OBJ)/mess/windows/dirio.o		\
	$(OBJ)/mess/windows/parallel.o	\
	$(OBJ)/mess/windows/glob.o



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
