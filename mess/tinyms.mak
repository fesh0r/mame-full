MESS=1

# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1 -DNEOFREE -DMESS=1
COREDEFS += -DTINY_NAME="driver_a2600"
COREDEFS += -DTINY_POINTER="&driver_a2600"


# uses these CPUs
CPUS+=M6502@

# uses these SOUNDs
SOUNDS+=TIA@

OBJS =    $(OBJ)/mess/machine/a2600.o \
	  $(OBJ)/mess/systems/a2600.o 

	  
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

