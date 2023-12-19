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

all: $(EXP_FILES) $(COM_FILES)

#------------------------------------------------------------------------------
%.obj: %.asm
	$(ASM) $(AFLAGS) -o $@ $<

%.exp: %.obj
	$(LINK) $(LNK_EXP) -o $@ $^

%.com: %.obj
	$(LINK) -o $@ $^

clean:
	$(RM) *.obj *.map *.exp *.com
