#!/bin/sh

gcc -I. -O2 -Wall -W -DHAVE_STDLIB_H -c slcommon.c -o slcommon.o
gcc -I. -O2 -Wall -W -DHAVE_STDLIB_H -c slprepr.c -o slprepr.o
gcc -I. -O2 -Wall -W -DHAVE_STDLIB_H -c slstring.c -o slstring.o
ar rs libslang.a slprepr.o slstring.o slcommon.o

gcc -I. -I.. -O2 -Wall -c ../mkmake.c -o mkmake.o
gcc -o mkmake mkmake.o -L. -lslang
strip mkmake
#rm -f *.o *.a
