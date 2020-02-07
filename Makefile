CC=gcc
# LIBRARIES=gconf-2.0 hildon-1 hildon-fm-2 gtk+-2.0 libosso gdk-2.0 gconf-2.0 gnome-vfs-2.0
LIBRARIES=gconf-2.0 hildon-1 gtk+-2.0 libosso gdk-2.0 gconf-2.0 gnome-vfs-2.0 telepathy-glib
CFLAGS= -Wall -std=c11 `pkg-config --cflags $(LIBRARIES)`
LDFLAGS=`pkg-config --libs $(LIBRARIES)`

all: dialer audio_setup

dialer: dialer.o tp.o
	$(CC) $(LDFLAGS) dialer.o tp.o -o dialer

dialer.o: dialer.c ui.h
	$(CC) $(CFLAGS) -c -o dialer.o dialer.c

tp.o: tp.c tp.h
	$(CC) $(CFLAGS) -c -o tp.o tp.c

audio_setup: audio_setup.c
	$(CC) -o audio_setup audio_setup.c

install:
	install -d /usr/bin
	install dialer /usr/bin

clean:
	rm -f dialer.o tp.o dialer audio_setup
