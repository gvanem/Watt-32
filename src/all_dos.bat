@echo off
if not %1. == clean. call configur.bat all
if %1. == clean.     call configur.bat clean

echo ---- running djgpp.mak %1 ---- > all_dos.log
make -f djgpp.mak %1 >&> all_dos.log

echo ---- running highc.mak %1 ---- >> all_dos.log
make -f highc.mak %1 >> all_dos.log

echo ---- running watcom_s.mak %1 ---- >> all_dos.log
wmake -h -f watcom_s.mak %1 >> all_dos.log

echo ---- running watcom_l.mak %1 ---- >> all_dos.log
wmake -h -f watcom_l.mak %1 >> all_dos.log

echo ---- running watcom_f.mak %1 ---- >> all_dos.log
wmake -h -f watcom_f.mak %1 >> all_dos.log

echo ---- running watcom_w.mak %1 ---- >> all_dos.log
wmake -h -f watcom_w.mak %1 >> all_dos.log

echo ---- running watcom_3.mak %1 ---- >> all_dos.log
wmake -h -f watcom_3.mak %1 >> all_dos.log

call setbcc 4
echo ---- running bcc_s.mak %1 ---- >> all_dos.log
maker -f bcc_s.mak %1 >> all_dos.log

echo ---- running bcc_l.mak %1 ---- >> all_dos.log
maker -f bcc_l.mak %1 >> all_dos.log

call setbcc 5
echo ---- running bcc_f.mak %1 ---- >> all_dos.log
maker -f bcc_f.mak %1 >> all_dos.log

