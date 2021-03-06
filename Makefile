CC=gcc
# LIBRARIES=gconf-2.0 hildon-1 hildon-fm-2 gtk+-2.0 libosso gdk-2.0 gconf-2.0 gnome-vfs-2.0
LIBRARIES=gconf-2.0 hildon-1 gtk+-2.0 libosso gdk-2.0 gconf-2.0 telepathy-glib
CFLAGS= -Wall -std=gnu11 -g `pkg-config --cflags $(LIBRARIES)`
LDFLAGS=`pkg-config --libs $(LIBRARIES)` -lm -pthread -lasound

all: dialer

dialer: dialer.o at.o audio_setup.o ring-audio.o daemonize.o
	$(CC) $(LDFLAGS) dialer.o at.o audio_setup.o ring-audio.o daemonize.o -o dialer

dialer.o: dialer.c ui.h
	$(CC) $(CFLAGS) -c -o dialer.o dialer.c

at.o: at.c at.h
	$(CC) $(CFLAGS) -c -o at.o at.c

daemonize.o: daemonize.c daemonize.h
	$(CC) $(CFLAGS) -c -o daemonize.o daemonize.c

audio_setup.o: audio_setup.c audio_setup.h
	$(CC) $(CFLAGS) -c -o audio_setup.o audio_setup.c

ring-audio.o:  ring-audio.c ring-audio.h
	$(CC) $(CFLAGS) -c -o ring-audio.o ring-audio.c

install: dialer
	install -d /usr/bin
	install dialer /usr/bin
	install -d /usr/share/icons/
	install rhizo_dialer_icon.png /usr/share/icons/
	install -d /usr/share/applications/hildon/
	install dialer.desktop /usr/share/applications/hildon/

clean:
	rm -f dialer.o at.o audio_setup.o ring_audio.o daemonize.o dialer
