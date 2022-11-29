CC=gcc
CFlags=
DEPS= client.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: client.c client.o
	$(CC) -o client client.o

.PHONY: clean

clean: 
	rm -f *.o client *.jpg *.jpeg *.svg *.png *.txt