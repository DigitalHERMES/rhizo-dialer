CC=gcc
# LIBRARIES=gconf-2.0 hildon-1 hildon-fm-2 gtk+-2.0 libosso gdk-2.0 gconf-2.0 gnome-vfs-2.0
LIBRARIES=glib-2.0 gtk+-2.0 telepathy-glib
CFLAGS= -Wall -std=c11 `pkg-config --cflags $(LIBRARIES)`
LDFLAGS=`pkg-config --libs $(LIBRARIES)`

all: dialer

dialer: dialer.o
	$(CC) $(LDFLAGS) dialer.o -o dialer

dialer.o: dialer.c
	$(CC) $(CFLAGS) -c -o dialer.o dialer.c

install:
	install -d /usr/bin
	install dialer /usr/bin

clean:
	rm -f dialer.o dialer
