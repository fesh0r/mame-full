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
	$(OBJ)/mess/mess.o			   \
	$(OBJ)/mess/image.o		       \
	$(OBJ)/mess/pool.o			   \
	$(OBJ)/mess/system.o	       \
	$(OBJ)/mess/device.o	       \
	$(OBJ)/mess/config.o	       \
	$(OBJ)/mess/inputx.o		   \
	$(OBJ)/mess/mesintrf.o	       \
	$(OBJ)/mess/filemngr.o	       \
	$(OBJ)/mess/compcfg.o	       \
	$(OBJ)/mess/tapectrl.o	       \
	$(OBJ)/mess/printer.o	       \
	$(OBJ)/mess/cassette.o	       \
	$(OBJ)/mess/utils.o            \
	$(OBJ)/mess/bcd.o              \
	$(OBJ)/mess/gregoria.o	       \
	$(OBJ)/mess/led.o              \
	$(OBJ)/mess/eventlst.o         \
	$(OBJ)/mess/videomap.o         \
	$(OBJ)/mess/bitbngr.o          \
	$(OBJ)/mess/snapquik.o          \
	$(OBJ)/mess/statetxt.o         \
	$(OBJ)/mess/formats.o          \
	$(OBJ)/mess/messfmts.o         \
	$(OBJ)/mess/machine/flopdrv.o  \

