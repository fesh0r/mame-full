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
	  $(OBJ)/mess/systems/coleco.o \
	  $(OBJ)/mess/vidhrdw/tms9928a.o
	  
# MESS specific core $(OBJ)s
COREOBJS += \
	$(OBJ)/cheat.o  	       \
	$(OBJ)/mess/mess.o	       \
	$(OBJ)/mess/device.o	       \
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
	$(OBJ)/mess/machine/flopdrv.o  \

