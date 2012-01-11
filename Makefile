CC=clang
LIBS=-lX11 -lXtst
CFLAGS=-g -std=c99 -Wall
LDFLAGS=-g ${LIBS}

SRC=joystick.c
OBJ=joystick.o

PREFIX=/usr/local

all: joystick

.c.o:
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h

joystick: ${OBJ}
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

install: all
	@echo Installing joystick to ${PREFIX}/bin
	@mkdir -p ${PREFIX}/bin
	@cp -f joystick ${PREFIX}/bin/
	@chmod 755 ${PREFIX}/bin/joystick

clean:
	@rm -f joystick ${OBJ}

