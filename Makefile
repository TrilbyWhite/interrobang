
PROG	=	interrobang
PREFIX	?=	/usr
CFLAGS	+=	-lX11

all: ${PROG} percontation

percontation: percontation.c
	@gcc -o percontation percontation.c

${PROG}: ${PROG}.c
	@gcc -o ${PROG} ${PROG}.c ${CFLAGS}
	@strip ${PROG}

debug: ${PROG}.c
	@gcc -o ${PROG} ${PROG}.c ${CFLAGS} -DDEBUG

clean:
	@rm -rf ${PROG}
	@rm -rf percontation

install: all
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
	@install -Dm755 percontation ${DESTDIR}${PREFIX}/bin/percontation

