CC=clang
LIBS=-lX11 -lXtst
CFLAGS=-g -std=c99 -Wall -O3
LDFLAGS=-g ${LIBS}

SRC=joyous.c
OBJ=joyous.o

PREFIX=/usr/local

all: joyous

.c.o:
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h

joystick: ${OBJ}
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

install: all
	@echo Installing joyous to ${PREFIX}/bin
	@mkdir -p ${PREFIX}/bin
	@cp -f joyous ${PREFIX}/bin/
	@chmod 755 ${PREFIX}/bin/joyous

clean:
	@rm -f joyous ${OBJ}

