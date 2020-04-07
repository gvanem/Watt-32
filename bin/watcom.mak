#
#  Makefile for Waterloo TCP sample applications
#
#  Watcom-16 real-mode (large/small) executables.
#
#  Usage: 'wmake -h -f watcom.mak'
#

sufix_small = s
sufix_large = l

MODEL   = large		# small or large

CC      = *wcc -0
CFLAGS  = -bt=dos -m$(sufix_$(MODEL)) -os -zc -DWATT32_STATIC  #-dMAKE_TSR
LFLAGS  = system dos option stack=15k
LIBRARY = library ../lib/wattcpw$(sufix_$(MODEL)).lib

BUILD_MESSAGE = Watcom/real-mode (model = $(MODEL)) binaries done

!include wccommon.mak
