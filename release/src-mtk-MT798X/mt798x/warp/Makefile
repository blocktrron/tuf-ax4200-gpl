LINUX_VERSION=$(LINUX_KERNEL)
INSTALLKMODDIR:=$(INSTALLDIR)/lib/modules/$(LINUX_VERSION)

PBBINS=$(wildcard prebuilt/*)
LIBS=$(filter %.so,$(PBBINS))
KMOD=$(filter %.ko,$(PBBINS))
BINS=$(filter %.bin,$(PBBINS))
EXEC=$(filter-out $(LIBS) $(KMOD) $(BINS),$(PBBINS))

all:

install:
#	@[ -d $(INSTALLDIR)/usr/lib ] || install -d $(INSTALLDIR)/usr/lib
#	@for i in $(LIBS); do install -D $$i $(INSTALLDIR)/usr/lib ; done
#	@$(STRIP) $(INSTALLDIR)/usr/lib/*
	@[ -d $(INSTALLKMODDIR) ] || install -d $(INSTALLKMODDIR)
	@for i in $(KMOD); do install -D $$i $(INSTALLKMODDIR) ; done
	@$(KSTRIPX) $(INSTALLKMODDIR)/*.ko
	@[ -d $(INSTALLDIR)/lib/firmware ] || install -d $(INSTALLDIR)/lib/firmware
	@for i in $(BINS); do install -D $$i $(INSTALLDIR)/lib/firmware ; done
#	@[ -d $(INSTALLDIR)/usr/bin ] || install -d $(INSTALLDIR)/usr/bin
#	@for i in $(EXEC); do install -D $$i $(INSTALLDIR)/usr/bin ; done
#	@$(STRIP) $(INSTALLDIR)/usr/bin/*

clean:

stage:
