
PROG	=	interrobang
ALTER	=	percontation
PREFIX	?=	/usr
LDFLAGS	+=	-lX11

all: ${PROG} ${ALTER}

${ALTER}: ${ALTER}.c
	@gcc -o ${ALTER} ${ALTER}.c ${CFLAGS}

${PROG}: ${PROG}.c
	@gcc -o ${PROG} ${PROG}.c ${CFLAGS} ${LDFLAGS}

debug: ${PROG}.c
	@gcc -o -g ${PROG} ${PROG}.c ${CFLAGS} -DDEBUG

clean:
	@rm -rf ${PROG} ${ALTER}

install: all
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
	@install -Dm755 ${ALTER} ${DESTDIR}${PREFIX}/bin/${ALTER}

