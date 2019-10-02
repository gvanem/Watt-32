#!/bin/sh

# djgpp cross-compiler must be on the PATH
# e.g.: /usr/local/cross-djgpp/bin

i586-pc-msdosdjgpp-gcc -I. -O2 -Wall -W -c slcommon.c -o slcommon.o
i586-pc-msdosdjgpp-gcc -I. -O2 -Wall -W -c slprepr.c -o slprepr.o
i586-pc-msdosdjgpp-gcc -I. -O2 -Wall -W -c slstring.c -o slstring.o
i586-pc-msdosdjgpp-ar rs libslang.a slcommon.o slprepr.o slstring.o

i586-pc-msdosdjgpp-gcc -I. -O2 -Wall -c ../mkmake.c -o mkmake.o
i586-pc-msdosdjgpp-gcc -o mkmake.exe mkmake.o -L. -lslang
i586-pc-msdosdjgpp-strip mkmake.exe

#rm -f *.o *.a
