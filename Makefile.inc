# Makefile.inc

etcdir      = ${prefix}/etc/pce
datarootdir = ${prefix}/share
datadir     = ${datarootdir}/pce

CC      = gcc
LD      = gcc
NASM    = 
IHEX    = 
LN_S    = ln -s
INSTALL = /usr/bin/install -c

CFLAGS         = -g -O2
CFLAGS_DEFAULT = $(CFLAGS) -Isrc

LDFLAGS         = 
LDFLAGS_DEFAULT = $(LDFLAGS)

EXEEXT := 

LIBS := 

ifneq "$(srcdir)" "."
CFLAGS_DEFAULT += -I$(srcdir)/src
endif

PCE_VERSION_MAJ := 0
PCE_VERSION_MIN := 2
PCE_VERSION_MIC := 2
PCE_VERSION_STR := 0.2.2

PCE_HAVE_NASM := 0
PCE_HAVE_IHEX := 0

PCE_ENABLE_X11 := 1
PCE_X11_CFLAGS := 
PCE_X11_LIBS   := -lX11

PCE_ENABLE_SDL := 1
PCE_SDL_CFLAGS := -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
PCE_SDL_LIBS   := -L/usr/lib/i386-linux-gnu -lSDL

PCE_HAVE_STDINT_H   := 1
PCE_HAVE_INTTYPES_H := 1

PCE_LARGE_FILE := 1

PCE_HOST_LINUX   := 1
PCE_HOST_WINDOWS := 0
PCE_HOST_SUNOS   := 0
PCE_HOST_NETBSD  := 0

PCE_HOST_IA32  := 0
PCE_HOST_PPC   := 0
PCE_HOST_SPARC := 0

ifneq "$(PCE_HAVE_STDINT_H)" "1"
CFLAGS_DEFAULT += -Isrc/lib/stdint
ifneq "$(srcdir)" "."
CFLAGS_DEFAULT += -I$(srcdir)/src/lib/stdint
endif
endif

ifeq "$(PCE_HOST_IA32)" "1"
CFLAGS_DEFAULT += -DPCE_HOST_IA32
endif

ifeq "$(PCE_HOST_PPC)" "1"
CFLAGS_DEFAULT += -DPCE_HOST_PPC
endif

ifeq "$(PCE_HOST_SPARC)" "1"
CFLAGS_DEFAULT += -DPCE_HOST_SPARC
endif

PCE_BUILD_IBMPC   := 1
PCE_BUILD_MACPLUS := 1
PCE_BUILD_RC759   := 1
PCE_BUILD_SIM405  := 1
PCE_BUILD_SIM6502 := 1
PCE_BUILD_SIMARM  := 1
PCE_BUILD_SIMS32  := 1

PCE_BUILD_IBMPC_ROM   := 0
PCE_BUILD_MACPLUS_ROM := 0

PCE_ENABLE_READLINE := 0
PCE_READLINE_LIBS   := 

PCE_ENABLE_TUN := 1

PCE_ENABLE_CHAR_POSIX  := 1
PCE_ENABLE_CHAR_PPP    := 1
PCE_ENABLE_CHAR_PTY    := 1
PCE_ENABLE_CHAR_SLIP   := 1
PCE_ENABLE_CHAR_TCP    := 1
PCE_ENABLE_CHAR_TIOS   := 1

PCE_ENABLE_SOUND_OSS := 1

distdir     := pce-$(PCE_VERSION_STR)

# ----------------------------------------------------------------------

DCL += Makefile Makefile.inc configure config.log config.status

DIST += AUTHORS COPYING ChangeLog INSTALL \
	Makefile.dep Makefile.in Makefile.inc.in \
	NEWS README TODO autogen.sh config.guess config.sub \
	configure configure.in gendep install-sh

DIST += doc/char-drivers.txt doc/keys.txt doc/messages.txt \
	doc/monitor.txt doc/pce-ibmpc.txt doc/pce-macplus.txt \
	doc/pbit-format.txt doc/pfdc-format.txt doc/rc759-keymap.txt \
	doc/sound-drivers.txt

# ----------------------------------------------------------------------

include $(srcdir)/Makefile.dep

include $(srcdir)/src/Makefile.inc
include $(srcdir)/src/cpu/Makefile.inc
include $(srcdir)/src/cpu/arm/Makefile.inc
include $(srcdir)/src/cpu/e6502/Makefile.inc
include $(srcdir)/src/cpu/e68000/Makefile.inc
include $(srcdir)/src/cpu/e8086/Makefile.inc
include $(srcdir)/src/cpu/ppc405/Makefile.inc
include $(srcdir)/src/cpu/sparc32/Makefile.inc
include $(srcdir)/src/chipset/80186/Makefile.inc
include $(srcdir)/src/chipset/82xx/Makefile.inc
include $(srcdir)/src/chipset/clock/Makefile.inc
include $(srcdir)/src/chipset/ppc405/Makefile.inc
include $(srcdir)/src/chipset/Makefile.inc
include $(srcdir)/src/devices/clock/Makefile.inc
include $(srcdir)/src/devices/video/Makefile.inc
include $(srcdir)/src/devices/Makefile.inc
include $(srcdir)/src/libini/Makefile.inc
include $(srcdir)/src/lib/Makefile.inc
include $(srcdir)/src/drivers/Makefile.inc
include $(srcdir)/src/drivers/block/Makefile.inc
include $(srcdir)/src/drivers/char/Makefile.inc
include $(srcdir)/src/drivers/pbit/Makefile.inc
include $(srcdir)/src/drivers/pfdc/Makefile.inc
include $(srcdir)/src/drivers/sound/Makefile.inc
include $(srcdir)/src/drivers/video/Makefile.inc
include $(srcdir)/src/arch/Makefile.inc
include $(srcdir)/src/arch/ibmpc/Makefile.inc
include $(srcdir)/src/arch/ibmpc/bios/Makefile.inc
include $(srcdir)/src/arch/ibmpc/pceutils/Makefile.inc
include $(srcdir)/src/arch/macplus/Makefile.inc
include $(srcdir)/src/arch/macplus/pcex/Makefile.inc
include $(srcdir)/src/arch/rc759/Makefile.inc
include $(srcdir)/src/arch/sim405/Makefile.inc
include $(srcdir)/src/arch/sim6502/Makefile.inc
include $(srcdir)/src/arch/simarm/Makefile.inc
include $(srcdir)/src/arch/sims32/Makefile.inc
include $(srcdir)/src/utils/Makefile.inc
include $(srcdir)/src/utils/pbit/Makefile.inc
include $(srcdir)/src/utils/pce-img/Makefile.inc
include $(srcdir)/src/utils/pfdc/Makefile.inc

TARGETS += $(BIN) $(ETC) $(SHARE)

# ----------------------------------------------------------------------

%: %.sh
	$(QP)echo "  SED    $@"
	$(QR)rm -f $@
	$(QR)sed -e "s/%PCE_VERSION_STR/$(PCE_VERSION_STR)/g" < $< > $@
	$(QR)chmod a+x $@

install-extra:
ifneq "$(SHARE_IBMPC)" ""
	$(QP)echo "  MKDIR  $(DESTDIR)$(datadir)/ibmpc"
	$(QR)$(INSTALL) -d -m 755 $(DESTDIR)$(datadir)/ibmpc
	$(QR)for f in $(SHARE_IBMPC) ; do \
		dst=$(DESTDIR)$(datadir)/ibmpc/`basename "$$f"` ; \
		if test x$(V) != x1 ; then echo "  CP     $$dst" ; fi ; \
		$(INSTALL) -m 644 "$$f" "$$dst" ; \
	done
endif
ifneq "$(SHARE_MACPLUS)" ""
	$(QP)echo "  MKDIR  $(DESTDIR)$(datadir)/macplus"
	$(QR)$(INSTALL) -d -m 755 $(DESTDIR)$(datadir)/macplus
	$(QR)for f in $(SHARE_MACPLUS) ; do \
		dst=$(DESTDIR)$(datadir)/macplus/`basename "$$f"` ; \
		if test x$(V) != x1 ; then echo "  CP     $$dst" ; fi ; \
		$(INSTALL) -m 644 "$$f" "$$dst" ; \
	done
endif
ifneq "$(SHARE_PCEUTILS)" ""
	$(QR)$(INSTALL) -d -m 755 $(DESTDIR)$(datadir)/ibmpc/pceutils
	$(QR)for f in $(SHARE_PCEUTILS) ; do \
		dst=$(DESTDIR)$(datadir)/ibmpc/pceutils/`basename "$$f"` ; \
		test x$(V) != x1 && echo "  CP     $$dst" ; \
		$(INSTALL) -m 644 "$$f" "$$dst" ; \
	done
endif
	$(QR)for src in $(srcdir)/contrib/rom/ibmpc/* ; do \
		test -f "$$src" || continue ; \
		dst=$(DESTDIR)$(datadir)/ibmpc/`basename $$src` ; \
		test -f "$$dst" && continue ; \
		test x$(V) != x1 && echo "  CP     $$dst" ; \
		$(INSTALL) -d -m 755 "$(DESTDIR)$(datadir)/ibmpc" ; \
		$(INSTALL) -m 644 "$$src" "$$dst" ; \
	done
	$(QR)for src in $(srcdir)/contrib/rom/macplus/* ; do \
		test -f "$$src" || continue ; \
		dst=$(DESTDIR)$(datadir)/macplus/`basename $$src` ; \
		test -f "$$dst" && continue ; \
		test x$(V) != x1 && echo "  CP     $$dst" ; \
		$(INSTALL) -d -m 755 "$(DESTDIR)$(datadir)/macplus" ; \
		$(INSTALL) -m 644 "$$src" "$$dst" ; \
	done
	$(QR)for src in $(srcdir)/contrib/rom/rc759/* ; do \
		test -f "$$src" || continue ; \
		dst=$(DESTDIR)$(datadir)/rc759/`basename $$src` ; \
		test -f "$$dst" && continue ; \
		test x$(V) != x1 && echo "  CP     $$dst" ; \
		$(INSTALL) -d -m 755 "$(DESTDIR)$(datadir)/rc759" ; \
		$(INSTALL) -m 644 "$$src" "$$dst" ; \
	done


dist-extra:
ifneq "$(SHARE_PCEUTILS)" ""
	$(QR)for f in $(PCEUTILS_BIN) ; do \
		test -f "$$f" || continue ; \
		dst=contrib/pceutils/`basename "$$f"` ; \
		test x$(V) != x1 && echo "  CP     $$dst" ; \
		mkdir -p "$(distdir)/contrib/pceutils" ; \
		cp -p "$$f" "$(distdir)/$$dst" ; \
	done
endif


dist-version:
	$(QP)echo "  GEN    version"
	$(QR)echo "pce  $(PCE_VERSION_MAJ) $(PCE_VERSION_MIN) $(PCE_VERSION_MIC)  $(PCE_VERSION_STR)" \
		> "$(distdir)/version"

release:
	$(MAKE) PCE_VERSION_STR=$(PCE_VERSION_MAJ).$(PCE_VERSION_MIN).$(PCE_VERSION_MIC) dist
