# SPDX-License-Identifier: GPL-2.0
ifeq ($(CONFIG_PANTHERA),y)
dtb-y += mt7986a-PANTHERA.dtb
else ifeq ($(CONFIG_PANTHERB),y)
dtb-y += mt7986b-PANTHERB.dtb
else ifeq ($(CONFIG_CHEETAH),y)
dtb-y += mt7981-CHEETAH.dtb
else ifeq ($(CONFIG_TUFAX4200),y)
dtb-y += mt7986a-TUF-AX4200.dtb
else ifeq ($(CONFIG_TUFAX6000),y)
dtb-y += mt7986a-TUF-AX6000.dtb
else ifeq ($(CONFIG_RTAX59U),y)
dtb-y += mt7986a-RT-AX59U.dtb
else
dtb-$(CONFIG_ARCH_MEDIATEK) += mt2712-evb.dtb
dtb-$(CONFIG_ARCH_MEDIATEK) += mt6755-evb.dtb
dtb-$(CONFIG_ARCH_MEDIATEK) += mt6795-evb.dtb
dtb-$(CONFIG_ARCH_MEDIATEK) += mt6797-evb.dtb
dtb-$(CONFIG_ARCH_MEDIATEK) += mt6797-x20-dev.dtb
dtb-$(CONFIG_ARCH_MEDIATEK) += mt7622-rfb1.dtb
dtb-$(CONFIG_ARCH_MEDIATEK) += mt7622-bananapi-bpi-r64.dtb
dtb-$(CONFIG_ARCH_MEDIATEK) += mt8173-evb.dtb
dtb-$(CONFIG_ARCH_MEDIATEK) += mt8183-evb.dtb
endif
