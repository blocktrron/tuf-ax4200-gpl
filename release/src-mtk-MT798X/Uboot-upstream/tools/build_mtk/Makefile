-include ../../include/config/auto.conf
-include ../../include/autoconf.mk

ifeq ($(CONFIG_MODEL),)
$(error No model!)
endif

all:
	./fiptool create --soc-fw $(CONFIG_MODEL)/bl31.bin --nt-fw ../../u-boot.bin $(CONFIG_MODEL)/fip.bin
	ln -sf $(CONFIG_MODEL)/bl2.img .
	ln -sf $(CONFIG_MODEL)/fip.bin .
	../mkimage -f ./uboot.its ../../u-boot_nand.img

clean:
	rm -f fip.bin bl2.img
