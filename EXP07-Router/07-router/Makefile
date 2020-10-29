TARGET = router

all: $(TARGET)

CC = gcc
LD = gcc

CFLAGS = -g -Wall -Iinclude
LDFLAGS = -L.

LIBS = -lipstack -lpthread

LIBIP = libipstack.a
LIBIP_SRCS = arp.c arpcache.c icmp.c ip_base.c packet.c rtable.c rtable_internal.c
LIBIP_OBJS = $(patsubst %.c,%.o,$(LIBIP_SRCS))

HDRS = ./include/*.h

$(LIBIP_OBJS) : %.o : %.c include/*.h
	$(CC) -c $(CFLAGS) $< -o $@

$(LIBIP): $(LIBIP_OBJS)
	ar rcs $(LIBIP) $(LIBIP_OBJS)

SRCS = main.c ip.c
OBJS = $(patsubst %.c,%.o,$(SRCS))

$(OBJS) : %.o : %.c include/*.h
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(LIBIP) $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(TARGET) $(LIBS) 

clean:
	rm -f *.o $(TARGET) $(LIBIP)

tags: $(SRCS) $(HDRS)
	ctags $(SRCS) $(HDRS)
