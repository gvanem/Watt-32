#!/bin/sh

# mingw32 cross-compiler must be on the PATH
# e.g.:  /usr/local/cross-win32/bin

#TOOLCHAIN_PREFIX=i686-w64-mingw32-
TOOLCHAIN_PREFIX=i686-pc-mingw32-

${TOOLCHAIN_PREFIX}gcc -I. -O2 -Wall -W -c slcommon.c -o slcommon.o
${TOOLCHAIN_PREFIX}gcc -I. -O2 -Wall -W -c slprepr.c -o slprepr.o
${TOOLCHAIN_PREFIX}gcc -I. -O2 -Wall -W -c slstring.c -o slstring.o
${TOOLCHAIN_PREFIX}ar rs libslang.a slprepr.o slstring.o slcommon.o

${TOOLCHAIN_PREFIX}gcc -I. -I.. -O2 -Wall -c ../mkmake.c -o mkmake.o
${TOOLCHAIN_PREFIX}gcc -o mkmake.exe mkmake.o -L. -lslang
${TOOLCHAIN_PREFIX}strip mkmake.exe

#rm -f *.o *.a
