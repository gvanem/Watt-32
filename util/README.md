MKMAKE
======

Due to various incompatibilities between various make programs for PC
systems as well as differing notions of what the path separator should
be, I have decided that the best and most maintainable approach is to
create a master makefile that that consists of system/compiler/make
sections from which the approriate makefile is constructed.  The
easiest way to do this is to run the master makefile through a
preprocessor.  To this end, I have created a master makefile for
DOS/OS2 systems called `makefile.all`.  This makefile is processed by
the executable `mkmake.exe` or `win32/mkmake.exe` to produce various makefiles.

For example:
```
    mkmake BCC < makefile.all > makefile
```

produces a makefile suitable for BCC, whereas:
```
    mkmake OS2EMX < makefile.all > makefile
```

produces a makefile for OS2 assuming that EMX is the compiler.

Use:
```
    mkmake < makefile.all | more
```

for more information.  (Better yet, look at `makefile.all`; it is best
viewed with a folding editor).

John E. Davis

mkmake modified by G.Vanem 1998

