@echo off

free386 -n -q nasm -f obj chk_f386.asm
free386 -n -q ..\flatlink.exp -o chk_f386.exp chk_f386.obj

echo;
free386 -n -q nasm -f obj chk_xms.asm
free386 -n -q ..\flatlink.exp -o chk_xms.com chk_xms.obj

echo;
free386 -n -q nasm -f obj msg_test.asm
free386 -n -q nasm -f obj msg_data.asm
free386 -n -q ..\flatlink.exp -o msg_test.com msg_test.obj msg_data.obj

