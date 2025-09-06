#
#  Makefile for Waterloo suck
#
#  Targets:
#     Borland 4.0+        (DOS large model)
#     Metaware HighC 3.1+ (PharLap 386|DosX)
#     Watcom C 10+        (DOS4GW DOS extender)
#     Digital Mars C 8+   (X32VM DOS extender)
#

!include "suck.v"

BORLAND_EXE = suck.exe
PHARLAP_EXP = suck.exp
WATCOM_EXE  = suck4.exe  # not yet
DIGMARS_EXE = suck3.exe  # not yet
QUICKC_EXE  = suck2.exe  # not yet

COMMON_DEFS = -DSUCK_VERSION="$(SUCK_VERSION)"

SRC = active.c batch.c both.c bphrases.c chkhist.c dedupe.c \
      killfile.c killprg.c lm_phras.c phrases.c rphrases.c  \
      suck.c timer.c tphrases.c utils.c

!include "..\common.mak"


