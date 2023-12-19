@echo off

386asm chk_ver.asm
386asm bin2hex.asm

free386 ..\..\flatlink.exp chk_ver.obj bin2hex.obj %1 %2 %3 %4 %5 %6 %7 %8 %9


