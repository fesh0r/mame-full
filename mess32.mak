# before compiling for the first time, use "make maketree" to create all the
# necessary subdirectories

# Set the version here
VERSION = -DVERSION=37

# uncomment out the BETA_VERSION = line to build a beta version of MAME
BETA_VERSION = -DBETA_VERSION=5

# uncomment this to build an release canidate version
# RELEASE_CANDIDATE = -DRELEASE_CANDIDATE=1

# uncomment out the MAME_DEBUG = line to build a version of MAME for debugging games
# MAME_DEBUG = -DMAME_DEBUG

# if MAME_MMX is defined, MMX will be compiled in
MAME_MMX = -DMAME_MMX

# if MAME_NET is defined, network support will be compiled in; requires wsock32.lib
# MAME_NET = -DMAME_NET

# whether to use fastcall or stdcall
USE_FASTCALL = 1

# uncomment to build without MIDAS
# NOMIDAS =

# uncomment to build Helpfiles
# HELPFILE = Mame32.hlp

# uncomment to build romcmp.exe utility
TOOLS = mkhdimg.exe imgtool.exe dat2html.exe

# exe name for MAME
EXENAME = mess32.exe

CC      = cl.exe
LD      = link.exe
RSC     = rc.exe
ASM     = nasmw.exe

ASMFLAGS = -f win32
ASMDEFS =

# uncomment out the DEBUG = line to build a debugging version of mame32.
# DEBUG = 1

# uncomment next line to do a smaller compile including only one driver
# TINY_COMPILE = 1
TINY_NAME = 
TINY_OBJS = 

# uncomment next line to use Assembler 68k engine
# X86_ASM_68K = 1

# uncomment next line to use Assembler z80 engine
# X86_ASM_Z80 = 1

!if "$(PROCESSOR_ARCHITECTURE)" == ""
PROCESSOR_ARCHITECTURE=x86
!endif

!ifndef TOOLS
TOOLS =
!endif

!ifdef MAME_NET
NET_LIBS = wsock32.lib
!else
NET_LIBS =
!endif

!ifdef X86_ASM_68K
M68KOBJS = $(OBJ)/cpu/m68000/asmintf.o $(OBJ)/cpu/m68000/68kem.oa
M68KDEF  = -DA68KEM
!else
M68KOBJS = $(OBJ)/cpu/m68000/m68kops.og $(OBJ)/cpu/m68000/m68kopac.og \
           $(OBJ)/cpu/m68000/m68kopdm.og $(OBJ)/cpu/m68000/m68kopnz.og \
           $(OBJ)/cpu/m68000/m68kcpu.o $(OBJ)/cpu/m68000/m68kmame.o
M68KDEF  =
!endif

!ifdef X86_ASM_Z80
Z80OBJS = $(OBJ)/cpu/z80/z80.oa
80DEF = -nasm -verbose 0
!else
Z80OBJS = $(OBJ)/cpu/z80/z80.o
z80DEF =
!endif

DEFS   = -DLSB_FIRST -DWIN32 -DPI=3.1415926535 \
         -DINLINE="static __inline" -Dinline=__inline -D__inline__=__inline \
	 -DPNG_SAVE_SUPPORT -D_WINDOWS -DZLIB_DLL

AUDIOFLAGS =
AUDIOOBJS  =
AUDIOLIBS  =

!ifndef NOMIDAS
AUDIOFLAGS = $(AUDIOFLAGS) -IMIDAS
AUDIOOBJS  = $(AUDIOOBJS) $(OBJ)/Win32/MIDASSound.o
AUDIOLIBS  = $(AUDIOLIBS) MIDAS/MIDASDLL.lib
!else
AUDIOFLAGS = $(AUDIOFLAGS) -DNOMIDAS
!endif

CFLAGSGLOBAL = -Gr -I. -Isrc -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000 -Isrc/Win32 -Imess/includes -Imess \
               -IZLIB $(AUDIOFLAGS) -W3 -nologo -MT \
               $(MAME_DEBUG) $(RELEASE_CANDIDATE) $(BETA_VERSION) $(VERSION) \
               $(MAME_NET) $(MAME_MMX) $(HAS_CPUS) $(HAS_SOUND) $(M68KDEF) \
			   -DMESS -DNEOFREE -DMAME32NAME="\"MESS32\"" -DMAMENAME="\"MESS\"" \
			   -DM_PI=3.14159265

CFLAGSDEBUG = -Zi -Od

CFLAGSOPTIMIZED = -DNDEBUG -Ox -G5 -Ob2

!ifdef DEBUG
OBJ = objdebug.mes
!else
OBJ = obj.mes
!endif

!ifdef DEBUG
CFLAGS = $(CFLAGSGLOBAL) $(CFLAGSDEBUG)
!else
CFLAGS = $(CFLAGSGLOBAL) $(CFLAGSOPTIMIZED)
!endif

!ifdef USE_FASTCALL
CFLAGS = $(CFLAGS) -DCLIB_DECL=__cdecl -DDECL_SPEC=__cdecl
!endif

WINDOWS_PROGRAM = -subsystem:windows
CONSOLE_PROGRAM = -subsystem:console

LDFLAGSGLOBAL = -machine:$(PROCESSOR_ARCHITECTURE) -nologo

LDFLAGSDEBUG = -debug:full
LDFLAGSOPTIMIZED = -release -incremental:no -map

!ifdef DEBUG
LDFLAGS = $(LDFLAGSGLOBAL) $(LDFLAGSDEBUG)
!else
LDFLAGS = $(LDFLAGSGLOBAL) $(LDFLAGSOPTIMIZED)
!endif

RCFLAGS = -l 0x409 -DNDEBUG -I./Win32 $(MAME_NET) $(MAME_MMX) -DMESS

LIBS   = kernel32.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib advapi32.lib \
         winmm.lib shell32.lib dinput.lib dxguid.lib vfw32.lib ZLIB\zlib.lib \
         $(AUDIOLIBS)

!ifdef MAME_NET
NET_OBJS = $(OBJ)/network.o $(OBJ)/Win32/net32.o $(OBJ)/Win32/netchat32.o
!else
NET_OBJS =
!endif

WIN32_OBJS = \
         $(OBJ)/Win32/osdepend.o \
         $(OBJ)/Win32/MAME32.o $(OBJ)/Win32/M32Util.o \
         $(OBJ)/Win32/DirectInput.o $(OBJ)/Win32/DIKeyboard.o $(OBJ)/Win32/DIJoystick.o \
         $(OBJ)/Win32/uclock.o \
         $(OBJ)/Win32/Display.o $(OBJ)/Win32/GDIDisplay.o $(OBJ)/Win32/DDrawWindow.o $(OBJ)/Win32/DDrawDisplay.o \
         $(OBJ)/Win32/DirectDraw.o $(OBJ)/Win32/RenderBitmap.o $(OBJ)/Win32/Dirty.o \
         $(OBJ)/Win32/led.o $(OBJ)/Win32/status.o \
         $(AUDIOOBJS) $(OBJ)/Win32/DirectSound.o $(OBJ)/Win32/NullSound.o \
         $(OBJ)/Win32/Keyboard.o $(OBJ)/Win32/Joystick.o $(OBJ)/Win32/Trak.o \
         $(OBJ)/Win32/file.o $(OBJ)/Win32/Directories.o $(OBJ)/Win32/mzip.o \
         $(OBJ)/Win32/debug.o \
         $(OBJ)/Win32/fmsynth.o $(OBJ)/Win32/NTFMSynth.o \
         $(OBJ)/Win32/audit32.o \
         $(OBJ)/Win32/mess32ui.o $(OBJ)/Win32/Properties.o $(OBJ)/Win32/ColumnEdit.o \
         $(OBJ)/Win32/Screenshot.o $(OBJ)/Win32/TreeView.o $(OBJ)/Win32/Splitters.o \
         $(OBJ)/Win32/options.o $(OBJ)/Win32/Bitmask.o $(OBJ)/Win32/DataMap.o \
         $(OBJ)/Win32/Avi.o \
         $(OBJ)/Win32/RenderMMX.o \
         $(OBJ)/Win32/dxdecode.o

CPUOBJS = \
          $(Z80OBJS) \
          $(OBJ)/cpu/i8085/i8085.o \
          $(OBJ)/cpu/m6502/m6502.o \
          $(OBJ)/cpu/h6280/h6280.o \
          $(OBJ)/cpu/i86/i86.o \
          $(OBJ)/cpu/nec/nec.o \
          $(OBJ)/cpu/i8039/i8039.o \
          $(OBJ)/cpu/m6800/m6800.o \
          $(OBJ)/cpu/m6805/m6805.o \
          $(OBJ)/cpu/m6809/m6809.o \
          $(M68KOBJS) \
          $(OBJ)/cpu/t11/t11.o \
          $(OBJ)/cpu/s2650/s2650.o \
          $(OBJ)/cpu/tms34010/tms34010.o $(OBJ)/cpu/tms34010/34010fld.o \
          $(OBJ)/cpu/tms9900/tms9980a.o \
          $(OBJ)/cpu/z8000/z8000.o \
          $(OBJ)/cpu/tms32010/tms32010.o \
          $(OBJ)/cpu/ccpu/ccpu.o $(OBJ)/vidhrdw/cinemat.o     \
          $(OBJ)/cpu/adsp2100/adsp2100.o \
          $(OBJ)/cpu/z80gb/z80gb.o $(OBJ)/cpu/z80gb/z80gbd.o \
          $(OBJ)/cpu/m6502/m65ce02.o \
          $(OBJ)/cpu/pdp1/pdp1.o \
          $(OBJ)/cpu/tms9900/tms9900.o \
          $(OBJ)/cpu/m6502/m6509.o \
          $(OBJ)/cpu/sc61860/sc61860.o \
          $(OBJ)/cpu/m6502/m4510.o \
          $(OBJ)/cpu/i86/i286.o \
          $(OBJ)/cpu/tms9900/tms9995.o \
          $(OBJ)/cpu/arm/arm.o

DBGOBJS = \
          $(OBJ)/cpu/z80/z80dasm.o \
          $(OBJ)/cpu/i8085/8085dasm.o \
          $(OBJ)/cpu/m6502/6502dasm.o \
          $(OBJ)/cpu/h6280/6280dasm.o \
          $(OBJ)/cpu/i86/i86dasm.o \
          $(OBJ)/cpu/nec/necdasm.o \
          $(OBJ)/cpu/i8039/8039dasm.o \
          $(OBJ)/cpu/m6800/6800dasm.o \
          $(OBJ)/cpu/m6805/6805dasm.o \
          $(OBJ)/cpu/m6809/6809dasm.o \
          $(OBJ)/cpu/konami/knmidasm.o \
          $(OBJ)/cpu/m68000/m68kdasm.o \
          $(OBJ)/cpu/t11/t11dasm.o \
          $(OBJ)/cpu/s2650/2650dasm.o \
          $(OBJ)/cpu/tms34010/34010dsm.o \
          $(OBJ)/cpu/tms9900/9900dasm.o \
          $(OBJ)/cpu/z8000/8000dasm.o \
          $(OBJ)/cpu/tms32010/32010dsm.o \
          $(OBJ)/cpu/ccpu/ccpudasm.o \
          $(OBJ)/cpu/adsp2100/2100dasm.o \
          $(OBJ)/cpu/pdp1/pdp1dasm.o \
          $(OBJ)/cpu/sc61860/disasm.o \
          $(OBJ)/cpu/arm/dasm.o

SNDOBJS = \
         $(OBJ)/sound/samples.o \
         $(OBJ)/sound/dac.o \
         $(OBJ)/sound/ay8910.o \
         $(OBJ)/sound/2151intf.o \
         $(OBJ)/sound/fm.o \
         $(OBJ)/sound/ym2151.o \
         $(OBJ)/sound/ym2413.o \
         $(OBJ)/sound/2608intf.o \
         $(OBJ)/sound/2610intf.o \
         $(OBJ)/sound/2612intf.o \
         $(OBJ)/sound/3812intf.o \
         $(OBJ)/sound/fmopl.o \
         $(OBJ)/sound/tms5220.o \
         $(OBJ)/sound/5220intf.o \
         $(OBJ)/sound/vlm5030.o \
         $(OBJ)/sound/pokey.o \
         $(OBJ)/sound/sn76496.o \
         $(OBJ)/sound/nes_apu.o \
         $(OBJ)/sound/astrocde.o \
		 $(OBJ)/sound/adpcm.o \
         $(OBJ)/sound/msm5205.o \
         $(OBJ)/sound/ymdeltat.o \
         $(OBJ)/sound/upd7759.o \
         $(OBJ)/sound/qsound.o \
         $(OBJ)/sound/tiasound.o \
         $(OBJ)/sound/tiaintf.o \
         $(OBJ)/sound/wave.o \
         $(OBJ)/sound/speaker.o

COREOBJS = \
         $(OBJ)/version.o $(OBJ)/mame.o \
         $(OBJ)/drawgfx.o $(OBJ)/common.o $(OBJ)/usrintrf.o $(OBJ)/ui_text.o \
         $(OBJ)/cpuintrf.o $(OBJ)/memory.o $(OBJ)/timer.o $(OBJ)/palette.o \
		 $(OBJ)/input.o $(OBJ)/inptport.o $(OBJ)/cheat.o $(OBJ)/unzip.o \
         $(OBJ)/audit.o $(OBJ)/info.o $(OBJ)/png.o $(OBJ)/artwork.o \
         $(OBJ)/tilemap.o $(OBJ)/sprite.o  $(OBJ)/gfxobj.o \
         $(OBJ)/state.o $(OBJ)/datafile.o $(OBJ)/hiscore.o \
		 $(CPUOBJS) \
         $(OBJ)/sndintrf.o \
         $(OBJ)/sound/streams.o $(OBJ)/sound/mixer.o \
         $(SNDOBJS) \
         $(OBJ)/sound/votrax.o \
         $(OBJ)/machine/z80fmly.o $(OBJ)/machine/6821pia.o \
         $(OBJ)/vidhrdw/generic.o $(OBJ)/vidhrdw/vector.o \
         $(OBJ)/vidhrdw/avgdvg.o $(OBJ)/machine/mathbox.o \
         $(OBJ)/machine/ticket.o $(OBJ)/machine/eeprom.o \
	 $(OBJ)/mamedbg.o $(OBJ)/window.o \
         $(OBJ)/profiler.o \
         $(DBGOBJS) \
         $(NET_OBJS) \
          $(OBJ)/mess/mess.o             \
          $(OBJ)/mess/system.o           \
          $(OBJ)/mess/config.o           \
          $(OBJ)/mess/filemngr.o         \
          $(OBJ)/mess/tapectrl.o         \
          $(OBJ)/mess/machine/6522via.o  \
          $(OBJ)/mess/machine/nec765.o   \
          $(OBJ)/mess/machine/dsk.o      \
          $(OBJ)/mess/machine/wd179x.o	\
          $(OBJ)/mess/sndhrdw/beep.o	\
		  $(OBJ)/mess/win32.o

DRV_OBJS = \
          $(OBJ)/mess/vidhrdw/tms9928a.o \
          $(OBJ)/mess/machine/coleco.o   \
          $(OBJ)/mess/vidhrdw/coleco.o   \
          $(OBJ)/mess/systems/coleco.o	\
          $(OBJ)/mess/vidhrdw/smsvdp.o   \
          $(OBJ)/mess/machine/sms.o      \
          $(OBJ)/mess/systems/sms.o      \
          $(OBJ)/mess/vidhrdw/genesis.o  \
          $(OBJ)/mess/machine/genesis.o  \
          $(OBJ)/mess/sndhrdw/genesis.o  \
          $(OBJ)/mess/systems/genesis.o	\
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
          $(OBJ)/mess/systems/a2600.o	\
          $(OBJ)/mess/vidhrdw/vectrex.o  \
          $(OBJ)/mess/machine/vectrex.o  \
          $(OBJ)/mess/systems/vectrex.o	\
          $(OBJ)/mess/machine/nes_mmc.o  \
          $(OBJ)/mess/vidhrdw/nes.o      \
          $(OBJ)/mess/machine/nes.o      \
          $(OBJ)/mess/systems/nes.o      \
          $(OBJ)/mess/vidhrdw/gb.o       \
          $(OBJ)/mess/machine/gb.o       \
          $(OBJ)/mess/systems/gb.o	\
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
          $(OBJ)/mess/sndhrdw/6581_.o	\
          $(OBJ)/mess/vidhrdw/m6847.o    \
          $(OBJ)/mess/machine/mc10.o     \
          $(OBJ)/mess/systems/mc10.o     \
          $(OBJ)/mess/formats/cocopak.o  \
          $(OBJ)/mess/vidhrdw/dragon.o   \
          $(OBJ)/mess/machine/dragon.o   \
          $(OBJ)/mess/systems/dragon.o	\
          $(OBJ)/mess/machine/trs80.o    \
          $(OBJ)/mess/vidhrdw/trs80.o    \
          $(OBJ)/mess/systems/trs80.o	\
          $(OBJ)/mess/machine/cgenie.o   \
          $(OBJ)/mess/vidhrdw/cgenie.o   \
          $(OBJ)/mess/sndhrdw/cgenie.o   \
          $(OBJ)/mess/systems/cgenie.o	\
          $(OBJ)/mess/vidhrdw/pdp1.o     \
          $(OBJ)/mess/machine/pdp1.o     \
          $(OBJ)/mess/systems/pdp1.o	\
          $(OBJ)/mess/machine/cpm_bios.o \
          $(OBJ)/mess/vidhrdw/kaypro.o   \
          $(OBJ)/mess/sndhrdw/kaypro.o   \
          $(OBJ)/mess/machine/kaypro.o   \
          $(OBJ)/mess/systems/kaypro.o   \
          $(OBJ)/mess/eventlst.o \
          $(OBJ)/mess/vidhrdw/border.o   \
          $(OBJ)/mess/vidhrdw/spectrum.o \
          $(OBJ)/mess/machine/spectrum.o \
          $(OBJ)/mess/systems/spectrum.o \
          $(OBJ)/mess/vidhrdw/zx.o       \
          $(OBJ)/mess/machine/zx.o       \
          $(OBJ)/mess/systems/zx.o	\
          $(OBJ)/mess/vidhrdw/apple1.o   \
          $(OBJ)/mess/machine/apple1.o   \
          $(OBJ)/mess/systems/apple1.o	\
          $(OBJ)/mess/machine/ay3600.o   \
          $(OBJ)/mess/machine/ap_disk2.o \
          $(OBJ)/mess/vidhrdw/apple2.o   \
          $(OBJ)/mess/machine/apple2.o   \
          $(OBJ)/mess/systems/apple2.o	\
          $(OBJ)/mess/sndhrdw/mac.o      \
          $(OBJ)/mess/machine/iwm.o      \
          $(OBJ)/mess/vidhrdw/mac.o      \
          $(OBJ)/mess/machine/mac.o      \
          $(OBJ)/mess/systems/mac.o	\
          $(OBJ)/mess/machine/tms9901.o  \
          $(OBJ)/mess/machine/ti99_4x.o  \
          $(OBJ)/mess/systems/ti99_4x.o  \
          $(OBJ)/mess/vidhrdw/astrocde.o \
          $(OBJ)/mess/machine/astrocde.o \
          $(OBJ)/mess/systems/astrocde.o	\
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
          $(OBJ)/mess/systems/pc.o	\
          $(OBJ)/mess/vidhrdw/saa5050.o   \
          $(OBJ)/mess/vidhrdw/p2000m.o    \
          $(OBJ)/mess/systems/p2000t.o    \
          $(OBJ)/mess/machine/p2000t.o    \
          $(OBJ)/mess/machine/mc6850.o    \
          $(OBJ)/mess/vidhrdw/uk101.o     \
          $(OBJ)/mess/machine/uk101.o     \
          $(OBJ)/mess/systems/uk101.o	\
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
          $(OBJ)/mess/vidhrdw/pcw16.o	\
          $(OBJ)/mess/vidhrdw/vdc.o      \
          $(OBJ)/mess/machine/pce.o      \
          $(OBJ)/mess/systems/pce.o	\
          $(OBJ)/mess/sndhrdw/dave.o     \
          $(OBJ)/mess/vidhrdw/epnick.o   \
          $(OBJ)/mess/vidhrdw/enterp.o   \
          $(OBJ)/mess/machine/enterp.o   \
          $(OBJ)/mess/systems/enterp.o	\
          $(OBJ)/mess/sndhrdw/scc.o      \
          $(OBJ)/mess/machine/msx.o      \
          $(OBJ)/mess/systems/msx.o	\
          $(OBJ)/mess/vidhrdw/kim1.o     \
          $(OBJ)/mess/machine/kim1.o     \
          $(OBJ)/mess/systems/kim1.o	\
          $(OBJ)/mess/vidhrdw/oric.o     \
          $(OBJ)/mess/machine/oric.o     \
          $(OBJ)/mess/systems/oric.o	\
          $(OBJ)/mess/vidhrdw/microtan.o \
          $(OBJ)/mess/machine/microtan.o \
          $(OBJ)/mess/systems/microtan.o \
          $(OBJ)/mess/vidhrdw/vtech1.o   \
          $(OBJ)/mess/machine/vtech1.o   \
          $(OBJ)/mess/systems/vtech1.o   \
          $(OBJ)/mess/vidhrdw/vtech2.o   \
          $(OBJ)/mess/machine/vtech2.o   \
          $(OBJ)/mess/systems/vtech2.o	\
          $(OBJ)/mess/vidhrdw/jupiter.o  \
          $(OBJ)/mess/machine/jupiter.o  \
          $(OBJ)/mess/systems/jupiter.o	\
          $(OBJ)/mess/vidhrdw/mbee.o     \
          $(OBJ)/mess/machine/mbee.o     \
          $(OBJ)/mess/systems/mbee.o	\
          $(OBJ)/mess/vidhrdw/advision.o \
          $(OBJ)/mess/machine/advision.o \
          $(OBJ)/mess/systems/advision.o	\
          $(OBJ)/mess/vidhrdw/nascom1.o  \
          $(OBJ)/mess/machine/nascom1.o  \
          $(OBJ)/mess/systems/nascom1.o	\
          $(OBJ)/mess/machine/i8271.o    \
          $(OBJ)/mess/vidhrdw/m6845.o    \
          $(OBJ)/mess/vidhrdw/bbc.o      \
          $(OBJ)/mess/machine/bbc.o      \
          $(OBJ)/mess/systems/bbc.o	\
          $(OBJ)/vidhrdw/cps1.o          \
          $(OBJ)/mess/systems/cpschngr.o	\
          $(OBJ)/mess/systems/mtx.o	\
          $(OBJ)/mess/machine/iwm_lisa.o     \
          $(OBJ)/mess/machine/lisa.o     \
          $(OBJ)/mess/systems/lisa.o	\
          $(OBJ)/mess/machine/atom.o     \
          $(OBJ)/mess/vidhrdw/atom.o     \
	  $(OBJ)/mess/systems/atom.o	 \
	  $(OBJ)/mess/systems/a310.o	\
          $(OBJ)/mess/machine/coupe.o    \
          $(OBJ)/mess/vidhrdw/coupe.o    \
          $(OBJ)/mess/systems/coupe.o	\
          $(OBJ)/mess/sndhrdw/mz700.o    \
          $(OBJ)/mess/machine/mz700.o    \
          $(OBJ)/mess/vidhrdw/mz700.o    \
          $(OBJ)/mess/systems/mz700.o    \
          $(OBJ)/mess/machine/pocketc.o  \
          $(OBJ)/mess/vidhrdw/pocketc.o  \
          $(OBJ)/mess/systems/pocketc.o  \
          $(OBJ)/mess/machine/aquarius.o \
          $(OBJ)/mess/vidhrdw/aquarius.o \
          $(OBJ)/mess/systems/aquarius.o


IMGTOOL_OBJS= \
		$(OBJ)/mess/tools/main.o \
		$(OBJ)/mess/tools/imgtool.o \
		$(OBJ)/mess/tools/pchd.o \
		$(OBJ)/mess/tools/rsdos.o \
		$(OBJ)/mess/tools/stream.o \
		$(OBJ)/mess/tools/stubs.o \
		$(OBJ)/mess/tools/crt.o \
		$(OBJ)/mess/tools/t64.o \
		$(OBJ)/unzip.o zlib\zlib.lib $(OBJ)/mess/config.o

!ifdef TINY_COMPILE
OBJS = $(TINY_OBJS)
TINYFLAGS = -DTINY_COMPILE -DTINY_NAME=$(TINY_NAME) -DNEOFREE
!else
!ifdef NEOFREE
OBJS = $(DRV_OBJS)
TINYFLAGS = -DNEOFREE
!else
OBJS = $(DRV_OBJS) $(NEO_OBJS)
TINYFLAGS =
!endif
!endif

CFLAGS = $(CFLAGS) $(TINYFLAGS)

RES    = $(OBJ)/Win32/MAME32.res


!ifdef HELPFILE
all: $(EXENAME) $(TOOLS) $(HELPFILE)
!else
all: $(EXENAME) $(TOOLS)
!endif

$(EXENAME): $(COREOBJS) $(WIN32_OBJS) $(OBJS) $(RES)
	$(LD) @<<
        $(LDFLAGS) $(WINDOWS_PROGRAM) -out:$(EXENAME) $(COREOBJS) $(WIN32_OBJS) $(OBJS) $(RES) $(LIBS) $(NET_LIBS)
<<

!ifdef HELPFILE
$(HELPFILE): src\Win32\Hlp\Mame32.cnt src\Win32\Hlp\Mame32.hlp
	@Makehelp.bat
!endif

romcmp: $(OBJ)/romcmp.o $(OBJ)/unzip.o $(OBJ)/Win32/dirent.o
	$(LD) $(LDFLAGS) -out:romcmp.exe $(CONSOLE_PROGRAM) $(OBJ)/romcmp.o $(OBJ)/unzip.o $(OBJ)/Win32/dirent.o $(LIBS)


dat2html.exe:	$(OBJ)/mess/tools/dat2html.o
	$(LD) $(LDFLAGS) -out:dat2html.exe $(CONSOLE_PROGRAM) $(OBJ)/mess/tools/mkhdimg.o


mkhdimg.exe:	$(OBJ)/mess/tools/mkhdimg.o
	$(LD) $(LDFLAGS) -out:mkhdimg.exe $(CONSOLE_PROGRAM) $(OBJ)/mess/tools/mkhdimg.o

imgtool.exe:	$(IMGTOOL_OBJS)
	$(LD) $(LDFLAGS) -out:imgtool.exe $(CONSOLE_PROGRAM) $(IMGTOOL_OBJS) setargv.obj

.asm.oa:
	$(ASM) -o $@ $(ASMFLAGS) $(ASMDEFS) $<

{src}.c{$(OBJ)}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/gensync}.c{$(OBJ)/cpu/gensync}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/z80}.c{$(OBJ)/cpu/z80}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/z80gb}.c{$(OBJ)/cpu/z80gb}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/z8000}.c{$(OBJ)/cpu/z8000}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/m6502}.c{$(OBJ)/cpu/m6502}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/h6280}.c{$(OBJ)/cpu/h6280}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/i86}.c{$(OBJ)/cpu/i86}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/nec}.c{$(OBJ)/cpu/nec}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/i8039}.c{$(OBJ)/cpu/i8039}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/i8085}.c{$(OBJ)/cpu/i8085}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/m6809}.c{$(OBJ)/cpu/m6809}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/konami}.c{$(OBJ)/cpu/konami}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/m68000}.c{$(OBJ)/cpu/m68000}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/s2650}.c{$(OBJ)/cpu/s2650}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/T11}.c{$(OBJ)/cpu/T11}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/tms34010}.c{$(OBJ)/cpu/tms34010}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/tms32010}.c{$(OBJ)/cpu/tms32010}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/tms9900}.c{$(OBJ)/cpu/tms9900}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/m6800}.c{$(OBJ)/cpu/m6800}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/m6805}.c{$(OBJ)/cpu/m6805}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/ccpu}.c{$(OBJ)/cpu/ccpu}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/adsp2100}.c{$(OBJ)/cpu/adsp2100}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/pdp1}.c{$(OBJ)/cpu/pdp1}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/mips}.c{$(OBJ)/cpu/mips}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/sc61860}.c{$(OBJ)/cpu/sc61860}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/arm}.c{$(OBJ)/cpu/arm}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/vidhrdw}.c{$(OBJ)/vidhrdw}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/sndhrdw}.c{$(OBJ)/sndhrdw}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/machine}.c{$(OBJ)/machine}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/drivers}.c{$(OBJ)/drivers}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/sound}.c{$(OBJ)/sound}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/Win32}.c{$(OBJ)/Win32}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/Win32}.rc{$(OBJ)/Win32}.res:
	$(RSC) $(RCFLAGS) -Fo$@ $<

{mess}.c{$(OBJ)/mess}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{mess/systems}.c{$(OBJ)/mess/systems}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{mess/vidhrdw}.c{$(OBJ)/mess/vidhrdw}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{mess/sndhrdw}.c{$(OBJ)/mess/sndhrdw}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{mess/machine}.c{$(OBJ)/mess/machine}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{mess/formats}.c{$(OBJ)/mess/formats}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{mess/tools}.c{$(OBJ)/mess/tools}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

CFLAGS_MAKE_68K = $(CFLAGS) -DWIN32
!ifdef USE_FASTCALL
CFLAGS_MAKE_68K = $(CFLAGS_MAKE_68K) -DFASTCALL
!endif

# generate C source files for the 68000 emulator
$(OBJ)/cpu/m68000/m68kops.h \
$(OBJ)/cpu/m68000/m68kops.c \
$(OBJ)/cpu/m68000/m68kopac.c \
$(OBJ)/cpu/m68000/m68kopdm.c \
$(OBJ)/cpu/m68000/m68kopnz.c : src/cpu/m68000/m68kmake.c src/cpu/m68000/m68k_in.c
	$(CC) $(CFLAGS_MAKE_68K) /Fe$(OBJ)/cpu/m68000/m68kmake.exe /Fo$(OBJ)/cpu/m68000/m68kmake.obj \
	src/cpu/m68000/m68kmake.c
	$(OBJ)\\cpu\\m68000\\m68kmake $(OBJ)/cpu/m68000 src/cpu/m68000/m68k_in.c

# generated C files for the 68000 emulator
{$(OBJ)/cpu/m68000}.c{$(OBJ)/cpu/m68000}.og:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

# generate asm source files for the 68000 emulator
$(OBJ)/cpu/m68000/68kem.asm:  src/cpu/m68000/make68k.c
	$(CC) $(CFLAGS_MAKE_68K) /Fe$(OBJ)/cpu/m68000/make68k.exe /Fo$(OBJ)/cpu/m68000/make68k.obj \
	src/cpu/m68000/make68k.c
	$(OBJ)\\cpu\\m68000\\make68k $@ $(OBJ)/cpu/m68000/comptab.asm

$(OBJ)/cpu/z80/z80.asm:  src/cpu/z80/makez80.c
	$(CC) $(CDEFS) $(CFLAGS) -Fo$(OBJ)/cpu/z80/makez80.exe src/cpu/z80/makez80.c
	$(OBJ)\\cpu\\z80\\makez80 $(Z80DEF) $(CDEFS) $(CFLAGS) $@

# dependencies
$(OBJ)/cpu/z80/z80.o:           src/cpu/z80/z80.c src/cpu/z80/z80.h src/cpu/z80/z80daa.h
$(OBJ)/cpu/z8000/z8000.o:       src/cpu/z8000/z8000.c src/cpu/z8000/z8000.h src/cpu/z8000/z8000cpu.h src/cpu/z8000/z8000dab.h src/cpu/z8000/z8000ops.c src/cpu/z8000/z8000tbl.c
$(OBJ)/cpu/s2650/s2650.o:       src/cpu/s2650/s2650.c src/cpu/s2650/s2650.h src/cpu/s2650/s2650cpu.h
$(OBJ)/cpu/h6280/h6280.o:       src/cpu/h6280/h6280.c src/cpu/h6280/h6280.h src/cpu/h6280/h6280ops.h src/cpu/h6280/tblh6280.c
$(OBJ)/cpu/i8039/i8039.o:       src/cpu/i8039/i8039.c src/cpu/i8039/i8039.h
$(OBJ)/cpu/i8085/i8085.o:       src/cpu/i8085/i8085.c src/cpu/i8085/i8085.h src/cpu/i8085/i8085cpu.h src/cpu/i8085/i8085daa.h
$(OBJ)/cpu/i86/i86.o:           src/cpu/i86/i86.c src/cpu/i86/instr86.c src/cpu/i86/i86.h src/cpu/i86/i86intf.h src/cpu/i86/ea.h src/cpu/i86/host.h src/cpu/i86/modrm.h
$(OBJ)/cpu/nec/nec.o:           src/cpu/nec/nec.c src/cpu/nec/nec.h src/cpu/nec/necintrf.h src/cpu/nec/necea.h src/cpu/nec/nechost.h src/cpu/nec/necinstr.h src/cpu/nec/necmodrm.h
$(OBJ)/cpu/m6502/m6502.o:       src/cpu/m6502/m6502.c src/cpu/m6502/m6502.h src/cpu/m6502/ops02.h src/cpu/m6502/t6502.c src/cpu/m6502/t65c02.c src/cpu/m6502/t65sc02.c src/cpu/m6502/t6510.c
$(OBJ)/cpu/m6502/m6509.o:       src/cpu/m6502/m6509.c src/cpu/m6502/m6509.h src/cpu/m6502/ops09.h src/cpu/m6502/t6509.c
$(OBJ)/cpu/m6800/m6800.o:	     src/cpu/m6800/m6800.c src/cpu/m6800/m6800.h src/cpu/m6800/6800ops.c src/cpu/m6800/6800tbl.c
$(OBJ)/cpu/m6805/m6805.o:	     src/cpu/m6805/m6805.c src/cpu/m6805/m6805.h src/cpu/m6805/6805ops.c
$(OBJ)/cpu/m6809/m6809.o:	     src/cpu/m6809/m6809.c src/cpu/m6809/m6809.h src/cpu/m6809/6809ops.c src/cpu/m6809/6809tbl.c
$(OBJ)/cpu/tms32010/tms32010.o: src/cpu/tms32010/tms32010.c src/cpu/tms32010/tms32010.h
$(OBJ)/cpu/tms34010/tms34010.o: src/cpu/tms34010/tms34010.c src/cpu/tms34010/tms34010.h src/cpu/tms34010/34010ops.c src/cpu/tms34010/34010tbl.c
$(OBJ)/cpu/tms9900/tms9900.o:   src/cpu/tms9900/tms9900.c src/cpu/tms9900/tms9900.h src/cpu/tms9900/9900stat.h
$(OBJ)/cpu/t11/t11.o:           src/cpu/t11/t11.c src/cpu/t11/t11.h src/cpu/t11/t11ops.c src/cpu/t11/t11table.c
$(OBJ)/cpu/m68000/m68kcpu.o:    $(OBJ)/cpu/m68000/m68kops.c src/cpu/m68000/m68kmake.c src/cpu/m68000/m68k_in.c
$(OBJ)/cpu/ccpu/ccpu.o:         src/cpu/ccpu/ccpu.c src/cpu/ccpu/ccpu.h src/cpu/ccpu/ccputabl.c
$(OBJ)/cpu/konami/konami.o:     src/cpu/konami/konami.c src/cpu/konami/konami.h src/cpu/konami/konamops.c src/cpu/konami/konamtbl.c
$(OBJ)/win32/mess32ui.o:		src/win32/mess32ui.c src/win32/win32ui.c src/win32/win32ui.h

.IGNORE:

maketree:
	md $(OBJ)
	md $(OBJ)\cpu
	md $(OBJ)\cpu\z80
	md $(OBJ)\cpu\z80gb
	md $(OBJ)\cpu\m6502
	md $(OBJ)\cpu\h6280
	md $(OBJ)\cpu\i86
	md $(OBJ)\cpu\nec
	md $(OBJ)\cpu\i8039
	md $(OBJ)\cpu\i8085
	md $(OBJ)\cpu\m6800
	md $(OBJ)\cpu\m6805
	md $(OBJ)\cpu\m6809
	md $(OBJ)\cpu\konami
	md $(OBJ)\cpu\m68000
	md $(OBJ)\cpu\s2650
	md $(OBJ)\cpu\t11
	md $(OBJ)\cpu\tms34010
	md $(OBJ)\cpu\tms9900
	md $(OBJ)\cpu\z8000
	md $(OBJ)\cpu\tms32010
	md $(OBJ)\cpu\ccpu
	md $(OBJ)\cpu\adsp2100
	md $(OBJ)\cpu\pdp1
	md $(OBJ)\cpu\mips
	md $(OBJ)\cpu\sc61860
	md $(OBJ)\cpu\arm
	md $(OBJ)\sound
	md $(OBJ)\drivers
	md $(OBJ)\machine
	md $(OBJ)\vidhrdw
	md $(OBJ)\sndhrdw
	md $(OBJ)\mess
	md $(OBJ)\mess\machine
	md $(OBJ)\mess\systems
	md $(OBJ)\mess\vidhrdw
	md $(OBJ)\mess\formats
	md $(OBJ)\mess\sndhrdw
	md $(OBJ)\mess\tools
	md $(OBJ)\Win32

clean:
	del $(OBJ)\*.o
	del $(OBJ)\*.a
	del $(OBJ)\cpu\z80\*.o
	del $(OBJ)\cpu\z80\*.oa
	del $(OBJ)\cpu\z80\*.asm
	del $(OBJ)\cpu\z80\*.exe
	del $(OBJ)\cpu\z80gb\*.o
	del $(OBJ)\cpu\m6502\*.o
	del $(OBJ)\cpu\h6280\*.o
	del $(OBJ)\cpu\i86\*.o
	del $(OBJ)\cpu\nec\*.o
	del $(OBJ)\cpu\i8039\*.o
	del $(OBJ)\cpu\i8085\*.o
	del $(OBJ)\cpu\m6800\*.o
	del $(OBJ)\cpu\m6800\*.oa
	del $(OBJ)\cpu\m6800\*.exe
	del $(OBJ)\cpu\m6805\*.o
	del $(OBJ)\cpu\m6809\*.o
	del $(OBJ)\cpu\konami\*.o
	del $(OBJ)\cpu\m68000\*.o
	del $(OBJ)\cpu\m68000\*.c
	del $(OBJ)\cpu\m68000\*.h
	del $(OBJ)\cpu\m68000\*.oa
	del $(OBJ)\cpu\m68000\*.og
	del $(OBJ)\cpu\m68000\*.asm
	del $(OBJ)\cpu\m68000\*.exe
	del $(OBJ)\cpu\s2650\*.o
	del $(OBJ)\cpu\t11\*.o
	del $(OBJ)\cpu\tms34010\*.o
	del $(OBJ)\cpu\tms9900\*.o
	del $(OBJ)\cpu\z8000\*.o
	del $(OBJ)\cpu\tms32010\*.o
	del $(OBJ)\cpu\ccpu\*.o
	del $(OBJ)\cpu\adsp2100\*.o
	del $(OBJ)\cpu\pdp1\*.o
	del $(OBJ)\cpu\mips\*.o
	del $(OBJ)\cpu\sc61860\*.o
	del $(OBJ)\cpu\arm\*.o
	del $(OBJ)\sound\*.o
	del $(OBJ)\drivers\*.o
	del $(OBJ)\machine\*.o
	del $(OBJ)\vidhrdw\*.o
	del $(OBJ)\sndhrdw\*.o
	del $(OBJ)\Win32\*.o
	del $(OBJ)\Win32\*.res
	del $(EXENAME)
	del $(OBJ)\mess\*.o
	del $(OBJ)\mess\machine\*.o
	del $(OBJ)\mess\systems\*.o
	del $(OBJ)\mess\vidhrdw\*.o
	del $(OBJ)\mess\formats\*.o
	del $(OBJ)\mess\sndhrdw\*.o
	del $(OBJ)\mess\tools\*.o
!ifdef HELPFILE
	del mame32.hlp
	del mame32.cnt
!endif

cleandebug:
	del $(OBJ)\*.o
	del $(OBJ)\cpu\z80\*.o
	del $(OBJ)\cpu\z80\*.oa
	del $(OBJ)\cpu\z80\*.asm
	del $(OBJ)\cpu\z80\*.exe
	del $(OBJ)\cpu\z80gb\*.o
	del $(OBJ)\cpu\m6502\*.o
	del $(OBJ)\cpu\h6280\*.o
	del $(OBJ)\cpu\i86\*.o
	del	$(OBJ)\cpu\nec\*.0
	del $(OBJ)\cpu\i8039\*.o
	del $(OBJ)\cpu\i8085\*.o
	del $(OBJ)\cpu\m6800\*.o
	del $(OBJ)\cpu\m6800\*.oa
	del $(OBJ)\cpu\m6800\*.exe
	del $(OBJ)\cpu\m6805\*.o
	del $(OBJ)\cpu\m6809\*.o
	del $(OBJ)\cpu\konami\*.o
	del $(OBJ)\cpu\m68000\*.o
	del $(OBJ)\cpu\m68000\*.c
	del $(OBJ)\cpu\m68000\*.h
	del $(OBJ)\cpu\m68000\*.oa
	del $(OBJ)\cpu\m68000\*.og
	del $(OBJ)\cpu\m68000\*.asm
	del $(OBJ)\cpu\m68000\*.exe
	del $(OBJ)\cpu\s2650\*.o
	del $(OBJ)\cpu\t11\*.o
	del $(OBJ)\cpu\tms34010\*.o
	del $(OBJ)\cpu\tms9900\*.o
	del $(OBJ)\cpu\z8000\*.o
	del $(OBJ)\cpu\tms32010\*.o
	del $(OBJ)\cpu\ccpu\*.o
	del $(OBJ)\cpu\adsp2100\*.o
	del $(OBJ)\cpu\pdp1\*.o
	del $(OBJ)\cpu\sc61860\*.o
	del $(OBJ)\cpu\arm\*.o
	del $(EXENAME)

cleantiny:
	del $(OBJ)\driver.o
	del $(OBJ)\usrintrf.o
	del $(OBJ)\cheat.o
	del $(OBJ)\Win32\*.o
