
PROG	=	interobang
PREFIX	?=	/usr
CFLAGS	+=	-lX11

all: ${PROG}

${PROG}: %: %.c
	@gcc -o $* $*.c -lX11
	@strip $*

clean:
	@rm -rf ${PROG}

