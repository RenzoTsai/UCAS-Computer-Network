TARGET = mospfd

all: $(TARGET)

CC = gcc
LD = gcc

CFLAGS = -g -Wall -Wno-unused-variable -Wno-unused-function -Iinclude
LDFLAGS = -L.

LIBS = -lipstack -lpthread

HDRS = ./include/*.h

SRCS = ip.c main.c mospf_database.c mospf_daemon.c mospf_proto.c 
OBJS = $(patsubst %.c,%.o,$(SRCS))

$(OBJS) : %.o : %.c include/*.h
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(LIBIP) $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(TARGET) $(LIBS) 

clean:
	rm -f *.o $(TARGET)
