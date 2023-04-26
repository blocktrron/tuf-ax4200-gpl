#MARCH := -march=aarch64

EXTRACFLAGS := -DLINUX26 -DCONFIG_RALINK -DMUSL_LIBC -DDEBUG_NOISY -DDEBUG_RCTEST -DKERNEL5_MUSL64 -pipe -funit-at-a-time -Wno-pointer-sign $(MARCH)

ifneq ($(findstring linux-3,$(LINUXDIR)),)
EXTRACFLAGS += -DLINUX30
endif

ifeq ($(MT798X),y)
EXTRACFLAGS += -DRALINK_DBDC_MODE
endif

export EXTRACFLAGS
