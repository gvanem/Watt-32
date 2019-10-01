#!/bin/sh

#
# Contributed by Ozkan Sezer <sezeroz@users.sourceforge.net>
# for cross-compiling Watt-32 on Linux. Targets are:
#   djgpp, mingw32, mingw64, cygwin, watcom, or borland
#
# What works:
# - generates suitable target makefile from makefile.all
# - generates the src/build/<target> objects directory
# - inc/sys/<target>.err and src/build/<target>/syserr.c
#   generation for dos targets, i.e. djgpp, watcom and
#   borland, provided that dosbox is installed.
#
# What does not work:
# - generation of inc/sys/<target>.err, e.g. djgpp.err, and
#   src/build/<target>/syserr.c won't happen except for dos
#   targets (djgpp, watcom and borland work: see above).
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
  echo You must export WATT_ROOT, like:   "export WATT_ROOT=$HOME/watt"
  echo and must run this script from within your \$WATT_ROOT/src directory.
  exit 4;
}

bad_usage ()
{
  echo Unknown option \'$1\'.
  echo Usage: $0 [djgpp mingw32 mingw64 cygwin watcom borland all clean]
  exit 2;
}

usage ()
{
  echo Configuring Watt-32 tcp/ip targets.
  echo Usage: $0 [djgpp mingw32 mingw64 cygwin watcom borland all clean]
  exit 1;
}

#
# generate-target functions:
#
gen_djgpp ()
{
  echo Generating DJGPP makefile, directory, errnos and dependencies
  ../util/linux/mkmake -o djgpp.mak -d build/djgpp makefile.all DJGPP FLAT
  ../util/linux/mkdep -s.o -p\$\(OBJDIR\)/ *.[ch] > build/djgpp/watt32.dep

  echo neterr.c: build/djgpp/syserr.c >> build/djgpp/watt32.dep

# requires a dosbox installation: dj_err.exe relies
# on being compiled as a dos/djgpp exe from errnos.c.
cat > ../djerr.bat << EOF
set PATH=c:\\util;%PATH%
c:
dj_err.exe -e > inc\\sys\\djgpp.err
dj_err.exe -s > src\\build\\djgpp\\syserr.c
exit
EOF
  dosbox -noautoexec -c "mount c $WATT_ROOT" -c "c:djerr.bat" -noconsole
  rm ../djerr.bat
  mv ../inc/sys/DJGPP.ERR ../inc/sys/djgpp.err 2> /dev/null
  mv build/djgpp/SYSERR.C build/djgpp/syserr.c 2> /dev/null

  dos2unix ../inc/sys/djgpp.err build/djgpp/syserr.c 2> /dev/null
  dos2unix djgpp.mak 2> /dev/null

  echo Run GNU make to make target:
  echo   make -f djgpp.mak
}

gen_mingw32 ()
{
  echo Generating MinGW32 makefile, directory, errnos and dependencies
  ../util/linux/mkmake -o MinGW32.mak -d build/MinGW32 makefile.all MINGW32 WIN32
  ../util/linux/mkdep -s.o -p\$\(OBJDIR\)/ *.[ch] > build/MinGW32/watt32.dep

  echo neterr.c: build/MinGW32/syserr.c >> build/MinGW32/watt32.dep

# not possible, because errnos.c relies on being compiled as a target-exe:
# ../util/mw_err -s > build/MinGW32/syserr.c
# ../util/mw_err -e > ../inc/sys/mingw32.err
  echo \*\* CANNOT GENERATE mingw32.err and MinGW32/syserr.c \*\*
  echo \*\* MW_ERR.EXE MUST BE RUN UNDER WINDOWS FOR THAT !! \*\*

  echo Run GNU make to make target:
  echo   make -f MinGW32.mak
  make -s -f ../util/pkg-conf.mak mingw32_pkg MINGW32_DIR=../lib
}

gen_mingw64 ()
{
  echo Generating MinGW64-w64 makefile, directory, errnos and dependencies
  ../util/linux/mkmake -o MinGW64_32.mak -d build/MinGW64/32bit makefile.all MINGW64 WIN32
  ../util/linux/mkmake -o MinGW64_64.mak -d build/MinGW64/64bit makefile.all MINGW64 WIN64
  ../util/linux/mkdep -s.o -p\$\(OBJDIR\)/32bit/ *.[ch] > build/MinGW64/32bit/watt32.dep
  ../util/linux/mkdep -s.o -p\$\(OBJDIR\)/64bit/ *.[ch] > build/MinGW64/64bit/watt32.dep

  echo neterr.c: build/MinGW64/syserr.c >> build/MinGW64/32bit/watt32.dep
  echo neterr.c: build/MinGW64/syserr.c >> build/MinGW64/64bit/watt32.dep

# not possible, because errnos.c relies on being compiled as a target-exe:
# ../util/mw64_err -s > build/MinGW64/syserr.c
# ../util/mw64_err -e > ../inc/sys/mingw64.err
  echo \*\* CANNOT GENERATE mingw64.err and MinGW64/syserr.c \*\*
  echo \*\* MW64_ERR.EXE MUST BE RUN UNDER WINDOWS FOR THAT! \*\*

  echo Run GNU make to make target:
  echo   make -f MinGW64_32.mak
  echo or
  echo   make -f MinGW64_64.mak
  make -s -f ../util/pkg-conf.mak mingw64_pkg MINGW64_DIR=../lib
}

gen_cygwin ()
{
  echo Generating CygWin makefile, directory and dependencies
  ../util/linux/mkmake -o CygWin.mak -d build/CygWin/32bit makefile.all CYGWIN WIN32
  ../util/linux/mkdep -s.o -p\$\(OBJDIR\)/ *.[ch] > build/CygWin/watt32.dep

  echo Run GNU make to make target:
  echo   make -f CygWin.mak
  make -s -f ../util/pkg-conf.mak cygwin_pkg CYGWIN_DIR=../lib
}

gen_cygwin64 ()
{
  echo Generating CygWin64 makefile, directory and dependencies
  ../util/linux/mkmake -o CygWin_64.mak -d build/CygWin/64bit makefile.all CYGWIN WIN64
  ../util/linux/mkdep -s.o -p\$\(OBJDIR\)/ *.[ch] > build/CygWin/watt32.dep

  echo Run GNU make to make target:
  echo   make -f CygWin_64.mak
  make -s -f ../util/pkg-conf.mak cygwin64_pkg CYGWIN_DIR=../lib
}

gen_watcom ()
{
  echo Generating Watcom makefiles, directories, errnos and dependencies
  ../util/linux/mkmake -o watcom_s.mak -d build/watcom/small makefile.all WATCOM SMALL
  ../util/linux/mkmake -o watcom_l.mak -d build/watcom/large makefile.all WATCOM LARGE
  ../util/linux/mkmake -o watcom_f.mak -d build/watcom/flat  makefile.all WATCOM FLAT
  ../util/linux/mkmake -o watcom_w.mak -d build/watcom/win32 makefile.all WATCOM WIN32

  ../util/linux/mkdep -s.obj -p\$\(OBJDIR\)/ *.[ch] > build/watcom/watt32.dep

  echo neterr.c: build/watcom/syserr.c >> build/watcom/watt32.dep

# requires a dosbox installation: wc_err.exe relies
# on being compiled as a 16 bit dos exe from errnos.c.
# note: watcom-dos/win32/os2 share the same errno.h,
#       but not watcom-linux *does not*.
cat > ../wcerr.bat << EOF
set PATH=c:\\util;%PATH%
c:
wc_err.exe -e > inc\\sys\\watcom.err
wc_err.exe -s > src\\build\\watcom\\syserr.c
exit
EOF
  dosbox -noautoexec -c "mount c $WATT_ROOT" -c "c:wcerr.bat" -noconsole
  rm ../wcerr.bat
  mv ../inc/sys/WATCOM.ERR ../inc/sys/watcom.err 2> /dev/null
  mv build/watcom/SYSERR.C build/watcom/syserr.c 2> /dev/null

  dos2unix ../inc/sys/watcom.err build/watcom/syserr.c 2> /dev/null
  dos2unix watcom*.mak 2> /dev/null

  echo Run wmake to make target\(s\):
  echo   E.g. "wmake -f watcom_l.mak" for large model
}

gen_borland ()
{
  echo Generating Borland-C makefiles, directories, errnos and dependencies
  ../util/linux/mkmake -o bcc_s.mak -d build/borland/small makefile.all BORLAND SMALL
  ../util/linux/mkmake -o bcc_l.mak -d build/borland/large makefile.all BORLAND LARGE
  ../util/linux/mkmake -o bcc_f.mak -d build/borland/flat  makefile.all BORLAND FLAT
  ../util/linux/mkmake -o bcc_w.mak -d build/borland/win32 makefile.all BORLAND WIN32

  ../util/linux/mkdep -s.obj -p\$\(OBJDIR\)/ *.[ch] > build/borland/watt32.dep

  echo neterr.c: build/borland/syserr.c >> build/borland/watt32.dep

# requires a dosbox installation: bcc_err.exe relies
# on being compiled as a 16 bit dos exe from errnos.c.
cat > ../bcerr.bat << EOF
set PATH=c:\\util;%PATH%
c:
bcc_err.exe -e > inc\\sys\\borlandc.err
bcc_err.exe -s > src\\build\\borland\\syserr.c
exit
EOF
  dosbox -noautoexec -c "mount c $WATT_ROOT" -c "c:bcerr.bat" -noconsole
  rm ../bcerr.bat
  mv ../inc/sys/BORLANDC.ERR ../inc/sys/borlandc.err 2> /dev/null
  mv build/borland/SYSERR.C build/borland/syserr.c 2> /dev/null

  dos2unix ../inc/sys/borlandc.err build/borland/syserr.c 2> /dev/null
  dos2unix bcc*.mak 2> /dev/null

  echo Run Borland or CBuilder make utility to make target\(s\) :
  echo   E.g. "%BCCDIR%\\bin\\make -f bcc_l.mak" for large model
}

gen_all ()
{
  gen_djgpp
  gen_mingw32
  gen_mingw64
  gen_cygwin
  gen_cygwin64
  gen_watcom
  gen_borland
}

do_clean ()
{
  rm -f djgpp.mak watcom*.mak MinGW*.mak CygWin*.mak bcc*.mak
  rm -f build/djgpp/watt32.dep build/MinGW32/watt32.dep build/MinGW64/32bit/watt32.dep build/MinGW64/64bit/watt32.dep
  rm -f build/CygWin/watt32.dep build/watcom/watt32.dep build/borland/watt32.dep
  rm -f build/djgpp/syserr.c build/watcom/syserr.c build/borland/syserr.c build/MinGW32/syserr.c build/MinGW64/syserr.c
  rm -f ../inc/sys/djgpp.err ../inc/sys/watcom.err ../inc/sys/borland.err ../inc/sys/mingw32.err ../inc/sys/mingw64.err
}

#
# Sanity check our pwd
#
test -f makefile.all || { missing_stuff ; }
test -d ../bin || { missing_stuff ; }
test -d ../inc || { missing_stuff ; }
test -d ../lib || { missing_stuff ; }
test -d ../util || { missing_stuff ; }

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
  djgpp|mingw32|mingw64|cygwin|watcom|borland|all|clean)
      ;;
  "-h"|"-?") usage ;;
  *)  bad_usage $1 ;;
esac

#
# Generate host binaries
#
make -C ../util linux || { exit 3 ; }

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
  cygwin)    gen_cygwin   ;;
  cygwin64)  gen_cygwin64 ;;
  watcom)    gen_watcom   ;;
  borland)   gen_borland  ;;
  *)         bad_usage $i;;
 esac
done
