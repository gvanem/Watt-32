#
# A GNU Makefile for _watt32.pyd - A simple Python module
# for Watt-32 library.
#
# Supports MSVC, clang-cl or MinGW (MinGW little tested).
# Needs SWIG and off-course Python 2/3 to build and test.
#
# A 'set SWIG_LIB=<python-root>\Scripts\swig-install\Lib'
# could be needed for Swig to finds it's modules like 'swig.swg' etc.
#
define Usage

  Usage:
    make CC=[cl | clang-cl | gcc] <PYTHON="py -2.x" | "py -3.x"> <WATT32_STATIC=1> [all | clean | vclean | install]
      CC=gcc      - use MinGW
      CC=cl       - use MSVC
      CC=clang-cl - use clang-cl
endef

#
# Using the Python Launcher argument 'PYTHON="py -x.y"', in a 'make py_info'
# invokations, things could look like this:
#
#   %WATT_ROOT%\src\Python> make CC=cl PYTHON="py -2" py_info
#   PY_ROOT:       F:/ProgramFiler/Python27
#   PY_MAJOR_VER:  2
#   PY_MINOR_VER:  7
#   PY_GIL_STATUS: 0
#
#   %WATT_ROOT%\src\Python> make CC=cl PYTHON="py -3.9" py_info
#   PY_ROOT:       F:/ProgramFiler/Python39
#   PY_MAJOR_VER:  3
#   PY_MINOR_VER:  9
#   PY_GIL_STATUS: 0
#
#   %WATT_ROOT%\src\Python> make CC=cl PYTHON="py -3.14t" py_info
#   PY_ROOT:       F:/ProgramFiler/Python314
#   PY_MAJOR_VER:  3
#   PY_MINOR_VER:  14
#   PY_GIL_STATUS: 1
#
# A 'PYTHON="py -3"', reporting Python 3.9 just means that the default
# Python version 3 is '3.9'. Allthough one could have a version 3.6
# installed as non-default.
#

export CL=

#
# The 'CPU' MUST match the bitness of chosen 'PYTHON'.
# E.g. if you run 'make CC=cl PYTHON="py -2"' and your Python 2.x is 32-bit,
# call the 'vcvarsall.bat x86' first.
#
CPU ?= x86

#
# This could invoke 'swig.exe' via a 'swig.bat' by a 'set SWIG=swig.bat'.
#
SWIG ?= swig

ifeq ($(CPU),x86)
  BITS          = 32
  WINDRES_FLAGS = --target=pe-i386

else ifeq ($(CPU),x64)
  BITS          = 64
  WINDRES_FLAGS = --target=pe-x86-64
else
  $(error CPU must be 'x86' or 'x64')
endif

#
# The only option at the moment:
#
WATT32_STATIC ?= 1

#
# Do e.g. a 'make CC=clang-cl cpp_swig' to inspect the preprocessed Swig code.
#
# Add support for BSD sockets?
#
USE_SOCKET_API ?= 1

#
# Enable Py 3.13+ no-GIL (or free-threading) mode.
# Ref: https://docs.python.org/3/howto/free-threading-python.html
#
USE_NO_GIL ?= 0

#
# Could be set in environment or on the cmd-line:
#
PYTHON ?= py -3

PY_ROOT       = $(realpath  $(shell $(PYTHON) -c "import sys; print (sys.prefix)"))
PY_MAJOR_VER  = $(shell $(PYTHON) -c "import sys; print(sys.version_info.major)" )
PY_MINOR_VER  = $(shell $(PYTHON) -c "import sys; print(sys.version_info.minor)" )
PY_MAXSIZE    = $(shell $(PYTHON) -c "import sys; print(sys.maxsize)" )
PY_GIL_STATUS = $(shell $(PYTHON) -c "import sysconfig; print(sysconfig.get_config_var('Py_GIL_DISABLED'))" )

#
# For 'make cpp_swig'
#
DEBUG_MACROS ?= 0
CLANG_CHECK  ?= 0
MSVC_CHECK   ?= 0

ifeq ($(CC),cl)
  OBJ_DIR = objects/MSVC
  RCFLAGS = -D_MSC_VER
  MSVC_CHECK  = 1

else ifeq ($(CC),clang-cl)
  OBJ_DIR = objects/clang
  RCFLAGS = -D__clang__
  CLANG_CHECK = 1

else ifeq ($(CC),gcc)
  OBJ_DIR = objects/gcc
  CFLAGS  =  -m$(BITS) -Wall -O2 -s
  RCFLAGS = -O COFF -D__MINGW32__ $(WINDRES_FLAGS)
  LDFLAGS = -m$(BITS) -s -shared -static-libgcc

else
  $(error $(Usage))
endif

ifneq ($(CC),gcc)
  RCFLAGS += -nologo
  CFLAGS   = -nologo -MD -Ox -Zi -W3 -Fd./$(OBJ_DIR)/vc14x.pdb
  LDFLAGS  = -nologo -debug -dll -verbose -incremental:no -nodefaultlib:uuid.lib -libpath:$(PY_ROOT)/libs
endif

CFLAGS += $(intcmp $(USE_NO_GIL),1,,-DPy_GIL_DISABLED,)

T = $(intcmp $(USE_NO_GIL),1,,t,)

# $(info T=$(T))

#
# This should work for a MinGW 'gcc' too.
#
PY_LIB = $(word 1, $(wildcard $(PY_ROOT)/libs/python$(PY_MAJOR_VER)*$(T).lib))

RCFLAGS += -DRC_BITS=$(BITS) \
           -I../../inc       \
           -I$(PY_ROOT)/include

CFLAGS += -I.. -I../../inc     \
          -I$(PY_ROOT)/include \
          -DPy_ENABLE_SHARED   \
          -DSWIG               \
        # -DSWIGRUNTIME_DEBUG  \
        # -DSWIG_LINK_RUNTIME

#
# Without a '-DSWIG_PYTHON_SILENT_MEMLEAK', there is this exception:
#   TypeError: in method '_ping', argument 2 of type 'DWORD'
#   swig/python detected a memory leak of type 'DWORD *', no destructor found.
#
CFLAGS += -DSWIG_PYTHON_SILENT_MEMLEAK

ifeq ($(USE_SOCKET_API),1)
  CFLAGS += -DUSE_SOCKET_API
endif

ifneq ($(CC),cl)
  CFLAGS += -Wno-unused-function \
            -Wno-visibility
endif

ifeq ($(WATT32_STATIC),1)
  CFLAGS += -DWATT32_STATIC

  ifeq ($(CC),cl)
    W32_LIB = ../../lib/$(CPU)/wattcpvc.lib
  else ifeq ($(CC),clang-cl)
    W32_LIB = ../../lib/$(CPU)/wattcp_clang.lib
  else
    W32_LIB = ../../lib/$(CPU)/libwatt32.a
  endif

  ifeq ($(CC),gcc)
    OS_LIBS = -luser32 -ladvapi32
  else
    OS_LIBS = user32.lib advapi32.lib
  endif

else
  CFLAGS  += -DWATT32_BUILD
  OS_LIBS =

  ifeq ($(CC),cl)
    W32_LIB = ../../lib/$(CPU)/wattcpvc_imp.lib
  else ifeq ($(CC),clang-cl)
    W32_LIB = ../../lib/$(CPU)/wattcp_clang_imp.lib
  else
    W32_LIB = ../../lib/$(CPU)/libwatt32.dll.a
  endif
endif

SWIG_FLAGS = -macroerrors -python $(filter -D% -I%, $(CFLAGS))
SWIG_FLAGS += -w451 -w454
SWIG_FLAGS += # -nodefaultctor -nodefaultdtor -debug-symbols -debug-csymbols

all: py_info py_check_1 py_check_2 _watt32.pyd epilogue

epilogue:
	$(call green_msg, Do a $(BRIGHT_WHITE)$(PYTHON) ./runme.py -d www.google.com $(BRIGHT_GREEN)to test it)
	$(call green_msg, Do a $(BRIGHT_WHITE)make CC=$(CC) install $(BRIGHT_GREEN)to install it)

py_info:
	$(call green_msg, Detected Python variables:)
	$(call green_msg0,  PY_ROOT:       $(BRIGHT_WHITE)$(PY_ROOT))
	$(call green_msg0,  PY_MAJOR_VER:  $(BRIGHT_WHITE)$(PY_MAJOR_VER))
	$(call green_msg0,  PY_MINOR_VER:  $(BRIGHT_WHITE)$(PY_MINOR_VER))
	$(call green_msg0,  PY_MAXSIZE:    $(BRIGHT_WHITE)$(PY_MAXSIZE))
	$(call green_msg0,  PY_GIL_STATUS: $(BRIGHT_WHITE)$(PY_GIL_STATUS))

py_check_1:
ifeq ($(PY_LIB),)
	@echo "A suiteable 'PY_LIB' was not found" ; exit 1
else
	$(call green_msg0,  PY_LIB:        $(BRIGHT_WHITE)$(PY_LIB))
endif

py_check_2:
ifeq ($(PY_MAXSIZE)-$(CPU),2147483647-x64)
	@echo "This python not suiteable for 'CPU=$(CPU)'" ; exit 1
else
	$(call green_msg0,  PY_MAXSIZE and CPU okay)
endif

_watt32.pyd: $(OBJ_DIR)/watt32_swig_wrap.o $(OBJ_DIR)/_watt32.res $(W32_LIB) $(PY_LIB)
	$(call link_PYD, $@, $^ $(OS_LIBS))

watt32.py: watt32_swig_wrap.c

watt32_swig_wrap.c: watt32.swg
	$(call green_msg, Generating Swig wrapper $(BRIGHT_WHITE)$@)
	$(SWIG) $(SWIG_FLAGS) -o $@ watt32.swg
	@echo

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(call C_compile, $@, $<)

$(OBJ_DIR)/_watt32.res: _watt32.rc | $(OBJ_DIR)
	$(call RC_compile, $@, $<)

$(OBJ_DIR):
	mkdir --parents $@

clean:
	rm -f link.tmp *.pdb *.pyc \
	      _watt32.{exp,lib,pyc} watt32.{py,pyo} watt32_swig_wrap.{c,i}
	rm -fr $(OBJ_DIR) __pycache__
	- @rmdir objects 2> /dev/null

vclean realclean: clean
	rm -f _watt32.pyd

install: _watt32.pyd watt32.py
	cp --update watt32.py    $(PY_ROOT)/Lib/site-packages
	cp --update _watt32.pyd  $(PY_ROOT)/DLLs
ifneq ($(CC),gcc)
	cp --update _watt32.pdb  $(PY_ROOT)/DLLs
endif

uninstall:
	rm -f $(PY_ROOT)/dlls/_watt32.{pyd,pdb}
	rm -f $(PY_ROOT)/Lib/site-packages/watt32.py

CPP_FILTER_ARGS = DEBUG_MACROS=$(DEBUG_MACROS)           \
                  CLANG_CHECK=$(CLANG_CHECK)             \
                  MSVC_CHECK=$(MSVC_CHECK)               \
                  REMOVE_DIRECTIVES=0                    \
                  CPP_FILTER_PY=$(OBJ_DIR)/cpp-filter.py \
                  EXTRA_CFLAGS='$(CFLAGS)'

cpp_swig: watt32_swig_wrap.c FORCE
	$(call green_msg, Generating $(BRIGHT_WHITE)watt32_swig_wrap.i $(BRIGHT_GREEN)with no preprocess filters\n)
	@rm -f $(OBJ_DIR)/cpp-filter.py
	$(MAKE) -f ../cpp_filter.mak $(CPP_FILTER_ARGS) watt32_swig_wrap.i

FORCE:

#
# The dependency must be updated by hand.
#
watt32_swig_wrap_DEPS =  \
  ../../inc/sys/w32api.h \
  ../wattcp.h            \
  ../sock_ini.h          \
  ../pcdbug.h            \
  ../pcicmp.h            \
  ../pcdns.h             \
  ../pctcp.h             \
  ../misc.h              \
  ../misc.c

ifeq ($(USE_SOCKET_API),1)
  watt32_swig_wrap_DEPS += ../socket.h ../socket.c
endif

watt32_swig_wrap.c: $(watt32_swig_wrap_DEPS)

#
# GNU-make macros:
#
# This assumes you have a MSys/Cygwin's 'echo.exe' program with ANSI colour support.
#
BRIGHT_GREEN = \e[1;32m
BRIGHT_WHITE = \e[1;37m

green_msg  = @echo -e "$(BRIGHT_GREEN)$(strip $(1))\e[0m"
green_msg0 = @echo -e "$(BRIGHT_GREEN)$(1)\e[0m"

ifeq ($(CC),gcc)
  define C_compile
    $(CC) -c $(CFLAGS) -o $(strip $(1) $(2))
    @echo
  endef

  define RC_compile
    windres -o $(1) $(RCFLAGS) $(2)
    @echo
  endef

  define link_PYD
    $(call green_msg, Linking $(BRIGHT_WHITE)$(strip $(1)))
    $(CC) -o $(1) $(LDFLAGS) $(2)
    @echo
  endef

else
  define C_compile
    $(CC) -c $(CFLAGS) -Fo$(strip $(1) $(2))
    @echo
  endef

  define RC_compile
    rc -Fo $(1) $(RCFLAGS) $(2)
    @echo
  endef

  define link_PYD
    $(file  > $(OBJ_DIR)/check-for-unused-libs.py,if 1:)
    $(file >> $(OBJ_DIR)/check-for-unused-libs.py,$(CHECK_FOR_UNUSED_LIBS_PY))
    $(call green_msg, Linking $(BRIGHT_WHITE)$(strip $(1)))
    link -out:$(strip $(1)) $(LDFLAGS) $(2) > link.tmp
    @rm -f $(1:.pyd=.exp) $(1:.pyd=.lib)
    @$(PYTHON) $(OBJ_DIR)/check-for-unused-libs.py link.tmp
  endef
endif

define CHECK_FOR_UNUSED_LIBS_PY
  import os, sys

  link_tmp    = sys.argv[1]
  ignore_libs = [ ]

  class State():
    IDLE   = 0
    UNUSED = 1

  class Color():
    RED = WHITE = RESET = ""

  try:
    from colorama import init, Fore, Style
    init()
    Color.RED   = Fore.RED + Style.BRIGHT
    Color.WHITE = Fore.WHITE + Style.BRIGHT
    Color.RESET = Style.RESET_ALL
  except:
    pass

  def process_map (state):
    unused_libs = []
    f = open (link_tmp, "rt")
    lines = f.readlines()
    f.close()
    for l in lines:
      l = l.strip()
      if l == "Unused libraries:":
         state = State.UNUSED
         continue
      if state == State.UNUSED:
         if l == "":
            break
         if os.path.basename (l).lower() not in ignore_libs:
            unused_libs.append (l)
    return unused_libs

  unused = process_map (State.IDLE)
  num = len(unused)
  if num > 0:
     print ("%s%d unused librar%s in %s:%s" % (Color.RED, num, [ "y", "ies" ][num > 1], link_tmp, Color.RESET))
     for u in sorted(unused):
         print ("  " + u)
  print ("%sDone%s\n" % (Color.WHITE, Color.RESET))
endef

