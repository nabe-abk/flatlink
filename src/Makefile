#
# Makefile for GCC on Linux
#

PROGRAM = ../flatlink
OBJS    = flatlink.o memory_x.o load_obj.o output.o

CC     = gcc
CFLAGS = -Wall -Os
RM     = rm -f

all: $(PROGRAM)


%.o: %.c $(OBJS:%.o=%.h)
	$(CC) $(CFLAGS) -c $<

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $^

clean:
	$(RM) *.o *.obj
