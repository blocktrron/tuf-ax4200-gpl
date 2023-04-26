PBBINS=$(wildcard prebuilt/*)
LIBS=$(filter %.so,$(PBBINS))
KMOD=$(filter %.ko,$(PBBINS))
EXEC=$(filter-out $(LIBS) $(KMOD),$(PBBINS))

all:

install:
	@[ -d $(INSTALLDIR)/usr/bin ] || install -d $(INSTALLDIR)/usr/bin
	@for i in $(EXEC); do install -D $$i $(INSTALLDIR)/usr/bin ; done
	@$(STRIP) $(INSTALLDIR)/usr/bin/*

clean:

stage:
