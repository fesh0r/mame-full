MESS=1

# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1 -DMESS=1 -DTINY_NAME=driver_nes

# uses these CPUs
CPUS+=M6502@
CPUS+=M65C02@
CPUS+=M65SC02@
CPUS+=M65CE02@
CPUS+=M6509@
CPUS+=M6510@
CPUS+=M6510T@
CPUS+=M7501@
CPUS+=M8502@
CPUS+=M4510@
CPUS+=N2A03@
CPUS += N2A03@


# uses these SOUNDs
SOUNDS+=NES@
SOUNDS+=SAMPLES@



OBJS =    $(OBJ)/mess/machine/nes_mmc.o  \
          $(OBJ)/mess/vidhrdw/nes.o      \
          $(OBJ)/mess/machine/nes.o      \
          $(OBJ)/mess/systems/nes.o      \




# MESS specific core $(OBJ)s
COREOBJS +=        \
          $(OBJ)/mess/mess.o             \
          $(OBJ)/mess/system.o           \
          $(OBJ)/mess/config.o           \
          $(OBJ)/mess/filemngr.o         \
          $(OBJ)/mess/tapectrl.o         \
          $(OBJ)/mess/machine/6522via.o  \
          $(OBJ)/mess/machine/nec765.o   \
          $(OBJ)/mess/machine/dsk.o      \
          $(OBJ)/mess/machine/wd179x.o   \
	  $(OBJ)/mess/machine/flopdrv.o  \






