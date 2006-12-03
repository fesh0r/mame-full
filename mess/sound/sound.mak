ifneq ($(filter BEEP,$(SOUNDS)),)
SOUNDDEFS += -DHAS_BEEP=1
SOUNDOBJS += $(OBJ)/mess/sound/beep.o
else
SOUNDDEFS += -DHAS_BEEP=0
endif

ifneq ($(filter WAVE,$(SOUNDS)),)
SOUNDDEFS += -DHAS_WAVE=1
SOUNDOBJS += $(OBJ)/mess/sound/wave.o
else
SOUNDDEFS += -DHAS_WAVE=0
endif

ifneq ($(filter SID6581,$(SOUNDS)),)
SOUNDDEFS += -DHAS_SID6581=1
SOUNDOBJS += $(OBJ)/mess/sound/sid6581.o $(OBJ)/mess/sound/sid.o $(OBJ)/mess/sound/sidenvel.o $(OBJ)/mess/sound/sidvoice.o
else
SOUNDDEFS += -DHAS_SID6581=0
endif

ifneq ($(filter SID8580,$(SOUNDS)),)
SOUNDDEFS += -DHAS_SID8580=1
SOUNDOBJS += $(OBJ)/mess/sound/sid6581.o $(OBJ)/mess/sound/sid.o $(OBJ)/mess/sound/sidenvel.o $(OBJ)/mess/sound/sidvoice.o
else
SOUNDDEFS += -DHAS_SID8580=0
endif

ifneq ($(filter SP0256,$(SOUNDS)),)
SOUNDDEFS += -DHAS_SP0256=1
SOUNDOBJS += $(OBJ)/mess/sound/sp0256.o
else
SOUNDDEFS += -DHAS_SP0256=0
endif
