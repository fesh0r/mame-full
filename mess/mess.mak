# this is a MESS build
MESS = 1

# core defines
COREDEFS += -DNEOFREE -DMESS

# CPU cores used in MESS
CPUS+=Z80@
CPUS+=Z180@
CPUS+=8080@
#CPUS+=8085A@
CPUS+=M6502@
CPUS+=M65C02@
CPUS+=M65SC02@
#CPUS+=M65CE02@
CPUS+=M6509@
CPUS+=M6510@
CPUS+=M6510T@
CPUS+=M7501@
CPUS+=M8502@
CPUS+=N2A03@
#CPUS+=DECO16@
CPUS+=M4510@
CPUS+=H6280@
CPUS+=I86@
CPUS+=I88@
#CPUS+=I186@
#CPUS+=I188@
CPUS+=I286@
CPUS+=V20@
#CPUS+=V30@
#CPUS+=V33@
#CPUS+=V60@
#CPUS+=V70@
#CPUS+=I8035@
CPUS+=I8039@
CPUS+=I8048@
#CPUS+=N7751@
#CPUS+=I8X41@
CPUS+=M6800@
#CPUS+=M6801@
#CPUS+=M6802@
CPUS+=M6803@
CPUS+=M6808@
CPUS+=HD63701@
CPUS+=NSC8105@
CPUS+=M6805@
#CPUS+=M68705@
#CPUS+=HD63705@
CPUS+=HD6309@
CPUS+=M6809@
#CPUS+=KONAMI@
CPUS+=M68000@
CPUS+=M68010@
CPUS+=M68EC020@
CPUS+=M68020@
#CPUS+=T11@
CPUS+=S2650@
#CPUS+=TMS34010@
#CPUS+=TMS34020@
CPUS+=TMS9900@
#CPUS+=TMS9940@
#CPUS+=TMS9980@
#CPUS+=TMS9985@
#CPUS+=TMS9989@
CPUS+=TMS9995@
#CPUS+=TMS99105A@
#CPUS+=TMS99110A@
#CPUS+=Z8000@
#CPUS+=TMS32010@
#CPUS+=TMS32025@
#CPUS+=TMS32031@
#CPUS+=CCPU@
#CPUS+=ADSP2100@
#CPUS+=ADSP2101@
#CPUS+=ADSP2105@
#CPUS+=ADSP2115@
CPUS+=PSXCPU@
#CPUS+=ASAP@
#CPUS+=UPD7810@
#CPUS+=UPD7807@
CPUS+=ARM@
CPUS+=JAGUAR@
#CPUS+=R3000@
#CPUS+=R4600@
#CPUS+=R5000@
CPUS+=SH2@
#CPUS+=DSP32C@
#CPUS+=PIC16C54@
#CPUS+=PIC16C55@
#CPUS+=PIC16C56@
#CPUS+=PIC16C57@
#CPUS+=PIC16C58@
CPUS+=Z80GB@
CPUS+=CDP1802@
CPUS+=SC61860@
CPUS+=G65816@
CPUS+=SPC700@
CPUS+=SATURN@
CPUS+=APEXC@
CPUS+=Z80_MSX@
CPUS+=F8@
CPUS+=CP1600@
CPUS+=TMS99010@
CPUS+=PDP1@

# SOUND cores used in MESS
SOUNDS+=CUSTOM@
SOUNDS+=SAMPLES@
SOUNDS+=DAC@
#SOUNDS+=DISCRETE@
SOUNDS+=AY8910@
SOUNDS+=YM2203@
# enable only one of the following two
#SOUNDS+=YM2151@
SOUNDS+=YM2151_ALT@
SOUNDS+=YM2608@
SOUNDS+=YM2610@
SOUNDS+=YM2610B@
SOUNDS+=YM2612@
#SOUNDS+=YM3438@
SOUNDS+=YM2413@
SOUNDS+=YM3812@
#SOUNDS+=YMZ280B@
#SOUNDS+=YM3526@
#SOUNDS+=Y8950@
#SOUNDS+=SN76477@
SOUNDS+=SN76496@
SOUNDS+=POKEY@
SOUNDS+=TIA@
SOUNDS+=NES@
SOUNDS+=ASTROCADE@
#SOUNDS+=NAMCO@
#SOUNDS+=TMS36XX@
SOUNDS+=TMS5110@
SOUNDS+=TMS5220@
#SOUNDS+=VLM5030@
#SOUNDS+=ADPCM@
SOUNDS+=OKIM6295@
#SOUNDS+=MSM5205@
#SOUNDS+=UPD7759@
#SOUNDS+=HC55516@
#SOUNDS+=K005289@
#SOUNDS+=K007232@
SOUNDS+=K051649@
#SOUNDS+=K053260@
#SOUNDS+=SEGAPCM@
#SOUNDS+=RF5C68@
#SOUNDS+=CEM3394@
#SOUNDS+=C140@
SOUNDS+=QSOUND@
SOUNDS+=SAA1099@
#SOUNDS+=IREMGA20@
#SOUNDS+=ES5505@
#SOUNDS+=ES5506@
#SOUNDS+=BSMT2000@
#SOUNDS+=YMF278B@
#SOUNDS+=GAELCO_CG1V@
#SOUNDS+=GAELCO_GAE1@
#SOUNDS+=X1_010@
SOUNDS+=SPEAKER@
SOUNDS+=WAVE@
SOUNDS+=BEEP@
#SOUNDS+=SP0250@

# Archive definitions
DRVLIBS = \
	$(OBJ)/coco.a     \
	$(OBJ)/nintendo.a \
	$(OBJ)/apple.a    \
	$(OBJ)/at.a       \
	$(OBJ)/pc.a       \
	$(OBJ)/pcshare.a  \
	$(OBJ)/sega.a     \
	$(OBJ)/acorn.a    \
	$(OBJ)/atari.a    \
	$(OBJ)/advision.a \
	$(OBJ)/mbee.a	  \
	$(OBJ)/vtech.a	  \
	$(OBJ)/jupiter.a  \
	$(OBJ)/trs80.a	  \
	$(OBJ)/gce.a	  \
	$(OBJ)/arcadia.a  \
	$(OBJ)/kaypro.a   \
	$(OBJ)/cgenie.a   \
	$(OBJ)/aquarius.a \
	$(OBJ)/tangerin.a \
	$(OBJ)/sord.a     \
	$(OBJ)/exidy.a    \
	$(OBJ)/samcoupe.a \
	$(OBJ)/p2000.a	  \
	$(OBJ)/tatung.a   \
	$(OBJ)/ep128.a	  \
	$(OBJ)/cpschngr.a \
	$(OBJ)/veb.a	  \
	$(OBJ)/amstrad.a  \
	$(OBJ)/necpc.a	  \
	$(OBJ)/nec.a	  \
	$(OBJ)/fairch.a   \
	$(OBJ)/ascii.a	  \
	$(OBJ)/nascom1.a  \
	$(OBJ)/magnavox.a \
	$(OBJ)/mtx.a	  \
	$(OBJ)/mk1.a      \
	$(OBJ)/mk2.a      \
	$(OBJ)/ti85.a     \
	$(OBJ)/galaxy.a   \
	$(OBJ)/vc4000.a   \
	$(OBJ)/lviv.a     \
	$(OBJ)/pmd85.a    \
	$(OBJ)/sinclair.a \
	$(OBJ)/lynx.a     \
	$(OBJ)/intv.a     \
	$(OBJ)/svision.a  \
	$(OBJ)/coleco.a   \
	$(OBJ)/apf.a      \
	$(OBJ)/bally.a	  \
	$(OBJ)/rca.a	  \
	$(OBJ)/teamconc.a \
	$(OBJ)/amiga.a    \
	$(OBJ)/svi.a      \
	$(OBJ)/ti99.a     \
	$(OBJ)/tutor.a    \
	$(OBJ)/apexc.a	  \
	$(OBJ)/pdp1.a	  \
	$(OBJ)/sharp.a    \
	$(OBJ)/aim65.a    \
	$(OBJ)/avigo.a    \
	$(OBJ)/motorola.a \
	$(OBJ)/ssystem3.a \
	$(OBJ)/hp48.a     \
	$(OBJ)/cbm.a      \
	$(OBJ)/cbmshare.a \
	$(OBJ)/kim1.a     \
	$(OBJ)/sym1.a     \
	$(OBJ)/sony.a     \
	$(OBJ)/concept.a  \
	$(OBJ)/dai.a      \


$(OBJ)/coleco.a:   \
	  $(OBJ)/mess/machine/coleco.o	 \
	  $(OBJ)/mess/systems/coleco.o

$(OBJ)/arcadia.a:  \
	  $(OBJ)/mess/sndhrdw/arcadia.o	 \
	  $(OBJ)/mess/vidhrdw/arcadia.o	 \
	  $(OBJ)/mess/systems/arcadia.o

$(OBJ)/sega.a:                       \
	  $(OBJ)/mess/vidhrdw/genesis.o  \
	  $(OBJ)/mess/machine/genesis.o  \
	  $(OBJ)/mess/sndhrdw/genesis.o  \
	  $(OBJ)/mess/systems/genesis.o  \
	  $(OBJ)/mess/systems/saturn.o	 \
	  $(OBJ)/mess/vidhrdw/smsvdp.o	 \
	  $(OBJ)/mess/machine/sms.o      \
	  $(OBJ)/mess/systems/sms.o

$(OBJ)/atari.a:                      \
	  $(OBJ)/vidhrdw/tia.o           \
	  $(OBJ)/mess/machine/atari.o	 \
	  $(OBJ)/mess/vidhrdw/antic.o	 \
	  $(OBJ)/mess/vidhrdw/gtia.o	 \
	  $(OBJ)/mess/systems/atari.o	 \
	  $(OBJ)/mess/vidhrdw/atari.o	 \
	  $(OBJ)/mess/machine/a7800.o	 \
	  $(OBJ)/mess/systems/a7800.o	 \
	  $(OBJ)/mess/vidhrdw/a7800.o	 \
	  $(OBJ)/mess/systems/a2600.o    \
	  $(OBJ)/mess/systems/jaguar.o   \
	  $(OBJ)/sndhrdw/jaguar.o        \
	  $(OBJ)/vidhrdw/jaguar.o        \
#	  $(OBJ)/mess/systems/atarist.o

$(OBJ)/gce.a:	                     \
	  $(OBJ)/mess/vidhrdw/vectrex.o  \
	  $(OBJ)/mess/machine/vectrex.o  \
	  $(OBJ)/mess/systems/vectrex.o

$(OBJ)/nintendo.a:                   \
	  $(OBJ)/mess/machine/nes_mmc.o  \
	  $(OBJ)/vidhrdw/ppu2c03b.o		 \
	  $(OBJ)/mess/vidhrdw/nes.o      \
	  $(OBJ)/mess/machine/nes.o	     \
	  $(OBJ)/mess/systems/nes.o	     \
	  $(OBJ)/mess/sndhrdw/gb.o       \
	  $(OBJ)/mess/vidhrdw/gb.o       \
	  $(OBJ)/mess/machine/gb.o       \
	  $(OBJ)/mess/systems/gb.o       \
	  $(OBJ)/sndhrdw/snes.o          \
	  $(OBJ)/machine/snes.o          \
	  $(OBJ)/vidhrdw/snes.o          \
	  $(OBJ)/mess/systems/snes.o	 

$(OBJ)/amiga.a: \
	  $(OBJ)/mess/vidhrdw/amiga.o	 \
	  $(OBJ)/mess/machine/amiga.o	 \
	  $(OBJ)/mess/systems/amiga.o

$(OBJ)/cbmshare.a: \
	  $(OBJ)/mess/machine/tpi6525.o  \
	  $(OBJ)/mess/machine/cia6526.o  \
	  $(OBJ)/mess/machine/cbm.o	     \
	  $(OBJ)/mess/sndhrdw/sid.o      \
	  $(OBJ)/mess/sndhrdw/sidenvel.o \
	  $(OBJ)/mess/sndhrdw/sidvoice.o \
	  $(OBJ)/mess/sndhrdw/sid6581.o  \
	  $(OBJ)/mess/machine/cbmdrive.o \
	  $(OBJ)/mess/machine/vc1541.o	 \
	  $(OBJ)/mess/machine/cbmieeeb.o \
	  $(OBJ)/mess/machine/cbmserb.o  \
	  $(OBJ)/mess/machine/c64.o      \
	  $(OBJ)/mess/vidhrdw/vic6567.o	 \
	  $(OBJ)/mess/machine/vc20tape.o

$(OBJ)/cbm.a: \
	  $(OBJ)/mess/vidhrdw/pet.o	     \
	  $(OBJ)/mess/systems/pet.o	     \
	  $(OBJ)/mess/machine/pet.o      \
	  $(OBJ)/mess/systems/c64.o      \
	  $(OBJ)/mess/machine/vc20.o	 \
	  $(OBJ)/mess/systems/vc20.o	 \
	  $(OBJ)/mess/sndhrdw/ted7360.o  \
	  $(OBJ)/mess/sndhrdw/t6721.o    \
	  $(OBJ)/mess/machine/c16.o	     \
	  $(OBJ)/mess/systems/c16.o      \
	  $(OBJ)/mess/systems/cbmb.o	 \
	  $(OBJ)/mess/machine/cbmb.o	 \
	  $(OBJ)/mess/vidhrdw/cbmb.o	 \
	  $(OBJ)/mess/systems/c65.o	     \
	  $(OBJ)/mess/machine/c65.o      \
	  $(OBJ)/mess/vidhrdw/vdc8563.o  \
	  $(OBJ)/mess/systems/c128.o     \
	  $(OBJ)/mess/machine/c128.o     \
	  $(OBJ)/mess/sndhrdw/vic6560.o  \
	  $(OBJ)/mess/vidhrdw/ted7360.o  \
	  $(OBJ)/mess/vidhrdw/vic6560.o  

$(OBJ)/coco.a:   \
	  $(OBJ)/mess/machine/6883sam.o  \
	  $(OBJ)/mess/machine/cococart.o \
	  $(OBJ)/mess/machine/ds1315.o	 \
	  $(OBJ)/mess/machine/ds1315.o   \
	  $(OBJ)/mess/machine/m6242b.o   \
	  $(OBJ)/mess/machine/mc10.o	 \
	  $(OBJ)/mess/systems/mc10.o	 \
	  $(OBJ)/mess/machine/dragon.o	 \
	  $(OBJ)/mess/vidhrdw/dragon.o	 \
	  $(OBJ)/mess/systems/dragon.o	 \
	  $(OBJ)/mess/formats/cocopak.o  \
	  $(OBJ)/mess/formats/cococas.o  \
	  $(OBJ)/mess/formats/coco_dsk.o \
	  $(OBJ)/mess/devices/coco_vhd.o 

$(OBJ)/trs80.a:    \
	  $(OBJ)/mess/machine/trs80.o	 \
	  $(OBJ)/mess/vidhrdw/trs80.o	 \
	  $(OBJ)/mess/systems/trs80.o

$(OBJ)/cgenie.a:   \
	  $(OBJ)/mess/machine/cgenie.o	 \
	  $(OBJ)/mess/vidhrdw/cgenie.o	 \
	  $(OBJ)/mess/sndhrdw/cgenie.o	 \
	  $(OBJ)/mess/systems/cgenie.o

$(OBJ)/pdp1.a:	   \
	  $(OBJ)/mess/vidhrdw/pdp1.o	 \
	  $(OBJ)/mess/machine/pdp1.o	 \
	  $(OBJ)/mess/systems/pdp1.o

$(OBJ)/apexc.a:     \
	  $(OBJ)/mess/systems/apexc.o

$(OBJ)/kaypro.a:   \
	  $(OBJ)/mess/machine/cpm_bios.o \
	  $(OBJ)/mess/vidhrdw/kaypro.o	 \
	  $(OBJ)/mess/sndhrdw/kaypro.o	 \
	  $(OBJ)/mess/machine/kaypro.o	 \
	  $(OBJ)/mess/systems/kaypro.o	 

$(OBJ)/sinclair.a: \
	  $(OBJ)/mess/vidhrdw/border.o	 \
	  $(OBJ)/mess/vidhrdw/spectrum.o \
	  $(OBJ)/mess/machine/spectrum.o \
	  $(OBJ)/mess/systems/spectrum.o \
	  $(OBJ)/mess/vidhrdw/zx.o	     \
	  $(OBJ)/mess/machine/zx.o	     \
	  $(OBJ)/mess/systems/zx.o

$(OBJ)/apple.a:   \
	  $(OBJ)/mess/machine/lisa.o	 \
	  $(OBJ)/mess/systems/lisa.o     \
	  $(OBJ)/mess/machine/iwm.o	     \
	  $(OBJ)/mess/machine/sonydriv.o \
	  $(OBJ)/mess/machine/ap_disk2.o \
	  $(OBJ)/mess/vidhrdw/apple2.o	 \
	  $(OBJ)/mess/machine/apple2.o	 \
	  $(OBJ)/mess/systems/apple2.o   \
	  $(OBJ)/mess/formats/ap2_disk.o \
	  $(OBJ)/mess/machine/ay3600.o	 \
	  $(OBJ)/mess/sndhrdw/mac.o	     \
	  $(OBJ)/mess/machine/sonydriv.o \
	  $(OBJ)/mess/vidhrdw/mac.o      \
	  $(OBJ)/mess/machine/mac.o	     \
	  $(OBJ)/mess/systems/mac.o      \
	  $(OBJ)/mess/vidhrdw/apple1.o	 \
	  $(OBJ)/mess/machine/apple1.o	 \
	  $(OBJ)/mess/systems/apple1.o   


$(OBJ)/avigo.a: \
	  $(OBJ)/mess/systems/avigo.o	 \
	  $(OBJ)/mess/vidhrdw/avigo.o

$(OBJ)/ti85.a: \
	  $(OBJ)/mess/systems/ti85.o	 \
	  $(OBJ)/mess/machine/ti85.o	 \
	  $(OBJ)/mess/vidhrdw/ti85.o	 \
	  $(OBJ)/mess/formats/ti85_ser.o	 

$(OBJ)/rca.a: \
	  $(OBJ)/mess/systems/studio2.o  \
	  $(OBJ)/mess/vidhrdw/studio2.o  

$(OBJ)/fairch.a: \
	  $(OBJ)/mess/vidhrdw/channelf.o \
	  $(OBJ)/mess/sndhrdw/channelf.o \
	  $(OBJ)/mess/systems/channelf.o 

$(OBJ)/ti99.a:	   \
	  $(OBJ)/mess/machine/tms9901.o  \
	  $(OBJ)/mess/machine/tms9902.o \
	  $(OBJ)/mess/sndhrdw/spchroms.o \
	  $(OBJ)/mess/machine/ti99_4x.o  \
	  $(OBJ)/mess/systems/ti99_4x.o  \
	  $(OBJ)/mess/machine/990_hd.o	 \
	  $(OBJ)/mess/machine/990_tap.o	 \
	  $(OBJ)/mess/vidhrdw/911_vdt.o  \
	  $(OBJ)/mess/systems/ti990_10.o \
	  $(OBJ)/mess/machine/ti990.o \
	  $(OBJ)/mess/machine/mm58274c.o \
	  $(OBJ)/mess/machine/994x_ser.o \
	  $(OBJ)/mess/systems/ti99_4p.o  \
	  $(OBJ)/mess/machine/at29040.o  \
	  $(OBJ)/mess/machine/99_dsk.o   \
	  $(OBJ)/mess/machine/99_ide.o   \
	  $(OBJ)/mess/machine/99_peb.o   \
	  $(OBJ)/mess/machine/99_hsgpl.o \
	  $(OBJ)/mess/machine/smc92x4.o  \
	  $(OBJ)/mess/machine/rtc65271.o \
	  $(OBJ)/mess/systems/geneve.o   \
	  $(OBJ)/mess/machine/geneve.o   \
#	  $(OBJ)/mess/systems/ti99_2.o	 \
#	  $(OBJ)/mess/systems/ti990_4.o  \

$(OBJ)/tutor.a:   \
	  $(OBJ)/mess/systems/tutor.o

$(OBJ)/bally.a:    \
	  $(OBJ)/sound/astrocde.o	 \
	  $(OBJ)/mess/vidhrdw/astrocde.o \
	  $(OBJ)/mess/machine/astrocde.o \
	  $(OBJ)/mess/systems/astrocde.o

$(OBJ)/pcshare.a:	   \
	  $(OBJ)/mess/machine/dma8237.o  \
	  $(OBJ)/mess/machine/pic8259.o  \
	  $(OBJ)/mess/sndhrdw/pc.o	     \
	  $(OBJ)/mess/sndhrdw/sblaster.o \
	  $(OBJ)/mess/machine/pc_fdc.o	 \
	  $(OBJ)/mess/devices/pc_hdc.o	 \
	  $(OBJ)/mess/machine/pcshare.o	 \
	  $(OBJ)/mess/vidhrdw/pc_mda.o	 \
	  $(OBJ)/mess/vidhrdw/pc_cga.o	 \
	  $(OBJ)/mess/vidhrdw/pc_vga.o	 \
	  $(OBJ)/mess/vidhrdw/pc_video.o 

$(OBJ)/pc.a:	   \
	  $(OBJ)/mess/vidhrdw/pc_aga.o	 \
	  $(OBJ)/mess/machine/ibmpc.o	 \
	  $(OBJ)/mess/machine/tandy1t.o  \
	  $(OBJ)/mess/machine/amstr_pc.o \
	  $(OBJ)/mess/machine/europc.o	 \
	  $(OBJ)/mess/machine/pc.o       \
	  $(OBJ)/mess/systems/pc.o		\
	  $(OBJ)/mess/vidhrdw/pc_t1t.o	 

$(OBJ)/at.a:	   \
	  $(OBJ)/mess/machine/pc_ide.o   \
	  $(OBJ)/mess/machine/ibmat.o    \
	  $(OBJ)/mess/machine/ps2.o	 \
	  $(OBJ)/mess/machine/at.o       \
	  $(OBJ)/mess/systems/at.o

$(OBJ)/p2000.a:    \
	  $(OBJ)/mess/vidhrdw/saa5050.o  \
	  $(OBJ)/mess/vidhrdw/p2000m.o	 \
	  $(OBJ)/mess/systems/p2000t.o	 \
	  $(OBJ)/mess/machine/p2000t.o	 \
	  $(OBJ)/mess/machine/mc6850.o	 \
	  $(OBJ)/mess/vidhrdw/uk101.o	 \
	  $(OBJ)/mess/machine/uk101.o	 \
	  $(OBJ)/mess/systems/uk101.o

$(OBJ)/amstrad.a:  \
	  $(OBJ)/mess/vidhrdw/nc.o	 \
	  $(OBJ)/mess/systems/nc.o	 \
	  $(OBJ)/mess/machine/nc.o	 \
	  $(OBJ)/mess/vidhrdw/pcw.o	 \
	  $(OBJ)/mess/systems/pcw.o	 \
	  $(OBJ)/mess/systems/pcw16.o	 \
	  $(OBJ)/mess/vidhrdw/pcw16.o	 \
	  $(OBJ)/mess/systems/amstrad.o  \
	  $(OBJ)/mess/machine/amstrad.o  \
	  $(OBJ)/mess/vidhrdw/amstrad.o  

$(OBJ)/veb.a:      \
	  $(OBJ)/mess/vidhrdw/kc.o	 \
	  $(OBJ)/mess/machine/kc.o	 \
	  $(OBJ)/mess/systems/kc.o

$(OBJ)/nec.a:	   \
	  $(OBJ)/mess/vidhrdw/vdc.o	 \
	  $(OBJ)/mess/machine/pce.o	 \
	  $(OBJ)/mess/systems/pce.o

$(OBJ)/necpc.a:	   \
	  $(OBJ)/mess/machine/pc8801.o	 \
	  $(OBJ)/mess/systems/pc8801.o	 \
	  $(OBJ)/mess/vidhrdw/pc8801.o

$(OBJ)/ep128.a :   \
	  $(OBJ)/mess/sndhrdw/dave.o	 \
	  $(OBJ)/mess/vidhrdw/epnick.o	 \
	  $(OBJ)/mess/vidhrdw/enterp.o	 \
	  $(OBJ)/mess/machine/enterp.o	 \
	  $(OBJ)/mess/systems/enterp.o

$(OBJ)/ascii.a :   \
	  $(OBJ)/mess/machine/msx.o	 \
	  $(OBJ)/mess/formats/fmsx_cas.o \
	  $(OBJ)/mess/systems/msx.o

$(OBJ)/kim1.a :    \
	  $(OBJ)/mess/vidhrdw/kim1.o	 \
	  $(OBJ)/mess/machine/kim1.o	 \
	  $(OBJ)/mess/systems/kim1.o

$(OBJ)/sym1.a :    \
	  $(OBJ)/mess/vidhrdw/sym1.o	 \
	  $(OBJ)/mess/machine/sym1.o	 \
	  $(OBJ)/mess/systems/sym1.o

$(OBJ)/aim65.a :    \
	  $(OBJ)/mess/vidhrdw/aim65.o	 \
	  $(OBJ)/mess/machine/aim65.o	 \
	  $(OBJ)/mess/systems/aim65.o

$(OBJ)/vc4000.a :   \
	  $(OBJ)/mess/vidhrdw/vc4000.o   \
	  $(OBJ)/mess/sndhrdw/vc4000.o   \
	  $(OBJ)/mess/systems/vc4000.o

$(OBJ)/tangerin.a :\
	  $(OBJ)/mess/devices/mfmdisk.o  \
	  $(OBJ)/mess/vidhrdw/microtan.o \
	  $(OBJ)/mess/machine/microtan.o \
	  $(OBJ)/mess/systems/microtan.o \
	  $(OBJ)/mess/formats/orictap.o  \
	  $(OBJ)/mess/vidhrdw/oric.o     \
	  $(OBJ)/mess/machine/oric.o     \
	  $(OBJ)/mess/systems/oric.o

$(OBJ)/vtech.a :   \
	  $(OBJ)/mess/vidhrdw/vtech1.o	 \
	  $(OBJ)/mess/machine/vtech1.o	 \
	  $(OBJ)/mess/systems/vtech1.o	 \
	  $(OBJ)/mess/vidhrdw/vtech2.o	 \
	  $(OBJ)/mess/machine/vtech2.o	 \
	  $(OBJ)/mess/systems/vtech2.o

$(OBJ)/jupiter.a : \
	  $(OBJ)/mess/vidhrdw/jupiter.o  \
	  $(OBJ)/mess/machine/jupiter.o  \
	  $(OBJ)/mess/systems/jupiter.o

$(OBJ)/mbee.a :    \
	  $(OBJ)/mess/vidhrdw/mbee.o	 \
	  $(OBJ)/mess/machine/mbee.o	 \
	  $(OBJ)/mess/systems/mbee.o


$(OBJ)/advision.a: \
	  $(OBJ)/mess/vidhrdw/advision.o \
	  $(OBJ)/mess/machine/advision.o \
	  $(OBJ)/mess/systems/advision.o

$(OBJ)/nascom1.a:  \
	  $(OBJ)/mess/vidhrdw/nascom1.o  \
	  $(OBJ)/mess/machine/nascom1.o  \
	  $(OBJ)/mess/systems/nascom1.o

$(OBJ)/cpschngr.a: \
	  $(OBJ)/machine/eeprom.o	     \
	  $(OBJ)/mess/systems/cpschngr.o \
	  $(OBJ)/vidhrdw/cps1.o

$(OBJ)/mtx.a:	   \
	  $(OBJ)/mess/systems/mtx.o

$(OBJ)/acorn.a:    \
	  $(OBJ)/mess/machine/i8271.o	 \
	  $(OBJ)/mess/machine/upd7002.o  \
	  $(OBJ)/mess/vidhrdw/bbc.o	     \
	  $(OBJ)/mess/machine/bbc.o	     \
	  $(OBJ)/mess/systems/bbc.o	     \
	  $(OBJ)/mess/systems/a310.o	 \
	  $(OBJ)/mess/systems/z88.o	     \
	  $(OBJ)/mess/vidhrdw/z88.o      \
	  $(OBJ)/mess/vidhrdw/atom.o	 \
	  $(OBJ)/mess/systems/atom.o	 \
	  $(OBJ)/mess/machine/atom.o	 

$(OBJ)/samcoupe.a: \
	  $(OBJ)/mess/machine/coupe.o	 \
	  $(OBJ)/mess/vidhrdw/coupe.o	 \
	  $(OBJ)/mess/systems/coupe.o

$(OBJ)/sharp.a:    \
	  $(OBJ)/mess/machine/mz700.o	 \
	  $(OBJ)/mess/vidhrdw/mz700.o	 \
	  $(OBJ)/mess/systems/mz700.o	 \
	  $(OBJ)/mess/systems/pocketc.o  \
	  $(OBJ)/mess/vidhrdw/pc1401.o	 \
	  $(OBJ)/mess/machine/pc1401.o	 \
	  $(OBJ)/mess/vidhrdw/pc1403.o	 \
	  $(OBJ)/mess/machine/pc1403.o	 \
	  $(OBJ)/mess/vidhrdw/pc1350.o	 \
	  $(OBJ)/mess/machine/pc1350.o	 \
	  $(OBJ)/mess/vidhrdw/pc1251.o	 \
	  $(OBJ)/mess/machine/pc1251.o	 \
	  $(OBJ)/mess/vidhrdw/pocketc.o  

$(OBJ)/hp48.a:     \
	  $(OBJ)/mess/machine/hp48.o     \
	  $(OBJ)/mess/vidhrdw/hp48.o     \
	  $(OBJ)/mess/systems/hp48.o

$(OBJ)/aquarius.a: \
	  $(OBJ)/mess/machine/aquarius.o \
	  $(OBJ)/mess/vidhrdw/aquarius.o \
	  $(OBJ)/mess/systems/aquarius.o

$(OBJ)/exidy.a:    \
	$(OBJ)/mess/machine/hd6402.o     \
	$(OBJ)/mess/vidhrdw/exidy.o      \
	$(OBJ)/mess/machine/exidy.o      \
	$(OBJ)/mess/systems/exidy.o

$(OBJ)/galaxy.a:   \
	  $(OBJ)/mess/machine/galaxy.o   \
	  $(OBJ)/mess/vidhrdw/galaxy.o   \
	  $(OBJ)/mess/systems/galaxy.o

$(OBJ)/lviv.a:   \
	  $(OBJ)/mess/machine/lviv.o   \
	  $(OBJ)/mess/vidhrdw/lviv.o   \
	  $(OBJ)/mess/systems/lviv.o   \
	  $(OBJ)/mess/formats/lviv_lvt.o

$(OBJ)/pmd85.a:   \
	  $(OBJ)/mess/machine/pmd85.o   \
	  $(OBJ)/mess/vidhrdw/pmd85.o   \
	  $(OBJ)/mess/systems/pmd85.o   

$(OBJ)/magnavox.a: \
	  $(OBJ)/mess/machine/odyssey2.o \
	  $(OBJ)/mess/vidhrdw/odyssey2.o \
	  $(OBJ)/mess/sndhrdw/odyssey2.o \
	  $(OBJ)/mess/systems/odyssey2.o

$(OBJ)/teamconc.a: \
	  $(OBJ)/mess/vidhrdw/comquest.o \
	  $(OBJ)/mess/systems/comquest.o

$(OBJ)/svision.a:  \
	  $(OBJ)/mess/systems/svision.o \
	  $(OBJ)/mess/sndhrdw/svision.o

$(OBJ)/lynx.a:     \
	  $(OBJ)/mess/systems/lynx.o     \
	  $(OBJ)/mess/sndhrdw/lynx.o     \
	  $(OBJ)/mess/machine/lynx.o

$(OBJ)/mk1.a:      \
	  $(OBJ)/mess/cpu/f8/f3853.o	 \
	  $(OBJ)/mess/vidhrdw/mk1.o      \
	  $(OBJ)/mess/systems/mk1.o

$(OBJ)/mk2.a:      \
	  $(OBJ)/mess/vidhrdw/mk2.o      \
	  $(OBJ)/mess/systems/mk2.o

$(OBJ)/ssystem3.a: \
	  $(OBJ)/mess/vidhrdw/ssystem3.o \
	  $(OBJ)/mess/systems/ssystem3.o

$(OBJ)/motorola.a: \
	  $(OBJ)/mess/vidhrdw/mekd2.o    \
	  $(OBJ)/mess/machine/mekd2.o    \
	  $(OBJ)/mess/systems/mekd2.o

$(OBJ)/svi.a:      \
	  $(OBJ)/mess/machine/svi318.o   \
	  $(OBJ)/mess/systems/svi318.o   \
	  $(OBJ)/mess/formats/svi_cas.o

$(OBJ)/exidy.a:    \
	  $(OBJ)/mess/machine/hd6402.o   \
	  $(OBJ)/mess/machine/exidy.o    \
	  $(OBJ)/mess/systems/exidy.o    \
      $(OBJ)/mess/vidhrdw/exidy.o

$(OBJ)/intv.a:     \
	$(OBJ)/mess/vidhrdw/intv.o	\
	$(OBJ)/mess/vidhrdw/stic.o	\
	$(OBJ)/mess/machine/intv.o	\
	$(OBJ)/mess/sndhrdw/intv.o	\
	$(OBJ)/mess/systems/intv.o

$(OBJ)/apf.a:      \
	$(OBJ)/mess/systems/apf.o	\
    $(OBJ)/mess/machine/apf.o	\
    $(OBJ)/mess/vidhrdw/apf.o   \
    $(OBJ)/mess/formats/apfapt.o

$(OBJ)/sord.a:     \
	$(OBJ)/mess/systems/sord.o

$(OBJ)/tatung.a:     \
	$(OBJ)/mess/systems/einstein.o

$(OBJ)/sony.a:     \
	$(OBJ)/mess/systems/psx.o	\
	$(OBJ)/mess/machine/psx.o \
	$(OBJ)/machine/psx.o	\
	$(OBJ)/vidhrdw/psx.o

$(OBJ)/dai.a:     \
	$(OBJ)/mess/systems/dai.o     \
	$(OBJ)/mess/machine/dai.o     \
	$(OBJ)/mess/vidhrdw/dai.o     \
	$(OBJ)/mess/machine/tms5501.o \

$(OBJ)/concept.a:  \
	$(OBJ)/mess/systems/concept.o   \
	$(OBJ)/mess/machine/concept.o

# MESS specific core $(OBJ)s
COREOBJS += \
	$(OBJ)/cheat.o  			   \
	$(OBJ)/vidhrdw/tms9928a.o      \
	$(OBJ)/mess/mess.o			   \
	$(OBJ)/mess/image.o		       \
	$(OBJ)/mess/system.o	       \
	$(OBJ)/mess/device.o	       \
	$(OBJ)/mess/config.o	       \
	$(OBJ)/mess/inputx.o		   \
	$(OBJ)/mess/artworkx.o		   \
	$(OBJ)/mess/mesintrf.o	       \
	$(OBJ)/mess/filemngr.o	       \
	$(OBJ)/mess/compcfg.o	       \
	$(OBJ)/mess/tapectrl.o	       \
	$(OBJ)/mess/utils.o            \
	$(OBJ)/mess/eventlst.o         \
	$(OBJ)/mess/videomap.o         \
	$(OBJ)/mess/formats.o          \
	$(OBJ)/mess/mscommon.o         \
	$(OBJ)/mess/devices/cartslot.o \
	$(OBJ)/mess/devices/messfmts.o \
	$(OBJ)/mess/devices/printer.o  \
	$(OBJ)/mess/devices/cassette.o \
	$(OBJ)/mess/devices/bitbngr.o  \
	$(OBJ)/mess/devices/snapquik.o \
	$(OBJ)/mess/devices/basicdsk.o \
	$(OBJ)/mess/devices/flopdrv.o  \
	$(OBJ)/mess/devices/mess_hd.o  \
	$(OBJ)/mess/devices/idedrive.o \
	$(OBJ)/mess/devices/pc_flopp.o \
	$(OBJ)/mess/devices/dsk.o      \
	$(OBJ)/mess/machine/6551.o     \
	$(OBJ)/mess/vidhrdw/m6847.o    \
	$(OBJ)/mess/vidhrdw/m6845.o    \
	$(OBJ)/mess/machine/msm8251.o  \
	$(OBJ)/mess/machine/tc8521.o   \
	$(OBJ)/mess/vidhrdw/v9938.o    \
	$(OBJ)/mess/vidhrdw/crtc6845.o \
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
	$(OBJ)/mess/machine/d88.o      \
	$(OBJ)/mess/machine/nec765.o   \
	$(OBJ)/mess/machine/wd179x.o   \
	$(OBJ)/mess/machine/serial.o   \
	$(OBJ)/mess/formats/wavfile.o


# additional tools
TOOLS =  dat2html$(EXE)		\
	tools/mkhdimg$(EXE)	\
	tools/imgtool$(EXE)	\
	tools/mkimage$(EXE)


ifdef MSVC
OUTOPT = $(DIRENTOBJS) -out:$@
else
OUTOPT = -o $@
endif

dat2html$(EXE): $(OBJ)/mess/tools/dat2html/dat2html.o $(OBJ)/mess/utils.o 
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(OUTOPT)

tools/mkhdimg$(EXE):	$(OBJ)/mess/tools/mkhdimg/mkhdimg.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(OUTOPT)

tools/mkimage$(EXE):	$(OBJ)/mess/tools/mkimage/mkimage.o $(OBJ)/mess/utils.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(OUTOPT)

tools/messroms$(EXE): $(OBJ)/mess/tools/messroms/main.o $(OBJ)/unzip.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(IMGTOOL_LIBS) $(OUTOPT)


tools/imgtool$(EXE):	                   \
	  $(PLATFORM_IMGTOOL_OBJS)	           \
	  $(OBJ)/unzip.o	                   \
	  $(OBJ)/harddisk.o	                   \
	  $(OBJ)/md5.o	                   \
	  $(OBJ)/mess/config.o	               \
	  $(OBJ)/mess/utils.o	               \
	  $(OBJ)/mess/formats.o                \
	  $(OBJ)/mess/formats/coco_dsk.o       \
	  $(OBJ)/mess/formats/fmsx_cas.o       \
	  $(OBJ)/mess/formats/svi_cas.o        \
	  $(OBJ)/mess/formats/cococas.o        \
	  $(OBJ)/mess/formats/ti85_ser.o       \
	  $(OBJ)/mess/tools/imgtool/stubs.o    \
	  $(OBJ)/mess/tools/imgtool/main.o     \
	  $(OBJ)/mess/tools/imgtool/imghd.o    \
	  $(OBJ)/mess/tools/imgtool/imgtool.o  \
	  $(OBJ)/mess/tools/imgtool/imgwave.o  \
	  $(OBJ)/mess/tools/imgtool/imgtfmts.o \
	  $(OBJ)/mess/tools/imgtool/imgtest.o  \
	  $(OBJ)/mess/tools/imgtool/tstsuite.o \
	  $(OBJ)/mess/tools/imgtool/filter.o   \
	  $(OBJ)/mess/tools/imgtool/filteoln.o \
	  $(OBJ)/mess/tools/imgtool/filtbas.o  \
	  $(OBJ)/mess/tools/imgtool/cococas.o  \
	  $(OBJ)/mess/tools/imgtool/vmsx_tap.o \
	  $(OBJ)/mess/tools/imgtool/vmsx_gm2.o \
	  $(OBJ)/mess/tools/imgtool/fmsx_cas.o \
	  $(OBJ)/mess/tools/imgtool/svi_cas.o  \
	  $(OBJ)/mess/tools/imgtool/msx_dsk.o  \
	  $(OBJ)/mess/tools/imgtool/xsa.o      \
	  $(OBJ)/mess/tools/imgtool/rsdos.o    \
	  $(OBJ)/mess/tools/imgtool/stream.o   \
	  $(OBJ)/mess/tools/imgtool/t64.o      \
	  $(OBJ)/mess/tools/imgtool/lynx.o     \
	  $(OBJ)/mess/tools/imgtool/crt.o      \
	  $(OBJ)/mess/tools/imgtool/d64.o      \
	  $(OBJ)/mess/tools/imgtool/fat.o      \
	  $(OBJ)/mess/tools/imgtool/mac.o      \
	  $(OBJ)/mess/tools/imgtool/rom16.o    \
	  $(OBJ)/mess/tools/imgtool/nccard.o   \
	  $(OBJ)/mess/tools/imgtool/ti85.o     \
	  $(OBJ)/mess/tools/imgtool/ti99.o     \
	  $(OBJ)/mess/tools/imgtool/ti990hd.o  \
	  $(OBJ)/mess/tools/imgtool/concept.o  \
	  $(OBJ)/mess/snprintf.o
	@echo Linking $@...
	$(LD) $(LDFLAGS) $^ $(LIBS) $(IMGTOOL_LIBS) $(OUTOPT)

# text files
TEXTS = mess.txt sysinfo.htm

mess.txt: $(EMULATORCLI)
	@echo Generating $@...
	@$(CURPATH)$(EMULATORCLI) -listtext -noclones -sortname > docs/mess.txt

sysinfo.htm: dat2html$(EXE)
	@echo Generating $@...
	@$(CURPATH)dat2html$(EXE) sysinfo.dat

mess/makedep/makedep$(EXE): $(wildcard mess/makedep/*.c) $(wildcard mess/makedep/*.h)
	make -Cmess/makedep

src/$(NAME).dep depend: mess/makedep/makedep$(EXE) src/$(TARGET).mak src/rules.mak src/core.mak
	mess/makedep/makedep$(EXE) -f - -p$(NAME).obj/ -DMESS -q -- $(INCLUDE_PATH) -- \
	src/*.c src/cpu/*/*.c src/sound/*.c \
	mess/*.c mess/systems/*.c* mess/machine/*.c* mess/vidhrdw/*.c* mess/sndhrdw/*.c* \
	mess/tools/*.c mess/formats/*.c mess/messroms/*.c >src/$(NAME).dep

# add following to dependancy generation if you want the few files in these
# directories
#	src/drivers/*.c src/machine/*.c src/vidhrdw/*.c src/sndhrdw/*.c \

## uncomment the following line to include dependencies
ifeq (src/$(NAME).dep,$(wildcard src/$(NAME).dep))
include src/$(NAME).dep
endif


