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


CPU=$(strip $(findstring CP1600@,$(CPUS)))
ifneq ($(CPU),)
CPD = mess/cpu/cp1600
OBJDIRS += $(OBJ)/$(CPD)
CPUDEFS += -DHAS_CP1600=1
CPUOBJS += $(OBJ)/$(CPD)/cp1600.o
DBGOBJS += $(OBJ)/$(CPD)/1600dasm.o
$(OBJ)/$(CPD)/cp1600.o: $(CPD)/cp1600.c $(CPD)/cp1600.h
else
CPUDEFS += -DHAS_CP1600=0
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


CPU=$(strip $(findstring G65816@,$(CPUS)))
ifneq ($(CPU),)
G6D = mess/cpu/g65816
OBJDIRS += $(OBJ)/$(G6D)
CPUDEFS += -DHAS_G65816=1
CPUOBJS += $(OBJ)/$(G6D)/g65816.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o0.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o1.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o2.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o3.o
CPUOBJS += $(OBJ)/$(G6D)/g65816o4.o
DBGOBJS += $(OBJ)/$(G6D)/g65816ds.o
$(OBJ)/$(G6D)/g65816.o: $(G6D)/g65816.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o0.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o1.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o2.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o3.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
$(OBJ)/$(G6D)/g65816o4.o: $(G6D)/g65816o0.c $(G6D)/g65816.h $(G6D)/g65816cm.h $(G6D)/g65816op.h
else
CPUDEFS += -DHAS_G65816=0
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


CPU=$(strip $(findstring SPC700@,$(CPUS)))
ifneq ($(CPU),)
SPCD = mess/cpu/spc700
OBJDIRS += $(OBJ)/$(SPCD)
CPUDEFS += -DHAS_SPC700=1
CPUOBJS += $(OBJ)/$(SPCD)/spc700.o
DBGOBJS += $(OBJ)/$(SPCD)/spc700ds.o
$(OBJ)/$(SPCD)/spc700/spc700.o: $(SPCD)/spc700.c $(SPCD)/spc700.h
else
CPUDEFS += -DHAS_SPC700=0
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

CPU=$(strip $(findstring Z80_MSX@,$(CPUS)))
ifneq ($(CPU),)
OBJDIRS += $(OBJ)/cpu/z80
CPUDEFS += -DHAS_Z80_MSX=1
CPUOBJS += $(OBJ)/cpu/z80/z80_msx.o
$(OBJ)/cpu/z80/z80_msx.o: z80_msx.c z80_msx.h z80daa.h z80.h z80.c
else
CPUDEFS += -DHAS_Z80_MSX=0
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

