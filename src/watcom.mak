#
# Makefile for OpenWatcom on Linux
#
CC     = wpp386
CFLAGS =
LINK   = wlink
LFLAGS = op q
RM     = rm -f

what: .SYMBOLIC
	# @echo please set build target "exp" or "linux"
	@%make exp

linux: .SYMBOLIC 
	@set CTARGET=-bt=linux
	@set LTARGET=sys linux
	@set OEXT=o
	@set PEXT=
	@%make ../flatlink

exp: .SYMBOLIC
	@set CTARGET=-bt=pharlap
	@set LTARGET=sys pharlap
	@set OEXT=obj
	@set PEXT=.exp
	@%make ../flatlink.exp

O = $(%OEXT)

HEADERS  = flatlink.h   memory_x.h   load_obj.h   output.h
OBJS     = flatlink.o   memory_x.o   load_obj.o   output.o
OBJS_EXP = flatlink.obj memory_x.obj load_obj.obj output.obj

#------------------------------------------------------------------------------
# build for linux binary
#------------------------------------------------------------------------------
flatlink.o: flatlink.c $(HEADERS)
	$(CC) $(CFLAGS) $(%CTARGET) -fo=$@ flatlink.c

memory_x.o: memory_x.c $(HEADERS)
	$(CC) $(CFLAGS) $(%CTARGET) -fo=$@ memory_x.c

load_obj.o: load_obj.c $(HEADERS)
	$(CC) $(CFLAGS) $(%CTARGET) -fo=$@ load_obj.c

output.o: output.c $(HEADERS)
	$(CC) $(CFLAGS) $(%CTARGET) -fo=$@ output.c


../flatlink: $(OBJS)
	$(LINK) $(LFLAGS) $(%LTARGET) name $@ file {$<}

#------------------------------------------------------------------------------
# build for PharLap(exp) binary
#------------------------------------------------------------------------------
flatlink.obj: flatlink.c $(HEADERS)
	$(CC) $(CFLAGS) $(%CTARGET) -fo=$@ flatlink.c

memory_x.obj: memory_x.c $(HEADERS)
	$(CC) $(CFLAGS) $(%CTARGET) -fo=$@ memory_x.c

load_obj.obj: load_obj.c $(HEADERS)
	$(CC) $(CFLAGS) $(%CTARGET) -fo=$@ load_obj.c

output.obj: output.c $(HEADERS)
	$(CC) $(CFLAGS) $(%CTARGET) -fo=$@ output.c


../flatlink.exp: $(OBJS_EXP)
	$(LINK) $(LFLAGS) $(%LTARGET) name $@ file {$<}

clean:
	$(RM) *.o *.obj
