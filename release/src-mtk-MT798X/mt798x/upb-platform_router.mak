include upb.inc
-include $(TOPPREFIX)/.config
-include $(LINUXDIR)/.config

define ll_tar
	if [ -z "$(V)" ] ; then echo "  [TAR]  $(3) -> $(3)" ; fi
	$(quiet)[ -e "$(2)" ] || mkdir -p "$(2)"
	$(quiet)cd "$(1)" && tar cf - $(3) | tar xf - -C "$(2)"
endef

prpb-y += conninfra
ifeq ($(MT7981),y)
prpb-y += warp_7981
prpb-y += mt_wifi_7981
else
prpb-y += warp
prpb-y += mt_wifi
endif
prpb-y += mii_mgr
prpb-y += libnl-tiny
prpb-y += switch
prpb-y += ated_ext
prpb-y += 8021xd
prpb-y += regs

all: $(prpb-y) $(prpb-m)


############################################################################
# Generate short variable for destination directory.
# NOTE: Only one variable can be defined in one target.
############################################################################
$(prpb-y) $(prpb-m): S=$(PLATFORM_TOPDIR)/$@
$(prpb-y) $(prpb-m): D=$(PLATFORM_PBTOPDIR)/$@/prebuilt


############################################################################
# Define special S or D variable here.
# NOTE: Only one variable can be defined in one target.
############################################################################


############################################################################
# Copy binary
############################################################################
conninfra:
	$(call inst,$(S),$(D),source/conninfra.ko)

ifeq ($(MT7981),y)
warp_7981:
	$(call inst,$(S),$(D),source/mtk_warp.ko)
	$(call inst,$(S),$(D),source/bin/7981_WOCPU0_RAM_CODE_release.bin)

mt_wifi_7981:
	$(call inst,$(S),$(D),source/mt_wifi_ap/mt_wifi.ko)

else
warp:
	$(call inst,$(S),$(D),source/mtk_warp.ko)
	$(call inst,$(S),$(D),source/bin/7986_WOCPU0_RAM_CODE_release.bin)
	$(call inst,$(S),$(D),source/bin/7986_WOCPU1_RAM_CODE_release.bin)

mt_wifi:
	$(call inst,$(S),$(D),source/mt_wifi_ap/mt_wifi.ko)

endif
mii_mgr:
	$(call inst,$(S),$(D),source/mii_mgr)

libnl-tiny:
	$(call inst,$(S),$(D),source/libnl-tiny.so)

switch:
	$(call inst,$(S),$(D),source/switch)

ated_ext:
	$(call inst,$(S),$(D),source/ated_ext)

8021xd:
	$(call inst,$(S),$(D),source/8021xd)

regs:
	$(call inst,$(S),$(D),source/regs)

.PHONY: all $(prpb-y) $(prpb-m)

