SYSDEP_DIR = $(OBJDIR)/sysdep
DSP_DIR = $(OBJDIR)/sysdep/dsp-drivers
MIXER_DIR = $(OBJDIR)/sysdep/mixer-drivers
VID_DIR = $(OBJDIR)/video-drivers
JOY_DIR = $(OBJDIR)/joystick-drivers
FRAMESKIP_DIR = $(OBJDIR)/frameskip-drivers

OBJDIRS += $(OBJDIR) $(SYSDEP_DIR) $(DSP_DIR) $(MIXER_DIR) $(VID_DIR) \
	$(JOY_DIR) $(FRAMESKIP_DIR)
