# Rules for MESS CPU's

CPU=$(strip $(findstring APEXC@,$(CPUS)))
ifneq ($(CPU),)
APEXCD = mess/cpu/apexc
OBJDIRS += $(OBJ)/$(APEXCD)
CPUDEFS += -DHAS_APEXC=1
CPUOBJS += $(OBJ)/$(APEXCD)/apexc.o
DBGOBJS += $(OBJ)/$(APEXCD)/apexcdsm.o
$(OBJ)/$(APEXCD)/apexc.o: $(APEXCD)/apexc.c $(APEXCD)/apexc.h
else
CPUDEFS += -DHAS_APEXC=0
endif


CPU=$(strip $(findstring CDP1802@,$(CPUS)))
ifneq ($(CPU),)
CDPD = mess/cpu/cdp1802
OBJDIRS += $(OBJ)/$(CDPD)
CPUDEFS += -DHAS_CDP1802=1
CPUOBJS += $(OBJ)/$(CDPD)/cdp1802.o
DBGOBJS += $(OBJ)/$(CDPD)/1802dasm.o
$(OBJ)/$(CDPD)/cdp1802.o: $(CDPD)/1802tbl.c
else
CPUDEFS += -DHAS_CDP1802=0
endif


CPU=$(strip $(findstring CP1610@,$(CPUS)))
ifneq ($(CPU),)
CPD = mess/cpu/cp1610
OBJDIRS += $(OBJ)/$(CPD)
CPUDEFS += -DHAS_CP1610=1
CPUOBJS += $(OBJ)/$(CPD)/cp1610.o
DBGOBJS += $(OBJ)/$(CPD)/1610dasm.o
$(OBJ)/$(CPD)/cp1610.o: $(CPD)/cp1610.c $(CPD)/cp1610.h
else
CPUDEFS += -DHAS_CP1610=0
endif


CPU=$(strip $(findstring F8@,$(CPUS)))
ifneq ($(CPU),)
F8D = mess/cpu/f8
OBJDIRS += $(OBJ)/$(F8D)
CPUDEFS += -DHAS_F8=1
CPUOBJS += $(OBJ)/$(F8D)/f8.o
DBGOBJS += $(OBJ)/$(F8D)/f8dasm.o
$(OBJ)/$(F8D)/f8.o: $(F8D)/f8.c $(F8D)/f8.h
else
CPUDEFS += -DHAS_F8=0
endif


CPU=$(strip $(findstring LH5801@,$(CPUS)))
ifneq ($(CPU),)
LHD = mess/cpu/lh5801
OBJDIRS += $(OBJ)/$(LHD)
CPUDEFS += -DHAS_LH5801=1
CPUOBJS += $(OBJ)/$(LHD)/lh5801.o
DBGOBJS += $(OBJ)/$(LHD)/5801dasm.o
$(OBJ)/$(LHD)/lh5801.o: $(LHD)/lh5801.c $(LHD)/5801tbl.c $(LHD)/lh5801.h
else
CPUDEFS += -DHAS_LH5801=0
endif


CPU=$(strip $(findstring PDP1@,$(CPUS)))
ifneq ($(CPU),)
PDPD = mess/cpu/pdp1
OBJDIRS += $(OBJ)/$(PDPD)
CPUDEFS += -DHAS_PDP1=1
CPUOBJS += $(OBJ)/$(PDPD)/pdp1.o
DBGOBJS += $(OBJ)/$(PDPD)/pdp1dasm.o
$(OBJ)/$(PDPD)/pdp1.o: $(PDPD)/pdp1.c $(PDPD)/pdp1.h
else
CPUDEFS += -DHAS_PDP1=0
endif


CPU=$(strip $(findstring SATURN@,$(CPUS)))
ifneq ($(CPU),)
SATD = mess/cpu/saturn
OBJDIRS += $(OBJ)/$(SATD)
CPUDEFS += -DHAS_SATURN=1
CPUOBJS += $(OBJ)/$(SATD)/saturn.o
DBGOBJS += $(OBJ)/$(SATD)/saturnds.o
$(OBJ)/$(SATD)/saturn.o: $(SATD)/saturn.c $(SATD)/sattable.c $(SATD)/satops.c $(SATD)/saturn.h $(SATD)/sat.h
else
CPUDEFS += -DHAS_SATURN=0
endif


CPU=$(strip $(findstring SC61860@,$(CPUS)))
ifneq ($(CPU),)
SCD = mess/cpu/sc61860
OBJDIRS += $(OBJ)/$(SCD)
CPUDEFS += -DHAS_SC61860=1
CPUOBJS += $(OBJ)/$(SCD)/sc61860.o
DBGOBJS += $(OBJ)/$(SCD)/scdasm.o
$(OBJ)/$(SCD)/sc61860.o: $(SCD)/sc61860.h  $(SCD)/sc.h $(SCD)/scops.c $(SCD)/sctable.c
else
CPUDEFS += -DHAS_SC61860=0
endif


CPU=$(strip $(findstring Z80GB@,$(CPUS)))
ifneq ($(CPU),)
GBD = mess/cpu/z80gb
OBJDIRS += $(OBJ)/$(GBD)
CPUDEFS += -DHAS_Z80GB=1
CPUOBJS += $(OBJ)/$(GBD)/z80gb.o
DBGOBJS += $(OBJ)/$(GBD)/z80gbd.o
$(OBJ)/$(GBD)/z80gb.o: $(GBD)/z80gb.c $(GBD)/z80gb.h $(GBD)/daa_tab.h $(GBD)/opc_cb.h $(GBD)/opc_main.h
else
CPUDEFS += -DHAS_Z80GB=0
endif

CPU=$(strip $(findstring TMS7000@,$(CPUS)))
ifneq ($(CPU),)
TM7D = mess/cpu/tms7000
OBJDIRS += $(OBJ)/$(TM7D)
CPUDEFS += -DHAS_TMS7000=1
CPUOBJS += $(OBJ)/$(TM7D)/tms7000.o
DBGOBJS += $(OBJ)/$(TM7D)/7000dasm.o
$(OBJ)/$(TM7D)/tms7000.o:	$(TM7D)/tms7000.h $(TM7D)/tms7000.c
$(OBJ)/$(TM7D)/7000dasm.o:	$(TM7D)/tms7000.h $(TM7D)/7000dasm.c
else
CPUDEFS += -DHAS_TMS7000=0
endif

CPU=$(strip $(findstring TMS7000_EXL@,$(CPUS)))
ifneq ($(CPU),)
TM7D = mess/cpu/tms7000
OBJDIRS += $(OBJ)/$(TM7D)
CPUDEFS += -DHAS_TMS7000_EXL=1
CPUOBJS += $(OBJ)/$(TM7D)/tms7000.o
DBGOBJS += $(OBJ)/$(TM7D)/7000dasm.o
$(OBJ)/$(TM7D)/tms7000.o:	$(TM7D)/tms7000.h $(TM7D)/tms7000.c
$(OBJ)/$(TM7D)/7000dasm.o:	$(TM7D)/tms7000.h $(TM7D)/7000dasm.c
else
CPUDEFS += -DHAS_TMS7000_EXL=0
endif

CPU=$(strip $(findstring TX0@,$(CPUS)))
ifneq ($(CPU),)
TX0D = mess/cpu/pdp1
OBJDIRS += $(OBJ)/$(TX0D)
CPUDEFS += -DHAS_TX0_64KW=1 -DHAS_TX0_8KW=1
CPUOBJS += $(OBJ)/$(TX0D)/tx0.o
DBGOBJS += $(OBJ)/$(TX0D)/tx0dasm.o
$(OBJ)/$(TX0D)/tx0.o:		$(TX0D)/tx0.h $(TX0D)/tx0.c
$(OBJ)/$(TX0D)/tx0dasm.o:	$(TX0D)/tx0.h $(TX0D)/tx0dasm.c
else
CPUDEFS += -DHAS_TX0_64KW=0 -DHAS_TX0_64KW=0
endif












SOUND=$(strip $(findstring BEEP@,$(SOUNDS)))
ifneq ($(SOUND),)
SOUNDDEFS += -DHAS_BEEP=1
SOUNDOBJS += $(OBJ)/mess/sound/beep.o
else
SOUNDDEFS += -DHAS_BEEP=0
endif

SOUND=$(strip $(findstring SPEAKER@,$(SOUNDS)))
ifneq ($(SOUND),)
SOUNDDEFS += -DHAS_SPEAKER=1
SOUNDOBJS += $(OBJ)/mess/sound/speaker.o
else
SOUNDDEFS += -DHAS_SPEAKER=0
endif

SOUND=$(strip $(findstring WAVE@,$(SOUNDS)))
ifneq ($(SOUND),)
SOUNDDEFS += -DHAS_WAVE=1
SOUNDOBJS += $(OBJ)/mess/sound/wave.o
else
SOUNDDEFS += -DHAS_WAVE=0
endif

