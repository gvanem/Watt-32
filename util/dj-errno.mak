# Makefile for djgpp dj_err.exe
#
# Targets: dj_err.exe
#
# To build under plain MSDOS:
#	make -f dj-errno.mak
#
# To build using a cross-compiler under Linux or Windows:
#	make -f dj-errno.mak CROSS=1
#

ifneq ($(CROSS),)
DJGCC=i586-pc-msdosdjgpp-gcc
else
DJGCC=gcc
endif

all: dj_err.exe

dj_err.exe: errnos.c
	$(DJGCC) -Wall -W -s -I../inc -o dj_err.exe errnos.c
