# src/drivers/pfdc/Makefile.inc

rel := src/drivers/pfdc

DIRS += $(rel)
DIST += $(rel)/Makefile.inc

DEV_PFDC_BAS := \
	pfdc-img-ana \
	pfdc-img-cp2 \
	pfdc-img-dc42 \
	pfdc-img-imd \
	pfdc-img-pfdc \
	pfdc-img-raw \
	pfdc-img-tc \
	pfdc-img-td0 \
	pfdc-img-xdf \
	pfdc-img \
	pfdc-io \
	pfdc

DEV_PFDC_BAS2 := \
	pfdc-img-pfdc0 \
	pfdc-img-pfdc1 \
	pfdc-img-pfdc2 \
	pfdc-img-pfdc4

DEV_PFDC_SRC := $(foreach f,$(DEV_PFDC_BAS) $(DEV_PFDC_BAS2),$(rel)/$(f).c)
DEV_PFDC_OBJ := $(foreach f,$(DEV_PFDC_BAS) $(DEV_PFDC_BAS2),$(rel)/$(f).o)
DEV_PFDC_HDR := $(foreach f,$(DEV_PFDC_BAS),$(rel)/$(f).h)
DEV_PFDC_ARC := $(rel)/pfdc.a

CLN  += $(DEV_PFDC_ARC) $(DEV_PFDC_OBJ)
DIST += $(DEV_PFDC_SRC) $(DEV_PFDC_HDR)

$(rel)/pfdc-img-ana.o:		$(rel)/pfdc-img-ana.c
$(rel)/pfdc-img-cp2.o:		$(rel)/pfdc-img-cp2.c
$(rel)/pfdc-img-dc42.o:		$(rel)/pfdc-img-dc42.c
$(rel)/pfdc-img-imd.o:		$(rel)/pfdc-img-imd.c
$(rel)/pfdc-img-pfdc.o:		$(rel)/pfdc-img-pfdc.c
$(rel)/pfdc-img-pfdc0.o:	$(rel)/pfdc-img-pfdc0.c
$(rel)/pfdc-img-pfdc1.o:	$(rel)/pfdc-img-pfdc1.c
$(rel)/pfdc-img-pfdc2.o:	$(rel)/pfdc-img-pfdc2.c
$(rel)/pfdc-img-pfdc4.o:	$(rel)/pfdc-img-pfdc4.c
$(rel)/pfdc-img-raw.o:		$(rel)/pfdc-img-raw.c
$(rel)/pfdc-img-tc.o:		$(rel)/pfdc-img-tc.c
$(rel)/pfdc-img-td0.o:		$(rel)/pfdc-img-td0.c
$(rel)/pfdc-img-xdf.o:		$(rel)/pfdc-img-xdf.c
$(rel)/pfdc-img.o:		$(rel)/pfdc-img.c
$(rel)/pfdc-io.o:		$(rel)/pfdc-io.c
$(rel)/pfdc.o:			$(rel)/pfdc.c

$(rel)/pfdc.a: $(DEV_PFDC_OBJ)
