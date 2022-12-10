CC=gcc
CFlags=
DEPS= server.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: server.c server.o
	$(CC) -o server server.o

.PHONY: clean

clean: 
	rm -f *.o server *.jpg *.jpeg *.svg *.png
