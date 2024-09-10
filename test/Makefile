#
# Makefile for Linux
#
ASM     = nasm
AFLAGS  = -f obj

LINK    = ../flatlink
LNK_EXP = -strip
RM      = rm -f

#------------------------------------------------------------------------------
EXP_FILES = chk_f386.exp
COM_FILES = chk_xms.com

all: $(EXP_FILES) $(COM_FILES) msg_test.com

#------------------------------------------------------------------------------
%.obj: %.asm
	$(ASM) $(AFLAGS) -o $@ $<

%.exp: %.obj
	$(LINK) $(LNK_EXP) -o $@ $^

%.com: %.obj
	$(LINK) -o $@ $^

msg_test.com: msg_test.obj msg_data.obj
	$(LINK) -o $@ $^

clean:
	$(RM) *.obj *.map *.exp *.com *.OBJ *.MAP *.EXP *.COM
