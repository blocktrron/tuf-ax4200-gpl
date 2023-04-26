include upb.inc
-include $(TOPPREFIX)/.config
-include $(LINUXDIR)/.config
-include $(SRCBASE)/.config

pb-y				+= flash
pb-$(RTCONFIG_WLCEVENTD)	+= iwevent
pb-$(CONFIG_PWM_MEDIATEK)	+= pwm-mtk-mm

all: $(pb-y) $(pb-m)

############################################################################
# Generate short variable for destination directory.
# NOTE: Only one variable can be defined in one target.
############################################################################
$(pb-y) $(pb-m): S=$(TOPPREFIX)/$@
$(pb-y) $(pb-m): D=$(PBTOPDIR)/$@/prebuild

############################################################################
# Define special S or D variable here.
# NOTE: Only one variable can be defined in one target.
############################################################################
pwm-mtk-mm: S=$(LINUXDIR)/drivers/pwm
pwm-mtk-mm: D=$(PBLINUXDIR)/drivers/pwm/.objs

############################################################################
# Copy binary
############################################################################
flash:
	$(call inst,$(S),$(D),flash)

iwevent:
	$(call inst,$(S),$(D),wlceventd)

pwm-mtk-mm:
	$(call _inst,$(S)/pwm-mtk-mm.o,$(D)/pwm-mtk-mm.o)
	

.PHONY: all $(pb-y) $(pb-m)
