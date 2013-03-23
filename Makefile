
PROG	=	interobang
PREFIX	?=	/usr
CFLAGS	+=	-lX11

all: ${PROG}.c
	@gcc -o ${PROG} ${PROG}.c ${CFLAGS}
	@strip ${PROG}

clean:
	@rm -rf ${PROG}

install: all
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
	@install -Dm666 ${PROG}rc ${DESTDIR}${PREFIX}/share/${PROG}/${PROG}rc

