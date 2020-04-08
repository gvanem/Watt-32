#!/bin/sh

#
# Contributed by Ozkan Sezer <sezeroz@users.sourceforge.net>
# for cross-compiling Watt-32 on Linux. Targets are:
#   djgpp, mingw32, mingw64, cygwin, clang or watcom
#
# What works:
# - generates suitable target makefile from makefile.all
# - generates the src/build/<target> objects directory
#
# What does not work:
# - generation of inc/sys/<target>.err, e.g. djgpp.err, and
#   src/build/<target>/syserr.c won't happen:
#   util/errnos.c relies on being compiled as a target-exe:
#   it relies on the sys_nerr value from the target libc and
#   the strerror() string returned from target's libc.  none
#   of these can be accomplished by a simple cross-build.
#

#
# target triplet for cross-djgpp toolchain:
#
DJGPP_PREFIX="i586-pc-msdosdjgpp"

#
# target triplet for cross-win64 toolchain, MinGW-w64:
#
MINGW64_PREFIX="x86_64-w64-mingw32"

#
# target triplet for cross-win32 toolchain, MinGW-xxx
# for MinGW x86 targets, one can use either MinGW-w64:
#MINGW_PREFIX="i686-w64-mingw32"
# ... or the plain old MinGW.org:
#
MINGW_PREFIX="i686-pc-mingw32"

#
# target triplet for cross-cygwin toolchain:
#
CYGWIN_PREFIX="i686-pc-cygwin"

#
# OpenWatcom linux distributions have no prefixing.
#
WATCOM_PREFIX=

#
# error-out functions:
#
missing_stuff ()
{
  echo "You must export WATT_ROOT, like:   'export WATT_ROOT=\$HOME/watt'"
  echo "and must run this script from within your \$WATT_ROOT/src directory."
  exit 4;
}

bad_usage ()
{
  echo "Unknown option '$1'."
  echo "Usage: $0 [djgpp mingw32 mingw64 cygwin clang watcom all clean]"
  exit 2;
}

usage ()
{
  echo "Configuring Watt-32 tcp/ip targets."
  echo "Usage: $0 [djgpp mingw32 mingw64 cygwin clang watcom all clean]"
  exit 1;
}

#
# generate-target functions:
#
gen_djgpp ()
{
  echo "Generating DJGPP makefile"
  ../../util/linux/mkmake -o djgpp.mak makefile.all DJGPP FLAT IS_GCC

  echo "Run GNU make to make target:"
  echo "  'make -f djgpp.mak'"
}

gen_mingw32 ()
{
  echo "Generating MinGW32 makefile"
  ../../util/linux/mkmake -o MinGW32.mak makefile.all MINGW32 WIN32 IS_GCC

  echo "Run GNU make to make target:"
  echo "  'make -f MinGW32.mak'"
}

gen_mingw64 ()
{
  echo "Generating MinGW64-w64 makefiles"
  ../../util/linux/mkmake -o MinGW64_32.mak makefile.all MINGW64 WIN32 IS_GCC
  ../../util/linux/mkmake -o MinGW64_64.mak makefile.all MINGW64 WIN64 IS_GCC

  echo "Run GNU make to make target(s):"
  echo "  'make -f MinGW64_32.mak'"
  echo "  'make -f MinGW64_64.mak'"
}

gen_cygwin32 ()
{
  echo "Generating CygWin32 makefile"
  ../../util/linux/mkmake -o CygWin.mak makefile.all CYGWIN WIN32 IS_GCC

  echo "Run GNU make to make target:"
  echo "  'make -f CygWin.mak'"
}

gen_cygwin64 ()
{
  echo "Generating CygWin64 makefile"
  ../../util/linux/mkmake -o CygWin_64.mak  makefile.all CYGWIN64 WIN64 IS_GCC

  echo "Run GNU make to make target:"
  echo "  'make -f CygWin_64.mak'"
}

#
# Highly experimental. I do not have Linux (or WSL for Win-10).
# So it's completely untested.
#
gen_clang ()
{
  echo "Generating clang-cl (Win32/Win64, release/debug) makefiles, directories, errnos and dependencies"
  ../../util/linux/mkmake -o clang_32.mak makefile.all CLANG WIN32
  ../../util/linux/mkmake -o clang_64.mak makefile.all CLANG WIN64

  echo "Run GNU make to make target(s):"
  echo "  'make -f clang_32.mak'"
  echo "  'make -f clang_64.mak'"
  echo "Depending on which clang-cl (32 or 64-bit) is first on your PATH, use the correct 'clang_32.mak' or 'clang_64.mak'."
}


gen_watcom ()
{
  echo "Generating Watcom makefiles"
  ../../util/linux/mkmake -o watcom_s.mak makefile.all WATCOM SMALL
  ../../util/linux/mkmake -o watcom_l.mak makefile.all WATCOM LARGE
  ../../util/linux/mkmake -o watcom_3.mak makefile.all WATCOM SMALL32
  ../../util/linux/mkmake -o watcom_f.mak makefile.all WATCOM FLAT
#  ../../util/linux/mkmake -o watcom_x.mak makefile.all WATCOM FLAT X32VM
  ../../util/linux/mkmake -o watcom_w.mak makefile.all WATCOM WIN32

  echo "Run wmake to make target(s):"
  echo "  'make -f watcom_s.mak' for small model (16-bit)"
  echo "  'make -f watcom_l.mak' for large model (16-bit)"
  echo "  'make -f watcom_3.mak' for small model (32-bit)"
  echo "  'make -f watcom_f.mak' for flat model"
#  echo "  'make -f watcom_x.mak' for flat/X32VM model"
  echo "  'make -f watcom_w.mak' for Win32"

gen_all ()
{
  gen_djgpp
  gen_mingw32
  gen_mingw64
  gen_cygwin32
  gen_cygwin64
  gen_watcom
}

do_clean ()
{
  rm -f djgpp.mak
  rm -f watcom_{f,l,s,w,x,3}.mak
  rm -f MinGW32.mak MinGW64.mak
  rm -f CygWin.mak CygWin_64.mak
  rm -f clang_{32,64}.mak
}

#
# Sanity check our pwd
#
test -f makefile.all || { missing_stuff ; }

#
# Make sure WATT_ROOT is set
#
if test "x$WATT_ROOT" = "x"; then
  missing_stuff
fi

#
# Check cmdline args
#
if test $# -lt 1; then
  usage
fi
case $1 in
  djgpp|mingw32|mingw64|cygwin|clang|watcom|all|clean)
      ;;
  "-h"|"-?") usage ;;
  *)  bad_usage $1 ;;
esac

#
# Process the cmdline args
#
for i in "$@"
do
 case $i in
  all)       gen_all      ;;
  clean)     do_clean     ;;
  djgpp)     gen_djgpp    ;;
  mingw32)   gen_mingw32  ;;
  mingw64)   gen_mingw64  ;;
  cygwin32)  gen_cygwin32 ;;
  cygwin64)  gen_cygwin64 ;;
  clang)     gen_clang    ;;
  watcom)    gen_watcom   ;;
  *)         bad_usage $i;;
 esac
done
