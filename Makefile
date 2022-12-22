CC=gcc
CFlags=-fdiagnostics-color=always -Wall -Wextra -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused -Wno-sign-compare
DEPS= GS.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

GS: server/GS.c server/GS.o
	$(CC) -o GS GS.o

player: client/player.c client/player.o
	$(CC) -o player player.o

.PHONY: clean

clean: 
	rm -f *.o GS player *.jpg *.jpeg *.svg *.png GAMES/*.txt SCOREBOARD.txt STATE_*.txt
