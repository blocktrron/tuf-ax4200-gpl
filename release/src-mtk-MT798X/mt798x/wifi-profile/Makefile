# files:
#    package/mtk/drivers/wifi-profile/files
# smp.sh, smp-mac80211.sh:
#    package/mtk/applications/mtk-base-files

include ../sdk.config

FILES_SUBDIR=files
L1PROFILE_SUBDIR=l1profile
SMP_SCRIPT_DIR=smp

ifeq ($(CONFIG_MTK_CHIP_MT7986),y)
SOC_SUBDIR=mt7986
ifeq ($(CONFIG_MTK_WIFI_SKU_TYPE),"AX6000")
PROFILE_SUBDIR=mt7986_ax6000
else
PROFILE_SUBDIR=mt7986_ax4200
endif
else ifeq ($(CONFIG_MTK_CHIP_MT7981),y)
SOC_SUBDIR=mt7981
PROFILE_SUBDIR=mt7981_ax3000
else
$(error Non CHIP defined!)
endif

PKG_DIR=$(shell pwd)

all:
	@#Nothing to do

install:
	install -d $(INSTALLDIR)/rom/etc/wireless/mediatek
	install -m 644 $(PKG_DIR)/$(FILES_SUBDIR)/$(SOC_SUBDIR)/* $(INSTALLDIR)/rom/etc/wireless/mediatek
	install -m 644 $(PKG_DIR)/$(L1PROFILE_SUBDIR)/$(PROFILE_SUBDIR)/* $(INSTALLDIR)/rom/etc/wireless
	# obey the old config path...
ifeq ($(CONFIG_MTK_CHIP_MT7986),y)
ifeq ($(CONFIG_MTK_WIFI_SKU_TYPE),"AX6000")
	ln -sf /etc/Wireless/RT2860/RT2860.dat $(INSTALLDIR)/rom/etc/wireless/mediatek/mt7986-ax6000.dbdc.b0.dat
	ln -sf /etc/Wireless/iNIC/iNIC_ap.dat $(INSTALLDIR)/rom/etc/wireless/mediatek/mt7986-ax6000.dbdc.b1.dat
else # AX4200 
	ln -sf /etc/Wireless/RT2860/RT2860.dat $(INSTALLDIR)/rom/etc/wireless/mediatek/mt7986-ax4200.dbdc.b0.dat
	ln -sf /etc/Wireless/iNIC/iNIC_ap.dat $(INSTALLDIR)/rom/etc/wireless/mediatek/mt7986-ax4200.dbdc.b1.dat
endif
else ifeq ($(CONFIG_MTK_CHIP_MT7981),y)
	ln -sf /etc/Wireless/RT2860/RT2860.dat $(INSTALLDIR)/rom/etc/wireless/mediatek/mt7981.dbdc.b0.dat
	ln -sf /etc/Wireless/iNIC/iNIC_ap.dat $(INSTALLDIR)/rom/etc/wireless/mediatek/mt7981.dbdc.b1.dat
endif
	ln -sf /etc/Wireless/DBDC_card0.dat $(INSTALLDIR)/rom/etc/wireless/mediatek/DBDC_card0.dat
	install -d $(INSTALLDIR)/sbin
	install -m 755 $(PKG_DIR)/$(SMP_SCRIPT_DIR)/* $(INSTALLDIR)/sbin
	@[ ! -e $(BUILD_NAME) ] || [ ! -e $(BUILD_NAME)/$(FILES_SUBDIR) ] || (install -m 644 $(BUILD_NAME)/$(FILES_SUBDIR)/* $(INSTALLDIR)/rom/etc/wireless/mediatek)
	@[ ! -e $(BUILD_NAME) ] || [ ! -e $(BUILD_NAME)/$(L1PROFILE_SUBDIR) ] || (install -m 644 $(BUILD_NAME)/$(L1PROFILE_SUBDIR)/* $(INSTALLDIR)/rom/etc/wireless)
	@[ ! -e $(BUILD_NAME) ] || [ ! -e $(BUILD_NAME)/$(SMP_SCRIPT_DIR) ] || (install -m 755 $(BUILD_NAME)/$(SMP_SCRIPT_DIR)/* $(INSTALLDIR)/sbin)

clean:
	@#Nothing to do
