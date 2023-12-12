#
#  GNU make makefile for the MkMake/MkDep/MkLang/MkConf/MkImp/bin2c
#  utilities used during Watt-32 development
#
#  DOS versions:   Requires djgpp2 + SLang 1.3+ library installed.
#  Win32 versions: Requires MinGW + Slang 1.3+ installed.
#  Linux versions: Requires gcc + Slang 1.3+ installed.
#
#  Setup notes:
#    Review and setup SLANG_ROOT_.... to point to location
#    where your Slang version is installed
#

.SUFFIXES: .exe .l .y

#
# Change to suite:
#
USE_MSVC         ?= 1
SLANG_ROOT_DOS   ?= $(DJDIR)/contrib/slang.210
SLANG_LIB_DOS    ?= $(SLANG_ROOT_DOS)/src/djgobjs/libslang.a

SLANG_ROOT_WIN   ?= $(realpath $(MINGW32))/src/TUI/Slang
SLANG_LIB_WIN_CL ?= $(SLANG_ROOT_WIN)/src/wslang-$(CPU).lib

SLANG_CFLAGS = $(shell pkg-config --cflags slang)
SLANG_LIBS   = $(shell pkg-config --libs slang)

CC     = gcc
CFLAGS = -Os -Wall -g0 -save-temps # -s

UTILS := mkmake mkdep mklang mkconf bin2c

DJGPP_UTILS := $(addsuffix .exe,$(UTILS)) mkimp.exe dxegen.exe
WIN32_UTILS := $(addprefix win32/,$(addsuffix .exe,$(UTILS)))
LINUX_UTILS := $(addprefix linux/,$(UTILS))

all:   $(DJGPP_UTILS)

win32: $(WIN32_UTILS)

linux: $(LINUX_UTILS)

#
# DOS/djgpp binaries:
#
mkmake.exe: mkmake.c
	$(CC) $(CFLAGS) -I$(SLANG_ROOT_DOS)/src -o $@ $^ $(SLANG_LIB_DOS)

%.exe: %.c
	$(CC) $(CFLAGS) -o $@ $^

#
# Win32 binaries:
#
CL_CFLAGS  = -nologo -MD -Zi -DWATT32_STATIC -I../inc
CL_LDFLAGS = "-link -incremental:no -debug:none -ignore:4099"

ifeq ($(USE_MSVC),1)
  win32/mkmake.exe: mkmake.c
	cl $(CL_CFLAGS) -Fe./$@ -I$(SLANG_ROOT_WIN)/src $(SLANG_LIB_WIN_CL) $< $(CL_LDFLAGS)

  win32/%.exe: %.c
	cl $(CL_CFLAGS) -Fe./$@ $< $(CL_LDFLAGS)

else
  win32/mkmake.exe: mkmake.c
	$(CC) -m32 $(CFLAGS) $(SLANG_CFLAGS) -o $@ $< $(SLANG_LIBS)

  win32/%.exe: %.c
	$(CC) -m32 $(CFLAGS) -o $@ $<
endif

#
# Linux binaries:
#
linux/mkmake: mkmake.c
	$(CC) $(CFLAGS) $(SLANG_CFLAGS) -o $@ $< $(SLANG_LIBS)

linux/%: %.c
	$(CC) $(CFLAGS) -o $@ $^


mkimp.c: mkimp.l
	flex -o$@ $^

clean:
	@rm -f $(DJGPP_UTILS)          \
               $(WIN32_UTILS)          \
               $(LINUX_UTILS)          \
               *.o win32/*.o linux/*.o \
               *.s win32/*.s linux/*.s \
               *.i win32/*.i linux/*.i \
               mkimp.c mkimp_gr.c

