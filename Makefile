CC=gcc
LIBS=alsa
CFLAGS=-Wall -g `pkg-config --cflags --libs ${LIBS}`

volmon: volmon.c
	${CC} volmon.c ${CFLAGS} -o volmon

clean:
	rm -f volmon

.PHONY: clean
