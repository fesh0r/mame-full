# before compiling for the first time, use "make maketree" to create all the
# necessary subdirectories

# Set the version here
VERSION = -DVERSION=37

# uncomment out the BETA_VERSION = line to build a beta version of MAME
BETA_VERSION = -DBETA_VERSION=9

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
TOOLS = romcmp

# exe name for MAME
EXENAME = mame32.exe

CC      = cl.exe
LD      = link.exe
RSC     = rc.exe
ASM     = nasmw.exe

ASMFLAGS = -f win32
ASMDEFS  =

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

CFLAGSGLOBAL = -Gr -I. -Isrc -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000 -Isrc/Win32 \
               -IZLIB $(AUDIOFLAGS) -W3 -nologo -MT \
               $(MAME_DEBUG) $(RELEASE_CANDIDATE) $(BETA_VERSION) $(VERSION) \
               $(MAME_NET) $(MAME_MMX) $(HAS_CPUS) $(HAS_SOUND) $(M68KDEF)

CFLAGSDEBUG = -Zi -Od

CFLAGSOPTIMIZED = -DNDEBUG -Ox -G5 -Ob2

!ifdef DEBUG
OBJ = objdebug
!else
OBJ = obj
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

RCFLAGS = -l 0x409 -DNDEBUG -I./Win32 $(MAME_NET) $(MAME_MMX)

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
         $(OBJ)/Win32/Win32ui.o $(OBJ)/Win32/Properties.o $(OBJ)/Win32/ColumnEdit.o \
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
          $(OBJ)/cpu/hd6309/hd6309.o \
          $(OBJ)/cpu/konami/konami.o \
          $(M68KOBJS) \
          $(OBJ)/cpu/t11/t11.o \
          $(OBJ)/cpu/s2650/s2650.o \
          $(OBJ)/cpu/tms34010/tms34010.o $(OBJ)/cpu/tms34010/34010fld.o \
          $(OBJ)/cpu/tms9900/tms9980a.o \
          $(OBJ)/cpu/z8000/z8000.o \
          $(OBJ)/cpu/tms32010/tms32010.o \
          $(OBJ)/cpu/ccpu/ccpu.o $(OBJ)/vidhrdw/cinemat.o \
          $(OBJ)/cpu/adsp2100/adsp2100.o \
          $(OBJ)/cpu/mips/mips.o

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
          $(OBJ)/cpu/hd6309/6309dasm.o \
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
          $(OBJ)/cpu/mips/mipsdasm.o

SNDOBJS = \
         $(OBJ)/sound/samples.o \
         $(OBJ)/sound/dac.o \
	 $(OBJ)/sound/ay8910.o \
         $(OBJ)/sound/2203intf.o \
         $(OBJ)/sound/2151intf.o \
         $(OBJ)/sound/fm.o \
         $(OBJ)/sound/ym2151.o \
         $(OBJ)/sound/ym2413.o \
         $(OBJ)/sound/2608intf.o \
         $(OBJ)/sound/2610intf.o \
         $(OBJ)/sound/2612intf.o \
         $(OBJ)/sound/3812intf.o \
         $(OBJ)/sound/ymz280b.o \
         $(OBJ)/sound/fmopl.o \
         $(OBJ)/sound/tms36xx.o \
         $(OBJ)/sound/tms5110.o \
         $(OBJ)/sound/tms5220.o \
         $(OBJ)/sound/5110intf.o \
         $(OBJ)/sound/5220intf.o \
         $(OBJ)/sound/vlm5030.o \
         $(OBJ)/sound/pokey.o \
         $(OBJ)/sound/sn76477.o \
         $(OBJ)/sound/sn76496.o \
         $(OBJ)/sound/nes_apu.o \
         $(OBJ)/sound/astrocde.o \
         $(OBJ)/sound/adpcm.o \
         $(OBJ)/sound/msm5205.o \
	 $(OBJ)/sound/namco.o \
         $(OBJ)/sound/ymdeltat.o \
	 $(OBJ)/sound/upd7759.o \
         $(OBJ)/sound/hc55516.o \
         $(OBJ)/sound/k007232.o \
         $(OBJ)/sound/k005289.o \
         $(OBJ)/sound/k051649.o \
	 $(OBJ)/sound/k053260.o \
	 $(OBJ)/sound/k054539.o \
         $(OBJ)/sound/segapcm.o \
	 $(OBJ)/sound/rf5c68.o \
         $(OBJ)/sound/cem3394.o \
         $(OBJ)/sound/c140.o \
	 $(OBJ)/sound/qsound.o \
	 $(OBJ)/sound/saa1099.o \
	 $(OBJ)/sound/iremga20.o \

COREOBJS = \
         $(OBJ)/driver.o \
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
         $(OBJ)/machine/8255ppi.o \
         $(OBJ)/vidhrdw/generic.o $(OBJ)/vidhrdw/vector.o \
         $(OBJ)/vidhrdw/avgdvg.o $(OBJ)/machine/mathbox.o \
         $(OBJ)/machine/ticket.o $(OBJ)/machine/eeprom.o \
         $(OBJ)/machine/6522via.o \
	 $(OBJ)/mamedbg.o $(OBJ)/window.o \
         $(OBJ)/profiler.o \
         $(DBGOBJS) \
         $(NET_OBJS)

DRV_OBJS = \
	$(OBJ)/drivers/pacman.o \
	$(OBJ)/machine/pacplus.o \
	$(OBJ)/machine/theglob.o \
	$(OBJ)/drivers/jrpacman.o $(OBJ)/vidhrdw/jrpacman.o \
	$(OBJ)/vidhrdw/pengo.o $(OBJ)/drivers/pengo.o \
	$(OBJ)/vidhrdw/cclimber.o $(OBJ)/sndhrdw/cclimber.o $(OBJ)/drivers/cclimber.o \
	$(OBJ)/drivers/yamato.o \
	$(OBJ)/vidhrdw/seicross.o $(OBJ)/sndhrdw/wiping.o $(OBJ)/drivers/seicross.o \
	$(OBJ)/vidhrdw/wiping.o $(OBJ)/drivers/wiping.o \
	$(OBJ)/vidhrdw/magmax.o $(OBJ)/drivers/magmax.o \
	$(OBJ)/vidhrdw/cop01.o $(OBJ)/drivers/cop01.o \
	$(OBJ)/vidhrdw/terracre.o $(OBJ)/drivers/terracre.o \
	$(OBJ)/vidhrdw/galivan.o $(OBJ)/drivers/galivan.o \
	$(OBJ)/vidhrdw/armedf.o $(OBJ)/drivers/armedf.o \
	$(OBJ)/machine/nb1413m3.o \
	$(OBJ)/vidhrdw/hyhoo.o $(OBJ)/drivers/hyhoo.o \
	$(OBJ)/vidhrdw/mjsikaku.o $(OBJ)/drivers/mjsikaku.o \
	$(OBJ)/vidhrdw/gionbana.o $(OBJ)/drivers/gionbana.o \
	$(OBJ)/vidhrdw/pstadium.o $(OBJ)/drivers/pstadium.o \
	$(OBJ)/drivers/safarir.o \
	$(OBJ)/vidhrdw/phoenix.o $(OBJ)/sndhrdw/phoenix.o $(OBJ)/drivers/phoenix.o \
	$(OBJ)/sndhrdw/pleiads.o \
	$(OBJ)/vidhrdw/naughtyb.o $(OBJ)/drivers/naughtyb.o \
	$(OBJ)/machine/geebee.o $(OBJ)/vidhrdw/geebee.o $(OBJ)/sndhrdw/geebee.o $(OBJ)/drivers/geebee.o \
	$(OBJ)/vidhrdw/warpwarp.o $(OBJ)/sndhrdw/warpwarp.o $(OBJ)/drivers/warpwarp.o \
	$(OBJ)/vidhrdw/tankbatt.o $(OBJ)/drivers/tankbatt.o \
	$(OBJ)/vidhrdw/galaxian.o $(OBJ)/sndhrdw/galaxian.o $(OBJ)/drivers/galaxian.o \
	$(OBJ)/vidhrdw/rallyx.o $(OBJ)/drivers/rallyx.o \
	$(OBJ)/drivers/locomotn.o \
	$(OBJ)/machine/bosco.o $(OBJ)/sndhrdw/bosco.o $(OBJ)/vidhrdw/bosco.o $(OBJ)/drivers/bosco.o \
	$(OBJ)/machine/galaga.o $(OBJ)/vidhrdw/galaga.o $(OBJ)/drivers/galaga.o \
	$(OBJ)/machine/digdug.o $(OBJ)/vidhrdw/digdug.o $(OBJ)/drivers/digdug.o \
	$(OBJ)/vidhrdw/xevious.o $(OBJ)/machine/xevious.o $(OBJ)/drivers/xevious.o \
	$(OBJ)/machine/superpac.o $(OBJ)/vidhrdw/superpac.o $(OBJ)/drivers/superpac.o \
	$(OBJ)/machine/phozon.o $(OBJ)/vidhrdw/phozon.o $(OBJ)/drivers/phozon.o \
	$(OBJ)/machine/mappy.o $(OBJ)/vidhrdw/mappy.o $(OBJ)/drivers/mappy.o \
	$(OBJ)/machine/grobda.o $(OBJ)/vidhrdw/grobda.o $(OBJ)/drivers/grobda.o \
	$(OBJ)/machine/gaplus.o $(OBJ)/vidhrdw/gaplus.o $(OBJ)/drivers/gaplus.o \
	$(OBJ)/machine/toypop.o $(OBJ)/vidhrdw/toypop.o $(OBJ)/drivers/toypop.o \
	$(OBJ)/machine/polepos.o $(OBJ)/vidhrdw/polepos.o $(OBJ)/sndhrdw/polepos.o $(OBJ)/drivers/polepos.o \
	$(OBJ)/vidhrdw/pacland.o $(OBJ)/drivers/pacland.o \
	$(OBJ)/vidhrdw/skykid.o $(OBJ)/drivers/skykid.o \
	$(OBJ)/vidhrdw/baraduke.o $(OBJ)/drivers/baraduke.o \
	$(OBJ)/vidhrdw/namcos86.o $(OBJ)/drivers/namcos86.o \
	$(OBJ)/machine/namcos1.o $(OBJ)/vidhrdw/namcos1.o $(OBJ)/drivers/namcos1.o \
	$(OBJ)/machine/namcos2.o $(OBJ)/vidhrdw/namcos2.o $(OBJ)/drivers/namcos2.o \
	$(OBJ)/vidhrdw/cosmic.o $(OBJ)/drivers/cosmic.o \
	$(OBJ)/vidhrdw/cheekyms.o $(OBJ)/drivers/cheekyms.o \
	$(OBJ)/vidhrdw/ladybug.o $(OBJ)/drivers/ladybug.o \
	$(OBJ)/vidhrdw/mrdo.o $(OBJ)/drivers/mrdo.o \
	$(OBJ)/machine/docastle.o $(OBJ)/vidhrdw/docastle.o $(OBJ)/drivers/docastle.o \
	$(OBJ)/vidhrdw/dkong.o $(OBJ)/sndhrdw/dkong.o $(OBJ)/drivers/dkong.o \
	$(OBJ)/vidhrdw/mario.o $(OBJ)/sndhrdw/mario.o $(OBJ)/drivers/mario.o \
	$(OBJ)/vidhrdw/popeye.o $(OBJ)/drivers/popeye.o \
	$(OBJ)/vidhrdw/punchout.o $(OBJ)/drivers/punchout.o \
	$(OBJ)/machine/rp5h01.o $(OBJ)/vidhrdw/ppu2c03b.o \
	$(OBJ)/machine/playch10.o $(OBJ)/vidhrdw/playch10.o $(OBJ)/drivers/playch10.o \
	$(OBJ)/machine/8080bw.o $(OBJ)/machine/74123.o \
	$(OBJ)/vidhrdw/8080bw.o $(OBJ)/sndhrdw/8080bw.o $(OBJ)/drivers/8080bw.o \
	$(OBJ)/vidhrdw/m79amb.o $(OBJ)/drivers/m79amb.o \
	$(OBJ)/sndhrdw/z80bw.o $(OBJ)/drivers/z80bw.o \
	$(OBJ)/drivers/lazercmd.o $(OBJ)/vidhrdw/lazercmd.o \
	$(OBJ)/drivers/meadows.o $(OBJ)/sndhrdw/meadows.o $(OBJ)/vidhrdw/meadows.o \
	$(OBJ)/machine/astrocde.o $(OBJ)/vidhrdw/astrocde.o \
	$(OBJ)/sndhrdw/astrocde.o $(OBJ)/sndhrdw/gorf.o $(OBJ)/drivers/astrocde.o \
	$(OBJ)/machine/mcr.o $(OBJ)/sndhrdw/mcr.o \
	$(OBJ)/vidhrdw/mcr12.o $(OBJ)/vidhrdw/mcr3.o \
	$(OBJ)/drivers/mcr1.o $(OBJ)/drivers/mcr2.o $(OBJ)/drivers/mcr3.o \
	$(OBJ)/vidhrdw/mcr68.o $(OBJ)/drivers/mcr68.o \
	$(OBJ)/vidhrdw/balsente.o $(OBJ)/drivers/balsente.o \
	$(OBJ)/vidhrdw/skychut.o $(OBJ)/drivers/skychut.o \
	$(OBJ)/drivers/olibochu.o \
	$(OBJ)/sndhrdw/irem.o \
	$(OBJ)/vidhrdw/mpatrol.o $(OBJ)/drivers/mpatrol.o \
	$(OBJ)/vidhrdw/troangel.o $(OBJ)/drivers/troangel.o \
	$(OBJ)/vidhrdw/yard.o $(OBJ)/drivers/yard.o \
	$(OBJ)/vidhrdw/travrusa.o $(OBJ)/drivers/travrusa.o \
	$(OBJ)/vidhrdw/m62.o $(OBJ)/drivers/m62.o \
	$(OBJ)/vidhrdw/vigilant.o $(OBJ)/drivers/vigilant.o \
	$(OBJ)/vidhrdw/m72.o $(OBJ)/sndhrdw/m72.o $(OBJ)/drivers/m72.o \
	$(OBJ)/vidhrdw/shisen.o $(OBJ)/drivers/shisen.o \
	$(OBJ)/machine/irem_cpu.o \
	$(OBJ)/vidhrdw/m90.o $(OBJ)/drivers/m90.o \
	$(OBJ)/vidhrdw/m92.o $(OBJ)/drivers/m92.o \
	$(OBJ)/drivers/m97.o \
	$(OBJ)/vidhrdw/m107.o $(OBJ)/drivers/m107.o \
	$(OBJ)/vidhrdw/gottlieb.o $(OBJ)/sndhrdw/gottlieb.o $(OBJ)/drivers/gottlieb.o \
	$(OBJ)/vidhrdw/crbaloon.o $(OBJ)/drivers/crbaloon.o \
	$(OBJ)/machine/qix.o $(OBJ)/vidhrdw/qix.o $(OBJ)/drivers/qix.o \
	$(OBJ)/machine/taitosj.o $(OBJ)/vidhrdw/taitosj.o $(OBJ)/drivers/taitosj.o \
	$(OBJ)/vidhrdw/bking2.o $(OBJ)/drivers/bking2.o \
	$(OBJ)/vidhrdw/gsword.o $(OBJ)/drivers/gsword.o $(OBJ)/machine/tait8741.o \
	$(OBJ)/machine/retofinv.o $(OBJ)/vidhrdw/retofinv.o $(OBJ)/drivers/retofinv.o \
	$(OBJ)/vidhrdw/tsamurai.o $(OBJ)/drivers/tsamurai.o \
	$(OBJ)/machine/flstory.o $(OBJ)/vidhrdw/flstory.o $(OBJ)/drivers/flstory.o \
	$(OBJ)/vidhrdw/gladiatr.o $(OBJ)/drivers/gladiatr.o \
	$(OBJ)/machine/lsasquad.o $(OBJ)/vidhrdw/lsasquad.o $(OBJ)/drivers/lsasquad.o \
	$(OBJ)/machine/bublbobl.o $(OBJ)/vidhrdw/bublbobl.o $(OBJ)/drivers/bublbobl.o \
	$(OBJ)/machine/mexico86.o $(OBJ)/vidhrdw/mexico86.o $(OBJ)/drivers/mexico86.o \
	$(OBJ)/vidhrdw/rastan.o $(OBJ)/sndhrdw/rastan.o $(OBJ)/drivers/rastan.o \
	$(OBJ)/machine/rainbow.o $(OBJ)/drivers/rainbow.o \
	$(OBJ)/machine/arkanoid.o $(OBJ)/vidhrdw/arkanoid.o $(OBJ)/drivers/arkanoid.o \
	$(OBJ)/vidhrdw/superqix.o $(OBJ)/drivers/superqix.o \
	$(OBJ)/vidhrdw/superman.o $(OBJ)/drivers/superman.o $(OBJ)/machine/cchip.o \
	$(OBJ)/vidhrdw/minivadr.o $(OBJ)/drivers/minivadr.o \
	$(OBJ)/vidhrdw/asuka.o $(OBJ)/drivers/asuka.o \
	$(OBJ)/vidhrdw/cadash.o $(OBJ)/drivers/cadash.o \
	$(OBJ)/machine/tnzs.o $(OBJ)/vidhrdw/tnzs.o $(OBJ)/drivers/tnzs.o \
	$(OBJ)/machine/buggychl.o $(OBJ)/vidhrdw/buggychl.o $(OBJ)/drivers/buggychl.o \
	$(OBJ)/machine/lkage.o $(OBJ)/vidhrdw/lkage.o $(OBJ)/drivers/lkage.o \
	$(OBJ)/vidhrdw/taito_l.o $(OBJ)/drivers/taito_l.o \
	$(OBJ)/vidhrdw/taito_b.o $(OBJ)/drivers/taito_b.o \
	$(OBJ)/vidhrdw/taitoic.o $(OBJ)/sndhrdw/taitosnd.o \
	$(OBJ)/vidhrdw/taito_f2.o $(OBJ)/drivers/taito_f2.o \
	$(OBJ)/machine/slapfght.o $(OBJ)/vidhrdw/slapfght.o $(OBJ)/drivers/slapfght.o \
	$(OBJ)/machine/twincobr.o $(OBJ)/vidhrdw/twincobr.o \
	$(OBJ)/drivers/twincobr.o $(OBJ)/drivers/wardner.o \
	$(OBJ)/machine/toaplan1.o $(OBJ)/vidhrdw/toaplan1.o $(OBJ)/drivers/toaplan1.o \
	$(OBJ)/vidhrdw/snowbros.o $(OBJ)/drivers/snowbros.o \
	$(OBJ)/vidhrdw/toaplan2.o $(OBJ)/drivers/toaplan2.o \
	$(OBJ)/vidhrdw/cave.o $(OBJ)/drivers/cave.o \
	$(OBJ)/drivers/kyugo.o $(OBJ)/vidhrdw/kyugo.o \
	$(OBJ)/machine/williams.o $(OBJ)/vidhrdw/williams.o $(OBJ)/sndhrdw/williams.o $(OBJ)/drivers/williams.o \
	$(OBJ)/vidhrdw/vulgus.o $(OBJ)/drivers/vulgus.o \
	$(OBJ)/vidhrdw/sonson.o $(OBJ)/drivers/sonson.o \
	$(OBJ)/vidhrdw/higemaru.o $(OBJ)/drivers/higemaru.o \
	$(OBJ)/vidhrdw/1942.o $(OBJ)/drivers/1942.o \
	$(OBJ)/vidhrdw/exedexes.o $(OBJ)/drivers/exedexes.o \
	$(OBJ)/vidhrdw/commando.o $(OBJ)/drivers/commando.o \
	$(OBJ)/vidhrdw/gng.o $(OBJ)/drivers/gng.o \
	$(OBJ)/vidhrdw/gunsmoke.o $(OBJ)/drivers/gunsmoke.o \
	$(OBJ)/vidhrdw/srumbler.o $(OBJ)/drivers/srumbler.o \
	$(OBJ)/vidhrdw/lwings.o $(OBJ)/drivers/lwings.o \
	$(OBJ)/vidhrdw/sidearms.o $(OBJ)/drivers/sidearms.o \
	$(OBJ)/vidhrdw/bionicc.o $(OBJ)/drivers/bionicc.o \
	$(OBJ)/vidhrdw/1943.o $(OBJ)/drivers/1943.o \
	$(OBJ)/vidhrdw/blktiger.o $(OBJ)/drivers/blktiger.o \
	$(OBJ)/vidhrdw/tigeroad.o $(OBJ)/drivers/tigeroad.o \
	$(OBJ)/vidhrdw/lastduel.o $(OBJ)/drivers/lastduel.o \
	$(OBJ)/vidhrdw/sf1.o $(OBJ)/drivers/sf1.o \
	$(OBJ)/machine/kabuki.o \
	$(OBJ)/vidhrdw/mitchell.o $(OBJ)/drivers/mitchell.o \
	$(OBJ)/vidhrdw/cbasebal.o $(OBJ)/drivers/cbasebal.o \
	$(OBJ)/vidhrdw/cps1.o $(OBJ)/drivers/cps1.o \
	$(OBJ)/drivers/zn.o \
	$(OBJ)/machine/capbowl.o $(OBJ)/vidhrdw/capbowl.o $(OBJ)/vidhrdw/tms34061.o $(OBJ)/drivers/capbowl.o \
	$(OBJ)/vidhrdw/blockade.o $(OBJ)/drivers/blockade.o \
	$(OBJ)/vidhrdw/vicdual.o $(OBJ)/drivers/vicdual.o \
	$(OBJ)/sndhrdw/carnival.o $(OBJ)/sndhrdw/depthch.o $(OBJ)/sndhrdw/invinco.o $(OBJ)/sndhrdw/pulsar.o \
	$(OBJ)/machine/segacrpt.o \
	$(OBJ)/vidhrdw/sega.o $(OBJ)/sndhrdw/sega.o $(OBJ)/machine/sega.o $(OBJ)/drivers/sega.o \
	$(OBJ)/vidhrdw/segar.o $(OBJ)/sndhrdw/segar.o $(OBJ)/machine/segar.o $(OBJ)/drivers/segar.o \
	$(OBJ)/vidhrdw/zaxxon.o $(OBJ)/sndhrdw/zaxxon.o $(OBJ)/drivers/zaxxon.o \
	$(OBJ)/drivers/congo.o \
	$(OBJ)/machine/turbo.o $(OBJ)/vidhrdw/turbo.o $(OBJ)/drivers/turbo.o \
	$(OBJ)/drivers/kopunch.o \
	$(OBJ)/vidhrdw/suprloco.o $(OBJ)/drivers/suprloco.o \
	$(OBJ)/vidhrdw/appoooh.o $(OBJ)/drivers/appoooh.o \
	$(OBJ)/vidhrdw/bankp.o $(OBJ)/drivers/bankp.o \
	$(OBJ)/vidhrdw/dotrikun.o $(OBJ)/drivers/dotrikun.o \
	$(OBJ)/vidhrdw/system1.o $(OBJ)/drivers/system1.o \
	$(OBJ)/machine/system16.o $(OBJ)/vidhrdw/system16.o $(OBJ)/sndhrdw/system16.o $(OBJ)/drivers/system16.o \
	$(OBJ)/vidhrdw/segac2.o $(OBJ)/drivers/segac2.o \
	$(OBJ)/vidhrdw/deniam.o $(OBJ)/drivers/deniam.o \
	$(OBJ)/machine/btime.o $(OBJ)/vidhrdw/btime.o $(OBJ)/drivers/btime.o \
	$(OBJ)/vidhrdw/astrof.o $(OBJ)/sndhrdw/astrof.o $(OBJ)/drivers/astrof.o \
	$(OBJ)/vidhrdw/kchamp.o $(OBJ)/drivers/kchamp.o \
	$(OBJ)/vidhrdw/firetrap.o $(OBJ)/drivers/firetrap.o \
	$(OBJ)/vidhrdw/brkthru.o $(OBJ)/drivers/brkthru.o \
	$(OBJ)/vidhrdw/shootout.o $(OBJ)/drivers/shootout.o \
	$(OBJ)/vidhrdw/sidepckt.o $(OBJ)/drivers/sidepckt.o \
	$(OBJ)/vidhrdw/exprraid.o $(OBJ)/drivers/exprraid.o \
	$(OBJ)/vidhrdw/pcktgal.o $(OBJ)/drivers/pcktgal.o \
	$(OBJ)/vidhrdw/battlera.o $(OBJ)/drivers/battlera.o \
	$(OBJ)/vidhrdw/actfancr.o $(OBJ)/drivers/actfancr.o \
	$(OBJ)/vidhrdw/dec8.o $(OBJ)/drivers/dec8.o \
	$(OBJ)/vidhrdw/karnov.o $(OBJ)/drivers/karnov.o \
	$(OBJ)/machine/dec0.o $(OBJ)/vidhrdw/dec0.o $(OBJ)/drivers/dec0.o \
	$(OBJ)/vidhrdw/stadhero.o $(OBJ)/drivers/stadhero.o \
	$(OBJ)/vidhrdw/madmotor.o $(OBJ)/drivers/madmotor.o \
	$(OBJ)/vidhrdw/vaportra.o $(OBJ)/drivers/vaportra.o \
	$(OBJ)/vidhrdw/cbuster.o $(OBJ)/drivers/cbuster.o \
	$(OBJ)/vidhrdw/darkseal.o $(OBJ)/drivers/darkseal.o \
	$(OBJ)/vidhrdw/supbtime.o $(OBJ)/drivers/supbtime.o \
	$(OBJ)/vidhrdw/cninja.o $(OBJ)/drivers/cninja.o \
	$(OBJ)/vidhrdw/tumblep.o $(OBJ)/drivers/tumblep.o \
	$(OBJ)/vidhrdw/funkyjet.o $(OBJ)/drivers/funkyjet.o \
	$(OBJ)/sndhrdw/senjyo.o $(OBJ)/vidhrdw/senjyo.o $(OBJ)/drivers/senjyo.o \
	$(OBJ)/vidhrdw/bombjack.o $(OBJ)/drivers/bombjack.o \
	$(OBJ)/vidhrdw/pbaction.o $(OBJ)/drivers/pbaction.o \
	$(OBJ)/vidhrdw/tehkanwc.o $(OBJ)/drivers/tehkanwc.o \
	$(OBJ)/vidhrdw/solomon.o $(OBJ)/drivers/solomon.o \
	$(OBJ)/vidhrdw/tecmo.o $(OBJ)/drivers/tecmo.o \
	$(OBJ)/vidhrdw/gaiden.o $(OBJ)/drivers/gaiden.o \
	$(OBJ)/vidhrdw/wc90.o $(OBJ)/drivers/wc90.o \
	$(OBJ)/vidhrdw/wc90b.o $(OBJ)/drivers/wc90b.o \
	$(OBJ)/vidhrdw/tecmo16.o $(OBJ)/drivers/tecmo16.o \
	$(OBJ)/machine/scramble.o $(OBJ)/sndhrdw/scramble.o $(OBJ)/drivers/scramble.o \
	$(OBJ)/vidhrdw/frogger.o $(OBJ)/sndhrdw/frogger.o $(OBJ)/drivers/frogger.o \
	$(OBJ)/drivers/scobra.o \
	$(OBJ)/vidhrdw/amidar.o $(OBJ)/drivers/amidar.o \
	$(OBJ)/vidhrdw/fastfred.o $(OBJ)/drivers/fastfred.o \
	$(OBJ)/sndhrdw/timeplt.o \
	$(OBJ)/vidhrdw/tutankhm.o $(OBJ)/drivers/tutankhm.o \
	$(OBJ)/drivers/junofrst.o \
	$(OBJ)/vidhrdw/pooyan.o $(OBJ)/drivers/pooyan.o \
	$(OBJ)/vidhrdw/timeplt.o $(OBJ)/drivers/timeplt.o \
	$(OBJ)/vidhrdw/megazone.o $(OBJ)/drivers/megazone.o \
	$(OBJ)/vidhrdw/pandoras.o $(OBJ)/drivers/pandoras.o \
	$(OBJ)/sndhrdw/gyruss.o $(OBJ)/vidhrdw/gyruss.o $(OBJ)/drivers/gyruss.o \
	$(OBJ)/machine/konami.o $(OBJ)/vidhrdw/trackfld.o $(OBJ)/sndhrdw/trackfld.o $(OBJ)/drivers/trackfld.o \
	$(OBJ)/vidhrdw/rocnrope.o $(OBJ)/drivers/rocnrope.o \
	$(OBJ)/vidhrdw/circusc.o $(OBJ)/drivers/circusc.o \
	$(OBJ)/vidhrdw/tp84.o $(OBJ)/drivers/tp84.o \
	$(OBJ)/vidhrdw/hyperspt.o $(OBJ)/drivers/hyperspt.o \
	$(OBJ)/vidhrdw/sbasketb.o $(OBJ)/drivers/sbasketb.o \
	$(OBJ)/vidhrdw/mikie.o $(OBJ)/drivers/mikie.o \
	$(OBJ)/vidhrdw/yiear.o $(OBJ)/drivers/yiear.o \
	$(OBJ)/vidhrdw/shaolins.o $(OBJ)/drivers/shaolins.o \
	$(OBJ)/vidhrdw/pingpong.o $(OBJ)/drivers/pingpong.o \
	$(OBJ)/vidhrdw/gberet.o $(OBJ)/drivers/gberet.o \
	$(OBJ)/vidhrdw/jailbrek.o $(OBJ)/drivers/jailbrek.o \
	$(OBJ)/vidhrdw/finalizr.o $(OBJ)/drivers/finalizr.o \
	$(OBJ)/vidhrdw/ironhors.o $(OBJ)/drivers/ironhors.o \
	$(OBJ)/machine/jackal.o $(OBJ)/vidhrdw/jackal.o $(OBJ)/drivers/jackal.o \
	$(OBJ)/machine/ddrible.o $(OBJ)/vidhrdw/ddrible.o $(OBJ)/drivers/ddrible.o \
	$(OBJ)/vidhrdw/contra.o $(OBJ)/drivers/contra.o \
	$(OBJ)/vidhrdw/combatsc.o $(OBJ)/drivers/combatsc.o \
	$(OBJ)/vidhrdw/hcastle.o $(OBJ)/drivers/hcastle.o \
	$(OBJ)/vidhrdw/nemesis.o $(OBJ)/drivers/nemesis.o \
	$(OBJ)/vidhrdw/konamiic.o \
	$(OBJ)/vidhrdw/rockrage.o $(OBJ)/drivers/rockrage.o \
	$(OBJ)/vidhrdw/flkatck.o $(OBJ)/drivers/flkatck.o \
	$(OBJ)/vidhrdw/fastlane.o $(OBJ)/drivers/fastlane.o \
	$(OBJ)/vidhrdw/labyrunr.o $(OBJ)/drivers/labyrunr.o \
	$(OBJ)/vidhrdw/battlnts.o $(OBJ)/drivers/battlnts.o \
	$(OBJ)/vidhrdw/bladestl.o $(OBJ)/drivers/bladestl.o \
	$(OBJ)/machine/ajax.o $(OBJ)/vidhrdw/ajax.o $(OBJ)/drivers/ajax.o \
	$(OBJ)/vidhrdw/thunderx.o $(OBJ)/drivers/thunderx.o \
	$(OBJ)/vidhrdw/mainevt.o $(OBJ)/drivers/mainevt.o \
	$(OBJ)/vidhrdw/88games.o $(OBJ)/drivers/88games.o \
	$(OBJ)/vidhrdw/gbusters.o $(OBJ)/drivers/gbusters.o \
	$(OBJ)/vidhrdw/crimfght.o $(OBJ)/drivers/crimfght.o \
	$(OBJ)/vidhrdw/spy.o $(OBJ)/drivers/spy.o \
	$(OBJ)/vidhrdw/bottom9.o $(OBJ)/drivers/bottom9.o \
	$(OBJ)/vidhrdw/blockhl.o $(OBJ)/drivers/blockhl.o \
	$(OBJ)/vidhrdw/aliens.o $(OBJ)/drivers/aliens.o \
	$(OBJ)/vidhrdw/surpratk.o $(OBJ)/drivers/surpratk.o \
	$(OBJ)/vidhrdw/parodius.o $(OBJ)/drivers/parodius.o \
	$(OBJ)/vidhrdw/rollerg.o $(OBJ)/drivers/rollerg.o \
	$(OBJ)/vidhrdw/xexex.o $(OBJ)/drivers/xexex.o \
	$(OBJ)/vidhrdw/gijoe.o $(OBJ)/drivers/gijoe.o \
	$(OBJ)/machine/simpsons.o $(OBJ)/vidhrdw/simpsons.o $(OBJ)/drivers/simpsons.o \
	$(OBJ)/vidhrdw/vendetta.o $(OBJ)/drivers/vendetta.o \
	$(OBJ)/vidhrdw/twin16.o $(OBJ)/drivers/twin16.o \
	$(OBJ)/vidhrdw/gradius3.o $(OBJ)/drivers/gradius3.o \
	$(OBJ)/vidhrdw/tmnt.o $(OBJ)/drivers/tmnt.o \
	$(OBJ)/vidhrdw/xmen.o $(OBJ)/drivers/xmen.o \
	$(OBJ)/vidhrdw/wecleman.o $(OBJ)/drivers/wecleman.o \
	$(OBJ)/vidhrdw/chqflag.o $(OBJ)/drivers/chqflag.o \
	$(OBJ)/vidhrdw/ultraman.o $(OBJ)/drivers/ultraman.o \
	$(OBJ)/vidhrdw/exidy.o $(OBJ)/sndhrdw/exidy.o $(OBJ)/drivers/exidy.o \
	$(OBJ)/sndhrdw/targ.o \
	$(OBJ)/vidhrdw/circus.o $(OBJ)/drivers/circus.o \
	$(OBJ)/vidhrdw/starfire.o $(OBJ)/drivers/starfire.o \
	$(OBJ)/vidhrdw/victory.o $(OBJ)/drivers/victory.o \
	$(OBJ)/sndhrdw/exidy440.o $(OBJ)/vidhrdw/exidy440.o $(OBJ)/drivers/exidy440.o \
	$(OBJ)/machine/atari_vg.o \
	$(OBJ)/machine/asteroid.o $(OBJ)/sndhrdw/asteroid.o \
	$(OBJ)/vidhrdw/llander.o $(OBJ)/sndhrdw/llander.o $(OBJ)/drivers/asteroid.o \
	$(OBJ)/drivers/bwidow.o \
	$(OBJ)/sndhrdw/bzone.o	$(OBJ)/drivers/bzone.o \
	$(OBJ)/sndhrdw/redbaron.o \
	$(OBJ)/drivers/tempest.o \
	$(OBJ)/machine/starwars.o $(OBJ)/machine/swmathbx.o \
	$(OBJ)/drivers/starwars.o $(OBJ)/sndhrdw/starwars.o \
	$(OBJ)/machine/mhavoc.o $(OBJ)/drivers/mhavoc.o \
	$(OBJ)/drivers/quantum.o \
	$(OBJ)/machine/atarifb.o $(OBJ)/vidhrdw/atarifb.o $(OBJ)/drivers/atarifb.o \
	$(OBJ)/machine/sprint2.o $(OBJ)/vidhrdw/sprint2.o $(OBJ)/drivers/sprint2.o \
	$(OBJ)/machine/sbrkout.o $(OBJ)/vidhrdw/sbrkout.o $(OBJ)/drivers/sbrkout.o \
	$(OBJ)/machine/dominos.o $(OBJ)/vidhrdw/dominos.o $(OBJ)/drivers/dominos.o \
	$(OBJ)/vidhrdw/nitedrvr.o $(OBJ)/machine/nitedrvr.o $(OBJ)/drivers/nitedrvr.o \
	$(OBJ)/vidhrdw/bsktball.o $(OBJ)/machine/bsktball.o $(OBJ)/drivers/bsktball.o \
	$(OBJ)/vidhrdw/copsnrob.o $(OBJ)/machine/copsnrob.o $(OBJ)/drivers/copsnrob.o \
	$(OBJ)/machine/avalnche.o $(OBJ)/vidhrdw/avalnche.o $(OBJ)/drivers/avalnche.o \
	$(OBJ)/machine/subs.o $(OBJ)/vidhrdw/subs.o $(OBJ)/drivers/subs.o \
	$(OBJ)/vidhrdw/canyon.o $(OBJ)/drivers/canyon.o \
	$(OBJ)/vidhrdw/skydiver.o $(OBJ)/drivers/skydiver.o \
	$(OBJ)/machine/videopin.o $(OBJ)/vidhrdw/videopin.o $(OBJ)/drivers/videopin.o \
	$(OBJ)/vidhrdw/warlord.o $(OBJ)/drivers/warlord.o \
	$(OBJ)/vidhrdw/centiped.o $(OBJ)/drivers/centiped.o \
	$(OBJ)/vidhrdw/milliped.o $(OBJ)/drivers/milliped.o \
	$(OBJ)/vidhrdw/qwakprot.o $(OBJ)/drivers/qwakprot.o \
	$(OBJ)/vidhrdw/kangaroo.o $(OBJ)/drivers/kangaroo.o \
	$(OBJ)/vidhrdw/arabian.o $(OBJ)/drivers/arabian.o \
	$(OBJ)/machine/missile.o $(OBJ)/vidhrdw/missile.o $(OBJ)/drivers/missile.o \
	$(OBJ)/vidhrdw/foodf.o $(OBJ)/drivers/foodf.o \
	$(OBJ)/vidhrdw/liberatr.o $(OBJ)/drivers/liberatr.o \
	$(OBJ)/vidhrdw/ccastles.o $(OBJ)/drivers/ccastles.o \
	$(OBJ)/vidhrdw/cloak.o $(OBJ)/drivers/cloak.o \
	$(OBJ)/vidhrdw/cloud9.o $(OBJ)/drivers/cloud9.o \
	$(OBJ)/machine/jedi.o $(OBJ)/vidhrdw/jedi.o $(OBJ)/sndhrdw/jedi.o $(OBJ)/drivers/jedi.o \
	$(OBJ)/machine/atarigen.o $(OBJ)/sndhrdw/atarijsa.o $(OBJ)/vidhrdw/ataripf.o \
	$(OBJ)/vidhrdw/atarimo.o $(OBJ)/vidhrdw/atarian.o $(OBJ)/vidhrdw/atarirle.o \
	$(OBJ)/machine/slapstic.o \
	$(OBJ)/vidhrdw/atarisy1.o $(OBJ)/drivers/atarisy1.o \
	$(OBJ)/vidhrdw/atarisy2.o $(OBJ)/drivers/atarisy2.o \
	$(OBJ)/machine/irobot.o $(OBJ)/vidhrdw/irobot.o $(OBJ)/drivers/irobot.o \
	$(OBJ)/machine/harddriv.o $(OBJ)/vidhrdw/harddriv.o $(OBJ)/drivers/harddriv.o \
	$(OBJ)/vidhrdw/gauntlet.o $(OBJ)/drivers/gauntlet.o \
	$(OBJ)/vidhrdw/atetris.o $(OBJ)/drivers/atetris.o \
	$(OBJ)/vidhrdw/toobin.o $(OBJ)/drivers/toobin.o \
	$(OBJ)/vidhrdw/vindictr.o $(OBJ)/drivers/vindictr.o \
	$(OBJ)/vidhrdw/klax.o $(OBJ)/drivers/klax.o \
	$(OBJ)/vidhrdw/blstroid.o $(OBJ)/drivers/blstroid.o \
	$(OBJ)/vidhrdw/xybots.o $(OBJ)/drivers/xybots.o \
	$(OBJ)/vidhrdw/eprom.o $(OBJ)/drivers/eprom.o \
	$(OBJ)/vidhrdw/skullxbo.o $(OBJ)/drivers/skullxbo.o \
	$(OBJ)/vidhrdw/badlands.o $(OBJ)/drivers/badlands.o \
	$(OBJ)/vidhrdw/cyberbal.o $(OBJ)/sndhrdw/cyberbal.o $(OBJ)/drivers/cyberbal.o \
	$(OBJ)/vidhrdw/rampart.o $(OBJ)/drivers/rampart.o \
	$(OBJ)/vidhrdw/shuuz.o $(OBJ)/drivers/shuuz.o \
	$(OBJ)/vidhrdw/atarig1.o $(OBJ)/drivers/atarig1.o \
	$(OBJ)/vidhrdw/thunderj.o $(OBJ)/drivers/thunderj.o \
	$(OBJ)/vidhrdw/batman.o $(OBJ)/drivers/batman.o \
	$(OBJ)/vidhrdw/relief.o $(OBJ)/drivers/relief.o \
	$(OBJ)/vidhrdw/offtwall.o $(OBJ)/drivers/offtwall.o \
	$(OBJ)/vidhrdw/arcadecl.o $(OBJ)/drivers/arcadecl.o \
	$(OBJ)/vidhrdw/rockola.o $(OBJ)/sndhrdw/rockola.o $(OBJ)/drivers/rockola.o \
	$(OBJ)/vidhrdw/lasso.o $(OBJ)/drivers/lasso.o \
	$(OBJ)/drivers/munchmo.o $(OBJ)/vidhrdw/munchmo.o \
	$(OBJ)/vidhrdw/marvins.o $(OBJ)/drivers/marvins.o \
	$(OBJ)/drivers/hal21.o \
	$(OBJ)/vidhrdw/snk.o $(OBJ)/drivers/snk.o \
	$(OBJ)/vidhrdw/snk68.o $(OBJ)/drivers/snk68.o \
	$(OBJ)/vidhrdw/prehisle.o $(OBJ)/drivers/prehisle.o \
	$(OBJ)/vidhrdw/alpha68k.o $(OBJ)/drivers/alpha68k.o \
	$(OBJ)/vidhrdw/champbas.o $(OBJ)/drivers/champbas.o \
	$(OBJ)/machine/exctsccr.o $(OBJ)/vidhrdw/exctsccr.o $(OBJ)/drivers/exctsccr.o \
	$(OBJ)/drivers/scregg.o \
	$(OBJ)/vidhrdw/tagteam.o $(OBJ)/drivers/tagteam.o \
	$(OBJ)/vidhrdw/ssozumo.o $(OBJ)/drivers/ssozumo.o \
	$(OBJ)/vidhrdw/mystston.o $(OBJ)/drivers/mystston.o \
	$(OBJ)/vidhrdw/bogeyman.o $(OBJ)/drivers/bogeyman.o \
	$(OBJ)/vidhrdw/matmania.o $(OBJ)/drivers/matmania.o $(OBJ)/machine/maniach.o \
	$(OBJ)/vidhrdw/renegade.o $(OBJ)/drivers/renegade.o \
	$(OBJ)/vidhrdw/xain.o $(OBJ)/drivers/xain.o \
	$(OBJ)/vidhrdw/battlane.o $(OBJ)/drivers/battlane.o \
	$(OBJ)/vidhrdw/ddragon.o $(OBJ)/drivers/ddragon.o \
	$(OBJ)/vidhrdw/spdodgeb.o $(OBJ)/drivers/spdodgeb.o \
	$(OBJ)/vidhrdw/ddragon3.o $(OBJ)/drivers/ddragon3.o \
	$(OBJ)/vidhrdw/blockout.o $(OBJ)/drivers/blockout.o \
	$(OBJ)/machine/berzerk.o $(OBJ)/vidhrdw/berzerk.o $(OBJ)/sndhrdw/berzerk.o $(OBJ)/drivers/berzerk.o \
	$(OBJ)/vidhrdw/gameplan.o $(OBJ)/drivers/gameplan.o \
	$(OBJ)/vidhrdw/route16.o $(OBJ)/drivers/route16.o \
	$(OBJ)/vidhrdw/ttmahjng.o $(OBJ)/drivers/ttmahjng.o \
	$(OBJ)/vidhrdw/zac2650.o $(OBJ)/drivers/zac2650.o \
	$(OBJ)/vidhrdw/zaccaria.o $(OBJ)/drivers/zaccaria.o \
	$(OBJ)/vidhrdw/nova2001.o $(OBJ)/drivers/nova2001.o \
	$(OBJ)/vidhrdw/pkunwar.o $(OBJ)/drivers/pkunwar.o \
	$(OBJ)/vidhrdw/ninjakd2.o $(OBJ)/drivers/ninjakd2.o \
	$(OBJ)/vidhrdw/mnight.o $(OBJ)/drivers/mnight.o \
	$(OBJ)/vidhrdw/bjtwin.o $(OBJ)/drivers/bjtwin.o \
	$(OBJ)/vidhrdw/exterm.o $(OBJ)/drivers/exterm.o \
	$(OBJ)/machine/wmsyunit.o $(OBJ)/vidhrdw/wmsyunit.o $(OBJ)/drivers/wmsyunit.o \
	$(OBJ)/machine/wmstunit.o $(OBJ)/vidhrdw/wmstunit.o $(OBJ)/drivers/wmstunit.o \
	$(OBJ)/machine/wmswolfu.o $(OBJ)/drivers/wmswolfu.o \
	$(OBJ)/vidhrdw/jack.o $(OBJ)/drivers/jack.o \
	$(OBJ)/sndhrdw/cinemat.o $(OBJ)/drivers/cinemat.o \
	$(OBJ)/machine/cchasm.o $(OBJ)/vidhrdw/cchasm.o $(OBJ)/sndhrdw/cchasm.o $(OBJ)/drivers/cchasm.o \
	$(OBJ)/vidhrdw/thepit.o $(OBJ)/drivers/thepit.o \
	$(OBJ)/machine/bagman.o $(OBJ)/vidhrdw/bagman.o $(OBJ)/drivers/bagman.o \
	$(OBJ)/vidhrdw/wiz.o $(OBJ)/drivers/wiz.o \
	$(OBJ)/vidhrdw/kncljoe.o $(OBJ)/drivers/kncljoe.o \
	$(OBJ)/machine/stfight.o $(OBJ)/vidhrdw/stfight.o $(OBJ)/drivers/stfight.o \
	$(OBJ)/sndhrdw/seibu.o \
	$(OBJ)/vidhrdw/dynduke.o $(OBJ)/drivers/dynduke.o \
	$(OBJ)/vidhrdw/raiden.o $(OBJ)/drivers/raiden.o \
	$(OBJ)/vidhrdw/dcon.o $(OBJ)/drivers/dcon.o \
	$(OBJ)/vidhrdw/cabal.o $(OBJ)/drivers/cabal.o \
	$(OBJ)/vidhrdw/toki.o $(OBJ)/drivers/toki.o \
	$(OBJ)/vidhrdw/bloodbro.o $(OBJ)/drivers/bloodbro.o \
	$(OBJ)/vidhrdw/exerion.o $(OBJ)/drivers/exerion.o \
	$(OBJ)/vidhrdw/aeroboto.o $(OBJ)/drivers/aeroboto.o \
	$(OBJ)/vidhrdw/citycon.o $(OBJ)/drivers/citycon.o \
	$(OBJ)/vidhrdw/pinbo.o $(OBJ)/drivers/pinbo.o \
	$(OBJ)/vidhrdw/psychic5.o $(OBJ)/drivers/psychic5.o \
	$(OBJ)/vidhrdw/ginganin.o $(OBJ)/drivers/ginganin.o \
	$(OBJ)/vidhrdw/skyfox.o $(OBJ)/drivers/skyfox.o \
	$(OBJ)/vidhrdw/cischeat.o $(OBJ)/drivers/cischeat.o \
	$(OBJ)/vidhrdw/megasys1.o $(OBJ)/drivers/megasys1.o \
	$(OBJ)/vidhrdw/rpunch.o $(OBJ)/drivers/rpunch.o \
	$(OBJ)/vidhrdw/tail2nos.o $(OBJ)/drivers/tail2nos.o \
	$(OBJ)/vidhrdw/pipedrm.o $(OBJ)/drivers/pipedrm.o \
	$(OBJ)/vidhrdw/aerofgt.o $(OBJ)/drivers/aerofgt.o \
	$(OBJ)/vidhrdw/psikyo.o $(OBJ)/drivers/psikyo.o \
	$(OBJ)/machine/8254pit.o $(OBJ)/drivers/leland.o $(OBJ)/vidhrdw/leland.o $(OBJ)/sndhrdw/leland.o \
	$(OBJ)/drivers/ataxx.o \
	$(OBJ)/vidhrdw/marineb.o $(OBJ)/drivers/marineb.o \
	$(OBJ)/vidhrdw/funkybee.o $(OBJ)/drivers/funkybee.o \
	$(OBJ)/vidhrdw/zodiack.o $(OBJ)/drivers/zodiack.o \
	$(OBJ)/vidhrdw/espial.o $(OBJ)/drivers/espial.o \
	$(OBJ)/vidhrdw/vastar.o $(OBJ)/drivers/vastar.o \
	$(OBJ)/vidhrdw/splash.o $(OBJ)/drivers/splash.o \
	$(OBJ)/vidhrdw/gaelco.o $(OBJ)/drivers/gaelco.o \
	$(OBJ)/vidhrdw/kaneko16.o $(OBJ)/drivers/kaneko16.o \
	$(OBJ)/vidhrdw/galpanic.o $(OBJ)/drivers/galpanic.o \
	$(OBJ)/vidhrdw/airbustr.o $(OBJ)/drivers/airbustr.o \
	$(OBJ)/machine/neogeo.o $(OBJ)/machine/pd4990a.o $(OBJ)/vidhrdw/neogeo.o $(OBJ)/drivers/neogeo.o \
	$(OBJ)/vidhrdw/hanaawas.o $(OBJ)/drivers/hanaawas.o \
	$(OBJ)/vidhrdw/seta.o $(OBJ)/sndhrdw/seta.o $(OBJ)/drivers/seta.o \
	$(OBJ)/vidhrdw/powerins.o $(OBJ)/drivers/powerins.o \
	$(OBJ)/vidhrdw/ohmygod.o $(OBJ)/drivers/ohmygod.o \
	$(OBJ)/drivers/shanghai.o \
	$(OBJ)/vidhrdw/shangha3.o $(OBJ)/drivers/shangha3.o \
	$(OBJ)/vidhrdw/goindol.o $(OBJ)/drivers/goindol.o \
	$(OBJ)/vidhrdw/bssoccer.o $(OBJ)/drivers/bssoccer.o \
	$(OBJ)/vidhrdw/gundealr.o $(OBJ)/drivers/gundealr.o \
	$(OBJ)/vidhrdw/dooyong.o $(OBJ)/drivers/dooyong.o \
	$(OBJ)/vidhrdw/pushman.o $(OBJ)/drivers/pushman.o \
	$(OBJ)/vidhrdw/zerozone.o $(OBJ)/drivers/zerozone.o \
	$(OBJ)/vidhrdw/galspnbl.o $(OBJ)/drivers/galspnbl.o \
	$(OBJ)/drivers/ladyfrog.o \
	$(OBJ)/vidhrdw/playmark.o $(OBJ)/drivers/playmark.o \
	$(OBJ)/vidhrdw/thief.o $(OBJ)/drivers/thief.o \
	$(OBJ)/vidhrdw/mrflea.o $(OBJ)/drivers/mrflea.o \
	$(OBJ)/vidhrdw/leprechn.o $(OBJ)/drivers/leprechn.o \
	$(OBJ)/machine/beezer.o $(OBJ)/vidhrdw/beezer.o $(OBJ)/drivers/beezer.o \
	$(OBJ)/vidhrdw/spacefb.o $(OBJ)/drivers/spacefb.o \
	$(OBJ)/vidhrdw/blueprnt.o $(OBJ)/drivers/blueprnt.o \
	$(OBJ)/drivers/omegrace.o \
	$(OBJ)/vidhrdw/dday.o $(OBJ)/drivers/dday.o \
	$(OBJ)/vidhrdw/hexa.o $(OBJ)/drivers/hexa.o \
	$(OBJ)/vidhrdw/redalert.o $(OBJ)/sndhrdw/redalert.o $(OBJ)/drivers/redalert.o \
	$(OBJ)/machine/spiders.o $(OBJ)/vidhrdw/crtc6845.o $(OBJ)/vidhrdw/spiders.o $(OBJ)/drivers/spiders.o \
	$(OBJ)/machine/stactics.o $(OBJ)/vidhrdw/stactics.o $(OBJ)/drivers/stactics.o \
	$(OBJ)/vidhrdw/kingobox.o $(OBJ)/drivers/kingobox.o \
	$(OBJ)/vidhrdw/speedbal.o $(OBJ)/drivers/speedbal.o \
	$(OBJ)/vidhrdw/sauro.o $(OBJ)/drivers/sauro.o \
	$(OBJ)/vidhrdw/ambush.o $(OBJ)/drivers/ambush.o \
	$(OBJ)/vidhrdw/starcrus.o $(OBJ)/drivers/starcrus.o \
	$(OBJ)/drivers/dlair.o \
	$(OBJ)/vidhrdw/meteor.o $(OBJ)/drivers/meteor.o \
	$(OBJ)/vidhrdw/aztarac.o $(OBJ)/sndhrdw/aztarac.o $(OBJ)/drivers/aztarac.o \
	$(OBJ)/vidhrdw/mole.o $(OBJ)/drivers/mole.o \
	$(OBJ)/vidhrdw/gotya.o $(OBJ)/sndhrdw/gotya.o $(OBJ)/drivers/gotya.o \
	$(OBJ)/vidhrdw/mrjong.o $(OBJ)/drivers/mrjong.o \
	$(OBJ)/vidhrdw/polyplay.o $(OBJ)/sndhrdw/polyplay.o $(OBJ)/drivers/polyplay.o \
	$(OBJ)/vidhrdw/mermaid.o $(OBJ)/drivers/mermaid.o \
	$(OBJ)/vidhrdw/magix.o $(OBJ)/drivers/magix.o \
	$(OBJ)/drivers/royalmah.o \
	$(OBJ)/drivers/mazinger.o \

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

# workaround for vc4.2 compiler optimization bug:
!ifndef DEBUG
$(OBJ)/vidhrdw/timeplt.o: src/vidhrdw/timeplt.c
	$(CC) $(DEFS) $(CFLAGSGLOBAL) -Oi -Ot -Oy -Ob1 -Gs -G5 -Gr -Fo$@ -c src/vidhrdw/timeplt.c
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

{src/cpu/cdp1802}.c{$(OBJ)/cpu/cdp1802}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/i8085}.c{$(OBJ)/cpu/i8085}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/m6809}.c{$(OBJ)/cpu/m6809}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/hd6309}.c{$(OBJ)/cpu/hd6309}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/konami}.c{$(OBJ)/cpu/konami}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/m68000}.c{$(OBJ)/cpu/m68000}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/s2650}.c{$(OBJ)/cpu/s2650}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/f8}.c{$(OBJ)/cpu/f8}.o:
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

{src/cpu/g65816}.c{$(OBJ)/cpu/g65816}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/cp1600}.c{$(OBJ)/cpu/cp1600}.o:
	$(CC) $(DEFS) $(CFLAGS) -Fo$@ -c $<

{src/cpu/sh2}.c{$(OBJ)/cpu/sh2}.o:
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

{mess/Win32}.c{$(OBJ)/mess/Win32}.o:
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
$(OBJ)/cpu/cdp1802/cdp1802.o:	src/cpu/cdp1802/cdp1802.c src/cpu/cdp1802/cdp1802.h src/cpu/cdp1802/table.c
$(OBJ)/cpu/i8085/i8085.o:       src/cpu/i8085/i8085.c src/cpu/i8085/i8085.h src/cpu/i8085/i8085cpu.h src/cpu/i8085/i8085daa.h
$(OBJ)/cpu/i86/i86.o:           src/cpu/i86/i86.c src/cpu/i86/instr86.c src/cpu/i86/i86.h src/cpu/i86/i86intf.h src/cpu/i86/ea.h src/cpu/i86/host.h src/cpu/i86/modrm.h
$(OBJ)/cpu/nec/nec.o:           src/cpu/nec/nec.c src/cpu/nec/nec.h src/cpu/nec/necintrf.h src/cpu/nec/necea.h src/cpu/nec/nechost.h src/cpu/nec/necinstr.h src/cpu/nec/necmodrm.h
$(OBJ)/cpu/m6502/m6502.o:       src/cpu/m6502/m6502.c src/cpu/m6502/m6502.h src/cpu/m6502/ops02.h src/cpu/m6502/t6502.c src/cpu/m6502/t65c02.c src/cpu/m6502/t65sc02.c src/cpu/m6502/t6510.c
$(OBJ)/cpu/m6502/m65ce02.o:     src/cpu/m6502/m65ce02.c src/cpu/m6502/m65ce02.h src/cpu/m6502/opsce02.h src/cpu/m6502/t6ce502.c
$(OBJ)/cpu/m6502/m6509.o:       src/cpu/m6502/m6509.c src/cpu/m6502/m6509.h src/cpu/m6502/ops09.h src/cpu/m6502/t6509.c
$(OBJ)/cpu/m6800/m6800.o:	     src/cpu/m6800/m6800.c src/cpu/m6800/m6800.h src/cpu/m6800/6800ops.c src/cpu/m6800/6800tbl.c
$(OBJ)/cpu/m6805/m6805.o:	     src/cpu/m6805/m6805.c src/cpu/m6805/m6805.h src/cpu/m6805/6805ops.c
$(OBJ)/cpu/m6809/m6809.o:	     src/cpu/m6809/m6809.c src/cpu/m6809/m6809.h src/cpu/m6809/6809ops.c src/cpu/m6809/6809tbl.c
$(OBJ)/cpu/hd6309/hd6309.o:	     src/cpu/hd6309/hd6309.c src/cpu/hd6309/hd6309.h src/cpu/hd6309/6309ops.c src/cpu/hd6309/6309tbl.c
$(OBJ)/cpu/tms32010/tms32010.o: src/cpu/tms32010/tms32010.c src/cpu/tms32010/tms32010.h
$(OBJ)/cpu/tms34010/tms34010.o: src/cpu/tms34010/tms34010.c src/cpu/tms34010/tms34010.h src/cpu/tms34010/34010ops.c src/cpu/tms34010/34010tbl.c
$(OBJ)/cpu/tms9900/tms9900.o:   src/cpu/tms9900/tms9900.c src/cpu/tms9900/tms9900.h src/cpu/tms9900/9900stat.h
$(OBJ)/cpu/t11/t11.o:           src/cpu/t11/t11.c src/cpu/t11/t11.h src/cpu/t11/t11ops.c src/cpu/t11/t11table.c
$(OBJ)/cpu/m68000/m68kcpu.o:    $(OBJ)/cpu/m68000/m68kops.c src/cpu/m68000/m68kmake.c src/cpu/m68000/m68k_in.c
$(OBJ)/cpu/ccpu/ccpu.o:         src/cpu/ccpu/ccpu.c src/cpu/ccpu/ccpu.h src/cpu/ccpu/ccputabl.c
$(OBJ)/cpu/konami/konami.o:     src/cpu/konami/konami.c src/cpu/konami/konami.h src/cpu/konami/konamops.c src/cpu/konami/konamtbl.c

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
	md $(OBJ)\cpu\hd6309
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
	md $(OBJ)\sound
	md $(OBJ)\drivers
	md $(OBJ)\machine
	md $(OBJ)\vidhrdw
	md $(OBJ)\sndhrdw
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
	del $(OBJ)\cpu\hd6309\*.o
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
	del $(OBJ)\sound\*.o
	del $(OBJ)\drivers\*.o
	del $(OBJ)\machine\*.o
	del $(OBJ)\vidhrdw\*.o
	del $(OBJ)\sndhrdw\*.o
	del $(OBJ)\Win32\*.o
	del $(OBJ)\Win32\*.res
	del $(EXENAME)
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
	del $(OBJ)\cpu\nec\*.0
	del $(OBJ)\cpu\i8039\*.o
	del $(OBJ)\cpu\i8085\*.o
	del $(OBJ)\cpu\m6800\*.o
	del $(OBJ)\cpu\m6800\*.oa
	del $(OBJ)\cpu\m6800\*.exe
	del $(OBJ)\cpu\m6805\*.o
	del $(OBJ)\cpu\m6809\*.o
	del $(OBJ)\cpu\hd6309\*.o
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
	del $(EXENAME)

cleantiny:
	del $(OBJ)\driver.o
	del $(OBJ)\usrintrf.o
	del $(OBJ)\cheat.o
	del $(OBJ)\Win32\*.o
