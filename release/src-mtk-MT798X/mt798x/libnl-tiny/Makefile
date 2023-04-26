LINUX_VERSION=$(LINUX_KERNEL)
INSTALLKMODDIR:=$(INSTALLDIR)/lib/modules/$(LINUX_VERSION)

PBBINS=$(wildcard prebuilt/*)
LIBS=$(filter %.so,$(PBBINS))
KMOD=$(filter %.ko,$(PBBINS))
EXEC=$(filter-out $(LIBS) $(KMOD),$(PBBINS))

all:

install:
	@[ -d $(INSTALLDIR)/usr/lib ] || install -d $(INSTALLDIR)/usr/lib
	@for i in $(LIBS); do install -D $$i $(INSTALLDIR)/usr/lib ; done
#	@$(STRIP) $(INSTALLDIR)/usr/lib/*
#	@[ -d $(INSTALLKMODDIR) ] || install -d $(INSTALLKMODDIR)
#	@for i in $(KMOD); do install -D $$i $(INSTALLKMODDIR) ; done
#	@$(KSTRIPX) $(INSTALLKMODDIR)/*.ko
#	@[ -d $(INSTALLDIR)/usr/bin ] || install -d $(INSTALLDIR)/usr/bin
#	@for i in $(EXEC); do install -D $$i $(INSTALLDIR)/usr/bin ; done
#	@$(STRIP) $(INSTALLDIR)/usr/bin/*

clean:

stage:
