CC=gcc
CFLAGS=-Wall -ggdb
BIN=efserv
MAINSO=efserv.so
SRCS=channels.c\
     commands.c\
     config.c\
     clients.c\
     clones.c\
     efserv.c\
     msg.c\
     utils.c
OBJS=${SRCS:%.c=%.o}
LIBS=-lefence -ldl

all: efserv efserv.so
${BIN} : modules.o Makefile
	${CC} -rdynamic ${CFLAGS} ${LDFLAGS} modules.o ${LIBS} -o ${BIN}
modules.o : modules.c
	${CC} -c ${CFLAGS} modules.c
${MAINSO} : ${OBJS}
	${CC} -shared ${LDFLAGS} ${OBJS} -o ${MAINSO}
%.o : %.c
	${CC} -c ${CFLAGS} $<

depend:
	${CC} -MM ${SRCS} >.depend

include .depend
