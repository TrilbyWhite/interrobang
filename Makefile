
PROG	=	interrobang
PREFIX	?=	/usr
CFLAGS	+=	-lX11

all: ${PROG}.c
	@gcc -o ${PROG} ${PROG}.c ${CFLAGS}
	@strip ${PROG}

debug: ${PROG}.c
	@gcc -o ${PROG} ${PROG}.c ${CFLAGS} -DDEBUG

clean:
	@rm -rf ${PROG}

install: all
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}

