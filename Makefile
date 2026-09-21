#
# make warmup2  — build the token-bucket emulator
#
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

OBJS = warmup2.o args.o timeutil.o my402list.o

warmup2: $(OBJS)
	$(CC) -o warmup2 -g $(OBJS) $(LDFLAGS)

warmup2.o: warmup2.c warmup2.h cs402.h
	$(CC) $(CFLAGS) -c warmup2.c

args.o: args.c warmup2.h cs402.h
	$(CC) $(CFLAGS) -c args.c

timeutil.o: timeutil.c warmup2.h
	$(CC) $(CFLAGS) -c timeutil.c

my402list.o: my402list.c my402list.h cs402.h
	$(CC) $(CFLAGS) -c my402list.c

clean:
	rm -f *.o warmup2
