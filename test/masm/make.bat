@echo off

masm chk_vcpi.asm chk_vcpi.obj nul nul
masm chk_lib.asm  chk_lib.obj  nul nul

free386 ..\..\flatlink.exp -o chk_vcpi.com chk_vcpi.obj chk_lib.obj %1 %2 %3 %4 %5 %6 %7 %8 %9


