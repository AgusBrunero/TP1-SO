CC = gcc
CFLAGS = -Wall -std=c99 -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE

all: master jugador vista

master: main.c
	$(CC) $(CFLAGS) -o master main.c

jugador: player.c
	$(CC) $(CFLAGS) -o jugador player.c

vista: vista.c
	$(CC) $(CFLAGS) -o vista vista.c

clean:
	rm -f master jugador vista

.PHONY: all clean