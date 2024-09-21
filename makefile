CFLAGS= -Wall -pedantic -std=gnu99

all: daemon

daemon:
	gcc $(CFLAGS) `pkg-config --cflags --libs libnotify` daemon.c -o daemon_exampled
