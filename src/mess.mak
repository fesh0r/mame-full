# this is a MESS build
MESS = 1

# core defines
COREDEFS += -DNEOFREE -DMESS

# CPU cores used in MESS
CPUS+=Z80@
CPUS+=Z80GB@
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
CPUS+=H6280@
CPUS+=I86@
CPUS+=I88@
CPUS+=I186@
CPUS+=I188@
CPUS+=I286@
CPUS+=I8039@
CPUS+=I8048@
CPUS+=M6800@
CPUS+=M6803@
CPUS+=M6808@
CPUS+=HD63701@
CPUS+=NSC8105@
CPUS+=M6809@
CPUS+=M68000@
CPUS+=TMS9900@
CPUS+=TMS9995@
CPUS+=PDP1@
CPUS+=SC61860@
CPUS+=ARM@

# SOUND cores used in MESS
SOUNDS+=CUSTOM@
SOUNDS+=SAMPLES@
SOUNDS+=DAC@
SOUNDS+=AY8910@
SOUNDS+=YM2151_ALT@
SOUNDS+=YM2608@
SOUNDS+=YM2610@
SOUNDS+=YM2612@
SOUNDS+=YM2413@
SOUNDS+=YM3812@
SOUNDS+=SN76496@
SOUNDS+=POKEY@
SOUNDS+=TIA@
SOUNDS+=NES@
SOUNDS+=ASTROCADE@
SOUNDS+=TMS5220@
SOUNDS+=OKIM6295@
SOUNDS+=QSOUND@
SOUNDS+=SPEAKER@
SOUNDS+=WAVE@
SOUNDS+=SAA1099@

# Archive definitions
DRVLIBS = $(OBJ)/advision.a \
          $(OBJ)/sega.a     \
          $(OBJ)/coleco.a   \
          $(OBJ)/atari.a    \
          $(OBJ)/nintendo.a \
          $(OBJ)/cbm.a      \
          $(OBJ)/dragon.a   \
          $(OBJ)/kaypro.a   \
          $(OBJ)/trs80.a    \
          $(OBJ)/cgenie.a   \
          $(OBJ)/pdp1.a     \
          $(OBJ)/sinclair.a \
          $(OBJ)/apple1.a   \
          $(OBJ)/apple2.a   \
          $(OBJ)/mac.a      \
          $(OBJ)/ti99.a     \
          $(OBJ)/bally.a    \
          $(OBJ)/pc.a       \
          $(OBJ)/p2000.a    \
          $(OBJ)/amstrad.a  \
          $(OBJ)/nec.a      \
          $(OBJ)/ep128.a    \
          $(OBJ)/ascii.a    \
          $(OBJ)/tangerin.a \
          $(OBJ)/vtech.a    \
          $(OBJ)/jupiter.a  \
          $(OBJ)/mbee.a     \
          $(OBJ)/nascom1.a  \
          $(OBJ)/bbc.a      \
          $(OBJ)/cpschngr.a \
          $(OBJ)/mtx.a      \
          $(OBJ)/acorn.a    \
          $(OBJ)/samcoupe.a \
          $(OBJ)/gce.a      \
          $(OBJ)/kim1.a     \
          $(OBJ)/sharp.a    \
          $(OBJ)/lisa.a     \
          $(OBJ)/aquarius.a \
          #$(OBJ)/motorola.a \


$(OBJ)/coleco.a:   \
          $(OBJ)/mess/vidhrdw/tms9928a.o \
          $(OBJ)/mess/machine/coleco.o   \
          $(OBJ)/mess/vidhrdw/coleco.o   \
          $(OBJ)/mess/systems/coleco.o

$(OBJ)/sega.a:     \
          $(OBJ)/mess/vidhrdw/smsvdp.o   \
          $(OBJ)/mess/machine/sms.o      \
          $(OBJ)/mess/systems/sms.o      \
          $(OBJ)/mess/vidhrdw/genesis.o  \
          $(OBJ)/mess/machine/genesis.o  \
          $(OBJ)/mess/sndhrdw/genesis.o  \
          $(OBJ)/mess/systems/genesis.o

$(OBJ)/atari.a:    \
          $(OBJ)/mess/machine/atari.o    \
          $(OBJ)/mess/vidhrdw/antic.o    \
          $(OBJ)/mess/vidhrdw/gtia.o     \
          $(OBJ)/mess/vidhrdw/atari.o    \
          $(OBJ)/mess/systems/atari.o    \
          $(OBJ)/mess/vidhrdw/a7800.o    \
          $(OBJ)/mess/machine/a7800.o    \
          $(OBJ)/mess/systems/a7800.o    \
          $(OBJ)/mess/machine/riot.o     \
          $(OBJ)/mess/machine/a2600.o    \
          $(OBJ)/mess/systems/a2600.o

$(OBJ)/gce.a:      \
          $(OBJ)/mess/vidhrdw/vectrex.o  \
          $(OBJ)/mess/machine/vectrex.o  \
          $(OBJ)/mess/systems/vectrex.o

$(OBJ)/nintendo.a: \
          $(OBJ)/mess/machine/nes_mmc.o  \
          $(OBJ)/mess/vidhrdw/nes.o      \
          $(OBJ)/mess/machine/nes.o      \
          $(OBJ)/mess/systems/nes.o      \
          $(OBJ)/mess/vidhrdw/gb.o       \
          $(OBJ)/mess/machine/gb.o       \
          $(OBJ)/mess/systems/gb.o

$(OBJ)/cbm.a:      \
          $(OBJ)/mess/vidhrdw/amiga.o    \
          $(OBJ)/mess/machine/amiga.o    \
          $(OBJ)/mess/systems/amiga.o    \
          $(OBJ)/mess/vidhrdw/crtc6845.o \
          $(OBJ)/mess/machine/tpi6525.o  \
          $(OBJ)/mess/machine/cbmieeeb.o \
          $(OBJ)/mess/vidhrdw/pet.o      \
          $(OBJ)/mess/systems/pet.o      \
          $(OBJ)/mess/machine/pet.o      \
          $(OBJ)/mess/systems/cbmb.o     \
          $(OBJ)/mess/machine/cbmb.o     \
          $(OBJ)/mess/vidhrdw/vic6560.o  \
          $(OBJ)/mess/sndhrdw/vic6560.o  \
          $(OBJ)/mess/machine/vc20.o     \
          $(OBJ)/mess/machine/vc20tape.o \
          $(OBJ)/mess/machine/vc1541.o   \
          $(OBJ)/mess/systems/vc20.o     \
          $(OBJ)/mess/vidhrdw/ted7360.o  \
          $(OBJ)/mess/sndhrdw/ted7360.o  \
          $(OBJ)/mess/machine/c1551.o    \
          $(OBJ)/mess/machine/c16.o      \
          $(OBJ)/mess/systems/c16.o      \
          $(OBJ)/mess/machine/cbm.o      \
          $(OBJ)/mess/machine/cbmdrive.o \
          $(OBJ)/mess/systems/c64.o      \
          $(OBJ)/mess/machine/cia6526.o  \
          $(OBJ)/mess/machine/c64.o      \
          $(OBJ)/mess/vidhrdw/vic6567.o  \
          $(OBJ)/mess/sndhrdw/sid6581.o  \
          $(OBJ)/mess/systems/c65.o      \
          $(OBJ)/mess/machine/c65.o      \
          $(OBJ)/mess/systems/c128.o     \
          $(OBJ)/mess/vidhrdw/vdc8563.o  \
          $(OBJ)/mess/vidhrdw/praster.o  \
          $(OBJ)/mess/machine/c128.o     \
          $(OBJ)/mess/sndhrdw/mixing.o   \
          $(OBJ)/mess/sndhrdw/envelope.o \
          $(OBJ)/mess/sndhrdw/samples.o  \
          $(OBJ)/mess/sndhrdw/6581_.o

$(OBJ)/dragon.a:   \
          $(OBJ)/mess/formats/cocopak.o  \
          $(OBJ)/mess/vidhrdw/m6847.o    \
          $(OBJ)/mess/machine/mc10.o     \
          $(OBJ)/mess/systems/mc10.o     \
          $(OBJ)/mess/vidhrdw/dragon.o   \
          $(OBJ)/mess/machine/dragon.o   \
          $(OBJ)/mess/systems/dragon.o

$(OBJ)/trs80.a:    \
          $(OBJ)/mess/machine/trs80.o    \
          $(OBJ)/mess/vidhrdw/trs80.o    \
          $(OBJ)/mess/systems/trs80.o

$(OBJ)/cgenie.a:   \
          $(OBJ)/mess/machine/cgenie.o   \
          $(OBJ)/mess/vidhrdw/cgenie.o   \
          $(OBJ)/mess/sndhrdw/cgenie.o   \
          $(OBJ)/mess/systems/cgenie.o

$(OBJ)/pdp1.a:     \
          $(OBJ)/mess/vidhrdw/pdp1.o     \
          $(OBJ)/mess/machine/pdp1.o     \
          $(OBJ)/mess/systems/pdp1.o

$(OBJ)/kaypro.a:   \
          $(OBJ)/mess/machine/cpm_bios.o \
          $(OBJ)/mess/vidhrdw/kaypro.o   \
          $(OBJ)/mess/sndhrdw/kaypro.o   \
          $(OBJ)/mess/machine/kaypro.o   \
          $(OBJ)/mess/systems/kaypro.o   \

$(OBJ)/sinclair.a: \
          $(OBJ)/mess/eventlst.o \
          $(OBJ)/mess/vidhrdw/border.o   \
          $(OBJ)/mess/vidhrdw/spectrum.o \
          $(OBJ)/mess/machine/spectrum.o \
          $(OBJ)/mess/systems/spectrum.o \
          $(OBJ)/mess/vidhrdw/zx.o       \
          $(OBJ)/mess/machine/zx.o       \
          $(OBJ)/mess/systems/zx.o

$(OBJ)/apple1.a:   \
          $(OBJ)/machine/6821pia.o       \
          $(OBJ)/mess/vidhrdw/apple1.o   \
          $(OBJ)/mess/machine/apple1.o   \
          $(OBJ)/mess/systems/apple1.o

$(OBJ)/apple2.a:   \
          $(OBJ)/mess/machine/ay3600.o   \
          $(OBJ)/mess/machine/ap_disk2.o \
          $(OBJ)/mess/vidhrdw/apple2.o   \
          $(OBJ)/mess/machine/apple2.o   \
          $(OBJ)/mess/systems/apple2.o

$(OBJ)/mac.a: \
          $(OBJ)/mess/sndhrdw/mac.o      \
          $(OBJ)/mess/machine/iwm.o      \
          $(OBJ)/mess/vidhrdw/mac.o      \
          $(OBJ)/mess/machine/mac.o      \
          $(OBJ)/mess/systems/mac.o

$(OBJ)/ti99.a:     \
          $(OBJ)/mess/machine/tms9901.o  \
          $(OBJ)/mess/machine/ti99_4x.o  \
          $(OBJ)/mess/systems/ti99_4x.o  \
         #$(OBJ)/mess/systems/ti99_2.o   \
         #$(OBJ)/mess/systems/ti990_4.o  \


$(OBJ)/bally.a:    \
          $(OBJ)/sound/astrocde.o        \
          $(OBJ)/mess/vidhrdw/astrocde.o \
          $(OBJ)/mess/machine/astrocde.o \
          $(OBJ)/mess/systems/astrocde.o

$(OBJ)/pc.a:       \
          $(OBJ)/mess/machine/pit8253.o  \
          $(OBJ)/mess/machine/uart8250.o \
          $(OBJ)/mess/machine/tandy1t.o  \
          $(OBJ)/mess/machine/amstr_pc.o \
          $(OBJ)/mess/machine/at.o       \
          $(OBJ)/mess/machine/dma8237.o  \
          $(OBJ)/mess/machine/pic8259.o  \
          $(OBJ)/mess/machine/mc146818.o \
          $(OBJ)/mess/machine/pc_fdc_h.o \
          $(OBJ)/mess/machine/pc_flopp.o \
          $(OBJ)/mess/machine/pc_mouse.o \
          $(OBJ)/mess/machine/pckeybrd.o \
          $(OBJ)/mess/machine/pclpt.o    \
          $(OBJ)/mess/vidhrdw/vga.o      \
          $(OBJ)/mess/sndhrdw/pc.o       \
          $(OBJ)/mess/vidhrdw/pc_cga.o   \
          $(OBJ)/mess/vidhrdw/pc_mda.o   \
          $(OBJ)/mess/vidhrdw/pc_t1t.o   \
          $(OBJ)/mess/machine/pc.o       \
          $(OBJ)/mess/machine/pc_fdc.o   \
          $(OBJ)/mess/machine/pc_hdc.o   \
          $(OBJ)/mess/machine/pc_ide.o   \
          $(OBJ)/mess/systems/pc.o

$(OBJ)/p2000.a:    \
          $(OBJ)/mess/vidhrdw/saa5050.o   \
          $(OBJ)/mess/vidhrdw/p2000m.o    \
          $(OBJ)/mess/systems/p2000t.o    \
          $(OBJ)/mess/machine/p2000t.o    \
          $(OBJ)/mess/machine/mc6850.o    \
          $(OBJ)/mess/vidhrdw/uk101.o     \
          $(OBJ)/mess/machine/uk101.o     \
          $(OBJ)/mess/systems/uk101.o

$(OBJ)/amstrad.a:  \
          $(OBJ)/machine/8255ppi.o       \
	  $(OBJ)/mess/machine/flopdrv.o  \
          $(OBJ)/mess/vidhrdw/amstrad.o  \
          $(OBJ)/mess/vidhrdw/kc.o       \
          $(OBJ)/mess/machine/amstrad.o  \
          $(OBJ)/mess/machine/kc.o       \
          $(OBJ)/mess/systems/amstrad.o  \
          $(OBJ)/mess/vidhrdw/pcw.o      \
          $(OBJ)/mess/systems/pcw.o      \
          $(OBJ)/mess/systems/pcw16.o    \
          $(OBJ)/mess/vidhrdw/pcw16.o

$(OBJ)/nec.a:      \
          $(OBJ)/mess/vidhrdw/vdc.o      \
          $(OBJ)/mess/machine/pce.o      \
          $(OBJ)/mess/systems/pce.o

$(OBJ)/ep128.a :   \
          $(OBJ)/mess/sndhrdw/dave.o     \
          $(OBJ)/mess/vidhrdw/epnick.o   \
          $(OBJ)/mess/vidhrdw/enterp.o   \
          $(OBJ)/mess/machine/enterp.o   \
          $(OBJ)/mess/systems/enterp.o

$(OBJ)/ascii.a :   \
          $(OBJ)/mess/sndhrdw/scc.o      \
          $(OBJ)/mess/machine/msx.o      \
          $(OBJ)/mess/systems/msx.o

$(OBJ)/kim1.a :    \
          $(OBJ)/mess/vidhrdw/kim1.o     \
          $(OBJ)/mess/machine/kim1.o     \
          $(OBJ)/mess/systems/kim1.o

$(OBJ)/tangerin.a :\
          $(OBJ)/mess/vidhrdw/oric.o     \
          $(OBJ)/mess/machine/oric.o     \
          $(OBJ)/mess/systems/oric.o     \
          $(OBJ)/mess/vidhrdw/microtan.o \
          $(OBJ)/mess/machine/microtan.o \
          $(OBJ)/mess/systems/microtan.o

$(OBJ)/vtech.a :   \
          $(OBJ)/mess/vidhrdw/vtech1.o   \
          $(OBJ)/mess/machine/vtech1.o   \
          $(OBJ)/mess/systems/vtech1.o   \
          $(OBJ)/mess/vidhrdw/vtech2.o   \
          $(OBJ)/mess/machine/vtech2.o   \
          $(OBJ)/mess/systems/vtech2.o

$(OBJ)/jupiter.a : \
          $(OBJ)/mess/vidhrdw/jupiter.o  \
          $(OBJ)/mess/machine/jupiter.o  \
          $(OBJ)/mess/systems/jupiter.o

$(OBJ)/mbee.a :    \
          $(OBJ)/mess/vidhrdw/mbee.o     \
          $(OBJ)/mess/machine/mbee.o     \
          $(OBJ)/mess/systems/mbee.o


$(OBJ)/advision.a: \
          $(OBJ)/mess/vidhrdw/advision.o \
          $(OBJ)/mess/machine/advision.o \
          $(OBJ)/mess/systems/advision.o

$(OBJ)/nascom1.a:  \
          $(OBJ)/mess/vidhrdw/nascom1.o  \
          $(OBJ)/mess/machine/nascom1.o  \
          $(OBJ)/mess/systems/nascom1.o

$(OBJ)/bbc.a:      \
          $(OBJ)/mess/machine/i8271.o    \
          $(OBJ)/mess/vidhrdw/m6845.o    \
          $(OBJ)/mess/vidhrdw/bbc.o      \
          $(OBJ)/mess/machine/bbc.o      \
          $(OBJ)/mess/systems/bbc.o

$(OBJ)/cpschngr.a: \
          $(OBJ)/machine/eeprom.o        \
          $(OBJ)/vidhrdw/cps1.o          \
          $(OBJ)/mess/systems/cpschngr.o

$(OBJ)/mtx.a:      \
          $(OBJ)/mess/systems/mtx.o

$(OBJ)/lisa.a:     \
          $(OBJ)/mess/machine/iwm_lisa.o \
          $(OBJ)/mess/machine/lisa.o     \
          $(OBJ)/mess/systems/lisa.o

$(OBJ)/acorn.a:    \
          $(OBJ)/mess/machine/atom.o     \
          $(OBJ)/mess/vidhrdw/atom.o     \
	  $(OBJ)/mess/systems/atom.o	 \
	  $(OBJ)/mess/systems/a310.o

$(OBJ)/samcoupe.a: \
          $(OBJ)/mess/machine/coupe.o    \
          $(OBJ)/mess/vidhrdw/coupe.o    \
          $(OBJ)/mess/systems/coupe.o

$(OBJ)/sharp.a:    \
          $(OBJ)/mess/sndhrdw/mz700.o    \
          $(OBJ)/mess/machine/mz700.o    \
          $(OBJ)/mess/vidhrdw/mz700.o    \
          $(OBJ)/mess/systems/mz700.o    \
          $(OBJ)/mess/machine/pocketc.o  \
          $(OBJ)/mess/vidhrdw/pocketc.o  \
          $(OBJ)/mess/systems/pocketc.o

$(OBJ)/aquarius.a: \
          $(OBJ)/mess/machine/aquarius.o \
          $(OBJ)/mess/vidhrdw/aquarius.o \
          $(OBJ)/mess/systems/aquarius.o

$(OBJ)/motorola.a: \
          $(OBJ)/mess/vidhrdw/mekd2.o    \
          $(OBJ)/mess/machine/mekd2.o    \
          $(OBJ)/mess/systems/mekd2.o



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
          $(OBJ)/mess/sndhrdw/beep.o     \

# additional tools
TOOLS += dat2html$(EXE) mkhdimg$(EXE) imgtool$(EXE)

dat2html$(EXE): $(OBJ)/mess/tools/dat2html.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ -lz -o $@

mkhdimg$(EXE):  $(OBJ)/mess/tools/mkhdimg.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ -lz -o $@

imgtool$(EXE):       \
          $(OBJ)/mess/tools/stubs.o   \
          $(OBJ)/mess/config.o        \
          $(OBJ)/unzip.o              \
          $(OBJ)/mess/tools/main.o    \
          $(OBJ)/mess/tools/imgtool.o \
          $(OBJ)/mess/tools/rsdos.o   \
          $(OBJ)/mess/tools/stream.o  \
          $(OBJ)/mess/tools/crt.o     \
          $(OBJ)/mess/tools/t64.o     \
          $(OBJ)/mess/tools/pchd.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ -lz -o $@


# text files
TEXTS = mess.txt
mess.txt: $(EMULATOR)
	@echo Generating $@...
	@$(EMULATOR) -listtext > mess.txt
	@$(EMULATOR) -listdevices >> mess.txt

# makedepend -f src/mess.mak -pmess.obj/ -Y. -Isrc -Imess mess/systems/*.c
#    mess/machine/*.c mess/vidhrdw/*.c mess/sndhrdw/*.c mess/formats/*.c mess/tools/*.c
# DO NOT DELETE

mess.obj/mess/systems/a2600.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/a2600.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/a2600.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/a2600.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/a2600.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/a2600.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/a2600.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/a2600.o: src/vidhrdw/generic.h src/cpu/m6502/m6502.h
mess.obj/mess/systems/a2600.o: src/sound/tiaintf.h src/machine/6821pia.h
mess.obj/mess/systems/a2600.o: src/sound/hc55516.h mess/machine/riot.h
mess.obj/mess/systems/a310.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/a310.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/a310.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/a310.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/a310.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/a310.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/a310.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/a310.o: src/vidhrdw/generic.h
mess.obj/mess/systems/a7800.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/a7800.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/a7800.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/a7800.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/a7800.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/a7800.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/a7800.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/a7800.o: src/vidhrdw/generic.h src/cpu/m6502/m6502.h
mess.obj/mess/systems/a7800.o: src/sound/tiaintf.h
mess.obj/mess/systems/advision.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/advision.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/advision.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/advision.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/advision.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/advision.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/advision.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/advision.o: src/cpu/i8039/i8039.h src/vidhrdw/generic.h
mess.obj/mess/systems/advision.o: mess/includes/advision.h
mess.obj/mess/systems/amiga.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/amiga.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/amiga.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/amiga.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/amiga.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/amiga.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/amiga.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/amiga.o: src/vidhrdw/generic.h
mess.obj/mess/systems/amstrad.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/amstrad.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/amstrad.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/amstrad.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/amstrad.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/amstrad.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/amstrad.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/amstrad.o: src/machine/8255ppi.h mess/vidhrdw/m6845.h
mess.obj/mess/systems/amstrad.o: mess/includes/amstrad.h
mess.obj/mess/systems/amstrad.o: mess/includes/nec765.h
mess.obj/mess/systems/amstrad.o: mess/includes/flopdrv.h mess/includes/dsk.h
mess.obj/mess/systems/amstrad.o: mess/eventlst.h
mess.obj/mess/systems/apple1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/apple1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/apple1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/apple1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/apple1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/apple1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/apple1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/apple1.o: src/cpu/m6502/m6502.h src/machine/6821pia.h
mess.obj/mess/systems/apple1.o: src/vidhrdw/generic.h mess/includes/apple1.h
mess.obj/mess/systems/apple2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/apple2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/apple2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/apple2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/apple2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/apple2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/apple2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/apple2.o: src/vidhrdw/generic.h
mess.obj/mess/systems/aquarius.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/aquarius.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/aquarius.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/aquarius.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/aquarius.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/aquarius.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/aquarius.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/aquarius.o: src/cpu/z80/z80.h src/vidhrdw/generic.h
mess.obj/mess/systems/aquarius.o: mess/includes/aquarius.h
mess.obj/mess/systems/astrocde.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/astrocde.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/astrocde.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/astrocde.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/astrocde.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/astrocde.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/astrocde.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/astrocde.o: src/sound/astrocde.h src/vidhrdw/generic.h
mess.obj/mess/systems/atari.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/atari.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/atari.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/atari.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/atari.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/atari.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/atari.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/atari.o: src/cpu/m6502/m6502.h mess/machine/atari.h
mess.obj/mess/systems/atari.o: mess/vidhrdw/atari.h
mess.obj/mess/systems/atom.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/atom.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/atom.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/atom.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/atom.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/atom.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/atom.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/atom.o: src/vidhrdw/generic.h src/machine/8255ppi.h
mess.obj/mess/systems/atom.o: mess/vidhrdw/m6847.h mess/includes/atom.h
mess.obj/mess/systems/bbc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/bbc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/bbc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/bbc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/bbc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/bbc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/bbc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/bbc.o: src/cpu/m6502/m6502.h mess/machine/bbc.h
mess.obj/mess/systems/bbc.o: mess/vidhrdw/bbc.h mess/machine/6522via.h
mess.obj/mess/systems/c128.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/c128.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/c128.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/c128.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/c128.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/c128.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/c128.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/c128.o: mess/includes/cbm.h mess/includes/cia6526.h
mess.obj/mess/systems/c128.o: mess/includes/vic6567.h mess/includes/praster.h
mess.obj/mess/systems/c128.o: mess/includes/vdc8563.h mess/includes/sid6581.h
mess.obj/mess/systems/c128.o: mess/includes/c1551.h mess/includes/cbmdrive.h
mess.obj/mess/systems/c128.o: mess/includes/vc1541.h mess/machine/6522via.h
mess.obj/mess/systems/c128.o: mess/includes/vc20tape.h mess/includes/c128.h
mess.obj/mess/systems/c128.o: mess/includes/c64.h mess/includes/cia6526.h
mess.obj/mess/systems/c16.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/c16.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/c16.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/c16.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/c16.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/c16.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/c16.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/c16.o: mess/includes/cbm.h mess/includes/c16.h
mess.obj/mess/systems/c16.o: mess/includes/c1551.h mess/includes/cbmdrive.h
mess.obj/mess/systems/c16.o: mess/includes/c1551.h mess/includes/vc1541.h
mess.obj/mess/systems/c16.o: mess/machine/6522via.h mess/includes/vc20tape.h
mess.obj/mess/systems/c16.o: mess/includes/ted7360.h mess/includes/sid6581.h
mess.obj/mess/systems/c64.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/c64.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/c64.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/c64.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/c64.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/c64.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/c64.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/c64.o: mess/includes/cbm.h mess/includes/cia6526.h
mess.obj/mess/systems/c64.o: mess/includes/vic6567.h mess/includes/praster.h
mess.obj/mess/systems/c64.o: mess/includes/sid6581.h mess/includes/c1551.h
mess.obj/mess/systems/c64.o: mess/includes/cbmdrive.h mess/includes/vc1541.h
mess.obj/mess/systems/c64.o: mess/machine/6522via.h mess/includes/vc20tape.h
mess.obj/mess/systems/c64.o: mess/includes/c64.h mess/includes/cia6526.h
mess.obj/mess/systems/c65.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/c65.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/c65.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/c65.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/c65.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/c65.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/c65.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/c65.o: mess/includes/cbm.h mess/includes/cia6526.h
mess.obj/mess/systems/c65.o: mess/includes/vic6567.h mess/includes/praster.h
mess.obj/mess/systems/c65.o: mess/includes/sid6581.h mess/includes/c1551.h
mess.obj/mess/systems/c65.o: mess/includes/cbmdrive.h mess/includes/vc1541.h
mess.obj/mess/systems/c65.o: mess/machine/6522via.h mess/includes/c65.h
mess.obj/mess/systems/c65.o: mess/includes/c64.h mess/includes/cia6526.h
mess.obj/mess/systems/cbmb.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/cbmb.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/cbmb.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/cbmb.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/cbmb.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/cbmb.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/cbmb.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/cbmb.o: src/cpu/m6502/m6509.h src/cpu/m6502/m6502.h
mess.obj/mess/systems/cbmb.o: mess/includes/cbm.h mess/includes/cia6526.h
mess.obj/mess/systems/cbmb.o: mess/includes/tpi6525.h mess/includes/vic6567.h
mess.obj/mess/systems/cbmb.o: mess/includes/praster.h
mess.obj/mess/systems/cbmb.o: mess/includes/crtc6845.h
mess.obj/mess/systems/cbmb.o: mess/includes/sid6581.h mess/includes/c1551.h
mess.obj/mess/systems/cbmb.o: mess/includes/cbmdrive.h mess/includes/vc1541.h
mess.obj/mess/systems/cbmb.o: mess/machine/6522via.h mess/includes/vc20tape.h
mess.obj/mess/systems/cbmb.o: mess/includes/cbmb.h mess/includes/cia6526.h
mess.obj/mess/systems/cgenie.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/cgenie.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/cgenie.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/cgenie.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/cgenie.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/cgenie.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/cgenie.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/cgenie.o: src/vidhrdw/generic.h mess/vidhrdw/cgenie.h
mess.obj/mess/systems/coleco.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/coleco.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/coleco.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/coleco.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/coleco.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/coleco.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/coleco.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/coleco.o: src/vidhrdw/generic.h src/sound/sn76496.h
mess.obj/mess/systems/coleco.o: mess/vidhrdw/tms9928a.h
mess.obj/mess/systems/coleco.o: mess/includes/coleco.h
mess.obj/mess/systems/coupe.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/coupe.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/coupe.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/coupe.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/coupe.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/coupe.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/coupe.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/coupe.o: src/cpu/z80/z80.h src/vidhrdw/generic.h
mess.obj/mess/systems/coupe.o: mess/includes/coupe.h
mess.obj/mess/systems/cpschngr.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/cpschngr.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/cpschngr.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/cpschngr.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/cpschngr.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/cpschngr.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/cpschngr.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/cpschngr.o: src/vidhrdw/generic.h src/machine/eeprom.h
mess.obj/mess/systems/cpschngr.o: src/drivers/cps1.h src/drivers/cps2.c
mess.obj/mess/systems/dragon.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/dragon.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/dragon.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/dragon.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/dragon.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/dragon.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/dragon.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/dragon.o: src/machine/6821pia.h mess/vidhrdw/m6847.h
mess.obj/mess/systems/dragon.o: src/vidhrdw/generic.h
mess.obj/mess/systems/enterp.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/enterp.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/enterp.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/enterp.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/enterp.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/enterp.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/enterp.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/enterp.o: mess/sndhrdw/dave.h mess/machine/enterp.h
mess.obj/mess/systems/enterp.o: mess/vidhrdw/enterp.h mess/vidhrdw/epnick.h
mess.obj/mess/systems/enterp.o: mess/machine/wd179x.h
mess.obj/mess/systems/gb.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/gb.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/gb.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/gb.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/gb.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/gb.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/gb.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/gb.o: src/vidhrdw/generic.h mess/machine/gb.h
mess.obj/mess/systems/genesis.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/genesis.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/genesis.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/genesis.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/genesis.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/genesis.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/genesis.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/genesis.o: src/vidhrdw/generic.h mess/machine/genesis.h
mess.obj/mess/systems/genesis.o: mess/vidhrdw/genesis.h src/sound/2612intf.h
mess.obj/mess/systems/genesis.o: src/sound/fm.h src/sound/ay8910.h
mess.obj/mess/systems/jupiter.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/jupiter.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/jupiter.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/jupiter.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/jupiter.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/jupiter.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/jupiter.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/jupiter.o: src/cpu/z80/z80.h src/vidhrdw/generic.h
mess.obj/mess/systems/jupiter.o: mess/includes/jupiter.h
mess.obj/mess/systems/kaypro.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/kaypro.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/kaypro.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/kaypro.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/kaypro.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/kaypro.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/kaypro.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/kaypro.o: mess/includes/kaypro.h
mess.obj/mess/systems/kaypro.o: mess/machine/cpm_bios.h
mess.obj/mess/systems/kim1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/kim1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/kim1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/kim1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/kim1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/kim1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/kim1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/lisa.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/lisa.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/lisa.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/lisa.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/lisa.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/lisa.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/lisa.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/lisa.o: src/vidhrdw/generic.h mess/machine/lisa.h
mess.obj/mess/systems/mac.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/mac.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/mac.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/mac.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/mac.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/mac.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/mac.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/mac.o: src/vidhrdw/generic.h mess/machine/6522via.h
mess.obj/mess/systems/mac.o: mess/includes/mac.h
mess.obj/mess/systems/mbee.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/mbee.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/mbee.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/mbee.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/mbee.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/mbee.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/mbee.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/mbee.o: src/machine/z80fmly.h src/vidhrdw/generic.h
mess.obj/mess/systems/mbee.o: mess/machine/wd179x.h mess/machine/mbee.h
mess.obj/mess/systems/mc10.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/mc10.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/mc10.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/mc10.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/mc10.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/mc10.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/mc10.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/mc10.o: src/cpu/m6800/m6800.h mess/vidhrdw/m6847.h
mess.obj/mess/systems/mc10.o: src/vidhrdw/generic.h
mess.obj/mess/systems/mekd2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/mekd2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/mekd2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/mekd2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/mekd2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/mekd2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/mekd2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/mekd2.o: src/vidhrdw/generic.h
mess.obj/mess/systems/microtan.o: mess/includes/microtan.h src/driver.h
mess.obj/mess/systems/microtan.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/systems/microtan.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/systems/microtan.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/systems/microtan.o: src/timer.h src/sndintrf.h
mess.obj/mess/systems/microtan.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/microtan.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/microtan.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/microtan.o: src/vidhrdw/generic.h src/cpu/m6502/m6502.h
mess.obj/mess/systems/microtan.o: mess/machine/6522via.h src/sound/ay8910.h
mess.obj/mess/systems/msx.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/msx.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/msx.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/msx.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/msx.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/msx.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/msx.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/msx.o: src/vidhrdw/generic.h src/machine/8255ppi.h
mess.obj/mess/systems/msx.o: mess/vidhrdw/tms9928a.h mess/machine/msx.h
mess.obj/mess/systems/msx.o: mess/sndhrdw/scc.h
mess.obj/mess/systems/mtx.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/mtx.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/mtx.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/mtx.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/mtx.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/mtx.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/mtx.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/mtx.o: src/vidhrdw/generic.h src/machine/z80fmly.h
mess.obj/mess/systems/mtx.o: mess/vidhrdw/tms9928a.h src/sound/sn76496.h
mess.obj/mess/systems/mtx.o: src/cpu/z80/z80.h
mess.obj/mess/systems/mz700.o: mess/includes/mz700.h src/driver.h
mess.obj/mess/systems/mz700.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/systems/mz700.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/systems/mz700.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/systems/mz700.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/systems/mz700.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/systems/mz700.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/systems/mz700.o: src/profiler.h src/vidhrdw/generic.h
mess.obj/mess/systems/mz700.o: src/cpu/z80/z80.h src/machine/8255ppi.h
mess.obj/mess/systems/mz700.o: mess/includes/pit8253.h
mess.obj/mess/systems/nascom1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/nascom1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/nascom1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/nascom1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/nascom1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/nascom1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/nascom1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/nascom1.o: src/cpu/z80/z80.h src/vidhrdw/generic.h
mess.obj/mess/systems/nascom1.o: mess/includes/nascom1.h
mess.obj/mess/systems/nes.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/nes.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/nes.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/nes.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/nes.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/nes.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/nes.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/nes.o: src/vidhrdw/generic.h mess/includes/nes.h
mess.obj/mess/systems/nes.o: src/cpu/m6502/m6502.h
mess.obj/mess/systems/oric.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/oric.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/oric.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/oric.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/oric.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/oric.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/oric.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/oric.o: src/vidhrdw/generic.h
mess.obj/mess/systems/p2000t.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/p2000t.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/p2000t.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/p2000t.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/p2000t.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/p2000t.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/p2000t.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/p2000t.o: src/cpu/z80/z80.h src/vidhrdw/generic.h
mess.obj/mess/systems/p2000t.o: mess/includes/saa5050.h
mess.obj/mess/systems/p2000t.o: mess/includes/p2000t.h
mess.obj/mess/systems/pc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/pc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/pc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/pc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/pc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/pc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/pc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/pc.o: src/sound/3812intf.h src/machine/8255ppi.h
mess.obj/mess/systems/pc.o: mess/includes/uart8250.h mess/includes/pic8259.h
mess.obj/mess/systems/pc.o: mess/includes/dma8237.h mess/includes/pit8253.h
mess.obj/mess/systems/pc.o: mess/includes/mc146818.h mess/includes/vga.h
mess.obj/mess/systems/pc.o: mess/includes/pc_cga.h mess/includes/pc_mda.h
mess.obj/mess/systems/pc.o: mess/includes/pc_t1t.h mess/includes/pc_fdc_h.h
mess.obj/mess/systems/pc.o: mess/includes/pc_flopp.h mess/includes/pckeybrd.h
mess.obj/mess/systems/pc.o: mess/includes/pclpt.h mess/includes/pc_mouse.h
mess.obj/mess/systems/pc.o: mess/includes/tandy1t.h mess/includes/amstr_pc.h
mess.obj/mess/systems/pc.o: mess/includes/at.h mess/includes/pc.h
mess.obj/mess/systems/pc.o: src/cpu/i86/i86intf.h src/vidhrdw/generic.h
mess.obj/mess/systems/pce.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/pce.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/pce.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/pce.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/pce.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/pce.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/pce.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/pce.o: src/vidhrdw/generic.h mess/vidhrdw/vdc.h
mess.obj/mess/systems/pce.o: src/cpu/h6280/h6280.h
mess.obj/mess/systems/pcw.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/pcw.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/pcw.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/pcw.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/pcw.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/pcw.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/pcw.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/pcw.o: mess/includes/nec765.h mess/includes/flopdrv.h
mess.obj/mess/systems/pcw.o: mess/includes/dsk.h mess/includes/pcw.h
mess.obj/mess/systems/pcw.o: mess/includes/beep.h
mess.obj/mess/systems/pcw16.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/pcw16.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/pcw16.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/pcw16.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/pcw16.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/pcw16.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/pcw16.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/pcw16.o: mess/includes/pcw16.h mess/includes/pclpt.h
mess.obj/mess/systems/pcw16.o: mess/includes/pckeybrd.h
mess.obj/mess/systems/pcw16.o: mess/includes/pc_fdc_h.h
mess.obj/mess/systems/pcw16.o: mess/includes/pc_flopp.h
mess.obj/mess/systems/pcw16.o: mess/includes/uart8250.h
mess.obj/mess/systems/pcw16.o: mess/includes/pc_mouse.h mess/includes/beep.h
mess.obj/mess/systems/pdp1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/pdp1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/pdp1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/pdp1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/pdp1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/pdp1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/pdp1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/pdp1.o: src/vidhrdw/generic.h mess/vidhrdw/pdp1.h
mess.obj/mess/systems/pet.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/pet.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/pet.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/pet.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/pet.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/pet.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/pet.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/pet.o: mess/includes/cbm.h src/machine/6821pia.h
mess.obj/mess/systems/pet.o: mess/machine/6522via.h mess/includes/pet.h
mess.obj/mess/systems/pet.o: mess/includes/praster.h mess/includes/crtc6845.h
mess.obj/mess/systems/pet.o: mess/includes/crtc6845.h mess/includes/c1551.h
mess.obj/mess/systems/pet.o: mess/includes/cbmdrive.h
mess.obj/mess/systems/pet.o: mess/includes/cbmieeeb.h
mess.obj/mess/systems/pocketc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/pocketc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/pocketc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/pocketc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/pocketc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/pocketc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/pocketc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/pocketc.o: src/cpu/sc61860/sc61860.h
mess.obj/mess/systems/pocketc.o: mess/includes/pocketc.h
mess.obj/mess/systems/sms.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/sms.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/sms.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/sms.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/sms.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/sms.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/sms.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/sms.o: src/sound/sn76496.h src/sound/2413intf.h
mess.obj/mess/systems/sms.o: src/vidhrdw/generic.h mess/vidhrdw/smsvdp.h
mess.obj/mess/systems/sms.o: mess/machine/sms.h
mess.obj/mess/systems/spectrum.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/spectrum.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/spectrum.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/spectrum.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/spectrum.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/spectrum.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/spectrum.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/spectrum.o: src/vidhrdw/generic.h
mess.obj/mess/systems/spectrum.o: mess/includes/spectrum.h mess/eventlst.h
mess.obj/mess/systems/spectrum.o: mess/includes/nec765.h
mess.obj/mess/systems/spectrum.o: mess/includes/flopdrv.h mess/includes/dsk.h
mess.obj/mess/systems/ti990_4.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/ti990_4.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/ti990_4.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/ti990_4.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/ti990_4.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/ti990_4.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/ti990_4.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/ti990_4.o: src/vidhrdw/generic.h
mess.obj/mess/systems/ti990_4.o: src/cpu/tms9900/tms9900.h
mess.obj/mess/systems/ti99_2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/ti99_2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/ti99_2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/ti99_2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/ti99_2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/ti99_2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/ti99_2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/ti99_2.o: src/vidhrdw/generic.h mess/machine/tms9901.h
mess.obj/mess/systems/ti99_2.o: mess/vidhrdw/tms9928a.h
mess.obj/mess/systems/ti99_2.o: src/cpu/tms9900/tms9900.h
mess.obj/mess/systems/ti99_4x.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/ti99_4x.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/ti99_4x.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/ti99_4x.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/ti99_4x.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/ti99_4x.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/ti99_4x.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/ti99_4x.o: mess/vidhrdw/tms9928a.h
mess.obj/mess/systems/ti99_4x.o: mess/machine/ti99_4x.h
mess.obj/mess/systems/ti99_4x.o: mess/machine/tms9901.h
mess.obj/mess/systems/trs80.o: mess/includes/trs80.h src/driver.h
mess.obj/mess/systems/trs80.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/systems/trs80.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/systems/trs80.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/systems/trs80.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/systems/trs80.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/systems/trs80.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/systems/trs80.o: src/profiler.h src/cpu/z80/z80.h
mess.obj/mess/systems/trs80.o: src/vidhrdw/generic.h mess/machine/wd179x.h
mess.obj/mess/systems/uk101.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/uk101.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/uk101.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/uk101.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/uk101.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/uk101.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/uk101.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/uk101.o: src/cpu/m6502/m6502.h src/vidhrdw/generic.h
mess.obj/mess/systems/uk101.o: mess/machine/mc6850.h mess/includes/uk101.h
mess.obj/mess/systems/vc20.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/vc20.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/vc20.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/vc20.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/vc20.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/vc20.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/vc20.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/vc20.o: mess/includes/cbm.h mess/includes/vc20.h
mess.obj/mess/systems/vc20.o: mess/includes/c1551.h mess/includes/cbmdrive.h
mess.obj/mess/systems/vc20.o: mess/machine/6522via.h mess/includes/c1551.h
mess.obj/mess/systems/vc20.o: mess/includes/vc1541.h mess/includes/vc20tape.h
mess.obj/mess/systems/vc20.o: mess/includes/vic6560.h
mess.obj/mess/systems/vectrex.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/vectrex.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/vectrex.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/vectrex.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/vectrex.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/vectrex.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/vectrex.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/vectrex.o: src/vidhrdw/vector.h src/artwork.h
mess.obj/mess/systems/vectrex.o: mess/machine/6522via.h
mess.obj/mess/systems/vtech1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/vtech1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/vtech1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/vtech1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/vtech1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/vtech1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/vtech1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/vtech1.o: src/vidhrdw/generic.h
mess.obj/mess/systems/vtech2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/vtech2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/vtech2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/vtech2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/vtech2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/vtech2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/vtech2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/vtech2.o: src/vidhrdw/generic.h
mess.obj/mess/systems/zx.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/systems/zx.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/systems/zx.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/systems/zx.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/systems/zx.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/systems/zx.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/systems/zx.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/systems/zx.o: src/vidhrdw/generic.h src/cpu/z80/z80.h
mess.obj/mess/machine/6522via.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/6522via.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/6522via.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/6522via.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/6522via.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/6522via.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/6522via.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/6522via.o: mess/machine/6522via.h
mess.obj/mess/machine/a2600.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/a2600.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/a2600.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/a2600.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/a2600.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/a2600.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/a2600.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/a2600.o: mess/machine/riot.h src/sound/tiaintf.h
mess.obj/mess/machine/a2600.o: src/sound/tiasound.h mess/machine/tia.h
mess.obj/mess/machine/a7800.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/a7800.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/a7800.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/a7800.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/a7800.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/a7800.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/a7800.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/a7800.o: src/vidhrdw/generic.h src/cpu/m6502/m6502.h
mess.obj/mess/machine/a7800.o: src/sound/tiasound.h
mess.obj/mess/machine/advision.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/advision.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/advision.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/advision.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/advision.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/advision.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/advision.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/advision.o: src/vidhrdw/generic.h
mess.obj/mess/machine/advision.o: mess/includes/advision.h
mess.obj/mess/machine/amiga.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/amiga.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/amiga.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/amiga.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/amiga.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/amiga.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/amiga.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/amiga.o: mess/includes/amiga.h
mess.obj/mess/machine/amstr_pc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/amstr_pc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/amstr_pc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/amstr_pc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/amstr_pc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/amstr_pc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/amstr_pc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/amstr_pc.o: mess/includes/pit8253.h mess/includes/pc.h
mess.obj/mess/machine/amstr_pc.o: src/cpu/i86/i86intf.h src/vidhrdw/generic.h
mess.obj/mess/machine/amstr_pc.o: mess/includes/amstr_pc.h
mess.obj/mess/machine/amstr_pc.o: mess/includes/pclpt.h mess/includes/vga.h
mess.obj/mess/machine/amstrad.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/amstrad.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/amstrad.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/amstrad.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/amstrad.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/amstrad.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/amstrad.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/amstrad.o: src/cpu/z80/z80.h 
mess.obj/mess/machine/amstrad.o: src/machine/8255ppi.h mess/includes/nec765.h
mess.obj/mess/machine/amstrad.o: mess/includes/flopdrv.h mess/includes/dsk.h
mess.obj/mess/machine/ap_disk2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/ap_disk2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/ap_disk2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/ap_disk2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/ap_disk2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/ap_disk2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/ap_disk2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/ap_disk2.o: mess/includes/apple2.h
mess.obj/mess/machine/apple1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/apple1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/apple1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/apple1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/apple1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/apple1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/apple1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/apple1.o: src/cpu/m6502/m6502.h src/machine/6821pia.h
mess.obj/mess/machine/apple1.o: mess/includes/apple1.h
mess.obj/mess/machine/apple2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/apple2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/apple2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/apple2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/apple2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/apple2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/apple2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/apple2.o: src/vidhrdw/generic.h src/cpu/m6502/m6502.h
mess.obj/mess/machine/apple2.o: mess/includes/apple2.h
mess.obj/mess/machine/aquarius.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/aquarius.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/aquarius.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/aquarius.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/aquarius.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/aquarius.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/aquarius.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/aquarius.o: src/cpu/z80/z80.h mess/includes/aquarius.h
mess.obj/mess/machine/astrocde.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/astrocde.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/astrocde.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/astrocde.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/astrocde.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/astrocde.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/astrocde.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/astrocde.o: src/cpu/z80/z80.h
mess.obj/mess/machine/at.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/at.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/at.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/at.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/at.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/at.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/at.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/at.o: mess/mess.h src/cpu/i86/i286.h src/cpu/i86/i86.h
mess.obj/mess/machine/at.o: mess/includes/pic8259.h mess/includes/mc146818.h
mess.obj/mess/machine/at.o: mess/includes/vga.h mess/includes/pc_cga.h
mess.obj/mess/machine/at.o: mess/includes/pc.h src/cpu/i86/i86intf.h
mess.obj/mess/machine/at.o: src/vidhrdw/generic.h mess/includes/at.h
mess.obj/mess/machine/at.o: mess/includes/pckeybrd.h
mess.obj/mess/machine/atari.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/atari.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/atari.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/atari.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/atari.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/atari.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/atari.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/atari.o: src/cpu/m6502/m6502.h mess/machine/atari.h
mess.obj/mess/machine/atari.o: mess/vidhrdw/atari.h
mess.obj/mess/machine/atom.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/atom.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/atom.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/atom.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/atom.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/atom.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/atom.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/atom.o: src/cpu/z80/z80.h src/machine/8255ppi.h
mess.obj/mess/machine/atom.o: mess/vidhrdw/m6847.h src/vidhrdw/generic.h
mess.obj/mess/machine/atom.o: mess/includes/atom.h
mess.obj/mess/machine/ay3600.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/ay3600.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/ay3600.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/ay3600.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/ay3600.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/ay3600.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/ay3600.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/bbc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/bbc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/bbc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/bbc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/bbc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/bbc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/bbc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/bbc.o: src/cpu/m6502/m6502.h mess/machine/6522via.h
mess.obj/mess/machine/bbc.o: mess/machine/wd179x.h mess/machine/bbc.h
mess.obj/mess/machine/bbc.o: mess/vidhrdw/bbc.h mess/machine/i8271.h
mess.obj/mess/machine/c128.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/c128.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/c128.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/c128.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/c128.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/c128.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/c128.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/c128.o: src/cpu/m6502/m6502.h mess/includes/cbm.h
mess.obj/mess/machine/c128.o: mess/includes/cia6526.h mess/includes/c1551.h
mess.obj/mess/machine/c128.o: mess/includes/cbmdrive.h mess/includes/vc1541.h
mess.obj/mess/machine/c128.o: mess/machine/6522via.h mess/includes/vc20tape.h
mess.obj/mess/machine/c128.o: mess/includes/vic6567.h mess/includes/praster.h
mess.obj/mess/machine/c128.o: mess/includes/vdc8563.h mess/includes/sid6581.h
mess.obj/mess/machine/c128.o: mess/includes/praster.h mess/includes/c128.h
mess.obj/mess/machine/c128.o: mess/includes/c64.h mess/includes/cia6526.h
mess.obj/mess/machine/c1551.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/c1551.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/c1551.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/c1551.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/c1551.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/c1551.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/c1551.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/c1551.o: mess/includes/cbm.h mess/includes/cbmdrive.h
mess.obj/mess/machine/c1551.o: mess/includes/c1551.h mess/includes/cbmdrive.h
mess.obj/mess/machine/c16.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/c16.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/c16.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/c16.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/c16.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/c16.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/c16.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/c16.o: src/cpu/m6502/m6502.h mess/includes/cbm.h
mess.obj/mess/machine/c16.o: mess/includes/tpi6525.h mess/includes/c1551.h
mess.obj/mess/machine/c16.o: mess/includes/cbmdrive.h mess/includes/vc1541.h
mess.obj/mess/machine/c16.o: mess/machine/6522via.h mess/includes/vc20tape.h
mess.obj/mess/machine/c16.o: mess/includes/ted7360.h mess/includes/sid6581.h
mess.obj/mess/machine/c16.o: mess/includes/c16.h mess/includes/c1551.h
mess.obj/mess/machine/c64.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/c64.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/c64.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/c64.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/c64.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/c64.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/c64.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/c64.o: src/cpu/m6502/m6502.h src/cpu/z80/z80.h
mess.obj/mess/machine/c64.o: mess/includes/cbm.h mess/includes/cia6526.h
mess.obj/mess/machine/c64.o: mess/includes/c1551.h mess/includes/cbmdrive.h
mess.obj/mess/machine/c64.o: mess/includes/vc1541.h mess/machine/6522via.h
mess.obj/mess/machine/c64.o: mess/includes/vc20tape.h mess/includes/vic6567.h
mess.obj/mess/machine/c64.o: mess/includes/praster.h mess/includes/vdc8563.h
mess.obj/mess/machine/c64.o: mess/includes/sid6581.h mess/includes/c128.h
mess.obj/mess/machine/c64.o: mess/includes/c64.h mess/includes/cia6526.h
mess.obj/mess/machine/c64.o: mess/includes/c65.h mess/includes/c64.h
mess.obj/mess/machine/c65.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/c65.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/c65.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/c65.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/c65.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/c65.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/c65.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/c65.o: src/cpu/m6502/m6502.h mess/includes/cbm.h
mess.obj/mess/machine/c65.o: mess/includes/cia6526.h mess/includes/c1551.h
mess.obj/mess/machine/c65.o: mess/includes/cbmdrive.h mess/includes/vc1541.h
mess.obj/mess/machine/c65.o: mess/machine/6522via.h mess/includes/vic6567.h
mess.obj/mess/machine/c65.o: mess/includes/praster.h mess/includes/sid6581.h
mess.obj/mess/machine/c65.o: mess/includes/c65.h mess/includes/c64.h
mess.obj/mess/machine/c65.o: mess/includes/cia6526.h
mess.obj/mess/machine/cbm.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/cbm.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/cbm.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/cbm.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/cbm.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/cbm.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/cbm.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/cbm.o: mess/includes/cbm.h
mess.obj/mess/machine/cbmb.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/cbmb.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/cbmb.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/cbmb.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/cbmb.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/cbmb.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/cbmb.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/cbmb.o: src/cpu/m6502/m6509.h src/cpu/m6502/m6502.h
mess.obj/mess/machine/cbmb.o: mess/includes/cbm.h mess/includes/cia6526.h
mess.obj/mess/machine/cbmb.o: mess/includes/tpi6525.h mess/includes/c1551.h
mess.obj/mess/machine/cbmb.o: mess/includes/cbmdrive.h mess/includes/vc1541.h
mess.obj/mess/machine/cbmb.o: mess/machine/6522via.h mess/includes/vc20tape.h
mess.obj/mess/machine/cbmb.o: mess/includes/cbmieeeb.h
mess.obj/mess/machine/cbmb.o: mess/includes/vic6567.h mess/includes/praster.h
mess.obj/mess/machine/cbmb.o: mess/includes/crtc6845.h
mess.obj/mess/machine/cbmb.o: mess/includes/sid6581.h mess/includes/cbmb.h
mess.obj/mess/machine/cbmb.o: mess/includes/cia6526.h
mess.obj/mess/machine/cbmdrive.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/cbmdrive.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/cbmdrive.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/cbmdrive.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/cbmdrive.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/cbmdrive.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/cbmdrive.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/cbmdrive.o: mess/includes/cbm.h mess/includes/c1551.h
mess.obj/mess/machine/cbmdrive.o: mess/includes/cbmdrive.h
mess.obj/mess/machine/cbmdrive.o: mess/includes/cbmieeeb.h
mess.obj/mess/machine/cbmdrive.o: mess/includes/cbmdrive.h
mess.obj/mess/machine/cbmieeeb.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/cbmieeeb.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/cbmieeeb.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/cbmieeeb.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/cbmieeeb.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/cbmieeeb.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/cbmieeeb.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/cbmieeeb.o: mess/includes/cbm.h
mess.obj/mess/machine/cbmieeeb.o: mess/includes/cbmdrive.h
mess.obj/mess/machine/cbmieeeb.o: mess/includes/c1551.h
mess.obj/mess/machine/cbmieeeb.o: mess/includes/cbmdrive.h
mess.obj/mess/machine/cbmieeeb.o: mess/includes/cbmieeeb.h
mess.obj/mess/machine/cgenie.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/cgenie.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/cgenie.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/cgenie.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/cgenie.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/cgenie.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/cgenie.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/cgenie.o: src/cpu/z80/z80.h src/vidhrdw/generic.h
mess.obj/mess/machine/cgenie.o: mess/vidhrdw/cgenie.h mess/machine/wd179x.h
mess.obj/mess/machine/cia6526.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/cia6526.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/cia6526.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/cia6526.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/cia6526.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/cia6526.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/cia6526.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/cia6526.o: mess/includes/cbm.h mess/includes/cia6526.h
mess.obj/mess/machine/coleco.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/coleco.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/coleco.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/coleco.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/coleco.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/coleco.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/coleco.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/coleco.o: src/vidhrdw/generic.h mess/vidhrdw/tms9928a.h
mess.obj/mess/machine/coleco.o: mess/includes/coleco.h
mess.obj/mess/machine/coupe.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/coupe.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/coupe.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/coupe.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/coupe.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/coupe.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/coupe.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/coupe.o: src/cpu/z80/z80.h mess/includes/coupe.h
mess.obj/mess/machine/cpm_bios.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/cpm_bios.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/cpm_bios.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/cpm_bios.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/cpm_bios.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/cpm_bios.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/cpm_bios.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/cpm_bios.o: src/cpu/z80/z80.h mess/machine/cpm_bios.h
mess.obj/mess/machine/cpm_bios.o: mess/machine/wd179x.h
mess.obj/mess/machine/cpm_bios.o: mess/machine/cpm_disk.c
mess.obj/mess/machine/dma8237.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/dma8237.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/dma8237.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/dma8237.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/dma8237.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/dma8237.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/dma8237.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/dma8237.o: mess/includes/dma8237.h
mess.obj/mess/machine/dragon.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/dragon.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/dragon.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/dragon.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/dragon.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/dragon.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/dragon.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/dragon.o: src/cpu/m6809/m6809.h src/machine/6821pia.h
mess.obj/mess/machine/dragon.o: mess/machine/wd179x.h mess/vidhrdw/m6847.h
mess.obj/mess/machine/dragon.o: src/vidhrdw/generic.h
mess.obj/mess/machine/dsk.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/dsk.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/dsk.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/dsk.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/dsk.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/dsk.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/dsk.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/dsk.o: mess/includes/nec765.h mess/includes/flopdrv.h
mess.obj/mess/machine/dsk.o: mess/includes/dsk.h
mess.obj/mess/machine/enterp.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/enterp.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/enterp.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/enterp.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/enterp.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/enterp.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/enterp.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/enterp.o: src/cpu/z80/z80.h mess/machine/enterp.h
mess.obj/mess/machine/flopdrv.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/flopdrv.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/flopdrv.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/flopdrv.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/flopdrv.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/flopdrv.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/flopdrv.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/flopdrv.o: mess/includes/flopdrv.h
mess.obj/mess/machine/gb.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/gb.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/gb.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/gb.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/gb.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/gb.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/gb.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/gb.o: mess/machine/gb.h src/cpu/z80gb/z80gb.h
mess.obj/mess/machine/genesis.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/genesis.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/genesis.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/genesis.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/genesis.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/genesis.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/genesis.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/genesis.o: mess/machine/genesis.h
mess.obj/mess/machine/genesis.o: mess/vidhrdw/genesis.h src/cpu/z80/z80.h
mess.obj/mess/machine/i8271.o: mess/machine/i8271.h src/driver.h src/memory.h
mess.obj/mess/machine/i8271.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/machine/i8271.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/machine/i8271.o: src/palette.h src/cpuintrf.h src/timer.h
mess.obj/mess/machine/i8271.o: src/sndintrf.h src/sound/mixer.h
mess.obj/mess/machine/i8271.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/machine/i8271.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/machine/i8271.o: src/profiler.h
mess.obj/mess/machine/iwm.o: mess/machine/iwm.h src/driver.h src/memory.h
mess.obj/mess/machine/iwm.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/machine/iwm.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/machine/iwm.o: src/palette.h src/cpuintrf.h src/timer.h
mess.obj/mess/machine/iwm.o: src/sndintrf.h src/sound/mixer.h
mess.obj/mess/machine/iwm.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/machine/iwm.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/machine/iwm.o: src/profiler.h
mess.obj/mess/machine/iwm_lisa.o: mess/machine/iwm_lisa.h src/driver.h
mess.obj/mess/machine/iwm_lisa.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/machine/iwm_lisa.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/machine/iwm_lisa.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/machine/iwm_lisa.o: src/timer.h src/sndintrf.h
mess.obj/mess/machine/iwm_lisa.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/iwm_lisa.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/iwm_lisa.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/jupiter.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/jupiter.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/jupiter.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/jupiter.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/jupiter.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/jupiter.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/jupiter.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/jupiter.o: src/cpu/z80/z80.h mess/includes/jupiter.h
mess.obj/mess/machine/kaypro.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/kaypro.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/kaypro.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/kaypro.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/kaypro.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/kaypro.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/kaypro.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/kaypro.o: mess/includes/kaypro.h
mess.obj/mess/machine/kaypro.o: mess/machine/cpm_bios.h src/cpu/z80/z80.h
mess.obj/mess/machine/kc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/kc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/kc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/kc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/kc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/kc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/kc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/kc.o: src/machine/z80fmly.h src/cpu/z80/z80.h
mess.obj/mess/machine/kc.o: mess/vidhrdw/kc.h
mess.obj/mess/machine/kim1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/kim1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/kim1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/kim1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/kim1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/kim1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/kim1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/kim1.o: src/cpu/m6502/m6502.h src/vidhrdw/generic.h
mess.obj/mess/machine/lisa.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/lisa.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/lisa.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/lisa.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/lisa.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/lisa.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/lisa.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/lisa.o: mess/machine/6522via.h mess/machine/iwm_lisa.h
mess.obj/mess/machine/lisa.o: mess/machine/iwm.h mess/machine/lisa.h
mess.obj/mess/machine/mac.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/mac.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/mac.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/mac.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/mac.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/mac.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/mac.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/mac.o: mess/machine/6522via.h src/vidhrdw/generic.h
mess.obj/mess/machine/mac.o: src/cpu/m68000/m68000.h src/mamedbg.h
mess.obj/mess/machine/mac.o: src/cpu/m68000/m68k.h src/cpu/m68000/m68kconf.h
mess.obj/mess/machine/mac.o: src/cpu/m68000/m68kmame.h mess/machine/iwm.h
mess.obj/mess/machine/mac.o: mess/includes/mac.h
mess.obj/mess/machine/mbee.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/mbee.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/mbee.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/mbee.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/mbee.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/mbee.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/mbee.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/mbee.o: src/machine/z80fmly.h src/vidhrdw/generic.h
mess.obj/mess/machine/mbee.o: mess/machine/wd179x.h mess/machine/mbee.h
mess.obj/mess/machine/mc10.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/mc10.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/mc10.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/mc10.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/mc10.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/mc10.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/mc10.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/mc10.o: mess/vidhrdw/m6847.h src/vidhrdw/generic.h
mess.obj/mess/machine/mc146818.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/mc146818.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/mc146818.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/mc146818.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/mc146818.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/mc146818.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/mc146818.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/mc146818.o: mess/includes/mc146818.h
mess.obj/mess/machine/mc6850.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/mc6850.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/mc6850.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/mc6850.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/mc6850.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/mc6850.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/mc6850.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/mc6850.o: mess/machine/mc6850.h
mess.obj/mess/machine/mekd2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/mekd2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/mekd2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/mekd2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/mekd2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/mekd2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/mekd2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/mekd2.o: src/cpu/m6502/m6502.h src/vidhrdw/generic.h
mess.obj/mess/machine/microtan.o: mess/includes/microtan.h src/driver.h
mess.obj/mess/machine/microtan.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/machine/microtan.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/machine/microtan.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/machine/microtan.o: src/timer.h src/sndintrf.h
mess.obj/mess/machine/microtan.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/microtan.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/microtan.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/microtan.o: src/vidhrdw/generic.h src/cpu/m6502/m6502.h
mess.obj/mess/machine/microtan.o: mess/machine/6522via.h src/sound/ay8910.h
mess.obj/mess/machine/msx.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/msx.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/msx.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/msx.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/msx.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/msx.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/msx.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/msx.o: mess/machine/msx.h src/vidhrdw/generic.h
mess.obj/mess/machine/msx.o: src/machine/8255ppi.h mess/vidhrdw/tms9928a.h
mess.obj/mess/machine/msx.o: mess/sndhrdw/scc.h
mess.obj/mess/machine/mz700.o: mess/includes/mz700.h src/driver.h
mess.obj/mess/machine/mz700.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/machine/mz700.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/machine/mz700.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/machine/mz700.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/machine/mz700.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/machine/mz700.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/machine/mz700.o: src/profiler.h src/vidhrdw/generic.h
mess.obj/mess/machine/mz700.o: src/cpu/z80/z80.h src/machine/8255ppi.h
mess.obj/mess/machine/mz700.o: mess/includes/pit8253.h
mess.obj/mess/machine/nascom1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/nascom1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/nascom1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/nascom1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/nascom1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/nascom1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/nascom1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/nascom1.o: src/cpu/z80/z80.h mess/includes/nascom1.h
mess.obj/mess/machine/nec765.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/nec765.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/nec765.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/nec765.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/nec765.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/nec765.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/nec765.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/nec765.o: mess/includes/nec765.h
mess.obj/mess/machine/nec765.o: mess/includes/flopdrv.h
mess.obj/mess/machine/nes.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/nes.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/nes.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/nes.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/nes.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/nes.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/nes.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/nes.o: src/cpu/m6502/m6502.h src/vidhrdw/generic.h
mess.obj/mess/machine/nes.o: mess/includes/nes.h mess/machine/nes_mmc.h
mess.obj/mess/machine/nes_mmc.o: src/cpu/m6502/m6502.h src/cpuintrf.h
mess.obj/mess/machine/nes_mmc.o: src/memory.h src/timer.h
mess.obj/mess/machine/nes_mmc.o: src/vidhrdw/generic.h src/driver.h
mess.obj/mess/machine/nes_mmc.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/machine/nes_mmc.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/machine/nes_mmc.o: src/palette.h src/sndintrf.h
mess.obj/mess/machine/nes_mmc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/nes_mmc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/nes_mmc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/nes_mmc.o: mess/includes/nes.h mess/machine/nes_mmc.h
mess.obj/mess/machine/oric.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/oric.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/oric.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/oric.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/oric.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/oric.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/oric.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/p2000t.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/p2000t.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/p2000t.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/p2000t.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/p2000t.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/p2000t.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/p2000t.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/p2000t.o: src/cpu/z80/z80.h mess/includes/p2000t.h
mess.obj/mess/machine/pc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/pc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pc.o: src/machine/8255ppi.h mess/includes/pic8259.h
mess.obj/mess/machine/pc.o: mess/includes/pit8253.h mess/includes/mc146818.h
mess.obj/mess/machine/pc.o: mess/includes/dma8237.h mess/includes/uart8250.h
mess.obj/mess/machine/pc.o: mess/includes/vga.h mess/includes/pc_cga.h
mess.obj/mess/machine/pc.o: mess/includes/pc_mda.h mess/includes/pc_flopp.h
mess.obj/mess/machine/pc.o: mess/includes/pc_mouse.h mess/includes/pckeybrd.h
mess.obj/mess/machine/pc.o: mess/includes/pc.h src/cpu/i86/i86intf.h
mess.obj/mess/machine/pc.o: src/vidhrdw/generic.h
mess.obj/mess/machine/pc_fdc.o: mess/includes/pic8259.h src/driver.h
mess.obj/mess/machine/pc_fdc.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/machine/pc_fdc.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/machine/pc_fdc.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/machine/pc_fdc.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/machine/pc_fdc.o: src/sound/streams.h src/usrintrf.h
mess.obj/mess/machine/pc_fdc.o: src/cheat.h src/tilemap.h src/sprite.h
mess.obj/mess/machine/pc_fdc.o: src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pc_fdc.o: mess/includes/dma8237.h mess/includes/pc.h
mess.obj/mess/machine/pc_fdc.o: src/cpu/i86/i86intf.h src/vidhrdw/generic.h
mess.obj/mess/machine/pc_fdc.o: mess/includes/pc_fdc_h.h
mess.obj/mess/machine/pc_fdc_h.o: mess/includes/pc_fdc_h.h src/driver.h
mess.obj/mess/machine/pc_fdc_h.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/machine/pc_fdc_h.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/machine/pc_fdc_h.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/machine/pc_fdc_h.o: src/timer.h src/sndintrf.h
mess.obj/mess/machine/pc_fdc_h.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pc_fdc_h.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pc_fdc_h.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pc_fdc_h.o: mess/includes/nec765.h
mess.obj/mess/machine/pc_fdc_h.o: mess/includes/flopdrv.h
mess.obj/mess/machine/pc_flopp.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/pc_flopp.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pc_flopp.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pc_flopp.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pc_flopp.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pc_flopp.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pc_flopp.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pc_flopp.o: mess/includes/pc_flopp.h
mess.obj/mess/machine/pc_flopp.o: mess/includes/flopdrv.h
mess.obj/mess/machine/pc_hdc.o: mess/includes/pic8259.h src/driver.h
mess.obj/mess/machine/pc_hdc.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/machine/pc_hdc.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/machine/pc_hdc.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/machine/pc_hdc.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/machine/pc_hdc.o: src/sound/streams.h src/usrintrf.h
mess.obj/mess/machine/pc_hdc.o: src/cheat.h src/tilemap.h src/sprite.h
mess.obj/mess/machine/pc_hdc.o: src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pc_hdc.o: mess/includes/dma8237.h mess/includes/pc.h
mess.obj/mess/machine/pc_hdc.o: src/cpu/i86/i86intf.h src/vidhrdw/generic.h
mess.obj/mess/machine/pc_ide.o: mess/includes/pc.h src/driver.h src/memory.h
mess.obj/mess/machine/pc_ide.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/machine/pc_ide.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/machine/pc_ide.o: src/palette.h src/cpuintrf.h src/timer.h
mess.obj/mess/machine/pc_ide.o: src/sndintrf.h src/sound/mixer.h
mess.obj/mess/machine/pc_ide.o: src/sound/streams.h src/usrintrf.h
mess.obj/mess/machine/pc_ide.o: src/cheat.h src/tilemap.h src/sprite.h
mess.obj/mess/machine/pc_ide.o: src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pc_ide.o: src/cpu/i86/i86intf.h src/vidhrdw/generic.h
mess.obj/mess/machine/pc_mouse.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/pc_mouse.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pc_mouse.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pc_mouse.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pc_mouse.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pc_mouse.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pc_mouse.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pc_mouse.o: mess/includes/uart8250.h
mess.obj/mess/machine/pc_mouse.o: mess/includes/pc_mouse.h
mess.obj/mess/machine/pce.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/pce.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pce.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pce.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pce.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pce.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pce.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pce.o: mess/vidhrdw/vdc.h src/vidhrdw/generic.h
mess.obj/mess/machine/pce.o: src/cpu/h6280/h6280.h
mess.obj/mess/machine/pckeybrd.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/pckeybrd.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pckeybrd.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pckeybrd.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pckeybrd.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pckeybrd.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pckeybrd.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pckeybrd.o: mess/includes/pckeybrd.h
mess.obj/mess/machine/pclpt.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/pclpt.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pclpt.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pclpt.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pclpt.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pclpt.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pclpt.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pclpt.o: mess/includes/pclpt.h
mess.obj/mess/machine/pdp1.o: src/cpu/pdp1/pdp1.h src/memory.h
mess.obj/mess/machine/pdp1.o: mess/vidhrdw/pdp1.h src/driver.h src/osdepend.h
mess.obj/mess/machine/pdp1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pdp1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pdp1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pdp1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pdp1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pdp1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pdp1.o: src/vidhrdw/generic.h
mess.obj/mess/machine/pet.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/pet.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pet.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pet.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pet.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pet.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pet.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pet.o: src/cpu/m6502/m6502.h src/cpu/m6809/m6809.h
mess.obj/mess/machine/pet.o: mess/includes/cbm.h mess/includes/vc20tape.h
mess.obj/mess/machine/pet.o: src/machine/6821pia.h mess/machine/6522via.h
mess.obj/mess/machine/pet.o: mess/includes/pet.h mess/includes/praster.h
mess.obj/mess/machine/pet.o: mess/includes/crtc6845.h mess/includes/c1551.h
mess.obj/mess/machine/pet.o: mess/includes/cbmdrive.h
mess.obj/mess/machine/pet.o: mess/includes/cbmieeeb.h
mess.obj/mess/machine/pic8259.o: mess/includes/pic8259.h src/driver.h
mess.obj/mess/machine/pic8259.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/machine/pic8259.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/machine/pic8259.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/machine/pic8259.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/machine/pic8259.o: src/sound/streams.h src/usrintrf.h
mess.obj/mess/machine/pic8259.o: src/cheat.h src/tilemap.h src/sprite.h
mess.obj/mess/machine/pic8259.o: src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pit8253.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/pit8253.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pit8253.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pit8253.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pit8253.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pit8253.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pit8253.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pit8253.o: mess/includes/pit8253.h
mess.obj/mess/machine/pocketc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/pocketc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/pocketc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/pocketc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/pocketc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/pocketc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/pocketc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/pocketc.o: src/cpu/sc61860/sc61860.h
mess.obj/mess/machine/pocketc.o: mess/includes/pocketc.h
mess.obj/mess/machine/riot.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/riot.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/riot.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/riot.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/riot.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/riot.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/riot.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/riot.o: mess/machine/riot.h
mess.obj/mess/machine/sms.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/sms.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/sms.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/sms.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/sms.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/sms.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/sms.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/sms.o: mess/machine/sms.h
mess.obj/mess/machine/spectrum.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/spectrum.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/spectrum.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/spectrum.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/spectrum.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/spectrum.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/spectrum.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/spectrum.o: src/cpu/z80/z80.h mess/includes/spectrum.h
mess.obj/mess/machine/spectrum.o: mess/eventlst.h mess/vidhrdw/border.h
mess.obj/mess/machine/tandy1t.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/tandy1t.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/tandy1t.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/tandy1t.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/tandy1t.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/tandy1t.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/tandy1t.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/tandy1t.o: mess/includes/pckeybrd.h mess/includes/pc.h
mess.obj/mess/machine/tandy1t.o: src/cpu/i86/i86intf.h src/vidhrdw/generic.h
mess.obj/mess/machine/tandy1t.o: mess/includes/tandy1t.h
mess.obj/mess/machine/tandy1t.o: mess/includes/pc_t1t.h
mess.obj/mess/machine/ti99_4x.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/ti99_4x.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/ti99_4x.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/ti99_4x.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/ti99_4x.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/ti99_4x.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/ti99_4x.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/ti99_4x.o: mess/machine/wd179x.h mess/machine/tms9901.h
mess.obj/mess/machine/ti99_4x.o: mess/vidhrdw/tms9928a.h
mess.obj/mess/machine/ti99_4x.o: mess/machine/ti99_4x.h
mess.obj/mess/machine/tms9901.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/tms9901.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/tms9901.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/tms9901.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/tms9901.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/tms9901.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/tms9901.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/tms9901.o: mess/machine/tms9901.h
mess.obj/mess/machine/tpi6525.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/tpi6525.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/tpi6525.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/tpi6525.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/tpi6525.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/tpi6525.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/tpi6525.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/tpi6525.o: mess/includes/cbm.h mess/includes/tpi6525.h
mess.obj/mess/machine/trs80.o: mess/includes/trs80.h src/driver.h
mess.obj/mess/machine/trs80.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/machine/trs80.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/machine/trs80.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/machine/trs80.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/machine/trs80.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/machine/trs80.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/machine/trs80.o: src/profiler.h src/cpu/z80/z80.h
mess.obj/mess/machine/trs80.o: src/vidhrdw/generic.h mess/machine/wd179x.h
mess.obj/mess/machine/uart8250.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/uart8250.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/uart8250.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/uart8250.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/uart8250.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/uart8250.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/uart8250.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/uart8250.o: mess/includes/uart8250.h
mess.obj/mess/machine/uart8250.o: mess/includes/pc_mouse.h
mess.obj/mess/machine/uk101.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/uk101.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/uk101.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/uk101.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/uk101.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/uk101.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/uk101.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/uk101.o: src/cpu/m6502/m6502.h mess/machine/mc6850.h
mess.obj/mess/machine/uk101.o: mess/includes/uk101.h
mess.obj/mess/machine/vc1541.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/vc1541.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/vc1541.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/vc1541.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/vc1541.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/vc1541.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/vc1541.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/vc1541.o: src/cpu/m6502/m6502.h mess/machine/6522via.h
mess.obj/mess/machine/vc1541.o: mess/includes/cbm.h mess/includes/cbmdrive.h
mess.obj/mess/machine/vc1541.o: mess/includes/tpi6525.h
mess.obj/mess/machine/vc1541.o: mess/includes/vc1541.h mess/machine/6522via.h
mess.obj/mess/machine/vc20.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/vc20.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/vc20.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/vc20.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/vc20.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/vc20.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/vc20.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/vc20.o: src/cpu/m6502/m6502.h src/vidhrdw/generic.h
mess.obj/mess/machine/vc20.o: mess/includes/cbm.h mess/includes/vc20.h
mess.obj/mess/machine/vc20.o: mess/includes/c1551.h mess/includes/cbmdrive.h
mess.obj/mess/machine/vc20.o: mess/includes/vc1541.h mess/machine/6522via.h
mess.obj/mess/machine/vc20.o: mess/includes/vc20tape.h mess/includes/c1551.h
mess.obj/mess/machine/vc20.o: mess/includes/cbmieeeb.h
mess.obj/mess/machine/vc20.o: mess/includes/vic6560.h
mess.obj/mess/machine/vc20tape.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/vc20tape.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/vc20tape.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/vc20tape.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/vc20tape.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/vc20tape.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/vc20tape.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/vc20tape.o: src/unzip.h mess/includes/cbm.h
mess.obj/mess/machine/vc20tape.o: mess/includes/vc20tape.h
mess.obj/mess/machine/vectrex.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/vectrex.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/vectrex.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/vectrex.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/vectrex.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/vectrex.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/vectrex.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/vectrex.o: src/vidhrdw/vector.h src/artwork.h
mess.obj/mess/machine/vectrex.o: mess/machine/6522via.h src/cpu/m6809/m6809.h
mess.obj/mess/machine/vtech1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/vtech1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/vtech1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/vtech1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/vtech1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/vtech1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/vtech1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/vtech1.o: src/vidhrdw/generic.h
mess.obj/mess/machine/vtech2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/vtech2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/vtech2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/vtech2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/vtech2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/vtech2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/vtech2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/vtech2.o: src/vidhrdw/generic.h
mess.obj/mess/machine/wd179x.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/wd179x.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/wd179x.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/wd179x.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/wd179x.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/wd179x.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/wd179x.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/wd179x.o: mess/machine/wd179x.h
mess.obj/mess/machine/zx.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/machine/zx.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/machine/zx.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/machine/zx.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/machine/zx.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/machine/zx.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/machine/zx.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/machine/zx.o: src/vidhrdw/generic.h src/cpu/z80/z80.h
mess.obj/mess/vidhrdw/a7800.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/a7800.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/a7800.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/a7800.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/a7800.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/a7800.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/a7800.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/a7800.o: src/vidhrdw/generic.h src/cpu/m6502/m6502.h
mess.obj/mess/vidhrdw/advision.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/advision.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/advision.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/advision.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/advision.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/advision.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/advision.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/advision.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/advision.o: mess/includes/advision.h
mess.obj/mess/vidhrdw/amiga.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/amiga.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/amiga.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/amiga.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/amiga.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/amiga.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/amiga.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/amiga.o: src/vidhrdw/generic.h mess/includes/amiga.h
mess.obj/mess/vidhrdw/amstrad.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/amstrad.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/amstrad.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/amstrad.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/amstrad.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/amstrad.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/amstrad.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/amstrad.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/amstrad.o: mess/includes/amstrad.h mess/vidhrdw/m6845.h
mess.obj/mess/vidhrdw/amstrad.o: mess/eventlst.h
mess.obj/mess/vidhrdw/antic.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/antic.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/antic.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/antic.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/antic.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/antic.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/antic.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/antic.o: src/cpu/m6502/m6502.h mess/machine/atari.h
mess.obj/mess/vidhrdw/antic.o: mess/vidhrdw/atari.h
mess.obj/mess/vidhrdw/apple1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/apple1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/apple1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/apple1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/apple1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/apple1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/apple1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/apple1.o: src/vidhrdw/generic.h mess/includes/apple1.h
mess.obj/mess/vidhrdw/apple2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/apple2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/apple2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/apple2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/apple2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/apple2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/apple2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/apple2.o: src/vidhrdw/generic.h mess/includes/apple2.h
mess.obj/mess/vidhrdw/aquarius.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/aquarius.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/aquarius.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/aquarius.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/aquarius.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/aquarius.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/aquarius.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/aquarius.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/aquarius.o: mess/includes/aquarius.h
mess.obj/mess/vidhrdw/astrocde.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/astrocde.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/astrocde.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/astrocde.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/astrocde.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/astrocde.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/astrocde.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/astrocde.o: src/vidhrdw/generic.h src/cpu/z80/z80.h
mess.obj/mess/vidhrdw/atari.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/atari.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/atari.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/atari.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/atari.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/atari.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/atari.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/atari.o: mess/machine/atari.h mess/vidhrdw/atari.h
mess.obj/mess/vidhrdw/atom.o: src/vidhrdw/generic.h src/driver.h src/memory.h
mess.obj/mess/vidhrdw/atom.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/vidhrdw/atom.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/vidhrdw/atom.o: src/palette.h src/cpuintrf.h src/timer.h
mess.obj/mess/vidhrdw/atom.o: src/sndintrf.h src/sound/mixer.h
mess.obj/mess/vidhrdw/atom.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/vidhrdw/atom.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/vidhrdw/atom.o: src/profiler.h mess/vidhrdw/m6847.h
mess.obj/mess/vidhrdw/atom.o: mess/includes/atom.h
mess.obj/mess/vidhrdw/bbc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/bbc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/bbc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/bbc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/bbc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/bbc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/bbc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/bbc.o: mess/machine/bbc.h mess/vidhrdw/bbc.h
mess.obj/mess/vidhrdw/bbc.o: mess/vidhrdw/m6845.h mess/vidhrdw/ttchar.h
mess.obj/mess/vidhrdw/border.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/border.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/border.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/border.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/border.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/border.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/border.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/border.o: mess/vidhrdw/border.h mess/eventlst.h
mess.obj/mess/vidhrdw/cgenie.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/cgenie.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/cgenie.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/cgenie.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/cgenie.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/cgenie.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/cgenie.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/cgenie.o: src/vidhrdw/generic.h mess/vidhrdw/cgenie.h
mess.obj/mess/vidhrdw/coleco.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/coleco.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/coleco.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/coleco.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/coleco.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/coleco.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/coleco.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/coleco.o: src/vidhrdw/generic.h mess/vidhrdw/tms9928a.h
mess.obj/mess/vidhrdw/coleco.o: mess/includes/coleco.h
mess.obj/mess/vidhrdw/coupe.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/coupe.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/coupe.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/coupe.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/coupe.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/coupe.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/coupe.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/coupe.o: src/vidhrdw/generic.h mess/includes/coupe.h
mess.obj/mess/vidhrdw/crtc6845.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/crtc6845.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/crtc6845.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/crtc6845.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/crtc6845.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/crtc6845.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/crtc6845.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/crtc6845.o: src/vidhrdw/generic.h mess/includes/cbm.h
mess.obj/mess/vidhrdw/crtc6845.o: mess/includes/praster.h
mess.obj/mess/vidhrdw/crtc6845.o: mess/includes/crtc6845.h
mess.obj/mess/vidhrdw/crtc6845.o: mess/includes/praster.h
mess.obj/mess/vidhrdw/dragon.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/dragon.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/dragon.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/dragon.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/dragon.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/dragon.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/dragon.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/dragon.o: src/machine/6821pia.h mess/vidhrdw/m6847.h
mess.obj/mess/vidhrdw/dragon.o: src/vidhrdw/generic.h src/cpu/m6809/m6809.h
mess.obj/mess/vidhrdw/enterp.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/enterp.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/enterp.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/enterp.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/enterp.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/enterp.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/enterp.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/enterp.o: mess/vidhrdw/nick.h mess/vidhrdw/epnick.h
mess.obj/mess/vidhrdw/enterp.o: mess/vidhrdw/enterp.h
mess.obj/mess/vidhrdw/epnick.o: mess/vidhrdw/epnick.h src/driver.h
mess.obj/mess/vidhrdw/epnick.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/vidhrdw/epnick.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/vidhrdw/epnick.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/vidhrdw/epnick.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/vidhrdw/epnick.o: src/sound/streams.h src/usrintrf.h
mess.obj/mess/vidhrdw/epnick.o: src/cheat.h src/tilemap.h src/sprite.h
mess.obj/mess/vidhrdw/epnick.o: src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/gb.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/gb.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/gb.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/gb.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/gb.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/gb.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/gb.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/gb.o: src/vidhrdw/generic.h mess/machine/gb.h
mess.obj/mess/vidhrdw/genesis.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/genesis.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/genesis.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/genesis.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/genesis.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/genesis.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/genesis.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/genesis.o: src/vidhrdw/generic.h mess/machine/genesis.h
mess.obj/mess/vidhrdw/genesis.o: mess/vidhrdw/genesis.h
mess.obj/mess/vidhrdw/gtia.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/gtia.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/gtia.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/gtia.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/gtia.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/gtia.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/gtia.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/gtia.o: src/cpu/m6502/m6502.h mess/machine/atari.h
mess.obj/mess/vidhrdw/gtia.o: mess/vidhrdw/atari.h
mess.obj/mess/vidhrdw/jupiter.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/jupiter.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/jupiter.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/jupiter.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/jupiter.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/jupiter.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/jupiter.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/jupiter.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/jupiter.o: mess/includes/jupiter.h
mess.obj/mess/vidhrdw/kaypro.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/kaypro.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/kaypro.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/kaypro.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/kaypro.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/kaypro.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/kaypro.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/kaypro.o: src/cpu/z80/z80.h src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/kaypro.o: mess/includes/kaypro.h
mess.obj/mess/vidhrdw/kaypro.o: mess/machine/cpm_bios.h
mess.obj/mess/vidhrdw/kc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/kc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/kc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/kc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/kc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/kc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/kc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/kc.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/kim1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/kim1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/kim1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/kim1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/kim1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/kim1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/kim1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/kim1.o: src/artwork.h src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/m6845.o: mess/vidhrdw/m6845.h
mess.obj/mess/vidhrdw/m6847.o: mess/vidhrdw/m6847.h src/driver.h src/memory.h
mess.obj/mess/vidhrdw/m6847.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/vidhrdw/m6847.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/vidhrdw/m6847.o: src/palette.h src/cpuintrf.h src/timer.h
mess.obj/mess/vidhrdw/m6847.o: src/sndintrf.h src/sound/mixer.h
mess.obj/mess/vidhrdw/m6847.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/vidhrdw/m6847.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/vidhrdw/m6847.o: src/profiler.h src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/mac.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/mac.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/mac.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/mac.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/mac.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/mac.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/mac.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/mac.o: src/vidhrdw/generic.h mess/includes/mac.h
mess.obj/mess/vidhrdw/mbee.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/mbee.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/mbee.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/mbee.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/mbee.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/mbee.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/mbee.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/mbee.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/mekd2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/mekd2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/mekd2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/mekd2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/mekd2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/mekd2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/mekd2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/mekd2.o: src/artwork.h src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/microtan.o: mess/includes/microtan.h src/driver.h
mess.obj/mess/vidhrdw/microtan.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/vidhrdw/microtan.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/vidhrdw/microtan.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/vidhrdw/microtan.o: src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/microtan.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/microtan.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/microtan.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/microtan.o: src/vidhrdw/generic.h src/cpu/m6502/m6502.h
mess.obj/mess/vidhrdw/microtan.o: mess/machine/6522via.h src/sound/ay8910.h
mess.obj/mess/vidhrdw/mz700.o: mess/includes/mz700.h src/driver.h
mess.obj/mess/vidhrdw/mz700.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/vidhrdw/mz700.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/vidhrdw/mz700.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/vidhrdw/mz700.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/vidhrdw/mz700.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/vidhrdw/mz700.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/vidhrdw/mz700.o: src/profiler.h src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/mz700.o: src/cpu/z80/z80.h src/machine/8255ppi.h
mess.obj/mess/vidhrdw/mz700.o: mess/includes/pit8253.h
mess.obj/mess/vidhrdw/nascom1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/nascom1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/nascom1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/nascom1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/nascom1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/nascom1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/nascom1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/nascom1.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/nascom1.o: mess/includes/nascom1.h
mess.obj/mess/vidhrdw/nes.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/nes.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/nes.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/nes.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/nes.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/nes.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/nes.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/nes.o: src/vidhrdw/generic.h mess/includes/nes.h
mess.obj/mess/vidhrdw/nes.o: mess/machine/nes_mmc.h
mess.obj/mess/vidhrdw/newgenes.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/newgenes.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/newgenes.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/newgenes.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/newgenes.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/newgenes.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/newgenes.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/newgenes.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/newgenes.o: mess/machine/genesis.h
mess.obj/mess/vidhrdw/newgenes.o: mess/vidhrdw/genesis.h
mess.obj/mess/vidhrdw/oric.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/oric.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/oric.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/oric.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/oric.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/oric.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/oric.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/oric.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/p2000m.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/p2000m.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/p2000m.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/p2000m.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/p2000m.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/p2000m.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/p2000m.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/p2000m.o: src/vidhrdw/generic.h mess/includes/p2000t.h
mess.obj/mess/vidhrdw/pc_cga.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/pc_cga.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/pc_cga.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/pc_cga.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/pc_cga.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/pc_cga.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/pc_cga.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/pc_cga.o: mess/includes/pc.h src/cpu/i86/i86intf.h
mess.obj/mess/vidhrdw/pc_cga.o: src/vidhrdw/generic.h mess/includes/pc_cga.h
mess.obj/mess/vidhrdw/pc_mda.o: mess/includes/pc.h src/driver.h src/memory.h
mess.obj/mess/vidhrdw/pc_mda.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/vidhrdw/pc_mda.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/vidhrdw/pc_mda.o: src/palette.h src/cpuintrf.h src/timer.h
mess.obj/mess/vidhrdw/pc_mda.o: src/sndintrf.h src/sound/mixer.h
mess.obj/mess/vidhrdw/pc_mda.o: src/sound/streams.h src/usrintrf.h
mess.obj/mess/vidhrdw/pc_mda.o: src/cheat.h src/tilemap.h src/sprite.h
mess.obj/mess/vidhrdw/pc_mda.o: src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/pc_mda.o: src/cpu/i86/i86intf.h src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/pc_mda.o: mess/includes/pc_mda.h
mess.obj/mess/vidhrdw/pc_t1t.o: mess/includes/pc.h src/driver.h src/memory.h
mess.obj/mess/vidhrdw/pc_t1t.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/vidhrdw/pc_t1t.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/vidhrdw/pc_t1t.o: src/palette.h src/cpuintrf.h src/timer.h
mess.obj/mess/vidhrdw/pc_t1t.o: src/sndintrf.h src/sound/mixer.h
mess.obj/mess/vidhrdw/pc_t1t.o: src/sound/streams.h src/usrintrf.h
mess.obj/mess/vidhrdw/pc_t1t.o: src/cheat.h src/tilemap.h src/sprite.h
mess.obj/mess/vidhrdw/pc_t1t.o: src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/pc_t1t.o: src/cpu/i86/i86intf.h src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/pc_t1t.o: mess/includes/pc_t1t.h
mess.obj/mess/vidhrdw/pcw.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/pcw.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/pcw.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/pcw.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/pcw.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/pcw.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/pcw.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/pcw.o: src/vidhrdw/generic.h mess/includes/pcw.h
mess.obj/mess/vidhrdw/pcw16.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/pcw16.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/pcw16.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/pcw16.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/pcw16.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/pcw16.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/pcw16.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/pcw16.o: mess/includes/pcw16.h src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/pdp1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/pdp1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/pdp1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/pdp1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/pdp1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/pdp1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/pdp1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/pdp1.o: src/vidhrdw/generic.h mess/vidhrdw/pdp1.h
mess.obj/mess/vidhrdw/pdp1.o: src/cpu/pdp1/pdp1.h
mess.obj/mess/vidhrdw/pet.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/pet.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/pet.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/pet.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/pet.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/pet.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/pet.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/pet.o: src/vidhrdw/generic.h mess/includes/cbm.h
mess.obj/mess/vidhrdw/pet.o: mess/includes/praster.h mess/includes/pet.h
mess.obj/mess/vidhrdw/pet.o: mess/includes/praster.h mess/includes/crtc6845.h
mess.obj/mess/vidhrdw/pocketc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/pocketc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/pocketc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/pocketc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/pocketc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/pocketc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/pocketc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/pocketc.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/pocketc.o: mess/includes/pocketc.h
mess.obj/mess/vidhrdw/praster.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/praster.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/praster.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/praster.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/praster.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/praster.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/praster.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/praster.o: src/vidhrdw/generic.h mess/includes/cbm.h
mess.obj/mess/vidhrdw/praster.o: mess/includes/praster.h
mess.obj/mess/vidhrdw/saa5050.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/saa5050.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/saa5050.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/saa5050.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/saa5050.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/saa5050.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/saa5050.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/saa5050.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/smsvdp.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/smsvdp.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/smsvdp.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/smsvdp.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/smsvdp.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/smsvdp.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/smsvdp.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/smsvdp.o: mess/vidhrdw/smsvdp.h src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/smsvdp.o: src/cpu/z80/z80.h
mess.obj/mess/vidhrdw/spectrum.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/spectrum.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/spectrum.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/spectrum.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/spectrum.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/spectrum.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/spectrum.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/spectrum.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/spectrum.o: mess/includes/spectrum.h mess/eventlst.h
mess.obj/mess/vidhrdw/spectrum.o: mess/vidhrdw/border.h
mess.obj/mess/vidhrdw/ted7360.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/ted7360.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/ted7360.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/ted7360.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/ted7360.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/ted7360.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/ted7360.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/ted7360.o: mess/includes/cbm.h mess/includes/c16.h
mess.obj/mess/vidhrdw/ted7360.o: mess/includes/c1551.h
mess.obj/mess/vidhrdw/ted7360.o: mess/includes/cbmdrive.h
mess.obj/mess/vidhrdw/ted7360.o: mess/includes/vc20tape.h
mess.obj/mess/vidhrdw/ted7360.o: mess/includes/c1551.h mess/includes/vc1541.h
mess.obj/mess/vidhrdw/ted7360.o: mess/machine/6522via.h
mess.obj/mess/vidhrdw/ted7360.o: mess/includes/ted7360.h
mess.obj/mess/vidhrdw/tms9928a.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/tms9928a.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/tms9928a.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/tms9928a.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/tms9928a.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/tms9928a.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/tms9928a.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/tms9928a.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/tms9928a.o: mess/vidhrdw/tms9928a.h
mess.obj/mess/vidhrdw/trs80.o: mess/includes/trs80.h src/driver.h
mess.obj/mess/vidhrdw/trs80.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/vidhrdw/trs80.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/vidhrdw/trs80.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/vidhrdw/trs80.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/vidhrdw/trs80.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/vidhrdw/trs80.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/vidhrdw/trs80.o: src/profiler.h src/cpu/z80/z80.h
mess.obj/mess/vidhrdw/trs80.o: src/vidhrdw/generic.h mess/machine/wd179x.h
mess.obj/mess/vidhrdw/uk101.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/uk101.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/uk101.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/uk101.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/uk101.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/uk101.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/uk101.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/uk101.o: src/vidhrdw/generic.h mess/includes/uk101.h
mess.obj/mess/vidhrdw/vdc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/vdc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/vdc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/vdc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/vdc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/vdc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/vdc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/vdc.o: src/vidhrdw/generic.h mess/vidhrdw/vdc.h
mess.obj/mess/vidhrdw/vdc8563.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/vdc8563.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/vdc8563.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/vdc8563.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/vdc8563.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/vdc8563.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/vdc8563.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/vdc8563.o: src/vidhrdw/generic.h mess/includes/cbm.h
mess.obj/mess/vidhrdw/vdc8563.o: mess/includes/praster.h
mess.obj/mess/vidhrdw/vdc8563.o: mess/includes/vdc8563.h
mess.obj/mess/vidhrdw/vdc8563.o: mess/includes/praster.h
mess.obj/mess/vidhrdw/vectrex.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/vectrex.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/vectrex.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/vectrex.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/vectrex.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/vectrex.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/vectrex.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/vectrex.o: src/vidhrdw/vector.h src/artwork.h
mess.obj/mess/vidhrdw/vectrex.o: mess/machine/6522via.h
mess.obj/mess/vidhrdw/vga.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/vga.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/vga.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/vga.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/vga.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/vga.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/vga.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/vga.o: mess/includes/cbm.h mess/includes/vga.h
mess.obj/mess/vidhrdw/vic4567.o: mess/includes/vic4567.h
mess.obj/mess/vidhrdw/vic6560.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/vic6560.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/vic6560.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/vic6560.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/vic6560.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/vic6560.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/vic6560.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/vic6560.o: mess/includes/cbm.h mess/includes/vc20.h
mess.obj/mess/vidhrdw/vic6560.o: mess/includes/c1551.h
mess.obj/mess/vidhrdw/vic6560.o: mess/includes/cbmdrive.h
mess.obj/mess/vidhrdw/vic6560.o: mess/includes/c1551.h mess/includes/vc1541.h
mess.obj/mess/vidhrdw/vic6560.o: mess/machine/6522via.h
mess.obj/mess/vidhrdw/vic6560.o: mess/includes/vc20tape.h
mess.obj/mess/vidhrdw/vic6560.o: mess/includes/vic6560.h
mess.obj/mess/vidhrdw/vic6567.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/vic6567.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/vic6567.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/vic6567.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/vic6567.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/vic6567.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/vic6567.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/vic6567.o: src/vidhrdw/generic.h mess/includes/cbm.h
mess.obj/mess/vidhrdw/vic6567.o: mess/includes/c64.h mess/includes/cia6526.h
mess.obj/mess/vidhrdw/vic6567.o: mess/includes/praster.h
mess.obj/mess/vidhrdw/vic6567.o: mess/includes/vic4567.h
mess.obj/mess/vidhrdw/vic6567.o: mess/includes/vic6567.h
mess.obj/mess/vidhrdw/vic6567.o: mess/vidhrdw/vic4567.c
mess.obj/mess/vidhrdw/vtech1.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/vtech1.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/vtech1.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/vtech1.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/vtech1.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/vtech1.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/vtech1.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/vtech1.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/vtech2.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/vtech2.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/vtech2.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/vtech2.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/vtech2.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/vtech2.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/vtech2.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/vtech2.o: src/vidhrdw/generic.h
mess.obj/mess/vidhrdw/zx.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/vidhrdw/zx.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/vidhrdw/zx.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/vidhrdw/zx.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/vidhrdw/zx.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/vidhrdw/zx.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/vidhrdw/zx.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/vidhrdw/zx.o: src/vidhrdw/generic.h src/cpu/z80/z80.h
mess.obj/mess/sndhrdw/6581_.o: mess/includes/cbm.h src/driver.h src/memory.h
mess.obj/mess/sndhrdw/6581_.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/sndhrdw/6581_.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/sndhrdw/6581_.o: src/palette.h src/cpuintrf.h src/timer.h
mess.obj/mess/sndhrdw/6581_.o: src/sndintrf.h src/sound/mixer.h
mess.obj/mess/sndhrdw/6581_.o: src/sound/streams.h src/usrintrf.h src/cheat.h
mess.obj/mess/sndhrdw/6581_.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/sndhrdw/6581_.o: src/profiler.h mess/sndhrdw/samples.h
mess.obj/mess/sndhrdw/6581_.o: mess/sndhrdw/mytypes.h mess/sndhrdw/opstruct.h
mess.obj/mess/sndhrdw/6581_.o: mess/sndhrdw/envelope.h mess/sndhrdw/mixing.h
mess.obj/mess/sndhrdw/6581_.o: mess/sndhrdw/wave6581.h
mess.obj/mess/sndhrdw/6581_.o: mess/sndhrdw/wave8580.h
mess.obj/mess/sndhrdw/6581_.o: mess/includes/sid6581.h
mess.obj/mess/sndhrdw/amstrad.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/amstrad.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/amstrad.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/amstrad.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/amstrad.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/amstrad.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/amstrad.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/beep.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/beep.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/beep.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/beep.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/beep.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/beep.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/beep.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/beep.o: mess/includes/beep.h
mess.obj/mess/sndhrdw/cgenie.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/cgenie.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/cgenie.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/cgenie.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/cgenie.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/cgenie.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/cgenie.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/dave.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/dave.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/dave.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/dave.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/dave.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/dave.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/dave.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/dave.o: mess/sndhrdw/dave.h
mess.obj/mess/sndhrdw/envelope.o: mess/includes/cbm.h src/driver.h
mess.obj/mess/sndhrdw/envelope.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/sndhrdw/envelope.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/sndhrdw/envelope.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/sndhrdw/envelope.o: src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/envelope.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/envelope.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/envelope.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/envelope.o: mess/sndhrdw/mytypes.h
mess.obj/mess/sndhrdw/envelope.o: mess/sndhrdw/opstruct.h
mess.obj/mess/sndhrdw/envelope.o: mess/sndhrdw/enve_dl.h
mess.obj/mess/sndhrdw/envelope.o: mess/sndhrdw/envelope.h
mess.obj/mess/sndhrdw/genesis.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/genesis.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/genesis.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/genesis.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/genesis.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/genesis.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/genesis.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/genesis.o: src/sound/2612intf.h src/sound/fm.h
mess.obj/mess/sndhrdw/genesis.o: src/sound/ay8910.h mess/machine/genesis.h
mess.obj/mess/sndhrdw/kaypro.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/kaypro.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/kaypro.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/kaypro.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/kaypro.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/kaypro.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/kaypro.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/mac.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/mac.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/mac.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/mac.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/mac.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/mac.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/mac.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/mac.o: mess/includes/mac.h
mess.obj/mess/sndhrdw/mixing.o: mess/includes/cbm.h src/driver.h src/memory.h
mess.obj/mess/sndhrdw/mixing.o: src/osdepend.h src/inptport.h src/input.h
mess.obj/mess/sndhrdw/mixing.o: src/mame.h src/drawgfx.h src/common.h
mess.obj/mess/sndhrdw/mixing.o: src/palette.h src/cpuintrf.h src/timer.h
mess.obj/mess/sndhrdw/mixing.o: src/sndintrf.h src/sound/mixer.h
mess.obj/mess/sndhrdw/mixing.o: src/sound/streams.h src/usrintrf.h
mess.obj/mess/sndhrdw/mixing.o: src/cheat.h src/tilemap.h src/sprite.h
mess.obj/mess/sndhrdw/mixing.o: src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/mixing.o: mess/sndhrdw/mytypes.h
mess.obj/mess/sndhrdw/mixing.o: mess/sndhrdw/opstruct.h
mess.obj/mess/sndhrdw/mixing.o: mess/sndhrdw/samples.h mess/sndhrdw/6581_.h
mess.obj/mess/sndhrdw/mixing.o: mess/sndhrdw/mixing.h
mess.obj/mess/sndhrdw/mz700.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/mz700.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/mz700.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/mz700.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/mz700.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/mz700.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/mz700.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/pc.o: src/sound/streams.h mess/includes/pc.h
mess.obj/mess/sndhrdw/pc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/pc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/pc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/pc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/pc.o: src/sound/mixer.h src/usrintrf.h src/cheat.h
mess.obj/mess/sndhrdw/pc.o: src/tilemap.h src/sprite.h src/gfxobj.h
mess.obj/mess/sndhrdw/pc.o: src/profiler.h src/cpu/i86/i86intf.h
mess.obj/mess/sndhrdw/pc.o: src/vidhrdw/generic.h mess/includes/pit8253.h
mess.obj/mess/sndhrdw/samples.o: mess/includes/cbm.h src/driver.h
mess.obj/mess/sndhrdw/samples.o: src/memory.h src/osdepend.h src/inptport.h
mess.obj/mess/sndhrdw/samples.o: src/input.h src/mame.h src/drawgfx.h
mess.obj/mess/sndhrdw/samples.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/sndhrdw/samples.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/sndhrdw/samples.o: src/sound/streams.h src/usrintrf.h
mess.obj/mess/sndhrdw/samples.o: src/cheat.h src/tilemap.h src/sprite.h
mess.obj/mess/sndhrdw/samples.o: src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/samples.o: mess/includes/sid6581.h
mess.obj/mess/sndhrdw/samples.o: mess/sndhrdw/samples.h
mess.obj/mess/sndhrdw/samples.o: mess/sndhrdw/mytypes.h
mess.obj/mess/sndhrdw/scc.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/scc.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/scc.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/scc.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/scc.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/scc.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/scc.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/scc.o: mess/sndhrdw/scc.h mess/sndhrdw/scc-u.c
mess.obj/mess/sndhrdw/sid6581.o: src/sound/streams.h src/mame.h
mess.obj/mess/sndhrdw/sid6581.o: src/osdepend.h src/inptport.h src/memory.h
mess.obj/mess/sndhrdw/sid6581.o: src/input.h src/drawgfx.h
mess.obj/mess/sndhrdw/sid6581.o: mess/includes/cbm.h src/driver.h
mess.obj/mess/sndhrdw/sid6581.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/sndhrdw/sid6581.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/sndhrdw/sid6581.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/sid6581.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/sid6581.o: mess/sndhrdw/6581_.h mess/sndhrdw/mytypes.h
mess.obj/mess/sndhrdw/sid6581.o: mess/sndhrdw/opstruct.h
mess.obj/mess/sndhrdw/sid6581.o: mess/sndhrdw/mixing.h
mess.obj/mess/sndhrdw/sid6581.o: mess/includes/sid6581.h
mess.obj/mess/sndhrdw/ted7360.o: src/sound/streams.h src/mame.h
mess.obj/mess/sndhrdw/ted7360.o: src/osdepend.h src/inptport.h src/memory.h
mess.obj/mess/sndhrdw/ted7360.o: src/input.h src/drawgfx.h
mess.obj/mess/sndhrdw/ted7360.o: mess/includes/cbm.h src/driver.h
mess.obj/mess/sndhrdw/ted7360.o: src/common.h src/palette.h src/cpuintrf.h
mess.obj/mess/sndhrdw/ted7360.o: src/timer.h src/sndintrf.h src/sound/mixer.h
mess.obj/mess/sndhrdw/ted7360.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/ted7360.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/ted7360.o: mess/includes/c16.h mess/includes/c1551.h
mess.obj/mess/sndhrdw/ted7360.o: mess/includes/cbmdrive.h
mess.obj/mess/sndhrdw/ted7360.o: mess/includes/ted7360.h
mess.obj/mess/sndhrdw/vic6560.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/sndhrdw/vic6560.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/sndhrdw/vic6560.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/sndhrdw/vic6560.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/sndhrdw/vic6560.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/sndhrdw/vic6560.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/sndhrdw/vic6560.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/sndhrdw/vic6560.o: mess/includes/cbm.h mess/includes/vic6560.h
mess.obj/mess/tools/crt.o: src/osdepend.h src/inptport.h src/memory.h
mess.obj/mess/tools/crt.o: src/input.h mess/tools/imgtool.h mess/mess.h
mess.obj/mess/tools/dat2html.o: mess/tools/osdtools.h src/osdepend.h
mess.obj/mess/tools/dat2html.o: src/inptport.h src/memory.h src/input.h
mess.obj/mess/tools/imgtool.o: src/osdepend.h src/inptport.h src/memory.h
mess.obj/mess/tools/imgtool.o: src/input.h mess/tools/imgtool.h mess/mess.h
mess.obj/mess/tools/imgtool.o: mess/config.h
mess.obj/mess/tools/main.o: mess/tools/imgtool.h src/osdepend.h
mess.obj/mess/tools/main.o: src/inptport.h src/memory.h src/input.h
mess.obj/mess/tools/main.o: mess/mess.h mess/tools/osdtools.h
mess.obj/mess/tools/mkhdimg.o: mess/tools/osdtools.h src/osdepend.h
mess.obj/mess/tools/mkhdimg.o: src/inptport.h src/memory.h src/input.h
mess.obj/mess/tools/pchd.o: src/osdepend.h src/inptport.h src/memory.h
mess.obj/mess/tools/pchd.o: src/input.h mess/tools/imgtool.h mess/mess.h
mess.obj/mess/tools/rsdos.o: src/osdepend.h src/inptport.h src/memory.h
mess.obj/mess/tools/rsdos.o: src/input.h mess/tools/imgtool.h mess/mess.h
mess.obj/mess/tools/stream.o: src/unzip.h src/osdepend.h src/inptport.h
mess.obj/mess/tools/stream.o: src/memory.h src/input.h mess/tools/imgtool.h
mess.obj/mess/tools/stream.o: mess/mess.h
mess.obj/mess/tools/stubs.o: src/driver.h src/memory.h src/osdepend.h
mess.obj/mess/tools/stubs.o: src/inptport.h src/input.h src/mame.h
mess.obj/mess/tools/stubs.o: src/drawgfx.h src/common.h src/palette.h
mess.obj/mess/tools/stubs.o: src/cpuintrf.h src/timer.h src/sndintrf.h
mess.obj/mess/tools/stubs.o: src/sound/mixer.h src/sound/streams.h
mess.obj/mess/tools/stubs.o: src/usrintrf.h src/cheat.h src/tilemap.h
mess.obj/mess/tools/stubs.o: src/sprite.h src/gfxobj.h src/profiler.h
mess.obj/mess/tools/t64.o: src/osdepend.h src/inptport.h src/memory.h
mess.obj/mess/tools/t64.o: src/input.h mess/tools/imgtool.h mess/mess.h
