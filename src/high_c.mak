
PROGRAM = ..\flatlink.exp
OBJS    = flatlink.obj memory_x.obj load_obj.obj output.obj
HEADERS = flatlink.h   memory_x.h   load_obj.h   output.h

CC     = hcd386
CFLAGS = 
LINK   = hc386
RM     = rm -f

all: $(PROGRAM)

flatlink.obj: flatlink.c $(HEADERS)
	$(CC) $(CFLAGS) flatlink.c

memory_x.obj: memory_x.c $(HEADERS)
	$(CC) $(CFLAGS) memory_x.c

load_obj.obj: load_obj.c $(HEADERS)
	$(CC) $(CFLAGS) load_obj.c

output.obj: output.c $(HEADERS)
	$(CC) $(CFLAGS) output.c

$(PROGRAM): $(OBJS)
	$(LINK) -o $@ $<
