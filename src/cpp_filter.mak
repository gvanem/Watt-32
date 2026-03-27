#
# A GNU-makefile to output a C preprocessed file of
# any .c-file in ./src or ./src/test.
#
# Example usage:
#   To C-preprocess using MSVC compiler, use:
#     make -f cpp_filter.mak MSVC_CHECK=1 socket.i
#
#   To C-preprocess using PelleC compiler, use:
#     make -f cpp_filter.mak POCC_CHECK=1 socket.i
#
#   To C-preprocess using djgpp/gcc compiler, use:
#     make -f cpp_filter.mak DJGPP_CHECK=1 socket.i
#
#   To C-preprocess using your default gcc compiler (1st on PATH), use:
#     make -f cpp_filter.mak socket.i
#
# This will produce socket.i from socket.c which you can inspect to see
# what the C-compiler is really given to compile.
#
# Requires Python 2+ and optionally Astyle or clang-format.
#
USE_ASTYLE       ?= 1
USE_CLANG_FORMAT ?= 0
MSVC_CHECK       ?= 0
CLANG_CHECK      ?= 0
POCC_CHECK       ?= 0
DJGPP_CHECK      ?= 0
CYGWIN_CHECK     ?= 0
PYTHON           ?= py -3

#
# For 'gcc' + 'clang-cl'; print built-in and macro values.
#
# Refs:
#   https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html
#   https://clang.llvm.org/docs/UsersManual.html
#
DEBUG_MACROS ?= 0

#
# Remove all '#pragma' and 'line' directives like:
#  #line 14 "f:/gv/WinKit/Include/10.0.22621.0/ucrt/sys/types.h" 3 for MSVC or
#  # 14 "f:/gv/WinKit/Include/10.0.22621.0/ucrt/sys/types.h" 3 for gcc / clang-cl
#
REMOVE_DIRECTIVES ?= 0

WATT_ROOT := $(realpath $(WATT_ROOT))

THIS_FILE      = $(firstword $(MAKEFILE_LIST))
CPP_FILTER_PY ?= $(dir $(THIS_FILE))cpp-filter.py

ifeq ($(DJGPP_CHECK),1)
  ifeq ($(OS),Windows_NT)
    #
    # Windows hosted djgpp cross compiler. Get it from:
    #   https://github.com/andrewwutw/build-djgpp/releases
    #
    # Define an env-var 'DJ_PREFIX=f:/gv/djgpp/bin/i586-pc-msdosdjgpp'
    # Thus the full path to 'gcc' becomes:
    #   $(DJ_PREFIX)-gcc.exe
    #
    ifeq ($(DJ_PREFIX),)
      $(error Define a $(DJ_PREFIX) to point to the ROOT of the djgpp cross compiler.)
    endif

    ifeq ($(wildcard $(DJ_PREFIX)gcc.exe),)
      $(error Failed to find 'i586-pc-msdosdjgpp-gcc.exe'.)
    endif

    CC = $(DJ_PREFIX)gcc
  else
    CC = gcc
  endif

  CFLAGS = -O2 -Wall

else ifeq ($(MSVC_CHECK),1)
  CC     = cl
  CFLAGS = -nologo -W3 -D_WIN32_WINNT=0x0601

else ifeq ($(CLANG_CHECK),1)
  CC     = clang-cl
  CFLAGS = -nologo -W3 -D_WIN32_WINNT=0x0601
  export CL=

else ifeq ($(POCC_CHECK),1)
  CC     = pocc
  CFLAGS = -Go -X -Tx86-coff -D_M_IX86=1 -W2 \
           -I$(realpath $(PELLESC)\Include) -I$(realpath $(PELLESC)\Include\win)

else  # MinGW, CygWin
  CC     = gcc
  CFLAGS = -O2 -Wall -D_WIN32_WINNT=0x0601

  ifeq ($(CYGWIN_CHECK),1)
    CFLAGS += -DWIN32 -D_WIN32
  endif
endif

CFLAGS += -DWATT32_BUILD   \
          -DW32_NAMESPACE= \
        # -DWATT32_NO_NAMESPACE

CFLAGS += -I$(WATT_ROOT)/inc \
          -I$(WATT_ROOT)/src \
          -I.

ifeq ($(DEBUG_MACROS),1)
  ifeq ($(CC),gcc)
    CFLAGS += -dD
  else ifeq ($(CC),cl)
    CFLAGS += -PD -Zc:preprocessor
  else ifeq ($(CC),clang-cl)
    CFLAGS += -d1PP
  endif
endif

EXTRA_CFLAGS ?=

PREPROCESS_C   = $(CC) -E $(EXTRA_CFLAGS) $(CFLAGS) $(1) | $(PYTHON) $(CPP_FILTER_PY)
PREPROCESS_CPP = $(CC) -E $(EXTRA_CFLAGS) $(CFLAGS) $(1) | $(PYTHON) $(CPP_FILTER_PY)

ifeq ($(USE_CLANG_FORMAT),1)
  PREPROCESS_C   += | clang-format -style=Mozilla -assume-filename=c
  PREPROCESS_CPP += | clang-format -style=Mozilla -assume-filename=c++

else ifeq ($(USE_ASTYLE),1)
  PREPROCESS_C   += | astyle
  PREPROCESS_CPP += | astyle
endif

all: $(CPP_FILTER_PY) $(MAKECMDGOALS)

%.i: %.c FORCE $(CPP_FILTER_PY)
	$(call PREPROCESS_C, $<) > $@
	@echo ''
	@echo 'Look at "$@" for the preprosessed results.'

%.i: %.cpp FORCE $(CPP_FILTER_PY)
	$(call PREPROCESS_CPP, $<) > $@
	@echo ''
	@echo 'Look at "$@" for the preprosessed results.'

test:
	@echo 'I am $$(THIS_FILE):     "$(THIS_FILE)".'
	@echo 'I am $$(CPP_FILTER_PY): "$(CPP_FILTER_PY)".'
	@echo '$$(CURDIR):             "$(CURDIR)".'
	@echo 'Goals $$(MAKECMDGOALS): "$(MAKECMDGOALS)".'

FORCE:

#
# Create 'cpp-filter.py' in the directory of $(THIS_FILE)
#
$(CPP_FILTER_PY): $(THIS_FILE)
	@echo 'Generating $@...'
	$(file >  $@,#!/usr/env/python)
	$(file >> $@,# DO NOT EDIT! This file ($@) was generated automatically)
	$(file >> $@,# from $(realpath $(THIS_FILE)). Edit that file instead.)
	$(file >> $@,#)
	$(file >> $@,from __future__ import print_function)
	$(file >> $@,if 1:)
	$(file >> $@,$(_CPP_FILTER_PY))

define _CPP_FILTER_PY
  import sys, os

  empty_lines = debug_lines = removed_directives = 0
  while True:
    line = sys.stdin.readline()
    if not line:
       break
    line = line.rstrip()
    if line == "":
       empty_lines += 1
       continue

    #
    # MSVC or clang-cl 'line' directive
    #
    l = line.lstrip()
    if l.startswith("#line") or l.startswith("# "):
       if $(REMOVE_DIRECTIVES):
          removed_directives += 1
          continue

       line = line.replace ("\\\\", "/")

    #
    # Remove '#pragma' lines?
    #
    if $(REMOVE_DIRECTIVES) and l.startswith("#pragma"):
       removed_directives += 1
       continue

    if l.startswith("#define "):
       debug_lines += 1

    print (line)

    #
    # Print a newline after a functions or structs
    #
    if l == "}" or l == "};":
       print ("")

  print ("Removed %d empty lines." % empty_lines, file=sys.stderr)
  if $(REMOVE_DIRECTIVES):
     print ("Removed %d line/pragma directives." % removed_directives, file=sys.stderr)
  if $(DEBUG_MACROS):
     print ("Found %d '#define' lines." % debug_lines, file=sys.stderr)
endef



