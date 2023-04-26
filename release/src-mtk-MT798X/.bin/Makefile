#### dumpimage is copied from Uboot-upstream/tools

ifeq ($(LINUXDIR),)
	$(error LINUXDIR is not defined!)
endif
ifeq ($(BUILD_NAME),)
	$(error BUILD_NAME is not defined!)
endif

include $(LINUXDIR)/.config

CC=gcc
TARGET_MODEL = $(subst -,,$(subst +,P,$(BUILD_NAME)))
CFLAGS=-DUSE_HOSTCC -I. -I$(TOP)/shared -D$(TARGET_MODEL) $(if $(CONFIG_FACTORY_NR_LEB),-DCONFIG_FACTORY_NR_LEB=$(CONFIG_FACTORY_NR_LEB))
TARGET=modifyData mk_factory_image

all: $(TARGET)

mk_factory_image: mk_factory_image.o crc32.o
	$(CC) -Wall -o $@ $^

%.o: %.c
	$(CC) -Wall -o $@ $(CFLAGS) -c $<

clean:
	$(RM) *.o $(TARGET)
