###########################################################################
#
#   mame.mak
#
#   MAME target makefile
#
#   Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################


MAMESRC = $(SRC)/mame
MAMEOBJ = $(OBJ)/mame

AUDIO = $(MAMEOBJ)/audio
DRIVERS = $(MAMEOBJ)/drivers
LAYOUT = $(MAMEOBJ)/layout
MACHINE = $(MAMEOBJ)/machine
VIDEO = $(MAMEOBJ)/video

OBJDIRS += \
	$(AUDIO) \
	$(DRIVERS) \
	$(LAYOUT) \
	$(MACHINE) \
	$(VIDEO) \



#-------------------------------------------------
# specify available CPU cores; some of these are
# only for MESS and so aren't included
#-------------------------------------------------

CPUS += Z80
CPUS += M68000



#-------------------------------------------------
# specify available sound cores; some of these are
# only for MESS and so aren't included
#-------------------------------------------------

SOUNDS += YM2151
SOUNDS += OKIM6295
SOUNDS += QSOUND
SOUNDS += YM2610
SOUNDS += YM2610B



#-------------------------------------------------
# this is the list of driver libraries that
# comprise MAME plus mamedriv.o which contains
# the list of drivers
#-------------------------------------------------

DRVLIBS = \
	$(MAMEOBJ)/mamedriv.o

ifneq ($(USE_DRIVER_SWITCH),)
DRVLIBS += $(MAMEOBJ)/mameplusdriv.o \
            $(MAMEOBJ)/mamehbdriv.o \
            $(MAMEOBJ)/mameneoddriv.o
endif

DRVLIBS += \
	$(MAMEOBJ)/capcom.a \
	$(MAMEOBJ)/neogeo.a \
	$(MAMEOBJ)/shared.a \



#-------------------------------------------------
# the following files are general components and
# shared across a number of drivers
#-------------------------------------------------

$(MAMEOBJ)/shared.a: \
	$(MACHINE)/pd4990a.o \


#-------------------------------------------------
# manufacturer-specific groupings for drivers
#-------------------------------------------------

$(MAMEOBJ)/capcom.a: \
	$(DRIVERS)/cps1.o $(VIDEO)/cps1.o \
	$(DRIVERS)/cps2.o \
	$(MACHINE)/cps2crpt.o \
	$(MACHINE)/kabuki.o \

$(MAMEOBJ)/neogeo.a: \
	$(DRIVERS)/neogeo.o $(MACHINE)/neogeo.o $(VIDEO)/neogeo.o \
	$(MACHINE)/neoboot.o \
	$(MACHINE)/neocrypt.o \
	$(MACHINE)/neoprot.o \
