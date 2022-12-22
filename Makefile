CC=gcc
CFlags=-fdiagnostics-color=always -Wall -Wextra -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused -Wno-sign-compare
DEPS_GS= server/GS.h
DEPS_PL= client/player.h

all: GS player

GS.o: server/GS.c $(DEPS_GS)
	$(CC) -c -o $@ $< $(CFLAGS)

player.o: client/player.c $(DEPS_PL)
	$(CC) -c -o $@ $< $(CFLAGS)

GS: server/GS.c GS.o
	$(CC) -o GS GS.o

player: client/player.c player.o
	$(CC) -o player player.o

.PHONY: clean

clean: 
	rm -f *.o GS player *.jpg *.jpeg *.svg *.png GAMES/*.txt SCOREBOARD.txt STATE_*.txt
