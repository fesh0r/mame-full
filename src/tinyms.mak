MESS=1

# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1 -DMESS=1
COREDEFS += -DTINY_NAME="driver_coleco"
COREDEFS += -DTINY_POINTER="&driver_coleco"


# uses these CPUs
CPUS+=Z80@

# uses these SOUNDs
SOUNDS+=SN76496@

OBJS =    $(OBJ)/mess/machine/coleco.o \
	  $(OBJ)/mess/vidhrdw/coleco.o \
	  $(OBJ)/mess/systems/coleco.o \





# MESS specific core $(OBJ)s
COREOBJS += \
	$(OBJ)/cheat.o  	       \
	$(OBJ)/mess/mess.o	       \
	$(OBJ)/mess/system.o	       \
	$(OBJ)/mess/config.o	       \
	$(OBJ)/mess/filemngr.o	       \
	$(OBJ)/mess/tapectrl.o	       \
	$(OBJ)/mess/menu.o	       \
	$(OBJ)/mess/printer.o	       \
	$(OBJ)/mess/menuentr.o	       \
	$(OBJ)/mess/utils.o	       \
	$(OBJ)/mess/bcd.o	       \
	$(OBJ)/mess/gregoria.o	       \
	$(OBJ)/mess/vidhrdw/state.o    \
	$(OBJ)/mess/vidhrdw/m6847.o    \
	$(OBJ)/mess/vidhrdw/m6845.o    \
	$(OBJ)/mess/vidhrdw/tms9928a.o \
	$(OBJ)/mess/machine/28f008sa.o \
	$(OBJ)/mess/machine/am29f080.o \
	$(OBJ)/mess/machine/rriot.o    \
	$(OBJ)/mess/machine/riot6532.o \
	$(OBJ)/mess/machine/pit8253.o  \
	$(OBJ)/mess/machine/mc146818.o \
	$(OBJ)/mess/machine/uart8250.o \
	$(OBJ)/mess/machine/pc_mouse.o \
	$(OBJ)/mess/machine/pclpt.o    \
	$(OBJ)/mess/machine/centroni.o \
	$(OBJ)/mess/machine/pckeybrd.o \
	$(OBJ)/mess/machine/pc_fdc_h.o \
	$(OBJ)/mess/machine/pc_flopp.o \
	$(OBJ)/mess/machine/basicdsk.o \
	$(OBJ)/mess/machine/d88.o      \
	$(OBJ)/mess/machine/wd179x.o   \
	$(OBJ)/mess/diskctrl.o	       \
	$(OBJ)/mess/machine/dsk.o      \
	$(OBJ)/mess/machine/flopdrv.o  \
	$(OBJ)/mess/machine/nec765.o   \
	$(OBJ)/mess/vidhrdw/rstrbits.o \
	$(OBJ)/mess/vidhrdw/rstrtrck.o






