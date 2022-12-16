CC=gcc
CFlags=-fdiagnostics-color=always -Wall -Wextra -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow -Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused -Wno-sign-compare
DEPS= server.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: server.c server.o
	$(CC) -o server server.o

.PHONY: clean

clean: 
	rm -f *.o server *.jpg *.jpeg *.svg *.png GAMES/*.txt SCOREBOARD.txt
