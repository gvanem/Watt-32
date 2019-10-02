#!/bin/sh

# WATCOM environment variable must be set.
# $WATCOM/binl must be on the PATH.

wcc386 -bt=linux -bm -fp5 -fpi87 -mf -oeatxh -w4 -ei -zp8 -j -zq -I$WATCOM/lh -I. -fo=slcommon.o slcommon.c
wcc386 -bt=linux -bm -fp5 -fpi87 -mf -oeatxh -w4 -ei -zp8 -j -zq -I$WATCOM/lh -I. -fo=slprepr.o slprepr.c
wcc386 -bt=linux -bm -fp5 -fpi87 -mf -oeatxh -w4 -ei -zp8 -j -zq -I$WATCOM/lh -I. -fo=slstring.o slstring.c
wlib -q -b -n -c -pa -s -t -zld -ii -io slang.lib slcommon.o slprepr.o slstring.o

wcc386 -bt=linux -bm -fp5 -fpi87 -mf -oeatxh -w4 -ei -zp8 -j -zq -I$WATCOM/lh -I. -I.. -fo=mkmake.o ../mkmake.c
wlink N mkmake SYS linux OP q LIBR slang.lib F mkmake.o

#rm -f *.o *.lib
