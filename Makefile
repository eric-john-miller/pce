# Makefile

prefix      = /usr/local
exec_prefix = ${prefix}
bindir      = ${exec_prefix}/bin
etcdir      = ${prefix}/etc
incdir      = ${prefix}/include
libdir      = ${exec_prefix}/lib
mandir      = ${datarootdir}/man
datarootdir = ${prefix}/share
datadir     = ${datarootdir}

srcdir := .



ifeq "$(V)" "1"
QP = @\#
QR =
else
QP = @
QR = @
endif


all: all2


AR     := /usr/bin/ar
RANLIB := ranlib

BIN   :=
BINS  :=
ETC   :=
MAN1  :=
MAN2  :=
MAN3  :=
SHARE :=

CLN :=
DCL :=

DIRS    :=
TARGETS :=
DIST    :=

include Makefile.inc

MANA := $(MAN1) $(MAN2) $(MAN3)
MANT := $(foreach f,$(MANA),$(f).txt $(f).ps)

CLN += $(MANT)


all2: subdirs $(TARGETS)


subdirs:
ifneq "$(DIRS)" ""
	$(QR)for f in $(DIRS) ; do \
		if test -d "$$f" ; then continue ; fi ; \
		if test x$(V) != x1 ; then echo "  MKDIR  $$f" ; fi ; \
		mkdir -p "$$f" ; \
	done
endif


clean:
ifneq "$(CLN)" ""
	$(QR)for f in $(CLN) ; do \
		if test x$(V) != x1 ; then echo "  RM     $$f" ; fi ; \
		rm -f "$$f" ; \
	done
endif


distclean: clean
ifneq "$(DCL)" ""
	$(QR)for f in $(DCL) ; do \
		if test x$(V) != x1 ; then echo "  RM     $$f" ; fi ; \
		rm -f "$$f" ; \
	done
endif


man: $(MANT)


install: install-bin install-bins install-etc install-man install-share install-extra

install-bin:
ifneq "$(BIN)" ""
	$(QP)echo "  MKDIR  $(bindir)"
	$(QR)$(INSTALL) -d -m 755 $(DESTDIR)$(bindir)
	$(QR)for f in $(BIN) ; do \
		dst=$(DESTDIR)$(bindir)/`basename "$$f"` ; \
		if test x$(V) != x1 ; then echo "  CP     $$dst" ; fi ; \
		$(INSTALL) -m 755 "$$f" "$$dst" ; \
	done
endif

install-bins:
ifneq "$(BINS)" ""
	$(QP)echo "  MKDIR  $(bindir)"
	$(QR)$(INSTALL) -d -m 755 $(DESTDIR)$(bindir)
	$(QR)for f in $(BINS) ; do \
		dst=$(DESTDIR)$(bindir)/`basename "$$f"` ; \
		if test x$(V) != x1 ; then echo "  CP     $$dst" ; fi ; \
		$(INSTALL) -m 755 "$$f" "$$dst" ; \
	done
endif

install-etc:
ifneq "$(ETC)" ""
	$(QP)echo "  MKDIR  $(DESTDIR)$(etcdir)"
	$(QR)$(INSTALL) -d -m 755 $(DESTDIR)$(etcdir)
	$(QR)for f in $(ETC) ; do \
		dst=$(DESTDIR)$(etcdir)/`basename "$$f"` ; \
		if test x$(V) != x1 ; then echo "  CP     $$dst" ; fi ; \
		$(INSTALL) -m 644 "$$f" "$$dst" ; \
	done
endif

install-man:
ifneq "$(MAN1)" ""
	$(QP)echo "  MKDIR  $(mandir)/man1"
	$(QR)$(INSTALL) -d -m 755 $(DESTDIR)$(mandir)/man1
	$(QR)for f in $(MAN1) ; do \
		dst=$(DESTDIR)$(mandir)/man1/`basename "$$f"` ; \
		if test x$(V) != x1 ; then echo "  CP     $$dst" ; fi ; \
		$(INSTALL) -m 644 "$(srcdir)/$$f" "$$dst" ; \
	done
endif

install-share:
ifneq "$(SHARE)" ""
	$(QP)echo "  MKDIR  $(DESTDIR)$(datadir)"
	$(QR)$(INSTALL) -d -m 755 $(DESTDIR)$(datadir)
	$(QR)for f in $(SHARE) ; do \
		dst=$(DESTDIR)$(datadir)/`basename "$$f"` ; \
		if test x$(V) != x1 ; then echo "  CP     $$dst" ; fi ; \
		$(INSTALL) -m 644 "$$f" "$$dst" ; \
	done
endif

install-extra:


dist: dist-dist dist-contrib dist-extra dist-version
	$(QP)echo "  TAR    $(distdir).tar"
	$(QR)( cd "$(distdir)"/.. && \
		tar -cvf "$(distdir).tar" `basename "$(distdir)"` > /dev/null )
	$(QP)echo "  GZIP   $(distdir).tar.gz"
	$(QR)rm -f "$(distdir).tar.gz"
	$(QR)gzip -9 "$(distdir).tar"

dist-dist:
ifneq "$(DIST)" ""
	$(QP)echo "  MKDIR  $(distdir)"
	$(QR)mkdir -p "$(distdir)"
	$(QR)for f in $(DIST) ; do \
		if test -f "$$f" ; then \
			src=$$f ; \
		elif test -f "$(srcdir)/$$f" ; then \
			src=$(srcdir)/$$f ; \
		else \
			if test x$(V) != x1 ; then  echo "  SKIP   $$f" ; fi ; \
			continue ; \
		fi ; \
		if test x$(V) != x1 ; then echo "  CP     $$f" ; fi ; \
		dir=$(distdir)/`dirname "$$f"` ; \
		mkdir -p "$$dir" ; \
		cp -p "$$src" "$$dir" ; \
	done
endif

dist-contrib:
	$(QR)if test -d "$(srcdir)/contrib" ; then \
		( cd "$(srcdir)" && find contrib/ -type f -print ) |\
		while read src ; do \
			test -f "$(distdir)/$$src" && continue ; \
			test x$(V) != x1 && echo "  CP     $$src" ; \
			dir=`dirname "$(distdir)/$$src"` ; \
			test -d "$$dir" || mkdir -p "$$dir" ; \
			cp -p "$(srcdir)/$$src" "$$dir" ; \
		done ; \
	fi

dist-extra:

# ----------------------------------------------------------------------

%.o: %.c
	$(QP)echo "  CC     $@"
	$(QR)$(CC) -c $(CFLAGS_DEFAULT) -o $@ $<

%.o: %.cxx
	$(QP)echo "  CXX    $@"
	$(QR)$(CXX) -c $(CXXFLAGS_DEFAULT) -o $@ $<

%.o: %.cpp
	$(QP)echo "  CXX    $@"
	$(QR)$(CXX) -c $(CXXFLAGS_DEFAULT) -o $@ $<

%.a:
	$(QP)echo "  AR     $@"
	$(QR)rm -f $@
	$(QR)$(AR) -rc $@ $^
	$(QP)echo "  RANLIB $@"
	$(QR)$(RANLIB) $@

%.1.ps: %.1
	$(QP)echo "  MAN    $@"
	$(QR)groff -Tps -mandoc < $< > $@

%.1.man: %.1
	$(QP)echo "  MAN    $@"
	$(QR)troff -Tlatin1 -mandoc < $< | grotty -c > $@

%.1.txt: %.1
	$(QP)echo "  MAN    $@"
	$(QR)troff -Tlatin1 -mandoc < $< | grotty -c -b -o -u > $@
