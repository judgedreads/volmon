CC=gcc
LIBS=alsa
CFLAGS=-Wall -g `pkg-config --cflags --libs ${LIBS}`

volmon: volmon.c
	${CC} volmon.c ${CFLAGS} -o volmon

clean:
	rm -f volmon

install: volmon
	cp volmon ${HOME}/bin/

uninstall:
	rm ${HOME}/bin/volmon -f

.PHONY: clean install uninstall
