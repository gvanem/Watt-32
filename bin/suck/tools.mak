#
#  GNU Makefile for suck tools
#
#  Only djgpp2 supported at this time.
#

.SUFFIXES: .exe

include suck.v

#
# Change to suit
#
DEFINES = -DDEBUG1 -DMYSIGNAL=SIGINT -DPOSIX_SIGNALS \
          -DHAVE_REGEX_H -DSUCK_VERSION=\"$(SUCK_VERSION)\"

W32ROOT= $(subst \,/,$(WATT_ROOT))
CC     = gcc.exe
CFLAGS = $(DEFINES) -Wall -g -I$(W32ROOT)/inc
LINK   = redir -o $*.map $(CC) -Xlinker -M
LFLAGS = $(W32ROOT)/lib/libwatt.a

all: testhost.exe lmove.exe rpost.exe lpost.exe mphrases.exe

.o.exe:
	$(LINK) -o $@ $^ $(LFLAGS)

testhost.exe:  testhost.o both.o tphrases.o bphrases.o
lmove.exe:     lmove.o both.o lm_phras.o bphrases.o
rpost.exe:     rpost.o both.o rphrases.o bphrases.o
lpost.exe:     lpost.o
mphrases.exe:  mphrases.o bphrases.o rphrases.o tphrases.o phrases.o \
               lm_phras.o

testhost.o:    testhost.c suck.h both.h config.h phrases.h
both.o:        both.c suck.h both.h config.h phrases.h
tphrases.o:    tphrases.c
lmove.o:       lmove.c suck.h config.h both.h phrases.h
rpost.o:       rpost.c suck.h config.h both.h phrases.h
lpost.o:       lpost.c suck.h config.h
mphrases.o:    mphrases.c suck.h config.h

