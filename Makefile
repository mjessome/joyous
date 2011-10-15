CC=clang
LIBS=-lX11 -lXtst
CFLAGS=-g -std=c99 -Wall
LDFLAGS=-g ${LIBS}


SRC=joystick.c
OBJ=joystick.o

.c.o:
	@${CC} -c ${CFLAGS} $<

all: joystick

joystick: ${OBJ}
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	@rm -f joystick ${OBJ}

