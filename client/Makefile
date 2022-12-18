CC=gcc
CFlags=-fdiagnostics-color=always -Wall -Wextra -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused -Wno-sign-compare
DEPS= player.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: player.c player.o
	$(CC) -o player player.o

.PHONY: clean

clean: 
	rm -f *.o player *.jpg *.jpeg *.svg *.png *.txt