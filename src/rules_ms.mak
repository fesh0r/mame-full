# Rules for MESS CPU's

CPU=$(strip $(findstring APEXC@,$(CPUS)))
ifneq ($(CPU),)
APEXCDIR = mess/cpu/apexc
OBJDIRS += $(OBJ)/$(APEXCDIR)
CPUDEFS += -DHAS_APEXC=1
CPUOBJS += $(OBJ)/$(APEXCDIR)/apexc.o
DBGOBJS += $(OBJ)/$(APEXCDIR)/apexcdsm.o
$(OBJ)/$(APEXCDIR)/apexc.o: $(APEXCDIR)/apexc.c $(APEXCDIR)/apexc.h
else
CPUDEFS += -DHAS_APEXC=0
endif
